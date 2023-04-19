uint32_t blend_rgb( uint32_t color1, uint32_t color2, uint8_t alpha ) {
    uint64_t c1 = (uint64_t) color1;
    uint64_t c2 = (uint64_t) color2;
    uint64_t a = (uint64_t)( alpha );
    // bit magic to alpha blend R G B with single mul
    c1 = ( c1 | ( c1 << 24 ) ) & 0x00ff00ff00ffull;
    c2 = ( c2 | ( c2 << 24 ) ) & 0x00ff00ff00ffull;
    uint64_t o = ( ( ( ( c2 - c1 ) * a ) >> 8 ) + c1 ) & 0x00ff00ff00ffull; 
    return (uint32_t) ( o | ( o >> 24 ) );
}


typedef enum gamestate_t {
    GAMESTATE_NO_CHANGE,
    GAMESTATE_BOOT,
    GAMESTATE_TITLE,
    GAMESTATE_LOCATION,
    GAMESTATE_DIALOG,
    GAMESTATE_EXIT,
    GAMESTATE_TERMINATE,
} gamestate_t;


typedef struct rgbimage_t {
    uint32_t width;
    uint32_t height;
    uint32_t pixels[ 1 ];
} rgbimage_t;


typedef struct stack_entry_t {
    bool is_location;
    int index;
} stack_entry_t;

typedef struct game_t {
    float delta_time;
    bool exit_flag;
    bool exit_requested;
    bool restart_requested;    
    bool quickload_requested;
    gamestate_t current_state;
    gamestate_t new_state;
    bool disable_transition;
    bool ingame_menu;
    int transition_counter;
    uint8_t* screen;
    uint32_t* screen_rgb;
    int screen_width;
    int screen_height;
    audiosys_t* audiosys;
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
        int current_music;
        int logo_index;
        array(bool)* flags;
        array(int)* items;
        array(int)* chars;
        array(stack_entry_t)* section_stack;
    } state, quicksave;
    struct {
        stb_vorbis** ogg_instances;
    } sound_state;
    rgbimage_t** rgbimages;
} game_t;


void game_restart( game_t* game ) {
    audiosys_stop_all( game->audiosys );
    game->state.current_location = -1;
    game->state.current_dialog = -1;
    game->state.current_image = -1;
    game->state.current_music = -1;

    game->state.logo_index = 0;
    array_clear( game->state.flags );
    array_clear( game->state.items  );
    array_clear( game->state.chars  );
    array_clear( game->state.section_stack );
    game->state.current_location = game->yarn->start_location;
    game->state.current_dialog = game->yarn->start_dialog;
    if( game->yarn->is_debug && game->yarn->debug_start_location >= 0 ) {
        game->state.current_location = game->yarn->debug_start_location;
        game->state.current_dialog = -1;
    }
    if( game->yarn->is_debug && game->yarn->debug_start_dialog  >= 0 ) {
        game->state.current_dialog = game->yarn->debug_start_dialog;
        game->state.current_location = -1;
    }
    for( int i = 0; i < game->yarn->flag_ids->count; ++i ) {
        bool value = false;
        if( game->yarn->is_debug ) {
            for( int j = 0; j < game->yarn->globals.debug_set_flags->count; ++j ) {
                if( CMP( game->yarn->flag_ids->items[ i ], game->yarn->globals.debug_set_flags->items[ j ] ) ) {
                    value = true;
                    break;
                }
            }
        }
        array_add( game->state.flags, &value );
    }

    for( int i = 0; i < game->yarn->item_ids->count; ++i ) {
        if( game->yarn->is_debug ) {
            for( int j = 0; j < game->yarn->globals.debug_get_items->count; ++j ) {
                if( CMP( game->yarn->item_ids->items[ i ], game->yarn->globals.debug_get_items->items[ j ] ) ) {
                    array_add( game->state.items, &i );
                    break;
                }
            }
        }
    }

    if( game->yarn->is_debug ) {
        for( int i = 0; i < game->yarn->globals.debug_attach_chars->count; ++i ) {
            for( int j = 0; j < game->yarn->characters->count; ++j ) {
                if( CMP( game->yarn->characters->items[ j ].id, game->yarn->globals.debug_attach_chars->items[ i ] ) ) {
                    array_add( game->state.chars, &j );
                    break;
                }
            }
        }
    }

    game->current_state = GAMESTATE_NO_CHANGE;
    game->new_state = GAMESTATE_BOOT;
    game->disable_transition = false;
    game->transition_counter = 10;
    game->queued_location = -1;
    game->queued_dialog = -1;
    game->restart_requested = false;    
    game->quickload_requested = false;
}


