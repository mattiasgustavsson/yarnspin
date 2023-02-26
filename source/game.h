

typedef enum gamestate_t {
    GAMESTATE_NO_CHANGE,
    GAMESTATE_BOOT,
    GAMESTATE_TITLE,
    GAMESTATE_LOCATION,
    GAMESTATE_DIALOG,
    GAMESTATE_EXIT,
    GAMESTATE_TERMINATE,
} gamestate_t;


typedef struct game_t {
    bool exit_flag;
    bool exit_requested;
    gamestate_t current_state;
    gamestate_t new_state;
    bool disable_transition;
    int transition_counter;
    uint8_t* screen;
    uint32_t* screen_rgb;
    int screen_width;
    int screen_height;
    input_t* input;
    int mouse_x;
    int mouse_y;
    int color_background;
    int color_disabled;
    int color_txt;
    int color_opt;
    int color_chr;
    int color_use;
    int color_name;
    int color_facebg;
    yarn_t* yarn;
    int queued_location;
    int queued_dialog;
    pixelfont_t* font_txt;
    pixelfont_t* font_opt;
    pixelfont_t* font_chr;
    pixelfont_t* font_use;
    pixelfont_t* font_name;
    struct {
        float limit;
        int phrase_index;
        int phrase_len;
        int chr_index;
        int enable_options;
    } dialog;
    struct {
        int current_location;
        int current_dialog;
        int current_image;
        int logo_index;
        array(bool)* flags;
        array(int)* items;
    } state;
} game_t;


void game_init( game_t* game, yarn_t* yarn, input_t* input, uint8_t* screen, uint32_t* screen_rgb, int width, int height ) {
    game->state.current_location = -1;
    game->state.current_dialog = -1;
    game->state.current_image = -1;
    game->state.logo_index = 0;
    game->state.flags = managed_array( bool );
    game->state.items = managed_array( int );
    game->state.current_location = yarn->start_location;
    game->state.current_dialog = yarn->start_dialog;
    for( int i = 0; i < yarn->flag_ids->count; ++i ) {
        bool value = false;
        array_add( game->state.flags, &value );
    }
    int darkest_index = 0;
    int darkest_luma = 65536;
    int brightest_index = 0;
    int brightest_luma = 0;
    int facebg_index = 0;
    int facebg_luma = 65536;
    int disabled_index = 0;
    int disabled_luma = 65536;

    int facebg_lumaref = 54 * 0x28 + 182 * 0x28 + 19 * 0x28;
    int disabled_lumaref = 54 * 0x70 + 182 * 0x70 + 19 * 0x70;
    for( int i = 0; i < yarn->assets.palette_count; ++i ) {
        int c = (int)( yarn->assets.palette[ i ] & 0x00ffffff );
        int r = c & 0xff;
        int g = ( c >> 8 ) & 0xff;
        int b = ( c >> 16 ) & 0xff;
        int l = 54 * r + 182 * g + 19 * b;
        if( l <= darkest_luma ) { darkest_luma = l; darkest_index = i; }
        if( l >= brightest_luma ) { brightest_luma = l; brightest_index = i; }
        int coldiff = ( abs( r - g ) + abs( g - b ) + abs( r - b ) ) * 64;
        if( abs( l - facebg_lumaref ) + coldiff <= facebg_luma ) { facebg_luma = abs( l - facebg_lumaref ) + coldiff; facebg_index = i; }
        if( abs( l - disabled_lumaref ) + coldiff <= disabled_luma ) { disabled_luma = abs( l - disabled_lumaref ) + coldiff; disabled_index = i; }
    }

    if( disabled_index == darkest_index ) {
        disabled_luma = 65536;
        for( int i = 0; i < yarn->assets.palette_count; ++i ) {
            if( i == darkest_index ) continue;
            int c = (int)( yarn->assets.palette[ i ] & 0x00ffffff );
            int r = c & 0xff;
            int g = ( c >> 8 ) & 0xff;
            int b = ( c >> 16 ) & 0xff;
            int l = 54 * r + 182 * g + 19 * b;
            if( l <= disabled_luma ) { disabled_luma = l; disabled_index = i; }
        }
    }

    game->exit_flag = false;
    game->exit_requested = false;
    game->current_state = GAMESTATE_NO_CHANGE;
    game->new_state = GAMESTATE_BOOT;
    game->disable_transition = false;
    game->transition_counter = 10;
    game->screen = screen;
    game->screen_rgb = screen_rgb;
    game->screen_width = width;
    game->screen_height = height;
    game->input = input;
    game->yarn = yarn;
    game->queued_location = -1;
    game->queued_dialog = -1;
    game->font_txt = yarn->assets.font_description;
    game->font_opt = yarn->assets.font_options;
    game->font_chr = yarn->assets.font_characters;
    game->font_use = yarn->assets.font_items;
    game->font_name = yarn->assets.font_name;
    game->color_background = yarn->globals.color_background;
    game->color_disabled = yarn->globals.color_disabled;
    game->color_txt = yarn->globals.color_txt;
    game->color_opt = yarn->globals.color_opt;
    game->color_chr = yarn->globals.color_chr;
    game->color_use = yarn->globals.color_use;
    game->color_name = yarn->globals.color_name;
    game->color_facebg = yarn->globals.color_facebg;

    if( game->color_background < 0 ) game->color_background = (uint8_t)darkest_index;
    if( game->color_disabled < 0 ) game->color_disabled = (uint8_t)disabled_index;
    if( game->color_txt < 0 ) game->color_txt = (uint8_t)brightest_index;
    if( game->color_opt < 0 ) game->color_opt = (uint8_t)brightest_index;
    if( game->color_chr < 0 ) game->color_chr = (uint8_t)brightest_index;
    if( game->color_use < 0 ) game->color_use = (uint8_t)brightest_index;
    if( game->color_name < 0 ) game->color_name = (uint8_t)brightest_index;
    if( game->color_facebg < 0 ) game->color_facebg = (uint8_t)facebg_index;
}


