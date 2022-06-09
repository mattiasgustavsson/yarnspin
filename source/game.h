
typedef struct game_state_t {
	int current_location;
    int current_dialog;
    int current_image;
    int logo_index;
    array(bool)* flags;
    array(int)* items;
} game_state_t;	


void game_state_init( game_state_t* game_state ) {
	game_state->current_location = -1;
    game_state->current_dialog = -1;
    game_state->current_image = -1;
    game_state->logo_index = 0;
    game_state->flags = managed_array( bool );
    game_state->items = managed_array( int );
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


typedef struct game_t {
	bool exit_flag;
    gamestate_t current_state;
    gamestate_t new_state;
    bool disable_transition;
    int transition_counter;
	uint8_t* screen;
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
	game_state_t* game_state;
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
} game_t;


void game_init( game_t* game, game_state_t* game_state, yarn_t* yarn, input_t* input, uint8_t* screen, int width, int height ) {
	game_state->current_location = yarn->start_location;
	game_state->current_dialog = yarn->start_dialog;
	for( int i = 0; i < yarn->flag_ids->count; ++i ) {
        bool value = false;
		array_add( game_state->flags, &value );
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
    game->current_state = GAMESTATE_NO_CHANGE;
    game->new_state = GAMESTATE_BOOT;
    game->disable_transition = false;
    game->transition_counter = 10;
	game->screen = screen;
	game->screen_width = width;
	game->screen_height = height;
	game->input = input;
	game->yarn = yarn;
	game->game_state = game_state;
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


gamestate_t boot_init( game_t* ctx );
gamestate_t boot_update( game_t* ctx );
gamestate_t title_init( game_t* ctx );
gamestate_t title_update( game_t* ctx );
gamestate_t location_init( game_t* ctx );
gamestate_t location_update( game_t* ctx );
gamestate_t dialog_init( game_t* ctx );
gamestate_t dialog_update( game_t* ctx );
gamestate_t exit_init( game_t* ctx );
gamestate_t exit_update( game_t* ctx );
gamestate_t terminate_init( game_t* ctx );
gamestate_t terminate_update( game_t* ctx );


bool game_update( game_t* game ) {
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
        return game->exit_flag;
    }
    if( game->transition_counter < 10 ) {
        game->transition_counter++;
    }
    if( game->transition_counter < 0 ) {
        return game->exit_flag;
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
    return game->exit_flag;
}


void cls( game_t* ctx ) { 
    memset( ctx->screen, ctx->color_background, (size_t) ctx->screen_width * ctx->screen_height ); 
}


void draw( game_t* ctx, palrle_data_t* bmp, int x, int y ) {
    palrle_blit( bmp, x, y, ctx->screen, ctx->screen_width, ctx->screen_height );
}

	
void box( game_t* ctx, int x, int y, int w, int h, int c ) { 
	for( int iy = 0; iy < h; ++iy ) {
		for( int ix = 0; ix < w; ++ix ) {
			int xp = x + ix;
			int yp = y + iy;
			if( xp >= 0 && xp < ctx->screen_width && yp >= 0 && yp < ctx->screen_height ) {
				ctx->screen[ xp + yp * ctx->screen_width ] = (uint8_t) c;
            }
		}
	}
}


pixelfont_bounds_t center( game_t* ctx, pixelfont_t* font, string str, int x, int y, int color ) { 
    pixelfont_bounds_t bounds;
    pixelfont_blit( font, x, y, str, (uint8_t)color, ctx->screen, ctx->screen_width, ctx->screen_height, 
        PIXELFONT_ALIGN_CENTER, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    return bounds; 
}


pixelfont_bounds_t center_wrap( game_t* ctx, pixelfont_t* font, string str, int x, int y, int color, int wrap_width ) { 
    pixelfont_bounds_t bounds;
    x -= wrap_width / 2;
    pixelfont_blit( font, x, y, str, (uint8_t)color, ctx->screen, ctx->screen_width, ctx->screen_height, 
        PIXELFONT_ALIGN_CENTER, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, 
        &bounds );
    return bounds; 
}


pixelfont_bounds_t text( game_t* ctx, pixelfont_t* font, string str, int x, int y, int color ) { 
    pixelfont_bounds_t bounds;
    pixelfont_blit( font, x, y, str, (uint8_t)color, ctx->screen, ctx->screen_width, ctx->screen_height, 
        PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    return bounds; 
}


void wrap( game_t* ctx, pixelfont_t* font, string str, int x, int y, int color, int wrap_width ) { 
    pixelfont_blit( font, x, y, str, (uint8_t)color, ctx->screen, ctx->screen_width, ctx->screen_height, 
        PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, 
        NULL );
}


void wrap_limit( game_t* ctx, pixelfont_t* font, string str, int x, int y, int color, int wrap_width, int limit ) { 
    pixelfont_blit( font, x, y, str, (uint8_t)color, ctx->screen, ctx->screen_width, ctx->screen_height, 
        PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, limit, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, 
        PIXELFONT_UNDERLINE_OFF, NULL );
}


bool was_key_pressed( game_t* ctx, int key ) { 
    return input_was_key_pressed( ctx->input, key ); 
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


bool test_cond( game_t* ctx, yarn_cond_t* cond ) {		
	bool result = true;
	for( int i = 0; i < cond->ands->count; ++i )		
		{
		bool or_result = false;
		yarn_cond_or_t* ors = &cond->ands->items[ i ];
		for( int j = 0; j < ors->flags->count; ++j ) {
			yarn_cond_flag_t* flag = &ors->flags->items[ j ];
			bool flag_val = ctx->game_state->flags->items[ flag->flag_index ];
			if( flag->is_not ) {
                flag_val = !flag_val;
            }
			or_result = or_result || flag_val;
		}
		result = result && or_result;
	}
	return result;
}		
	

bool do_actions( game_t* ctx, array_param(yarn_act_t)* act_param ) {
    array(yarn_act_t)* act = ARRAY_CAST( act_param );
	for( int i = 0; i < act->count; ++i ) {
		yarn_act_t* action = &act->items[ i ];
		if( !test_cond( ctx, &action->cond ) ) {
            continue;
        }
		game_state_t* state= ctx->game_state;
		switch( action->type ) {
			case ACTION_TYPE_GOTO_LOCATION: {
				ctx->queued_dialog = -1;
			    ctx->queued_location = action->param_location_index;
            } break;
			case ACTION_TYPE_GOTO_DIALOG: {
				ctx->queued_location = -1;
				ctx->queued_dialog = action->param_dialog_index;
            } break;
			case ACTION_TYPE_EXIT: {
				return false;
            } break;
			case ACTION_TYPE_FLAG_SET: {
				state->flags->items[ action->param_flag_index ] = true;
            } break;
			case ACTION_TYPE_FLAG_CLEAR: {
				state->flags->items[ action->param_flag_index ] = false;
            } break;
			case ACTION_TYPE_FLAG_TOGGLE: {
				state->flags->items[ action->param_flag_index ] = !state->flags->items[ action->param_flag_index ];
            } break;
			case ACTION_TYPE_ITEM_GET: {
				bool found = false;
				for( int j = 0;  j < state->items->count; ++j ) {
					if( state->items->items[ j ] == action->param_item_index ) {
                        found = true;
                        break;
                    }
				}
				if( !found ) {
                    array_add( state->items, &action->param_item_index );
                }
			} break;
			case ACTION_TYPE_ITEM_DROP: {
				for( int j = 0;  j < state->items->count; ++j ) {
					if( state->items->items[ j ] == action->param_item_index ) {
						array_remove( state->items, j );
						break;
					}
				}
            } break;
			case ACTION_TYPE_NONE: {
				break;
            }
		}		
	}		
    return true;
}


// boot
gamestate_t boot_init( game_t* ctx ) {
	cls( ctx );
	ctx->game_state->logo_index = -1;
	if( ctx->yarn->screen_names->count > 0 ) {
		return GAMESTATE_TITLE;
    } else if( ctx->game_state->current_location >= 0 ) {
		return GAMESTATE_LOCATION;
    } else if( ctx->game_state->current_dialog >= 0 ) {
		return GAMESTATE_DIALOG;
    } else {
		return GAMESTATE_EXIT;
	}
}


gamestate_t boot_update( game_t* ctx ) {
    (void) ctx;
    return GAMESTATE_NO_CHANGE;
}


// title
gamestate_t title_init( game_t* ctx ) {
	++ctx->game_state->logo_index;
    return GAMESTATE_NO_CHANGE;
}

gamestate_t title_update( game_t* ctx ) {
	if( ctx->game_state->logo_index < ctx->yarn->globals.logo_indices->count ) {
		cls( ctx );
		palrle_data_t* img = ctx->yarn->assets.bitmaps->items[ ctx->yarn->globals.logo_indices->items[ ctx->game_state->logo_index ] ];
		draw( ctx, img, 0, 0 );
	}

    if( was_key_pressed( ctx, APP_KEY_LBUTTON ) || was_key_pressed( ctx, APP_KEY_SPACE ) || was_key_pressed( ctx, APP_KEY_ESCAPE ) ) {
		if( ctx->game_state->logo_index < ctx->yarn->globals.logo_indices->count - 1 ) {
			return GAMESTATE_TITLE;
        } else {
			return GAMESTATE_LOCATION;
		}
	}
    return GAMESTATE_NO_CHANGE;
}	


// location
gamestate_t location_init( game_t* ctx ) {
    ctx->queued_location = -1;
    ctx->queued_dialog = -1;

  	yarn_t* yarn = ctx->yarn;		
	game_state_t* state= ctx->game_state;
	yarn_location_t* location = &yarn->locations->items[ state->current_location ];
	
    // act:
	do_actions( ctx, location->act );
    return GAMESTATE_NO_CHANGE;
}
				

gamestate_t location_update( game_t* ctx ) {
    cls( ctx );
    	
	yarn_t* yarn = ctx->yarn;		
	game_state_t* state= ctx->game_state;
		
	if( state->current_location < 0 && state->current_dialog >= 0 ) {
		return GAMESTATE_DIALOG;
	}
			
	if( ctx->game_state->current_location < 0  ) return GAMESTATE_TERMINATE;

    yarn_location_t* location = &yarn->locations->items[ state->current_location ];
		
	int mouse_x = input_get_mouse_x( ctx->input );
	int mouse_y = input_get_mouse_y( ctx->input );

    if( ctx->queued_dialog >= 0 && ( was_key_pressed( ctx, APP_KEY_LBUTTON ) || was_key_pressed( ctx, APP_KEY_SPACE ) ) ) {
        state->current_location = -1;
		if( state->current_dialog >= 0 ) {
            state->current_dialog = ctx->queued_dialog;
            ctx->disable_transition = true;
			return GAMESTATE_DIALOG;
        } else {
            state->current_dialog = ctx->queued_dialog;
			return GAMESTATE_DIALOG;
        }
    } else if( ctx->queued_location >= 0 && ( was_key_pressed( ctx, APP_KEY_LBUTTON ) || was_key_pressed( ctx, APP_KEY_SPACE ) ) ) {
        state->current_location = ctx->queued_location;
        state->current_dialog = -1;
		return GAMESTATE_LOCATION;
    }
		

    // background_location:
	if( yarn->globals.background_location >= 0 ) {
		draw( ctx, yarn->assets.bitmaps->items[ yarn->globals.background_location ], 0, 0 );
    }

	// img:
    for( int i = 0; i < location->img->count; ++i ) {
    	if( test_cond( ctx, &location->img->items[ i ].cond ) )  {
    		ctx->game_state->current_image = location->img->items[ i ].image_index;
        }
    }
    if( ctx->game_state->current_image >= 0 ) {
        draw( ctx, yarn->assets.bitmaps->items[ ctx->game_state->current_image ], 64, 4 );
    }


	// txt:
	string txt = "";
    for( int i = 0; i < location->txt->count; ++i ) {
    	if( !test_cond( ctx, &location->txt->items[ i ].cond ) ) {
            continue;
        }
    	txt = cstr_cat( txt, cstr_cat( location->txt->items[ i ].text, "\n" ) );
    }
	wrap( ctx, ctx->font_txt, txt, 5, 116, ctx->color_txt, 310 );
		
	// opt:
	int opt = -1;
	if( ctx->queued_dialog < 0 && ctx->queued_location < 0 ) {
		if( was_key_pressed( ctx, APP_KEY_1 ) ) opt = 0;
		if( was_key_pressed( ctx, APP_KEY_2 ) ) opt = 1;
		if( was_key_pressed( ctx, APP_KEY_3 ) ) opt = 2;
		if( was_key_pressed( ctx, APP_KEY_4 ) ) opt = 3;

		int c = 0;
		for( int i = 0; i < location->opt->count; ++i ) {
			if( !test_cond( ctx, &location->opt->items[ i ].cond ) ) {
                continue;
            }
			int ypos = 161 + ctx->font_opt->height * c;
			pixelfont_bounds_t b = text( ctx, ctx->font_opt, location->opt->items[ i ].text, 5, ypos, ctx->color_opt );
			if( mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x < 277 ) {
				box( ctx, 4, ypos, 315, b.height, ctx->color_opt );
				text( ctx, ctx->font_opt, location->opt->items[ i ].text, 5, ypos, ctx->color_background );
				if( was_key_pressed( ctx, APP_KEY_LBUTTON ) ) {
					opt = c;
                }
			}
			++c;
		}
    }

	// use:
	int use = -1;
	int c = 0;
    for( int i = 0; i < state->items->count; ++i ) {
    	string usetxt = ctx->yarn->item_ids->items[ state->items->items[ i ] ];
    	uint8_t color = (uint8_t) ctx->color_disabled;
    	bool enabled = false;
    	for( int j = 0; j < location->use->count; ++j ) {
    		if( !test_cond( ctx, &location->use->items[ j ].cond ) ) {
                continue;
            }
    		for( int k = 0; k < location->use->items[ j ].item_indices->count; ++k ) {
    			if( state->items->items[ i ] == location->use->items[ j ].item_indices->items[ k ] ) {
		            if( ctx->queued_dialog < 0 && ctx->queued_location < 0 ) {
    					color = (uint8_t) ctx->color_use;
    					enabled = true;
    				}
                }
            }
        }
    	int ypos = 40 + c * ctx->font_use->height;
		pixelfont_bounds_t b = center( ctx, ctx->font_use, usetxt, 287, ypos, color );
		if( enabled && mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x > 259 ) {
			box( ctx, 260, ypos, 56, b.height, ctx->color_use );
			center( ctx, ctx->font_use, usetxt, 287, ypos, ctx->color_background );
			if( was_key_pressed( ctx, APP_KEY_LBUTTON ) ) {
				use = c;
            }
		}
		++c;
    }

    // chr:
	int chr = -1;
	c = 0;
    for( int i = 0; i < location->chr->count; ++i ) {
    	if( !test_cond( ctx, &location->chr->items[ i ].cond ) ) {
            continue;
        }
    	int ypos = 40 + c * ctx->font_chr->height;
        int color = ctx->color_chr;
        if( ctx->queued_dialog >= 0 || ctx->queued_location >= 0 ) {
            color = ctx->color_disabled;
        }

		pixelfont_bounds_t b = center( ctx,  ctx->font_chr, ctx->yarn->characters->items[ location->chr->items[ i ].chr_indices->items[ 0 ] ].short_name, 32, ypos, color );
    	if( ctx->queued_dialog < 0 && ctx->queued_location < 0 ) {
			if( mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x < 60 ) {
				box( ctx, 4, ypos, 56, b.height, ctx->color_chr );
				center( ctx,  ctx->font_chr, ctx->yarn->characters->items[ location->chr->items[ i ].chr_indices->items[ 0 ] ].short_name, 32, ypos, ctx->color_background );
				if( was_key_pressed( ctx, APP_KEY_LBUTTON ) ) {
					chr = c;
                }
			}
        }
        ++c;
    }
    if( c == 0 ) {
		center_wrap( ctx,  ctx->font_chr, yarn->globals.alone_text, 32, 40, ctx->color_disabled, 56 );
    }


    if( was_key_pressed( ctx, APP_KEY_ESCAPE ) ) {
        return GAMESTATE_TERMINATE;
    }

	if( ctx->queued_dialog < 0 && ctx->queued_location < 0 ) {
		c = 0;
		for( int i = 0; i < location->opt->count; ++i ) {
			if( !test_cond( ctx, &location->opt->items[ i ].cond ) ) {
                continue;
            }
			if( c == opt ) {
                do_actions( ctx, location->opt->items[ i ].act );
            }
			++c;
		}
			
		c = 0;
		for( int i = 0; i < location->chr->count; ++i ) {
			if( !test_cond( ctx, &location->chr->items[ i ].cond ) ) {
                continue;
            }
			if( c == chr ) {
                do_actions( ctx, location->chr->items[ i ].act );
            }
			++c;
		}

    	for( int i = 0; i < state->items->count; ++i ) {
    		for( int j = 0; j < location->use->count; ++j ) {
    			if( !test_cond( ctx, &location->use->items[ j ].cond ) ) {
                    continue;
                }
    			for( int k = 0; k < location->use->items[ j ].item_indices->count; ++k ) {
    				if( state->items->items[ i ] == location->use->items[ j ].item_indices->items[ k ] ) {
    					if( i == use ) {
                            do_actions( ctx, location->use->items[ j ].act );
                        }
    				}
                }
    		}
		}

        if( ctx->queued_dialog >= 0 ) {
            state->current_location = -1;
		    if( state->current_dialog >= 0 ) {
                state->current_dialog = ctx->queued_dialog;
                ctx->disable_transition = true;
			    return GAMESTATE_DIALOG;
            } else {
                state->current_dialog = ctx->queued_dialog;
			    return GAMESTATE_DIALOG;
            }
        } else if( ctx->queued_location >= 0 ) {
            state->current_location = ctx->queued_location;
            state->current_dialog = -1;
			return GAMESTATE_LOCATION;
        }
	}

    return GAMESTATE_NO_CHANGE;
}


// dialog
gamestate_t dialog_init( game_t* ctx ) {
    ctx->queued_location = -1;
    ctx->queued_dialog = -1;

    ctx->dialog.limit = 0.0f;
	ctx->dialog.phrase_index = 0;
	ctx->dialog.phrase_len = -1;
    ctx->dialog.chr_index = -1;
    ctx->dialog.enable_options = 0;

	yarn_t* yarn = ctx->yarn;		
	game_state_t* state= ctx->game_state;
	yarn_dialog_t* dialog = &yarn->dialogs->items[ state->current_dialog ];

	// act:
	do_actions( ctx, dialog->act );

    for( int i = 0; i < dialog->phrase->count; ++i ) {
    	if( !test_cond( ctx, &dialog->phrase->items[ i ].cond ) ) {
            continue;
        }
    	if( ctx->dialog.phrase_len < 0 ) {
            ctx->dialog.phrase_len = (int) cstr_len( dialog->phrase->items[ i ].text );
        }
		if( dialog->phrase->items[ i ].character_index >= 0 ) {
			ctx->dialog.chr_index = dialog->phrase->items[ i ].character_index;
			break;
		}
	}

    return GAMESTATE_NO_CHANGE;
}
		

gamestate_t dialog_update( game_t* ctx ) {
	if( ctx->game_state->current_dialog < 0 ) {
        return GAMESTATE_TERMINATE;
    }

    cls( ctx );
	yarn_t* yarn = ctx->yarn;		
	game_state_t* state= ctx->game_state;

	yarn_dialog_t* dialog = &yarn->dialogs->items[ state->current_dialog ];

	int mouse_x = input_get_mouse_x( ctx->input );
	int mouse_y = input_get_mouse_y( ctx->input );
		
		
	// background_dialog:
	if( yarn->globals.background_dialog >= 0 ) {
		draw( ctx, ctx->yarn->assets.bitmaps->items[ yarn->globals.background_dialog ], 0, 0 );
    }
			
	// phrase:
	int phrase_count = 0;
    for( int i = 0; i < dialog->phrase->count; ++i ) {
    	if( !test_cond( ctx, &dialog->phrase->items[ i ].cond ) ) {
            continue;
        }
    	if( phrase_count == ctx->dialog.phrase_index ) {
    		string txt = dialog->phrase->items[ i ].text;
    		if( dialog->phrase->items[ i ].character_index >= 0 ) {
    			ctx->dialog.chr_index = dialog->phrase->items[ i ].character_index;
            }
    		ctx->dialog.phrase_len = (int) cstr_len( txt );
    		if( dialog->phrase->items[ i ].character_index >= 0 ) {
    			wrap_limit( ctx, ctx->font_txt, txt, 5, 116, ctx->color_txt, 310, (int)ctx->dialog.limit );
            } else {
				wrap_limit( ctx, ctx->font_opt, txt, 5, 161, ctx->color_txt, 310, (int)ctx->dialog.limit );
            }
    	}
		++phrase_count;
    }
	ctx->dialog.limit += 1.0f;
    if( ctx->dialog.limit > ctx->dialog.phrase_len && ( was_key_pressed( ctx, APP_KEY_LBUTTON) || was_key_pressed( ctx, APP_KEY_SPACE ) ) ) { 
    	if( ctx->dialog.phrase_index < phrase_count - 1 ) {
    		++ctx->dialog.phrase_index; 
    		ctx->dialog.limit = 0; 
		} else if( ctx->dialog.enable_options == 0 ) {
			ctx->dialog.enable_options = 1;
		}
    } else if( was_key_pressed( ctx, APP_KEY_LBUTTON) || was_key_pressed( ctx, APP_KEY_SPACE ) )  {
    	ctx->dialog.limit = (float) ctx->dialog.phrase_len;
        ctx->dialog.enable_options = 0;
    }

    if( ctx->dialog.enable_options == 1 && !was_key_pressed( ctx, APP_KEY_LBUTTON ) && !was_key_pressed( ctx, APP_KEY_SPACE ) ) {
        ctx->dialog.enable_options = 2;
	}	

    if( ctx->dialog.chr_index >= 0 ) {
    	yarn_character_t* character = &ctx->yarn->characters->items[ ctx->dialog.chr_index ];
		center( ctx, ctx->font_name, character->name, 160, 4, ctx->color_name );
		if( character->face_index >= 0 ) {
			box( ctx, 115, 17, 90, 90, ctx->color_facebg );
			draw( ctx, ctx->yarn->assets.bitmaps->items[ character->face_index ], 115, 17 );
		}
	}
			
	// say:
	int say = -1;
    if( ctx->dialog.enable_options == 2 ) {
		if( ctx->queued_dialog < 0 && ctx->queued_location < 0 ) {
			if( was_key_pressed( ctx, APP_KEY_1 ) ) say = 0;
			if( was_key_pressed( ctx, APP_KEY_2 ) ) say = 1;
			if( was_key_pressed( ctx, APP_KEY_3 ) ) say = 2;
			if( was_key_pressed( ctx, APP_KEY_4 ) ) say = 3;
        }

		int c = 0;
		for( int i = 0; i < dialog->say->count; ++i ) {
			if( !test_cond( ctx, &dialog->say->items[ i ].cond ) ) {
                continue;
            }
			int ypos = 161 + ctx->font_opt->height * c;
			pixelfont_bounds_t b = text( ctx, ctx->font_opt, dialog->say->items[ i ].text, 5, ypos, ctx->color_opt );
    		if( ctx->queued_dialog < 0 && ctx->queued_location < 0 ) {
				if( mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x < 277 ) {
					box( ctx, 4, ypos, 315, b.height, ctx->color_opt );
					text( ctx, ctx->font_opt, dialog->say->items[ i ].text, 5, ypos, ctx->color_background );
					if( was_key_pressed( ctx, APP_KEY_LBUTTON ) ) {
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
    for( int i = 0; i < state->items->count; ++i ) {
    	string txt = ctx->yarn->item_ids->items[ state->items->items[ i ] ];
    	uint8_t color = (uint8_t) ctx->color_disabled;
    	bool enabled = false;
    	for( int j = 0; j < dialog->use->count; ++j ) {
    		if( !test_cond( ctx, &dialog->use->items[ j ].cond ) ) {
                continue;
            }
    		for( int k = 0; k < dialog->use->items[ j ].item_indices->count; ++k ) {
    			if( state->items->items[ i ] == dialog->use->items[ j ].item_indices->items[ k ] ) {
    				if( ctx->dialog.enable_options == 2 ) {
		                if( ctx->queued_dialog < 0 && ctx->queued_location < 0 ) {
    						color = (uint8_t) ctx->color_use;
    						enabled = true;
                        }
    				}
    			}
            }
    	}
    	int ypos = 40 + c * ctx->font_use->height;
		pixelfont_bounds_t b = center( ctx, ctx->font_use, txt, 287, ypos, color );
		if( enabled && mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x > 259 ) {
			box( ctx, 260, ypos, 56, b.height, ctx->color_use );
			center( ctx, ctx->font_use, txt, 287, ypos, ctx->color_background );
			if( was_key_pressed( ctx, APP_KEY_LBUTTON ) ) {
				use = c;
            }
		}
		++c;
    }


	if( was_key_pressed( ctx, APP_KEY_ESCAPE ) ) {
        return GAMESTATE_TERMINATE;
    }

    if( ctx->queued_dialog >= 0 && ctx->dialog.enable_options == 2  ) {
        state->current_location = -1;
		if( state->current_dialog >= 0 ) {
            state->current_dialog = ctx->queued_dialog;
			ctx->disable_transition = true;
            return GAMESTATE_DIALOG;
        } else {
            state->current_dialog = ctx->queued_dialog;
            return GAMESTATE_DIALOG;
        }
    }
    else if( ctx->queued_location >= 0 && ctx->dialog.enable_options == 2  ) {
        state->current_location = ctx->queued_location;
        state->current_dialog = -1;
        return GAMESTATE_LOCATION;
    }

    if( ctx->dialog.enable_options == 2 ) {

		if( ctx->queued_dialog < 0 && ctx->queued_location < 0 ) {
		    c = 0;
		    for( int i = 0; i < dialog->say->count; ++i ) {
			    if( !test_cond( ctx, &dialog->say->items[ i ].cond ) ) {
                    continue;
                }
			    if( c == say ) {
                    do_actions( ctx, dialog->say->items[ i ].act );
                }
			    ++c;
			}
			
    	    for( int i = 0; i < state->items->count; ++i ) {
    		    for( int j = 0; j < dialog->use->count; ++j ) {
    			    if( !test_cond( ctx, &dialog->use->items[ j ].cond ) ) {
                        continue;
                    }
    			    for( int k = 0; k < dialog->use->items[ j ].item_indices->count; ++k ) {
    				    if( state->items->items[ i ] == dialog->use->items[ j ].item_indices->items[ k ] ) {
    					    if( i == use ) {
                                do_actions( ctx, dialog->use->items[ j ].act );
                            }
    					}
    			    }
			    }
            }
            if( ctx->queued_dialog >= 0 ) {
                state->current_location = -1;
		        if( state->current_dialog >= 0 ) {
                    state->current_dialog = ctx->queued_dialog;
			        ctx->disable_transition = true;
                    return GAMESTATE_DIALOG;
                } else {
                    state->current_dialog = ctx->queued_dialog;
                    return GAMESTATE_DIALOG;
                }
            } else if( ctx->queued_location >= 0 ) {
                state->current_location = ctx->queued_location;
                state->current_dialog = -1;
                return GAMESTATE_LOCATION;
            }
		}
        
    }

    return GAMESTATE_NO_CHANGE;
}


gamestate_t exit_init( game_t* ctx ) {
    (void) ctx;
    return GAMESTATE_NO_CHANGE;
}


gamestate_t exit_update( game_t* ctx ) {
	if( was_key_pressed( ctx, APP_KEY_LBUTTON ) || was_key_pressed( ctx, APP_KEY_SPACE ) || was_key_pressed( ctx, APP_KEY_ESCAPE ) ) {
		return GAMESTATE_TERMINATE;
    }
    return GAMESTATE_NO_CHANGE;
}


gamestate_t terminate_init( game_t* ctx ) {
    ctx->exit_flag = true;
    return GAMESTATE_NO_CHANGE;
}


gamestate_t terminate_update( game_t* ctx ) {
    (void) ctx;
    return GAMESTATE_NO_CHANGE;
}