void game_quicksave( game_t* game ) {
    game->quicksave.current_location = game->state.current_location;    
    game->quicksave.current_dialog = game->state.current_dialog;    
    game->quicksave.current_image = game->state.current_image;    
    game->quicksave.logo_index = game->state.logo_index;    
    array_clear( game->quicksave.flags );
    for( int i = 0; i < game->state.flags->count; ++i ) {
        array_add( game->quicksave.flags, &game->state.flags->items[ i ] );
    }
    array_clear( game->quicksave.items );
    for( int i = 0; i < game->state.items->count; ++i ) {
        array_add( game->quicksave.items, &game->state.items->items[ i ] );
    }
    array_clear( game->quicksave.chars );
    for( int i = 0; i < game->state.chars->count; ++i ) {
        array_add( game->quicksave.chars, &game->state.chars->items[ i ] );
    }
    array_clear( game->quicksave.section_stack );
    for( int i = 0; i < game->state.section_stack->count; ++i ) {
        array_add( game->quicksave.section_stack, &game->state.section_stack->items[ i ] );
    }
}


void game_quickload( game_t* game ) {
    game_restart( game );
    game->quickload_requested = false;

    game->state.current_location = game->quicksave.current_location;    
    game->state.current_dialog = game->quicksave.current_dialog;    
    game->state.current_image = game->quicksave.current_image;    
    game->state.logo_index = game->quicksave.logo_index;    
    array_clear( game->state.flags );
    for( int i = 0; i < game->quicksave.flags->count; ++i ) {
        array_add( game->state.flags, &game->quicksave.flags->items[ i ] );
    }
    array_clear( game->state.items );
    for( int i = 0; i < game->quicksave.items->count; ++i ) {
        array_add( game->state.items, &game->quicksave.items->items[ i ] );
    }
    array_clear( game->state.chars );
    for( int i = 0; i < game->quicksave.chars->count; ++i ) {
        array_add( game->state.chars, &game->quicksave.chars->items[ i ] );
    }
    array_clear( game->state.section_stack );
    for( int i = 0; i < game->quicksave.section_stack->count; ++i ) {
        array_add( game->state.section_stack, &game->quicksave.section_stack->items[ i ] );
    }

    if( game->state.current_location >= 0 ) {
        game->new_state = GAMESTATE_LOCATION;
    } else if( game->state.current_dialog >= 0 ) {
        game->new_state = GAMESTATE_DIALOG;
    }
}