gamestate_t boot_init( game_t* game );
gamestate_t boot_update( game_t* game );
gamestate_t title_init( game_t* game );
gamestate_t title_update( game_t* game );
gamestate_t location_init( game_t* game );
gamestate_t location_update( game_t* game );
gamestate_t dialog_init( game_t* game );
gamestate_t dialog_update( game_t* game );
gamestate_t exit_init( game_t* game );
gamestate_t exit_update( game_t* game );
gamestate_t terminate_init( game_t* game );
gamestate_t terminate_update( game_t* game );


void game_update( game_t* game ) {
    if( game->new_state != GAMESTATE_NO_CHANGE ) {
        if( !game->disable_transition ) {
            game->transition_counter = -10;
        }
        game->disable_transition = false;
        game->current_state = game->new_state;
        gamestate_t new_state = GAMESTATE_NO_CHANGE;
        switch( game->current_state ) {
            case GAMESTATE_NO_CHANGE:
                break;
            case GAMESTATE_BOOT:
                new_state = boot_init( game );
                break;
            case GAMESTATE_TITLE:
                new_state = title_init( game );
                break;
            case GAMESTATE_LOCATION:
                new_state = location_init( game );
                break;
            case GAMESTATE_DIALOG:
                new_state = dialog_init( game );
                break;
            case GAMESTATE_EXIT:
                new_state = exit_init( game );
                break;
            case GAMESTATE_TERMINATE:
                new_state = terminate_init( game );
                break;
        }
        game->new_state = new_state;
        return;
    }

    if( game->transition_counter < 10 ) {
        game->transition_counter++;
    }
    if( game->transition_counter < 0 ) {
        return;
    }

    gamestate_t new_state = GAMESTATE_NO_CHANGE;
    switch( game->current_state ) {
        case GAMESTATE_NO_CHANGE:
            break;
        case GAMESTATE_BOOT:
            new_state = boot_update( game );
            break;
        case GAMESTATE_TITLE:
            new_state = title_update( game );
            break;
        case GAMESTATE_LOCATION:
            new_state = location_update( game );
            break;
        case GAMESTATE_DIALOG:
            new_state = dialog_update( game );
            break;
        case GAMESTATE_EXIT:
            new_state = exit_update( game );
            break;
        case GAMESTATE_TERMINATE:
            new_state = terminate_update( game );
            break;
    }
    game->new_state = new_state;
    if( game->exit_requested ) {
        game->new_state = GAMESTATE_EXIT;
        game->disable_transition = true;
        game->exit_requested = false;
    }
}


void cls( game_t* game ) {
    if( game->screen ) {
        memset( game->screen, game->color_background, (size_t) game->screen_width * game->screen_height );
    } else {
        memset( game->screen_rgb, game->color_background, sizeof( uint32_t) *  game->screen_width * game->screen_height );
    }
}


void scale_for_resolution( game_t* game, int* x, int* y ) {
    float scale_factors[] = { 1.0f, 1.5f, 2.0f, 4.5f };
    if( x ) {
        *x = (int)( *x * scale_factors[ (int) game->yarn->globals.resolution ] );
    }
    if( y ) {
        *y = (int)( *y * scale_factors[ (int) game->yarn->globals.resolution ] );
    }
}