void game_init( game_t* game, yarn_t* yarn, input_t* input, audiosys_t* audiosys, uint8_t* screen, uint32_t* screen_rgb, int width, int height ) {
    memset( game, 0, sizeof( *game ) );
    game->delta_time = 0.0f;
    game->exit_flag = false;
    game->exit_requested = false;
    game->screen = screen;
    game->screen_rgb = screen_rgb;
    game->screen_width = width;
    game->screen_height = height;
    game->audiosys = audiosys;
    game->input = input;
    game->yarn = yarn;
    
    game->state.flags = managed_array( bool );
    game->state.items = managed_array( int );
    game->state.chars = managed_array( int );
    game->state.section_stack = managed_array( stack_entry_t );

    game->quicksave.flags = managed_array( bool );
    game->quicksave.items = managed_array( int );
    game->quicksave.chars = managed_array( int );
    game->quicksave.section_stack = managed_array( stack_entry_t );

    game_restart( game );

    game->sound_state.ogg_instances = (stb_vorbis**)manage_alloc( malloc( sizeof( stb_vorbis* ) * game->yarn->assets.audio->count ) );
    memset( game->sound_state.ogg_instances, 0, sizeof( stb_vorbis* ) * game->yarn->assets.audio->count );

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

    if( game->yarn->globals.colormode != YARN_COLORMODE_PALETTE ) {
        game->rgbimages = (rgbimage_t**)manage_alloc( malloc( sizeof( rgbimage_t* ) * game->yarn->assets.bitmaps->count ) );
        for( int i = 0; i < game->yarn->assets.bitmaps->count; ++i ) {
            bool jpeg = game->yarn->globals.colormode == YARN_COLORMODE_RGB && game->yarn->globals.resolution == YARN_RESOLUTION_FULL;
            qoi_data_t* qoi = (qoi_data_t*)game->yarn->assets.bitmaps->items[ i ];
            if( !jpeg ) {        
                qoi_desc desc;
                uint32_t* pixels = (uint32_t*)qoi_decode( qoi->data, qoi->size, &desc, 4 ); 
                rgbimage_t* image = (rgbimage_t*)manage_alloc( malloc( sizeof( rgbimage_t) + ( desc.width * desc.height - 1 ) * sizeof( uint32_t ) ) );
                image->width = desc.width;
                image->height = desc.height;
                uint32_t bgcolor = game->yarn->assets.palette[ game->color_background ];
                for( int i = 0; i < image->width * image->height; ++i ) {
                    uint32_t c = pixels[ i ];
                    uint8_t a = (uint8_t)( c >> 24 );
                    image->pixels[ i ] = blend_rgb( bgcolor, c, a );
                }
                game->rgbimages[ i ] = image;
                free( pixels );
            } else {
                int width, height, c;
                stbi_uc* pixels = stbi_load_from_memory( qoi->data, qoi->size, &width, &height, &c, 4 );
                rgbimage_t* image = (rgbimage_t*)manage_alloc( malloc( sizeof( rgbimage_t) + ( width * height - 1 ) * sizeof( uint32_t ) ) );
                image->width = width;
                image->height = height;
                memcpy( image->pixels, pixels, width * height * sizeof( uint32_t ) );
                game->rgbimages[ i ] = image;
                free( pixels );
            }
        }
    }
}