void scale_for_resolution_inverse( game_t* game, int* x, int* y ) {
    float scale_factors[] = { 1.0f, 1.5f, 2.0f, 4.5f };
    if( x ) {
        *x = (int)( *x / scale_factors[ (int) game->yarn->globals.resolution ] );
    }
    if( y ) {
        *y = (int)( *y / scale_factors[ (int) game->yarn->globals.resolution ] );
    }
}


void draw( game_t* game, palrle_data_t* bmp, int x, int y ) {
    scale_for_resolution( game, &x, &y );
    if( game->screen ) {
        palrle_blit( bmp, x, y, game->screen, game->screen_width, game->screen_height );
    } else {
        rgbimage_t* image = (rgbimage_t*) bmp;
        for( int i = 0; i < image->height; ++i ) {
            memcpy( game->screen_rgb + x + ( y +i ) * game->screen_width, image->pixels + i * image->width, image->width * sizeof( uint32_t ) );
        }
    }
}


void box( game_t* game, int x, int y, int w, int h, int c ) {
    scale_for_resolution( game, &x, &y );
    scale_for_resolution( game, &w, &h );
    if( game->screen ) {
        for( int iy = 0; iy < h; ++iy ) {
            for( int ix = 0; ix < w; ++ix ) {
                int xp = x + ix;
                int yp = y + iy;
                if( xp >= 0 && xp < game->screen_width && yp >= 0 && yp < game->screen_height ) {
                    game->screen[ xp + yp * game->screen_width ] = (uint8_t) c;
                }
            }
        }
    } else {
        for( int iy = 0; iy < h; ++iy ) {
            for( int ix = 0; ix < w; ++ix ) {
                int xp = x + ix;
                int yp = y + iy;
                if( xp >= 0 && xp < game->screen_width && yp >= 0 && yp < game->screen_height ) {
                    game->screen_rgb[ xp + yp * game->screen_width ] = game->yarn->assets.palette[ c ];
                }
            }
        }
    }
}


pixelfont_bounds_t center( game_t* game, pixelfont_t* font, string str, int x, int y, int color ) {
    scale_for_resolution( game, &x, &y );
    pixelfont_bounds_t bounds;
    if( game->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, game->screen, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_CENTER, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    } else {
        pixelfont_blit_rgb( font, x, y, str, game->yarn->assets.palette[ color ], game->screen_rgb, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_CENTER, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    }
    scale_for_resolution_inverse( game, &bounds.width, &bounds.height );
    return bounds;
}


pixelfont_bounds_t center_wrap( game_t* game, pixelfont_t* font, string str, int x, int y, int color, int wrap_width ) {
    scale_for_resolution( game, &x, &y );
    scale_for_resolution( game, &wrap_width, NULL );
    pixelfont_bounds_t bounds;
    x -= wrap_width / 2;
    if( game->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, game->screen, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_CENTER, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            &bounds );
    } else {
        pixelfont_blit_rgb( font, x, y, str, game->yarn->assets.palette[ color ], game->screen_rgb, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_CENTER, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            &bounds );
    }
    scale_for_resolution_inverse( game, &bounds.width, &bounds.height );
    return bounds;
}


pixelfont_bounds_t text( game_t* game, pixelfont_t* font, string str, int x, int y, int color ) {
    scale_for_resolution( game, &x, &y );
    pixelfont_bounds_t bounds;
    if( game->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, game->screen, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    } else {
        pixelfont_blit_rgb( font, x, y, str, game->yarn->assets.palette[ color ], game->screen_rgb, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    }
    scale_for_resolution_inverse( game, &bounds.width, &bounds.height );
    return bounds;
}


void wrap( game_t* game, pixelfont_t* font, string str, int x, int y, int color, int wrap_width ) {
    scale_for_resolution( game, &x, &y );
    scale_for_resolution( game, &wrap_width, NULL );
    if( game->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, game->screen, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            NULL );
    } else {
        pixelfont_blit_rgb( font, x, y, str, game->yarn->assets.palette[ color ], game->screen_rgb, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            NULL );
    }
}


void wrap_limit( game_t* game, pixelfont_t* font, string str, int x, int y, int color, int wrap_width, int limit ) {
    scale_for_resolution( game, &x, &y );
    scale_for_resolution( game, &wrap_width, NULL );
    if( game->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, game->screen, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, limit, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF,
            PIXELFONT_UNDERLINE_OFF, NULL );
    } else {
        pixelfont_blit_rgb( font, x, y, str, game->yarn->assets.palette[ color ], game->screen_rgb, game->screen_width, game->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, limit, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF,
            PIXELFONT_UNDERLINE_OFF, NULL );
    }
}


int font_height( game_t* game, int height ) {
    scale_for_resolution_inverse( game, &height, NULL );
    return height;
}


bool was_key_pressed( game_t* game, int key ) {
    return input_was_key_pressed( game->input, key );
}


yarn_location_t* find_location( yarn_t* yarn, string location_id ) {
    for( int i = 0; i < yarn->locations->count; ++i ) {
        yarn_location_t* location = &yarn->locations->items[ i ];
        if( cstr_compare_nocase( location->id, location_id ) == 0 ) {
            return location;
        }
    }
    return NULL;
}


yarn_dialog_t* find_dialog( yarn_t* yarn, string dialog_id ) {
    for( int i = 0; i < yarn->dialogs->count; ++i ) {
        yarn_dialog_t* dialog = &yarn->dialogs->items[ i ];
        if( cstr_compare_nocase( dialog->id, dialog_id ) == 0 ) {
            return dialog;
        }
    }
    return 0;
}


bool test_cond( game_t* game, yarn_cond_t* cond ) {
    bool result = true;
    for( int i = 0; i < cond->ands->count; ++i )
        {
        bool or_result = false;
        yarn_cond_or_t* ors = &cond->ands->items[ i ];
        for( int j = 0; j < ors->flags->count; ++j ) {
            yarn_cond_flag_t* flag = &ors->flags->items[ j ];
            bool flag_val = game->state.flags->items[ flag->flag_index ];
            if( flag->is_not ) {
                flag_val = !flag_val;
            }
            or_result = or_result || flag_val;
        }
        result = result && or_result;
    }
    return result;
}


void do_actions( game_t* game, array_param(yarn_act_t)* act_param ) {
    array(yarn_act_t)* act = ARRAY_CAST( act_param );
    for( int i = 0; i < act->count; ++i ) {
        yarn_act_t* action = &act->items[ i ];
        if( !test_cond( game, &action->cond ) ) {
            continue;
        }
        switch( action->type ) {
            case ACTION_TYPE_GOTO_LOCATION: {
                game->queued_dialog = -1;
                game->queued_location = action->param_location_index;
            } break;
            case ACTION_TYPE_GOTO_DIALOG: {
                game->queued_location = -1;
                game->queued_dialog = action->param_dialog_index;
            } break;
            case ACTION_TYPE_EXIT: {
                game->exit_requested = true;
            } break;
            case ACTION_TYPE_FLAG_SET: {
                game->state.flags->items[ action->param_flag_index ] = true;
            } break;
            case ACTION_TYPE_FLAG_CLEAR: {
                game->state.flags->items[ action->param_flag_index ] = false;
            } break;
            case ACTION_TYPE_FLAG_TOGGLE: {
                game->state.flags->items[ action->param_flag_index ] = !game->state.flags->items[ action->param_flag_index ];
            } break;
            case ACTION_TYPE_ITEM_GET: {
                bool found = false;
                for( int j = 0;  j < game->state.items->count; ++j ) {
                    if( game->state.items->items[ j ] == action->param_item_index ) {
                        found = true;
                        break;
                    }
                }
                if( !found ) {
                    array_add( game->state.items, &action->param_item_index );
                }
            } break;
            case ACTION_TYPE_ITEM_DROP: {
                for( int j = 0;  j < game->state.items->count; ++j ) {
                    if( game->state.items->items[ j ] == action->param_item_index ) {
                        array_remove( game->state.items, j );
                        break;
                    }
                }
            } break;
            case ACTION_TYPE_NONE: {
                break;
            }
        }
    }
}


// boot
gamestate_t boot_init( game_t* game ) {
    cls( game );
    game->state.logo_index = -1;
    if( game->yarn->screen_names->count > 0 ) {
        return GAMESTATE_TITLE;
    } else if( game->state.current_location >= 0 ) {
        return GAMESTATE_LOCATION;
    } else if( game->state.current_dialog >= 0 ) {
        return GAMESTATE_DIALOG;
    } else {
        return GAMESTATE_EXIT;
    }
}


gamestate_t boot_update( game_t* game ) {
    (void) game;
    return GAMESTATE_NO_CHANGE;
}


// title
gamestate_t title_init( game_t* game ) {
    ++game->state.logo_index;
    return GAMESTATE_NO_CHANGE;
}

gamestate_t title_update( game_t* game ) {
    if( game->state.logo_index < game->yarn->globals.logo_indices->count ) {
        cls( game );
        palrle_data_t* img = game->yarn->assets.bitmaps->items[ game->yarn->globals.logo_indices->items[ game->state.logo_index ] ];
        draw( game, img, 0, 0 );
    }

    if( was_key_pressed( game, APP_KEY_LBUTTON ) || was_key_pressed( game, APP_KEY_SPACE ) || was_key_pressed( game, APP_KEY_ESCAPE ) ) {
        if( game->state.logo_index < game->yarn->globals.logo_indices->count - 1 ) {
            return GAMESTATE_TITLE;
        } else {
            return GAMESTATE_LOCATION;
        }
    }
    return GAMESTATE_NO_CHANGE;
}