bool was_key_pressed( game_t* game, int key ) {
    return input_was_key_pressed( game->input, key );
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

void ingame_menu_update( game_t* game );


void audio_ogg_release( void* instance ) {
    stb_vorbis** vorbis = (stb_vorbis**) instance;
    if( *vorbis ) {
        stb_vorbis_close( *vorbis );
        *vorbis = NULL;
    }
}


int audio_ogg_read_samples( void* instance, float* sample_pairs, int sample_pairs_count ) {
    stb_vorbis** vorbis = (stb_vorbis**) instance;
    if( *vorbis ) {
        return stb_vorbis_get_samples_float_interleaved( *vorbis, 2, sample_pairs, sample_pairs_count * 2 );
    } else {
        memset( sample_pairs, 0, sizeof( float ) * 2 * sample_pairs_count );
        return sample_pairs_count;
    }
}


void audio_ogg_restart( void* instance ) {
    stb_vorbis** vorbis = (stb_vorbis**) instance;
    if( *vorbis ) {
        stb_vorbis_seek_start( *vorbis );
    }
}


void audio_ogg_set_position( void* instance, int position_in_sample_pairs ) { 
    stb_vorbis** vorbis = (stb_vorbis**) instance;
    if( *vorbis ) {
        stb_vorbis_seek_frame( *vorbis, position_in_sample_pairs );
    }
}


int audio_ogg_get_position( void* instance ) {
    stb_vorbis** vorbis = (stb_vorbis**) instance;
    if( *vorbis ) {
        return stb_vorbis_get_sample_offset( *vorbis );
    }
    return 0;
}


bool audio_ogg_source( game_t* game, int audio_index, audiosys_audio_source_t* src ) {
    stb_vorbis** vorbis = &game->sound_state.ogg_instances[ audio_index ];
    if( *vorbis == NULL ) {
        audio_data_t* audio_data = game->yarn->assets.audio->items[ audio_index ];
        int error = 0;
        *vorbis = stb_vorbis_open_memory( audio_data->data, audio_data->size, &error, NULL );
    }
    if( *vorbis ) {
        src->instance = vorbis;
        src->release = audio_ogg_release;
        src->read_samples = audio_ogg_read_samples;
        src->restart = audio_ogg_restart;
        src->set_position = audio_ogg_set_position;
        src->get_position = audio_ogg_get_position;
        return true;
    } else {
        return false;
    }
}


void game_update( game_t* game, float delta_time ) {
    if( game->ingame_menu ) {
        ingame_menu_update( game );
        return;
    } else {
        if( was_key_pressed( game, APP_KEY_ESCAPE ) ) {
            game->ingame_menu = true;
            audiosys_pause( game->audiosys );
            if( game->screen ) {
                for( int y = 0; y < game->screen_height; ++y ) {
                    for( int x = 0; x < game->screen_width; ++x ) {
                        if( ( x + y ) & 1 ) {
                            game->screen[ x + y * game->screen_width ] = game->color_background;
                        }
                    }
                }
            } else {
                for( int y = 0; y < game->screen_height; ++y ) {
                    for( int x = 0; x < game->screen_width; ++x ) {
                        game->screen_rgb[ x + y * game->screen_width ] = blend_rgb( game->screen_rgb[ x + y * game->screen_width ], 0x000000, 127 );
                    }
                }
            }
            return;
        }
    }

    game->delta_time = delta_time;

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
        if( game->restart_requested ) {
            game_restart( game );
            return;
        }
        if( game->quickload_requested ) {
            game_quickload( game );
            return;
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
    if( game->restart_requested ) {
        game_restart( game );
        return;
    }
    if( game->quickload_requested ) {
        game_quickload( game );
        return;
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
        uint32_t c = game->yarn->assets.palette[ game->color_background ];
        for( int i = 0; i < game->screen_width * game->screen_height ; ++i ) {
            game->screen_rgb[ i ] = c;
        }
    }
}


void scale_for_resolution( game_t* game, int* x, int* y ) {
    float scale_factors[] = { 1.0f, 1.25f, 1.5f, 2.0f, 4.5f };
    if( x ) {
        *x = (int)( *x * scale_factors[ (int) game->yarn->globals.resolution ] );
    }
    if( y ) {
        *y = (int)( *y * scale_factors[ (int) game->yarn->globals.resolution ] );
    }
}


void scale_for_resolution_inverse( game_t* game, int* x, int* y ) {
    float scale_factors[] = { 1.0f, 1.25f, 1.5f, 2.0f, 4.5f };
    if( x ) {
        *x = (int)( *x / scale_factors[ (int) game->yarn->globals.resolution ] );
    }
    if( y ) {
        *y = (int)( *y / scale_factors[ (int) game->yarn->globals.resolution ] );
    }
}


void draw( game_t* game, int bitmap_index, int x, int y ) {
    scale_for_resolution( game, &x, &y );
    if( game->screen ) {
        palrle_blit( game->yarn->assets.bitmaps->items[ bitmap_index ], x, y, game->screen, game->screen_width, game->screen_height );
    } else {
        rgbimage_t* image = game->rgbimages[ bitmap_index ];
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


void do_audio( game_t* game, array_param(yarn_audio_t)* audio_param ) {
    array(yarn_audio_t)* audio = ARRAY_CAST( audio_param );
    for( int i = 0; i < audio->count; ++i ) {
        if( test_cond( game, &audio->items[ i ].cond ) )  {
            if( audio->items[ i ].stop ) {
                game->state.current_music = -1;
                audiosys_music_stop( game->audiosys, 0.5f );
            } else {
                int music_index = audio->items[ i ].audio_index;
                if( music_index == game->state.current_music ) {
                    if( audio->items[ i ].restart ) {
                        audiosys_music_position_set( game->audiosys, 0.0f );
                    }
                } else {
                    audiosys_audio_source_t src;
                    if( audio_ogg_source( game, music_index, &src ) ) {
                        audiosys_music_switch( game->audiosys, src, 0.5f, 0.0f );
                    } else {
                        audiosys_music_stop( game->audiosys, 0.5f );
                    }
                }
                game->state.current_music = music_index;
            }
        }
    }
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
            case ACTION_TYPE_RETURN: {
                if( game->state.section_stack->count > 1 ) {
                    --game->state.section_stack->count;
                    stack_entry_t entry = game->state.section_stack->items[ --game->state.section_stack->count ];
                    if( entry.is_location ) {
                        game->queued_dialog = -1;
                        game->queued_location = entry.index;
                    } else {
                        game->queued_location = -1;
                        game->queued_dialog = entry.index;
                    }
                }
            } break;
            case ACTION_TYPE_RESTART: {
                game->restart_requested = true;    
            } break;
            case ACTION_TYPE_QUICKSAVE: {
                game_quicksave( game );
            } break;
            case ACTION_TYPE_QUICKLOAD: {
                game->quickload_requested = true;
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
            case ACTION_TYPE_CHAR_ATTACH: {
                bool found = false;
                for( int j = 0;  j < game->state.chars->count; ++j ) {
                    if( game->state.chars->items[ j ] == action->param_char_index ) {
                        found = true;
                        break;
                    }
                }
                if( !found ) {
                    array_add( game->state.chars, &action->param_char_index );
                }
            } break;
            case ACTION_TYPE_CHAR_DETACH: {
                for( int j = 0;  j < game->state.chars->count; ++j ) {
                    if( game->state.chars->items[ j ] == action->param_char_index ) {
                        array_remove( game->state.chars, j );
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



void ingame_menu_update( game_t* game ) {
    box( game, 104, 46, 123, 163, game->color_background );
    box( game, 99, 39, 124, 164, game->color_background );
    box( game, 100, 40, 121, 161, game->color_opt );
    box( game, 102, 42, 118, 158, game->color_background );
    int spacing = 25;
    int ypos = 50 + ( 160 - ( 5 * spacing ) ) / 2 - spacing;
    center( game, game->font_name, "RESUME", 160, ypos+=spacing, game->color_opt );
    center( game, game->font_name, "SAVE GAME", 160, ypos+=spacing, game->color_opt );
    center( game, game->font_name, "LOAD GAME", 160, ypos+=spacing, game->color_opt );
    center( game, game->font_name, "RESTART", 160, ypos+=spacing, game->color_opt );
    center( game, game->font_name, "QUIT", 160, ypos+=spacing, game->color_opt );
    if( was_key_pressed( game, APP_KEY_ESCAPE ) || was_key_pressed( game, APP_KEY_1 ) ) {
        game->ingame_menu = false;
        audiosys_resume( game->audiosys );
    } else if( was_key_pressed( game, APP_KEY_2 ) ) {
    } else if( was_key_pressed( game, APP_KEY_3 ) ) {
    } else if( was_key_pressed( game, APP_KEY_4 ) ) {
    } else if( was_key_pressed( game, APP_KEY_5 ) ) {
        game->new_state = GAMESTATE_TERMINATE;
        game->ingame_menu = false;
    }
}



// boot
gamestate_t boot_init( game_t* game ) {
    game->state.logo_index = -1;
    cls( game );
    return GAMESTATE_NO_CHANGE;
}


gamestate_t boot_update( game_t* game ) {
    (void) game;
    #ifdef __wasm__
        static int blink_count = 0;
        static int blink_wait = 100;
        static bool visible = true;
	    if( blink_count > 0 ) {
		    --blink_count;
		    if( blink_count > 0 ) {
			    visible = blink_count % 30 < 15;
		    } else {
			    visible = true;
		    }
	    } else {
		    --blink_wait;
		    if( blink_wait <= 0 ) {
			    blink_wait = 200;
			    blink_count = 100;
		    }
	    }
        cls( game );
        if( visible ) {
            center( game, game->yarn->assets.font_name, "CLICK TO START", 160, 120 - game->yarn->assets.font_name->height / 2, game->color_name );
        }
        if( !was_key_pressed( game, APP_KEY_LBUTTON ) ) {
            return GAMESTATE_NO_CHANGE;
        }
    #endif

    if( game->yarn->screen_names->count > 0 && !( game->yarn->is_debug && ( game->yarn->debug_start_dialog >= 0 || game->yarn->debug_start_location >= 0  ) ) ) {
        audiosys_audio_source_t src;
        if( audio_ogg_source( game, game->yarn->globals.logo_music, &src ) ) {
            audiosys_music_play( game->audiosys, src, 0.0f );
            game->state.current_music = game->yarn->globals.logo_music;
        } else {
            game->state.current_music = -1;
        }
        return GAMESTATE_TITLE;
    } else if( game->state.current_location >= 0 ) {
        return GAMESTATE_LOCATION;
    } else if( game->state.current_dialog >= 0 ) {
        return GAMESTATE_DIALOG;
    } else {
        return GAMESTATE_EXIT;
    }
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
        draw( game, game->yarn->globals.logo_indices->items[ game->state.logo_index ], 0, 0 );
    }

    if( was_key_pressed( game, APP_KEY_LBUTTON ) || was_key_pressed( game, APP_KEY_SPACE ) ) {
        if( game->state.logo_index < game->yarn->globals.logo_indices->count - 1 ) {
            return GAMESTATE_TITLE;
        } else {
            if( game->state.current_location >= 0 ) {
                return GAMESTATE_LOCATION;
            } else if( game->state.current_dialog >= 0 ) {
                return GAMESTATE_DIALOG;
            }
        }
    }
    return GAMESTATE_NO_CHANGE;
}


// location
gamestate_t location_init( game_t* game ) {
    stack_entry_t entry;
    entry.is_location = true;
    entry.index = game->state.current_location;
    array_add( game->state.section_stack, &entry );

    game->queued_location = -1;
    game->queued_dialog = -1;

    yarn_t* yarn = game->yarn;
    yarn_location_t* location = &yarn->locations->items[ game->state.current_location ];

    // mus: amb: snd:
    do_audio( game, location->audio );

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
        draw( game, yarn->globals.background_location, 0, 0 );
    }

    // img:
    for( int i = 0; i < location->img->count; ++i ) {
        if( test_cond( game, &location->img->items[ i ].cond ) )  {
            game->state.current_image = location->img->items[ i ].image_index;
        }
    }
    if( game->state.current_image >= 0 ) {
        draw( game, game->state.current_image, 64, 10 );
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
                box( game, 4, ypos - 1, 315, b.height + 1, game->color_opt );
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
            box( game, 260, ypos - 1, 56, b.height + 1, game->color_use );
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
                box( game, 4, ypos - 1, 56, b.height + 1, game->color_chr );
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

    // companions
    for( int i = 0; i < game->state.chars->count; ++i ) {
        bool found = false;
        for( int j = 0; j < location->chr->count; ++j ) {
            if( game->state.chars->items[ i ] == location->chr->items[ j ].chr_indices->items[ 0 ] && test_cond( game, &location->chr->items[ j ].cond ) ) {
                found = true;
                break;
            }
        }

        if( !found ) {
            int ypos = 4 + ( ( 117 - ( chr_count * font_height( game, game->font_chr->height ) ) ) / 2 ) + c * font_height( game, game->font_chr->height );
            int color = game->color_disabled;
            pixelfont_bounds_t b = center( game,  game->font_chr, game->yarn->characters->items[ game->state.chars->items[ i ] ].short_name, 32, ypos, color );
            ++c;
        }
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
    stack_entry_t entry;
    entry.is_location = false;
    entry.index = game->state.current_dialog;
    array_add( game->state.section_stack, &entry );

    game->queued_location = -1;
    game->queued_dialog = -1;

    game->dialog.limit = -30.0f;
    game->dialog.phrase_index = 0;
    game->dialog.phrase_len = -1;
    game->dialog.chr_index = -1;
    game->dialog.enable_options = 0;

    yarn_t* yarn = game->yarn;
    yarn_dialog_t* dialog = &yarn->dialogs->items[ game->state.current_dialog ];

    //  mus: amb: snd:
    do_audio( game, dialog->audio );

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

    if( game->dialog.phrase_len == 0 ) {
        game->dialog.enable_options = 1;
        game->dialog.limit = 0.0f;
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
        draw( game, yarn->globals.background_dialog, 0, 0 );
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
                wrap_limit( game, game->font_txt, txt, 5, 136, game->color_txt, 310, game->dialog.limit < 0.0f ? 0 : (int)game->dialog.limit );
            } else {
                wrap_limit( game, game->font_opt, txt, 5, 197, game->color_txt, 310, game->dialog.limit < 0.0f ? 0 : (int)game->dialog.limit );
            }
        }
        ++phrase_count;
    }
    game->dialog.limit += game->delta_time * 80.0f;
    if( game->dialog.limit > game->dialog.phrase_len && ( was_key_pressed( game, APP_KEY_LBUTTON) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
        if( game->dialog.phrase_index < phrase_count - 1 ) {
            ++game->dialog.phrase_index;
            game->dialog.limit = -30.0f;
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
        center( game, game->font_name, character->name, 160, 6, game->color_name );
        if( character->face_index >= 0 ) {
            box( game, 104, 18, 112, 112, game->color_facebg );
            draw( game, character->face_index, 104, 18 );
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
                    box( game, 4, ypos - 1, 315, b.height + 1, game->color_opt );
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
            box( game, 260, ypos - 1, 56, b.height + 1, game->color_use );
            center( game, game->font_use, txt, 287, ypos, game->color_background );
            if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                use = c;
            }
        }
        ++c;
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