// location
gamestate_t location_init( game_t* game ) {
    game->queued_location = -1;
    game->queued_dialog = -1;

    yarn_t* yarn = game->yarn;
    yarn_location_t* location = &yarn->locations->items[ game->state.current_location ];

    // act:
    do_actions( game, location->act );
    return GAMESTATE_NO_CHANGE;
}


gamestate_t location_update( game_t* game ) {
    cls( game );

    yarn_t* yarn = game->yarn;

    if( game->state.current_location < 0 && game->state.current_dialog >= 0 ) {
        return GAMESTATE_DIALOG;
    }

    if( game->state.current_location < 0  ) return GAMESTATE_TERMINATE;

    yarn_location_t* location = &yarn->locations->items[ game->state.current_location ];

    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game, &mouse_x, &mouse_y );

    if( game->queued_dialog >= 0 && ( was_key_pressed( game, APP_KEY_LBUTTON ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
        game->state.current_location = -1;
        if( game->state.current_dialog >= 0 ) {
            game->state.current_dialog = game->queued_dialog;
            game->disable_transition = true;
            return GAMESTATE_DIALOG;
        } else {
            game->state.current_dialog = game->queued_dialog;
            return GAMESTATE_DIALOG;
        }
    } else if( game->queued_location >= 0 && ( was_key_pressed( game, APP_KEY_LBUTTON ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
        game->state.current_location = game->queued_location;
        game->state.current_dialog = -1;
        return GAMESTATE_LOCATION;
    }


    // background_location:
    if( yarn->globals.background_location >= 0 ) {
        draw( game, yarn->assets.bitmaps->items[ yarn->globals.background_location ], 0, 0 );
    }

    // img:
    for( int i = 0; i < location->img->count; ++i ) {
        if( test_cond( game, &location->img->items[ i ].cond ) )  {
            game->state.current_image = location->img->items[ i ].image_index;
        }
    }
    if( game->state.current_image >= 0 ) {
        draw( game, yarn->assets.bitmaps->items[ game->state.current_image ], 64, 10 );
    }

    // txt:
    string txt = "";
    for( int i = 0; i < location->txt->count; ++i ) {
        if( !test_cond( game, &location->txt->items[ i ].cond ) ) {
            continue;
        }
        txt = cstr_cat( txt, cstr_cat( location->txt->items[ i ].text, "\n" ) );
    }
    wrap( game, game->font_txt, txt, 5, 146, game->color_txt, 310 );

    // opt:
    int opt = -1;
    if( game->queued_dialog < 0 && game->queued_location < 0 ) {
        if( was_key_pressed( game, APP_KEY_1 ) ) opt = 0;
        if( was_key_pressed( game, APP_KEY_2 ) ) opt = 1;
        if( was_key_pressed( game, APP_KEY_3 ) ) opt = 2;
        if( was_key_pressed( game, APP_KEY_4 ) ) opt = 3;

        int c = 0;
        for( int i = 0; i < location->opt->count; ++i ) {
            if( !test_cond( game, &location->opt->items[ i ].cond ) ) {
                continue;
            }
            int ypos = 197 + font_height( game, game->font_opt->height ) * c;
            pixelfont_bounds_t b = text( game, game->font_opt, location->opt->items[ i ].text, 5, ypos, game->color_opt );
            if( mouse_y >= ypos && mouse_y < ypos + b.height ) {
                box( game, 4, ypos, 315, b.height, game->color_opt );
                text( game, game->font_opt, location->opt->items[ i ].text, 5, ypos, game->color_background );
                if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                    opt = c;
                }
            }
            ++c;
        }
    }

    // use:
    int use = -1;
    int c = 0;
    for( int i = 0; i < game->state.items->count; ++i ) {
        string usetxt = game->yarn->item_ids->items[ game->state.items->items[ i ] ];
        uint8_t color = (uint8_t) game->color_disabled;
        bool enabled = false;
        for( int j = 0; j < location->use->count; ++j ) {
            if( !test_cond( game, &location->use->items[ j ].cond ) ) {
                continue;
            }
            for( int k = 0; k < location->use->items[ j ].item_indices->count; ++k ) {
                if( game->state.items->items[ i ] == location->use->items[ j ].item_indices->items[ k ] ) {
                    if( game->queued_dialog < 0 && game->queued_location < 0 ) {
                        color = (uint8_t) game->color_use;
                        enabled = true;
                    }
                }
            }
        }
        int ypos = 4 + ( ( 117 - ( game->state.items->count * font_height( game, game->font_use->height ) ) ) / 2 ) + c * font_height( game, game->font_use->height );
        pixelfont_bounds_t b = center( game, game->font_use, usetxt, 287, ypos, color );
        if( enabled && mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x > 259 ) {
            box( game, 260, ypos, 56, b.height, game->color_use );
            center( game, game->font_use, usetxt, 287, ypos, game->color_background );
            if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                use = c;
            }
        }
        ++c;
    }

    // chr:
    int chr_count = 0;
    for( int i = 0; i < location->chr->count; ++i ) {
        if( !test_cond( game, &location->chr->items[ i ].cond ) ) {
            continue;
        }
        ++chr_count;
    }
    int chr = -1;
    c = 0;
    for( int i = 0; i < location->chr->count; ++i ) {
        if( !test_cond( game, &location->chr->items[ i ].cond ) ) {
            continue;
        }
        int ypos = 4 + ( ( 117 - ( chr_count * font_height( game, game->font_chr->height ) ) ) / 2 ) + c * font_height( game, game->font_chr->height );
        int color = game->color_chr;
        if( game->queued_dialog >= 0 || game->queued_location >= 0 ) {
            color = game->color_disabled;
        }

        pixelfont_bounds_t b = center( game,  game->font_chr, game->yarn->characters->items[ location->chr->items[ i ].chr_indices->items[ 0 ] ].short_name, 32, ypos, color );
        if( game->queued_dialog < 0 && game->queued_location < 0 ) {
            if( mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x < 60 ) {
                box( game, 4, ypos, 56, b.height, game->color_chr );
                center( game,  game->font_chr, game->yarn->characters->items[ location->chr->items[ i ].chr_indices->items[ 0 ] ].short_name, 32, ypos, game->color_background );
                if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                    chr = c;
                }
            }
        }
        ++c;
    }
    if( c == 0 ) {
        int ypos = 4 + ( ( 117 - ( 2 * font_height( game, game->font_chr->height ) ) ) / 2 );
        center_wrap( game,  game->font_chr, yarn->globals.alone_text, 32, ypos, game->color_disabled, 56 );
    }


    if( was_key_pressed( game, APP_KEY_ESCAPE ) ) {
        return GAMESTATE_TERMINATE;
    }

    if( game->queued_dialog < 0 && game->queued_location < 0 ) {
        c = 0;
        for( int i = 0; i < location->opt->count; ++i ) {
            if( !test_cond( game, &location->opt->items[ i ].cond ) ) {
                continue;
            }
            if( c == opt ) {
                do_actions( game, location->opt->items[ i ].act );
            }
            ++c;
        }

        c = 0;
        for( int i = 0; i < location->chr->count; ++i ) {
            if( !test_cond( game, &location->chr->items[ i ].cond ) ) {
                continue;
            }
            if( c == chr ) {
                do_actions( game, location->chr->items[ i ].act );
            }
            ++c;
        }

        for( int i = 0; i < game->state.items->count; ++i ) {
            for( int j = 0; j < location->use->count; ++j ) {
                if( !test_cond( game, &location->use->items[ j ].cond ) ) {
                    continue;
                }
                for( int k = 0; k < location->use->items[ j ].item_indices->count; ++k ) {
                    if( game->state.items->items[ i ] == location->use->items[ j ].item_indices->items[ k ] ) {
                        if( i == use ) {
                            do_actions( game, location->use->items[ j ].act );
                        }
                    }
                }
            }
        }

        if( game->queued_dialog >= 0 ) {
            game->state.current_location = -1;
            if( game->state.current_dialog >= 0 ) {
                game->state.current_dialog = game->queued_dialog;
                game->disable_transition = true;
                return GAMESTATE_DIALOG;
            } else {
                game->state.current_dialog = game->queued_dialog;
                return GAMESTATE_DIALOG;
            }
        } else if( game->queued_location >= 0 ) {
            game->state.current_location = game->queued_location;
            game->state.current_dialog = -1;
            return GAMESTATE_LOCATION;
        }
    }

    return GAMESTATE_NO_CHANGE;
}


// dialog
gamestate_t dialog_init( game_t* game ) {
    game->queued_location = -1;
    game->queued_dialog = -1;

    game->dialog.limit = 0.0f;
    game->dialog.phrase_index = 0;
    game->dialog.phrase_len = -1;
    game->dialog.chr_index = -1;
    game->dialog.enable_options = 0;

    yarn_t* yarn = game->yarn;
    yarn_dialog_t* dialog = &yarn->dialogs->items[ game->state.current_dialog ];

    // act:
    do_actions( game, dialog->act );

    for( int i = 0; i < dialog->phrase->count; ++i ) {
        if( !test_cond( game, &dialog->phrase->items[ i ].cond ) ) {
            continue;
        }
        if( game->dialog.phrase_len < 0 ) {
            game->dialog.phrase_len = (int) cstr_len( dialog->phrase->items[ i ].text );
        }
        if( dialog->phrase->items[ i ].character_index >= 0 ) {
            game->dialog.chr_index = dialog->phrase->items[ i ].character_index;
            break;
        }
    }

    return GAMESTATE_NO_CHANGE;
}


gamestate_t dialog_update( game_t* game ) {
    if( game->state.current_dialog < 0 ) {
        return GAMESTATE_TERMINATE;
    }

    cls( game );
    yarn_t* yarn = game->yarn;
    yarn_dialog_t* dialog = &yarn->dialogs->items[ game->state.current_dialog ];

    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game, &mouse_x, &mouse_y );

    // background_dialog:
    if( yarn->globals.background_dialog >= 0 ) {
        draw( game, game->yarn->assets.bitmaps->items[ yarn->globals.background_dialog ], 0, 0 );
    }

    // phrase:
    int phrase_count = 0;
    for( int i = 0; i < dialog->phrase->count; ++i ) {
        if( !test_cond( game, &dialog->phrase->items[ i ].cond ) ) {
            continue;
        }
        if( phrase_count == game->dialog.phrase_index ) {
            string txt = dialog->phrase->items[ i ].text;
            if( dialog->phrase->items[ i ].character_index >= 0 ) {
                game->dialog.chr_index = dialog->phrase->items[ i ].character_index;
            }
            game->dialog.phrase_len = (int) cstr_len( txt );
            if( dialog->phrase->items[ i ].character_index >= 0 ) {
                wrap_limit( game, game->font_txt, txt, 5, 126, game->color_txt, 310, (int)game->dialog.limit );
            } else {
                wrap_limit( game, game->font_opt, txt, 5, 197, game->color_txt, 310, (int)game->dialog.limit );
            }
        }
        ++phrase_count;
    }
    game->dialog.limit += 1.0f;
    if( game->dialog.limit > game->dialog.phrase_len && ( was_key_pressed( game, APP_KEY_LBUTTON) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
        if( game->dialog.phrase_index < phrase_count - 1 ) {
            ++game->dialog.phrase_index;
            game->dialog.limit = 0;
        } else if( game->dialog.enable_options == 0 ) {
            game->dialog.enable_options = 1;
        }
    } else if( was_key_pressed( game, APP_KEY_LBUTTON) || was_key_pressed( game, APP_KEY_SPACE ) )  {
        game->dialog.limit = (float) game->dialog.phrase_len;
        game->dialog.enable_options = 0;
    }

    if( game->dialog.enable_options == 1 && !was_key_pressed( game, APP_KEY_LBUTTON ) && !was_key_pressed( game, APP_KEY_SPACE ) ) {
        game->dialog.enable_options = 2;
    }

    if( game->dialog.chr_index >= 0 ) {
        yarn_character_t* character = &game->yarn->characters->items[ game->dialog.chr_index ];
        center( game, game->font_name, character->name, 160, 10, game->color_name );
        if( character->face_index >= 0 ) {
            box( game, 115, 26, 90, 90, game->color_facebg );
            draw( game, game->yarn->assets.bitmaps->items[ character->face_index ], 115, 26 );
        }
    }
    
    // say:
    int say = -1;
    if( game->dialog.enable_options == 2 ) {
        if( game->queued_dialog < 0 && game->queued_location < 0 ) {
            if( was_key_pressed( game, APP_KEY_1 ) ) say = 0;
            if( was_key_pressed( game, APP_KEY_2 ) ) say = 1;
            if( was_key_pressed( game, APP_KEY_3 ) ) say = 2;
            if( was_key_pressed( game, APP_KEY_4 ) ) say = 3;
        }

        int c = 0;
        for( int i = 0; i < dialog->say->count; ++i ) {
            if( !test_cond( game, &dialog->say->items[ i ].cond ) ) {
                continue;
            }
            int ypos = 197 + font_height( game, game->font_opt->height ) * c;
            pixelfont_bounds_t b = text( game, game->font_opt, dialog->say->items[ i ].text, 5, ypos, game->color_opt );
            if( game->queued_dialog < 0 && game->queued_location < 0 ) {
                if( mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x < 277 ) {
                    box( game, 4, ypos, 315, b.height, game->color_opt );
                    text( game, game->font_opt, dialog->say->items[ i ].text, 5, ypos, game->color_background );
                    if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                        say = c;
                    }
                }
            }
            ++c;
        }
    }

    // use:
    int use = -1;
    int c = 0;
    for( int i = 0; i < game->state.items->count; ++i ) {
        string txt = game->yarn->item_ids->items[ game->state.items->items[ i ] ];
        uint8_t color = (uint8_t) game->color_disabled;
        bool enabled = false;
        for( int j = 0; j < dialog->use->count; ++j ) {
            if( !test_cond( game, &dialog->use->items[ j ].cond ) ) {
                continue;
            }
            for( int k = 0; k < dialog->use->items[ j ].item_indices->count; ++k ) {
                if( game->state.items->items[ i ] == dialog->use->items[ j ].item_indices->items[ k ] ) {
                    if( game->dialog.enable_options == 2 ) {
                        if( game->queued_dialog < 0 && game->queued_location < 0 ) {
                            color = (uint8_t) game->color_use;
                            enabled = true;
                        }
                    }
                }
            }
        }
        int ypos = 4 + ( ( 117 - ( game->state.items->count * font_height( game, game->font_use->height ) ) ) / 2 ) + c * font_height( game, game->font_use->height );
        pixelfont_bounds_t b = center( game, game->font_use, txt, 287, ypos, color );
        if( enabled && mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x > 259 ) {
            box( game, 260, ypos, 56, b.height, game->color_use );
            center( game, game->font_use, txt, 287, ypos, game->color_background );
            if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                use = c;
            }
        }
        ++c;
    }


    if( was_key_pressed( game, APP_KEY_ESCAPE ) ) {
        return GAMESTATE_TERMINATE;
    }

    if( game->queued_dialog >= 0 && game->dialog.enable_options == 2  ) {
        game->state.current_location = -1;
        if( game->state.current_dialog >= 0 ) {
            game->state.current_dialog = game->queued_dialog;
            game->disable_transition = true;
            return GAMESTATE_DIALOG;
        } else {
            game->state.current_dialog = game->queued_dialog;
            return GAMESTATE_DIALOG;
        }
    }
    else if( game->queued_location >= 0 && game->dialog.enable_options == 2  ) {
        game->state.current_location = game->queued_location;
        game->state.current_dialog = -1;
        return GAMESTATE_LOCATION;
    }

    if( game->dialog.enable_options == 2 ) {

        if( game->queued_dialog < 0 && game->queued_location < 0 ) {
            c = 0;
            for( int i = 0; i < dialog->say->count; ++i ) {
                if( !test_cond( game, &dialog->say->items[ i ].cond ) ) {
                    continue;
                }
                if( c == say ) {
                    do_actions( game, dialog->say->items[ i ].act );
                }
                ++c;
            }

            for( int i = 0; i < game->state.items->count; ++i ) {
                for( int j = 0; j < dialog->use->count; ++j ) {
                    if( !test_cond( game, &dialog->use->items[ j ].cond ) ) {
                        continue;
                    }
                    for( int k = 0; k < dialog->use->items[ j ].item_indices->count; ++k ) {
                        if( game->state.items->items[ i ] == dialog->use->items[ j ].item_indices->items[ k ] ) {
                            if( i == use ) {
                                do_actions( game, dialog->use->items[ j ].act );
                            }
                        }
                    }
                }
            }
            if( game->queued_dialog >= 0 ) {
                game->state.current_location = -1;
                if( game->state.current_dialog >= 0 ) {
                    game->state.current_dialog = game->queued_dialog;
                    game->disable_transition = true;
                    return GAMESTATE_DIALOG;
                } else {
                    game->state.current_dialog = game->queued_dialog;
                    return GAMESTATE_DIALOG;
                }
            } else if( game->queued_location >= 0 ) {
                game->state.current_location = game->queued_location;
                game->state.current_dialog = -1;
                return GAMESTATE_LOCATION;
            }
        }

    }

    return GAMESTATE_NO_CHANGE;
}


gamestate_t exit_init( game_t* game ) {
    (void) game;
    return GAMESTATE_NO_CHANGE;
}


gamestate_t exit_update( game_t* game ) {
    if( was_key_pressed( game, APP_KEY_LBUTTON ) || was_key_pressed( game, APP_KEY_SPACE ) || was_key_pressed( game, APP_KEY_ESCAPE ) ) {
        return GAMESTATE_TERMINATE;
    }
    return GAMESTATE_NO_CHANGE;
}


gamestate_t terminate_init( game_t* game ) {
    (void) game;
    return GAMESTATE_NO_CHANGE;
}


gamestate_t terminate_update( game_t* game ) {
    game->exit_flag = true;
    return GAMESTATE_NO_CHANGE;
}
