


typedef enum gamestate_t {
    GAMESTATE_NO_CHANGE,
    GAMESTATE_BOOT,
    GAMESTATE_SCREEN,
    GAMESTATE_LOCATION,
    GAMESTATE_DIALOG,
    GAMESTATE_EXIT,
    GAMESTATE_TERMINATE,
} gamestate_t;


typedef enum stack_entry_type_t {
    STACK_ENTRY_SCREEN,
    STACK_ENTRY_LOCATION,
    STACK_ENTRY_DIALOG,
} stack_entry_type_t;

typedef struct stack_entry_t {
    stack_entry_type_t type;
    int index;
} stack_entry_t;


typedef struct state_data_t {
    int current_screen;
    int current_location;
    int current_dialog;
    int current_image;
    int current_music;
    int current_ambience;
    bool first_chr_or_use;
    array(bool)* flags;
    array(int)* items;
    array(int)* chars;
    array(stack_entry_t)* section_stack;
} state_data_t;


void state_data_reset( state_data_t* data ) {
    data->current_screen = -1;
    data->current_location = -1;
    data->current_dialog = -1;
    data->current_image = -1;
    data->current_music = -1;
    data->current_ambience = -1;
    data->first_chr_or_use = true;
    array_clear( data->flags );
    array_clear( data->items  );
    array_clear( data->chars  );
    array_clear( data->section_stack );
}


void state_data_copy( state_data_t* dest, state_data_t* src ) {
    dest->current_screen = src->current_screen;    
    dest->current_location = src->current_location;    
    dest->current_dialog = src->current_dialog;    
    dest->current_image = src->current_image;    
    //dest->current_music = src->current_music;    
    //dest->current_ambience = src->current_ambience;    
    dest->first_chr_or_use = src->first_chr_or_use;

    array_clear( dest->flags );
    for( int i = 0; i < src->flags->count; ++i ) {
        array_add( dest->flags, &src->flags->items[ i ] );
    }
    
    array_clear( dest->items );
    for( int i = 0; i < src->items->count; ++i ) {
        array_add( dest->items, &src->items->items[ i ] );
    }
    
    array_clear( dest->chars );
    for( int i = 0; i < src->chars->count; ++i ) {
        array_add( dest->chars, &src->chars->items[ i ] );
    }
    
    array_clear( dest->section_stack );
    for( int i = 0; i < src->section_stack->count; ++i ) {
        array_add( dest->section_stack, &src->section_stack->items[ i ] );
    }
}


void state_data_write( state_data_t* data, buffer_t* buffer ) {
    buffer_write_i32( buffer, &data->current_screen, 1 );
    buffer_write_i32( buffer, &data->current_location, 1 );
    buffer_write_i32( buffer, &data->current_dialog, 1 );
    buffer_write_i32( buffer, &data->current_image, 1 );
    buffer_write_i32( buffer, &data->current_music, 1 );
    buffer_write_i32( buffer, &data->current_ambience, 1 );
    buffer_write_bool( buffer, &data->first_chr_or_use, 1 );

    buffer_write_i32( buffer, &data->flags->count, 1 );
    buffer_write_bool( buffer, data->flags->items, data->flags->count );

    buffer_write_i32( buffer, &data->items->count, 1 );
    buffer_write_i32( buffer, data->items->items, data->items->count );

    buffer_write_i32( buffer, &data->chars->count, 1 );
    buffer_write_i32( buffer, data->chars->items, data->chars->count );

    buffer_write_i32( buffer, &data->section_stack->count, 1 );
    for( int i = 0; i < data->section_stack->count; ++i ) {
        int type = (int) data->section_stack->items[ i ].type;
        buffer_write_i32( buffer, &type, 1 );
        buffer_write_i32( buffer, &data->section_stack->items[ i ].index, 1  );
    }
}


bool state_data_read( state_data_t* data, buffer_t* buffer ) {
    bool failed = false;
    failed = failed || 0 == buffer_read_i32( buffer, &data->current_screen, 1 );
    failed = failed || 0 == buffer_read_i32( buffer, &data->current_location, 1 );
    failed = failed || 0 == buffer_read_i32( buffer, &data->current_dialog, 1 );
    failed = failed || 0 == buffer_read_i32( buffer, &data->current_image, 1 );
    failed = failed || 0 == buffer_read_i32( buffer, &data->current_music, 1 );
    failed = failed || 0 == buffer_read_i32( buffer, &data->current_ambience, 1 );
    failed = failed || 0 == buffer_read_bool( buffer, &data->first_chr_or_use, 1 );

    data->flags = managed_array( bool );
    data->items = managed_array( int );
    data->chars = managed_array( int );
    data->section_stack = managed_array( stack_entry_t );

    int flags_count = 0;
    failed = failed || 0 == buffer_read_i32( buffer, &flags_count, 1 );
    failed = failed || ( flags_count < 0 || flags_count > 65536 );
    if( !failed ) {
        for( int i = 0; i < flags_count; ++i ) {
            bool flag = false;
            failed = failed || 0 == buffer_read_bool( buffer, &flag, 1 );
            if( failed ) break;
            array_add( data->flags, &flag );
        }
    }

    int items_count = 0;
    failed = failed || 0 == buffer_read_i32( buffer, &items_count, 1 );
    failed = failed || ( items_count < 0 || items_count > 65536 );
    if( !failed ) {
        for( int i = 0; i < items_count; ++i ) {
            int item = false;
            failed = failed || 0 == buffer_read_i32( buffer, &item, 1 );
            if( failed ) break;
            array_add( data->items, &item );
        }
    }

    int chars_count = 0;
    failed = failed || 0 == buffer_read_i32( buffer, &chars_count, 1 );
    failed = failed || ( chars_count < 0 || chars_count > 65536 );
    if( !failed ) {
        for( int i = 0; i < chars_count; ++i ) {
            int item = false;
            failed = failed || 0 == buffer_read_i32( buffer, &item, 1 );
            if( failed ) break;
            array_add( data->chars, &item );
        }
    }

    int stack_count = 0;
    failed = failed || 0 == buffer_read_i32( buffer, &stack_count, 1 );
    failed = failed || ( stack_count < 0 || stack_count > 65536 );
    if( !failed ) {
        for( int i = 0; i < stack_count; ++i ) {
            stack_entry_t stack;
            int type = 0;
            failed = failed || 0 == buffer_read_i32( buffer, &type, 1 );
            stack.type = (stack_entry_type_t)type; 
            failed = failed || 0 == buffer_read_i32( buffer, &stack.index, 1 );
            if( failed ) break;
            array_add( data->section_stack, &stack );
        }
    }

    return !failed;
}


typedef struct savegame_t {
    uint32_t version;
    bool fullscreen;
    int sound_level;
    int display_filter;
    int thumb_width;
    int thumb_height;
    uint8_t* thumb;
    char date[ 12 ];
    char time[ 6 ];
    state_data_t state;
} savegame_t;


typedef struct game_t {
    float delta_time;
    int blink_count;
    int blink_wait;
    int sound_level;
    int display_filter_index;
    bool fullscreen;
    bool blink_visible;
    bool exit_flag;
    bool exit_requested;
    bool exit_dialog;
    bool exit_confirmed;
    bool restart_requested;    
    bool quickload_requested;
    gamestate_t current_state;
    gamestate_t new_state;
    bool disable_transition;
    bool ingame_menu;
    bool savegame_menu;
    bool loadgame_menu;
    bool settings_menu;
    int transition_counter;
    render_t* render;
    audiosys_t* audiosys;
    rnd_pcg_t* rnd;
    input_t* input;
    int mouse_x;
    int mouse_y;
    yarn_t* yarn;
    int queued_screen;
    int queued_location;
    int queued_dialog;
    float limit;
    struct {
        int phrase_index;
        int phrase_len;
        int chr_index;
        int enable_options;
    } dialog;
    state_data_t state;
    state_data_t quicksave;
    savegame_t savegames[ 9 ];
    struct {
        struct {
            int audio_index;
            AUDIOSYS_U64 handle;
        }* sounds;
        int sounds_count;
        int sounds_capacity;
    } sound_state;
} game_t;


typedef struct qoa_decode_t {
    qoa_data_t* audio_data;
    qoa_desc desc;
    uint32_t start_pos;
    uint32_t decode_pos;
    uint32_t audio_position;
    int samples_count;
    short samples[ QOA_FRAME_LEN ];
} qoa_decode_t;


float audio_qoa_get_length( void* instance ) {
    qoa_decode_t* qoa = (qoa_decode_t*) instance;
    if( qoa ) {
        return qoa->desc.samples / 22050.0f;
    }
    return 0.0f;
}


void audio_qoa_release( void* instance ) {
    qoa_decode_t* qoa = (qoa_decode_t*) instance;
    if( qoa ) {
        free( qoa );
    }
}


int audio_qoa_read_samples( void* instance, float* sample_pairs, int sample_pairs_count ) {
    qoa_decode_t* qoa = (qoa_decode_t*) instance;
    if( qoa ) {
        int total_count = 0;
        bool done = false;
        while( sample_pairs_count > 0 ) {
            int count = sample_pairs_count / 2 > qoa->samples_count ? qoa->samples_count : sample_pairs_count / 2;
            for( int i = 0; i < count; ++i ) {
                float sample = qoa->samples[ i ] / 32767.0f;
                sample_pairs[ i * 4 + 0 ] = sample;
                sample_pairs[ i * 4 + 1 ] = sample;
                sample_pairs[ i * 4 + 2 ] = sample;
                sample_pairs[ i * 4 + 3 ] = sample;
            }
            sample_pairs += 4 * count;
            sample_pairs_count -= 2 * count;
            total_count += count * 2;

            memmove( qoa->samples, qoa->samples + count, sizeof( short ) * ( qoa->samples_count - count ) );
            qoa->samples_count -= count;

            if( qoa->samples_count == 0 && done ) {
                break;
            }

            if( qoa->samples_count <= 0 ) {
                uint32_t frame_len = 0;
                uint32_t frame_size = qoa_decode_frame( qoa->audio_data->data + qoa->decode_pos, 
                    qoa->audio_data->size - qoa->decode_pos, &qoa->desc, qoa->samples, &frame_len ); 
                qoa->decode_pos += frame_size;
                qoa->samples_count = frame_len;
                if( frame_size < QOA_FRAME_SIZE( 1, QOA_SLICES_PER_FRAME ) ) {
                    done = true;
                }
            }
        }
        qoa->audio_position += total_count;
        return total_count;
    } else {
        memset( sample_pairs, 0, sizeof( float ) * 2 * sample_pairs_count );
        return sample_pairs_count;
    }
}


void audio_qoa_restart( void* instance ) {
    qoa_decode_t* qoa = (qoa_decode_t*) instance;
    if( qoa ) {
        qoa->audio_position = 0;
        qoa->samples_count = 0;
        qoa->decode_pos = qoa->start_pos;
    }
}


void audio_qoa_set_position( void* instance, int position_in_sample_pairs ) { 
    qoa_decode_t* qoa = (qoa_decode_t*) instance;
    if( qoa ) {
        int frame = position_in_sample_pairs / QOA_FRAME_LEN;
        frame = frame < 0 ? 0 : frame >= qoa->desc.samples / QOA_FRAME_LEN ? qoa->desc.samples / QOA_FRAME_LEN : frame;
        qoa->audio_position = frame * QOA_FRAME_LEN;
        qoa->samples_count = 0;
        qoa->decode_pos = qoa->start_pos + frame * qoa_max_frame_size( &qoa->desc );
    }
}


int audio_qoa_get_position( void* instance ) {
    qoa_decode_t* qoa = (qoa_decode_t*) instance;
    if( qoa ) {
          return qoa->audio_position;
    }
    return 0;
}


bool audio_qoa_source( game_t* game, int audio_index, audiosys_audio_source_t* src ) {
    if( audio_index < 0 ) {
        return false;
    }
    qoa_data_t* audio_data = game->yarn->assets.audio->items[ audio_index ];
    qoa_desc desc;
	uint32_t pos = qoa_decode_header( audio_data->data, audio_data->size, &desc );
	if( !pos ) {
		return false;
	}

    qoa_decode_t* qoa = (qoa_decode_t*) malloc( sizeof( qoa_decode_t ) );
    qoa->audio_data = audio_data;
    qoa->desc = desc;
    qoa->start_pos = pos;
    qoa->decode_pos = pos;
    qoa->audio_position = 0;
    qoa->samples_count = 0;

    src->instance = qoa;
    src->release = audio_qoa_release;
    src->read_samples = audio_qoa_read_samples;
    src->restart = audio_qoa_restart;
    src->set_position = audio_qoa_set_position;
    src->get_position = audio_qoa_get_position;
    return true;
}


void game_restart( game_t* game ) {
    audiosys_stop_all( game->audiosys );
    state_data_reset( &game->state );

    game->state.current_screen = game->yarn->start_screen;
    game->state.current_location = game->yarn->start_location;
    game->state.current_dialog = game->yarn->start_dialog;
    if( game->yarn->is_debug && game->yarn->debug_start_screen >= 0 ) {
        game->state.current_screen = game->yarn->debug_start_screen;
        game->state.current_location = -1;
        game->state.current_dialog = -1;
    }
    if( game->yarn->is_debug && game->yarn->debug_start_location >= 0 ) {
        game->state.current_screen = -1;
        game->state.current_location = game->yarn->debug_start_location;
        game->state.current_dialog = -1;
    }
    if( game->yarn->is_debug && game->yarn->debug_start_dialog  >= 0 ) {
        game->state.current_screen = -1;
        game->state.current_location = -1;
        game->state.current_dialog = game->yarn->debug_start_dialog;
    }
    for( int i = 0; i < game->yarn->flag_ids->count; ++i ) {
        bool value = false;
        if( game->yarn->is_debug ) {
            for( int j = 0; j < game->yarn->globals.debug_set_flags->count; ++j ) {
                if( cstr_compare_nocase( game->yarn->flag_ids->items[ i ], game->yarn->globals.debug_set_flags->items[ j ] ) == 0 ) {
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
                if( cstr_compare_nocase( game->yarn->item_ids->items[ i ], game->yarn->globals.debug_get_items->items[ j ] ) == 0 ) {
                    array_add( game->state.items, &i );
                    break;
                }
            }
        }
    }

    if( game->yarn->is_debug ) {
        for( int i = 0; i < game->yarn->globals.debug_attach_chars->count; ++i ) {
            for( int j = 0; j < game->yarn->characters->count; ++j ) {
                if( cstr_compare_nocase( game->yarn->characters->items[ j ].id, game->yarn->globals.debug_attach_chars->items[ i ] ) == 0 ) {
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
    game->queued_screen = -1;
    game->queued_location = -1;
    game->queued_dialog = -1;
    game->restart_requested = false;    
    game->quickload_requested = false;
}


void game_load_state( game_t* game, state_data_t* data ) {
    game_restart( game );

    state_data_copy( &game->state, data );

    if( game->state.current_screen >= 0 ) {
        game->new_state = GAMESTATE_SCREEN;
    } else if( game->state.current_location >= 0 ) {
        game->new_state = GAMESTATE_LOCATION;
    } else if( game->state.current_dialog >= 0 ) {
        game->new_state = GAMESTATE_DIALOG;
    }
}


void game_save_state( game_t* game, state_data_t* data ) {
    state_data_copy( data, &game->state );   
    game->quicksave.current_music = game->state.current_music;
    game->quicksave.current_ambience = game->state.current_ambience;
}


void game_quicksave( game_t* game ) {
    game_save_state( game, &game->quicksave );
}


void game_quickload( game_t* game ) {
    game->quickload_requested = false;
    game_load_state( game, &game->quicksave );

    audiosys_music_stop( game->audiosys, 0.1f );
    int music_index = game->quicksave.current_music;
    if( music_index >= 0 ) {
        audiosys_audio_source_t src;
        if( audio_qoa_source( game, music_index, &src ) ) {
            audiosys_music_play( game->audiosys, src, 0.5f );
        }
        //audiosys_music_position_set( game->audiosys, length );
        //audiosys_music_volume_set( game->audiosys, volume );
        //audiosys_music_loop_set( game->audiosys, mus->loop ? AUDIOSYS_LOOP_ON : AUDIOSYS_LOOP_OFF );
    }
    game->state.current_music = music_index;
        
    audiosys_ambience_stop( game->audiosys, 0.1f );
    int ambience_index = game->quicksave.current_ambience;
    if( ambience_index >= 0 ) {
        audiosys_audio_source_t src;
        if( audio_qoa_source( game, ambience_index, &src ) ) {
            audiosys_ambience_play( game->audiosys, src, 0.5f );
        }
        //audiosys_ambience_position_set( game->audiosys, length );
        //audiosys_ambience_volume_set( game->audiosys, volume );
        //audiosys_ambience_loop_set( game->audiosys, mus->loop ? AUDIOSYS_LOOP_ON : AUDIOSYS_LOOP_OFF );
    }
    game->state.current_ambience = ambience_index;
}


void game_init( game_t* game, yarn_t* yarn, render_t* render, input_t* input, audiosys_t* audiosys, rnd_pcg_t* rnd ) {
    memset( game, 0, sizeof( *game ) );
    game->delta_time = 0.0f;
    game->exit_flag = false;
    game->exit_requested = false;
    game->exit_dialog = false;
    game->exit_confirmed = false;
    game->sound_level = 3;
    game->render = render;
    game->audiosys = audiosys;
    game->rnd = rnd;
    game->input = input;
    game->yarn = yarn;
    game->display_filter_index = 0;

    game->blink_count = 0;
    game->blink_wait = 100;
    game->blink_visible = true;

    game->state.flags = managed_array( bool );
    game->state.items = managed_array( int );
    game->state.chars = managed_array( int );
    game->state.section_stack = managed_array( stack_entry_t );

    game->quicksave.flags = managed_array( bool );
    game->quicksave.items = managed_array( int );
    game->quicksave.chars = managed_array( int );
    game->quicksave.section_stack = managed_array( stack_entry_t );

    game_restart( game );

    game->sound_state.sounds_count = 0;
    game->sound_state.sounds_capacity = 256;
    game->sound_state.sounds = ARRAY_CAST( manage_alloc( malloc( sizeof( *game->sound_state.sounds ) * game->sound_state.sounds_capacity ) ) );
    memset( game->sound_state.sounds, 0, sizeof( *game->sound_state.sounds ) * game->sound_state.sounds_capacity );
}

bool was_key_pressed( game_t* game, int key ) {
    return input_was_key_pressed( game->input, key );
}


gamestate_t boot_init( game_t* game );
gamestate_t boot_update( game_t* game );
gamestate_t screen_init( game_t* game );
gamestate_t screen_update( game_t* game );
gamestate_t location_init( game_t* game );
gamestate_t location_update( game_t* game );
gamestate_t dialog_init( game_t* game );
gamestate_t dialog_update( game_t* game );
gamestate_t exit_init( game_t* game );
gamestate_t exit_update( game_t* game );
gamestate_t terminate_init( game_t* game );
gamestate_t terminate_update( game_t* game );

void ingame_menu_update( game_t* game );
void exit_dialog_update( game_t* game );

void enter_menu( game_t* game ) {
    if( game->render->screen ) {
        memcpy( game->render->screenshot, game->render->screen, sizeof( uint8_t ) * game->render->screen_width * game->render->screen_height );
    } else {
        glFlush();

        GLint viewport[ 4 ];
        glGetIntegerv( GL_VIEWPORT, viewport );
        glReadPixels( viewport[ 0 ], viewport[ 1 ], viewport[ 2 ], viewport[ 3 ], GL_RGBA, GL_UNSIGNED_BYTE, game->render->screenshot_rgb );
    }
    game->ingame_menu = true;
    audiosys_pause( game->audiosys );
    if( game->render->screen ) {
        for( int y = 0; y < game->render->screen_height; ++y ) {
            for( int x = 0; x < game->render->screen_width; ++x ) {
                if( ( x + y ) & 1 ) {
                    game->render->screen[ x + y * game->render->screen_width ] = game->render->color_background;
                }
            }
        }
    } else {
        box( game->render, 0, 0, game->render->screen_width, game->render->screen_height, 0x80000000 );
    }
}


void game_update( game_t* game, float delta_time ) {    
    if( game->exit_confirmed ) {
        game->new_state = GAMESTATE_TERMINATE;
        game->disable_transition = true;
        game->exit_dialog = false;
        game->exit_confirmed = false;
        game->ingame_menu = false;
    }

    if( game->blink_count > 0 ) {
		--game->blink_count;
		if( game->blink_count > 0 ) {
			game->blink_visible = game->blink_count % 30 < 15;
		} else {
			game->blink_visible = true;
		}
	} else {
		--game->blink_wait;
		if( game->blink_wait <= 0 ) {
			game->blink_wait = 200;
			game->blink_count = 100;
		}
	}

    if( game->exit_dialog ) {
        exit_dialog_update( game );
        return;
    }

    bool menu_requested = false;
    if( game->ingame_menu ) {
        ingame_menu_update( game );
        if( !game->ingame_menu && game->new_state != GAMESTATE_TERMINATE) {
            audiosys_resume( game->audiosys );
        }
        return;
    } else if( game->current_state != GAMESTATE_BOOT ) {
        if( was_key_pressed( game, APP_KEY_ESCAPE ) ) {
            menu_requested = true;
        }
    }

    game->delta_time = delta_time;

    if( game->new_state != GAMESTATE_NO_CHANGE ) {
        if( !game->disable_transition ) {
            game->transition_counter = -10;
        }
        game->disable_transition = false;
        if( game->current_state == GAMESTATE_DIALOG && game->new_state == GAMESTATE_DIALOG ) {
            game->limit = -30.0f;
            game->dialog.enable_options = 0;
            dialog_update( game );
        }
        game->current_state = game->new_state;
        gamestate_t new_state = GAMESTATE_NO_CHANGE;
        switch( game->current_state ) {
            case GAMESTATE_NO_CHANGE:
                break;
            case GAMESTATE_BOOT:
                new_state = boot_init( game );
                break;
            case GAMESTATE_SCREEN:
                new_state = screen_init( game );
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
        case GAMESTATE_SCREEN:
            new_state = screen_update( game );
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
    if( menu_requested ) {
        enter_menu( game );
        return;
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
        if( game->render->screen ) {
            memcpy( game->render->screenshot, game->render->screen, sizeof( uint8_t ) * game->render->screen_width * game->render->screen_height );
        } else {
            glFlush();

            GLint viewport[ 4 ];
            glGetIntegerv( GL_VIEWPORT, viewport );
            glReadPixels( viewport[ 0 ], viewport[ 1 ], viewport[ 2 ], viewport[ 3 ], GL_RGBA, GL_UNSIGNED_BYTE, game->render->screenshot_rgb );
        }
        game->new_state = GAMESTATE_EXIT;
        game->disable_transition = true;
        game->exit_requested = false;
        game->ingame_menu = false;
    }
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
            switch( audio->items[ i ].type ) {
                case YARN_AUDIO_TYPE_MUSIC: {
                    yarn_audio_t const* mus = &audio->items[ i ];
                    if( mus->stop ) {
                        game->state.current_music = -1;
                        float fade_time = mus->crossfade_min + ( mus->crossfade_max - mus->crossfade_min ) * rnd_pcg_nextf( game->rnd );
                        if( fade_time == 0.0f ) fade_time = 1.0f;
                        audiosys_music_stop( game->audiosys, fade_time < 0.1f ? 0.1f : fade_time );
                    } else {
                        int music_index = mus->audio_index;
                        if( music_index == game->state.current_music ) {
                            if( mus->restart ) {
                                audiosys_music_position_set( game->audiosys, 0.0f );
                            }
                            float volume = mus->volume_min + ( mus->volume_max - mus->volume_min ) * rnd_pcg_nextf( game->rnd );
                            audiosys_music_volume_set( game->audiosys, volume );
                        } else {
                            audiosys_audio_source_t src;
                            if( audio_qoa_source( game, music_index, &src ) ) {
                                float fade_time = mus->crossfade_min + ( mus->crossfade_max - mus->crossfade_min ) * rnd_pcg_nextf( game->rnd );
                                if( fade_time > 0.0f ) {
                                    audiosys_music_cross_fade( game->audiosys, src, fade_time < 0.1f ? 0.1f : fade_time );
                                } else {
                                    audiosys_music_switch( game->audiosys, src, 0.5f, 0.0f );
                                }
                                if( mus->random ) {
                                    float length = audio_qoa_get_length( src.instance );
                                    float r = rnd_pcg_nextf( game->rnd );
                                    audiosys_music_position_set( game->audiosys, length * r );
                                }
                            } else {
                                audiosys_music_stop( game->audiosys, 1.0f );
                            }
                            float volume = mus->volume_min + ( mus->volume_max - mus->volume_min ) * rnd_pcg_nextf( game->rnd );
                            audiosys_music_volume_set( game->audiosys, volume );
                        }
                        audiosys_music_loop_set( game->audiosys, mus->loop ? AUDIOSYS_LOOP_ON : AUDIOSYS_LOOP_OFF );
                        game->state.current_music = music_index;
                    }
                } break;
                case YARN_AUDIO_TYPE_AMBIENCE: {
                    yarn_audio_t const* amb = &audio->items[ i ];
                    if( amb->stop ) {
                        game->state.current_ambience = -1;
                        float fade_time = amb->crossfade_min + ( amb->crossfade_max - amb->crossfade_min ) * rnd_pcg_nextf( game->rnd );
                        if( fade_time == 0.0f ) fade_time = 1.0f;
                        audiosys_ambience_stop( game->audiosys, fade_time < 0.1f ? 0.1f : fade_time );
                    } else {
                        int ambience_index = amb->audio_index;
                        if( ambience_index == game->state.current_ambience ) {
                            if( amb->restart ) {
                                audiosys_ambience_position_set( game->audiosys, 0.0f );
                            }
                            float volume = amb->volume_min + ( amb->volume_max - amb->volume_min ) * rnd_pcg_nextf( game->rnd );
                            audiosys_ambience_volume_set( game->audiosys, volume );
                        } else {
                            audiosys_audio_source_t src;
                            if( audio_qoa_source( game, ambience_index, &src ) ) {
                                float fade_time = amb->crossfade_min + ( amb->crossfade_max - amb->crossfade_min ) * rnd_pcg_nextf( game->rnd );
                                audiosys_ambience_cross_fade( game->audiosys, src, fade_time < 0.1f ? 0.1f : fade_time );
                                if( amb->random ) {
                                    float length = audio_qoa_get_length( src.instance );
                                    float r = rnd_pcg_nextf( game->rnd );
                                    audiosys_ambience_position_set( game->audiosys, length * r );
                                }
                            } else {
                                audiosys_ambience_stop( game->audiosys, 1.0f );
                            }
                            float volume = amb->volume_min + ( amb->volume_max - amb->volume_min ) * rnd_pcg_nextf( game->rnd );
                            audiosys_ambience_volume_set( game->audiosys, volume );
                        }
                        audiosys_ambience_loop_set( game->audiosys, amb->loop ? AUDIOSYS_LOOP_ON : AUDIOSYS_LOOP_OFF );
                        game->state.current_ambience = ambience_index;
                    }
                } break;
                case YARN_AUDIO_TYPE_SOUND: {
                    yarn_audio_t const* snd = &audio->items[ i ];
                    for( int j = 0; j < game->sound_state.sounds_count; ++j ) {
                        if( ( snd->stop && snd->audio_index == -1 ) || game->sound_state.sounds[ j ].audio_index == snd->audio_index ) {
                            float fade_time = snd->delay_min + ( snd->delay_max - snd->delay_min ) * rnd_pcg_nextf( game->rnd );
                            audiosys_sound_stop( game->audiosys, game->sound_state.sounds[ j ].handle, fade_time < 0.1f ? 0.1f : fade_time );
                            game->sound_state.sounds[ j-- ] = game->sound_state.sounds[ --game->sound_state.sounds_count ];
                        }
                    }
                    if( !snd->stop ) {
                        audiosys_audio_source_t src;
                        if( audio_qoa_source( game, snd->audio_index, &src ) ) {
                            AUDIOSYS_U64 handle = audiosys_sound_play( game->audiosys, src, 0.0f, 0.0f );
                            game->sound_state.sounds[ game->sound_state.sounds_count ].audio_index = snd->audio_index;
                            game->sound_state.sounds[ game->sound_state.sounds_count ].handle = handle;                        
                            game->sound_state.sounds_count++;
                            float volume = snd->volume_min + ( snd->volume_max - snd->volume_min ) * rnd_pcg_nextf( game->rnd );
                            audiosys_sound_volume_set( game->audiosys, handle, volume );
                            if( snd->loop ) {
                                audiosys_sound_loop_set( game->audiosys, handle, AUDIOSYS_LOOP_ON );
                            }
                        }
                    }
                } break;
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
            case ACTION_TYPE_GOTO_SCREEN: {
                game->queued_screen = action->param_screen_index;
                game->queued_dialog = -1;
                game->queued_location = -1;
            } break;
            case ACTION_TYPE_GOTO_LOCATION: {
                game->queued_screen = -1;
                game->queued_dialog = -1;
                game->queued_location = action->param_location_index;
            } break;
            case ACTION_TYPE_GOTO_DIALOG: {
                game->queued_screen = -1;
                game->queued_location = -1;
                game->queued_dialog = action->param_dialog_index;
            } break;
            case ACTION_TYPE_EXIT: {
                game->exit_requested = true;
                game->transition_counter = 0;
            } break;
            case ACTION_TYPE_RETURN: {
                if( game->state.section_stack->count > 1 ) {
                    --game->state.section_stack->count;
                    stack_entry_t entry = game->state.section_stack->items[ --game->state.section_stack->count ];
                    if( entry.type == STACK_ENTRY_SCREEN ) {
                        game->queued_screen = entry.index;
                        game->queued_dialog = -1;
                        game->queued_location = -1;
                    } else if( entry.type == STACK_ENTRY_LOCATION ) {
                        game->queued_screen = -1;
                        game->queued_dialog = -1;
                        game->queued_location = entry.index;
                    } else {
                        game->queued_screen = -1;
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



static char const* savegame_names[] = { "savegame.001", "savegame.002", "savegame.003", "savegame.004", "savegame.005", 
    "savegame.006", "savegame.007", "savegame.008", "savegame.009",  };


void save_game( game_t* game, int slot ) {
    if( slot < 1 || slot > 9 ) {
        return;
    }

    int thumb_width = 75;
    int thumb_height = 57;
    scale_for_resolution( game->render, &thumb_width, &thumb_height );
    uint32_t* thumb_rgb = game->render->screenshot_rgb ? make_thumbnail_rgb( game->render->screenshot_rgb, game->render->screen_width, game->render->screen_height, thumb_width, thumb_height ) : NULL;
    uint8_t* thumb = game->render->screen ? make_thumbnail( game->render->screenshot, game->render->screen_width, game->render->screen_height, thumb_width, thumb_height ) : NULL;
 
    buffer_t* buffer = buffer_create();

    int version_len = (int) strlen( game->yarn->globals.version );
    buffer_write_i32( buffer, &version_len, 1 );
    buffer_write_char( buffer, game->yarn->globals.version, version_len );
    buffer_write_bool( buffer, &game->fullscreen, 1 );
    buffer_write_i32( buffer, &game->sound_level, 1 );
    buffer_write_i32( buffer, &game->display_filter_index, 1 );
    buffer_write_i32( buffer, &thumb_width, 1 );
    buffer_write_i32( buffer, &thumb_height, 1 );
    
    if( thumb ) {
        buffer_write_u8( buffer, thumb, thumb_width * thumb_height );
    } else if( thumb_rgb ) {
        buffer_write_u32( buffer, thumb_rgb, thumb_width * thumb_height );
    }

    datetime_t dt = get_datetime();
    char str[ 16 ];
    char const* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    sprintf( str, "%02d %s %04d", dt.day, months[ dt.month ], dt.year );
    buffer_write_char( buffer, str, 12 );
    sprintf( str, "%02d:%02d", dt.hour, dt.minute );
    buffer_write_char( buffer, str, 6 );   

    state_data_write( &game->state, buffer );

    char const* name = savegame_names[ slot - 1 ];

    bool success = save_data( game->yarn->globals.title, name, buffer_data( buffer ), buffer_size( buffer ) );
    if( success ) {
        game->ingame_menu = false;
        game->savegame_menu = false;
    }

    buffer_destroy( buffer );

    if( thumb_rgb ) {
        free( thumb_rgb );
    }
    if( thumb ) {
        free( thumb );
    }
}


void load_game( game_t* game, int slot ) {
    if( slot < 1 || slot > 9 ) {
        return;
    }
    savegame_t* savegame = &game->savegames[ slot - 1 ];
    if( savegame->thumb_width && savegame->thumb_height ) {
        game->sound_level = savegame->sound_level < 0 ? 0 : savegame->sound_level > 3 ? 3 : savegame->sound_level;
        float volumes[] = { 0.0f, 0.33f, 0.66f, 1.0f };
        audiosys_master_volume_set( game->audiosys, volumes[ game->sound_level ] );
        game->display_filter_index = savegame->display_filter < 0 ? 0 : savegame->display_filter >= game->yarn->globals.display_filters->count ? game->yarn->globals.display_filters->count - 1 : savegame->display_filter;
        game->fullscreen = savegame->fullscreen;
        game_load_state( game, &savegame->state );   
        
        audiosys_music_stop( game->audiosys, 0.1f );
        int music_index = savegame->state.current_music;
        if( music_index >= 0 ) {
            audiosys_audio_source_t src;
            if( audio_qoa_source( game, music_index, &src ) ) {
                audiosys_music_play( game->audiosys, src, 0.5f );
            }
            //audiosys_music_position_set( game->audiosys, length );
            //audiosys_music_volume_set( game->audiosys, volume );
            //audiosys_music_loop_set( game->audiosys, mus->loop ? AUDIOSYS_LOOP_ON : AUDIOSYS_LOOP_OFF );
        }
        game->state.current_music = music_index;
        
        audiosys_ambience_stop( game->audiosys, 0.1f );
        int ambience_index = savegame->state.current_ambience;
        if( ambience_index >= 0 ) {
            audiosys_audio_source_t src;
            if( audio_qoa_source( game, ambience_index, &src ) ) {
                audiosys_ambience_play( game->audiosys, src, 0.5f );
            }
            //audiosys_ambience_position_set( game->audiosys, length );
            //audiosys_ambience_volume_set( game->audiosys, volume );
            //audiosys_ambience_loop_set( game->audiosys, mus->loop ? AUDIOSYS_LOOP_ON : AUDIOSYS_LOOP_OFF );
        }
        game->state.current_ambience = ambience_index;
        
        game->ingame_menu = false;
        game->loadgame_menu = false;
    }
}


void load_savegames( game_t* game ) {
    int thumb_width = 75;
    int thumb_height = 57;
    scale_for_resolution( game->render, &thumb_width, &thumb_height );

    memset( game->savegames, 0, sizeof( game->savegames ) );
    for( int i = 0; i < 9; ++i ) {
        savegame_t* savegame = &game->savegames[ i ];
        size_t size = 0;
        void* data = load_data( game->yarn->globals.title, savegame_names[ i ], &size );
        if( data ) {
            buffer_t* buffer = buffer_map( data, size );
            int version_len = 0;
            buffer_read_i32( buffer, &version_len, 1 );
            if( version_len >= 256 ) {
                savegame->thumb_width= 0;
                savegame->thumb_height = 0;
                buffer_destroy( buffer );
                free( data );
                continue;
            }
            char* version = (char*) malloc( version_len + 1 );
            buffer_read_char( buffer, version, version_len );
            version[version_len] = 0;
            if( cstr_compare( cstr_trim( game->yarn->globals.version ), cstr_trim( version ) ) != 0 ) {
                free( version );
                savegame->thumb_width= 0;
                savegame->thumb_height = 0;
                buffer_destroy( buffer );
                free( data );
                continue;
            }
            free( version );
            buffer_read_bool( buffer, &savegame->fullscreen, 1 );
            buffer_read_i32( buffer, &savegame->sound_level, 1 );
            buffer_read_i32( buffer, &savegame->display_filter, 1 );

            buffer_read_i32( buffer, &savegame->thumb_width, 1 );
            buffer_read_i32( buffer, &savegame->thumb_height, 1 );
            if( savegame->thumb_width != thumb_width || savegame->thumb_height != thumb_height ) {
                savegame->thumb_width= 0;
                savegame->thumb_height = 0;
                buffer_destroy( buffer );
                free( data );
                continue;
            }
            savegame->thumb = game->render->screen ? (uint8_t*) manage_alloc( malloc( savegame->thumb_width * savegame->thumb_height * sizeof( uint8_t ) ) ) : NULL;
            if( savegame->thumb ) {
                int count = buffer_read_u8( buffer, savegame->thumb, savegame->thumb_width * savegame->thumb_height );
                if( count < savegame->thumb_width * savegame->thumb_height ) {
                    savegame->thumb_width= 0;
                    savegame->thumb_height = 0;
                    free( savegame->thumb );
                    savegame->thumb = NULL;
                    buffer_destroy( buffer );
                    free( data );
                    continue;
                }
            } else {
                uint32_t* thumb_rgb = (uint32_t*) malloc( savegame->thumb_width * savegame->thumb_height * sizeof( uint32_t )  );
                int count = buffer_read_u32( buffer, thumb_rgb, savegame->thumb_width * savegame->thumb_height );
                if( count < savegame->thumb_width * savegame->thumb_height ) {
                    savegame->thumb_width= 0;
                    savegame->thumb_height = 0;
                    free( thumb_rgb );
                    buffer_destroy( buffer );
                    free( data );
                    continue;
                }
                generate_savegame_texture( game->render, thumb_rgb, savegame->thumb_width, savegame->thumb_height, i );
                free( thumb_rgb );
           }

            if( buffer_read_char( buffer, savegame->date, 12 ) != 12 ) {
                savegame->thumb_width= 0;
                savegame->thumb_height = 0;
                buffer_destroy( buffer );
                free( data );
            }
            if( buffer_read_char( buffer, savegame->time, 6 ) != 6 ) {
                savegame->thumb_width= 0;
                savegame->thumb_height = 0;
                buffer_destroy( buffer );
                free( data );
            }

            if( !state_data_read( &savegame->state, buffer ) ) {
                savegame->thumb_width= 0;
                savegame->thumb_height = 0;
            }

            buffer_destroy( buffer );
            free( data );
        }
    }
}


void savegame_menu_update( game_t* game ) {
    cls( game->render );    

    center( game->render, game->render->font_name, "SAVE GAME", 160, 6, game->render->color_opt );

    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game->render, &mouse_x, &mouse_y );

    bool cancel_hover = mouse_y < 16 && mouse_x > 295;
    if( cancel_hover ) {
        box( game->render, 308, 0, 11, 10, game->render->color_opt );
    }
    cancel_icon( game->render, 309, 1, cancel_hover ? game->render->color_background : game->render->color_opt );    

    int hover_index = 0;
    for( int y = 0; y < 3; ++y ) {
        for( int x = 0; x < 3; ++x ) {
            savegame_t* savegame = &game->savegames[ x + y * 3 ];
            bool valid_slot = savegame->thumb_width && savegame->thumb_height;
            int xp = 31 + x * 96;
            int yp = 19 + y * 72;
            bool hover = mouse_x >= xp && mouse_x < xp + 78 && mouse_y >= yp && mouse_y < yp + 60;
            if( hover ) {
                hover_index = x + y * 3 + 1;
            }
            if( !valid_slot ) {
                frame( game->render, 32 + x * 96, 20 + y * 72, 75, 57, game->render->color_background, hover ? game->render->color_opt : game->render->color_disabled );
            }
            char str[ 2 ] = { 0, 0 };
            *str = '1' + x + y * 3;
            text( game->render, game->render->font_opt, str, xp - 8, yp + 3, game->render->color_opt );

            if( valid_slot ) {
                if( savegame->thumb ) {
                    draw_savegame_thumb( game->render, 32 + x * 96, 20 + y * 72, savegame->thumb, savegame->thumb_width, savegame->thumb_height, game->render->color_background, hover ? game->render->color_opt : game->render->color_disabled );
                } else if( savegame->thumb_width && savegame->thumb_height ){
                    draw_savegame_bitmap( game->render, 32 + x * 96, 20 + y * 72, x + y * 3, savegame->thumb_width, savegame->thumb_height, game->render->color_background, hover ? game->render->color_opt : game->render->color_disabled );
                }

                char datetime[ 20 ] = "";
                strcat( datetime, savegame->date );
                strcat( datetime, "  " );
                strcat( datetime, savegame->time );
                center( game->render, game->render->font_txt, datetime, xp + 39, yp + 60, game->render->color_txt );
            }
       }
    }

    if( was_key_pressed( game, APP_KEY_1 ) || ( hover_index == 1 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 1 );
    }

    if( was_key_pressed( game, APP_KEY_2 ) || ( hover_index == 2 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 2 );
    }

    if( was_key_pressed( game, APP_KEY_3 ) || ( hover_index == 3 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 3 );
    }

    if( was_key_pressed( game, APP_KEY_4 ) || ( hover_index == 4 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 4 );
    }

    if( was_key_pressed( game, APP_KEY_5 ) || ( hover_index == 5 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 5 );
    }

    if( was_key_pressed( game, APP_KEY_6 ) || ( hover_index == 6 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 6 );
    }

    if( was_key_pressed( game, APP_KEY_7 ) || ( hover_index == 7 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 7 );
    }

    if( was_key_pressed( game, APP_KEY_8 ) || ( hover_index == 8 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 8 );
    }

    if( was_key_pressed( game, APP_KEY_9 ) || ( hover_index == 9 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        save_game( game, 9 );
    }

    if( was_key_pressed( game, APP_KEY_ESCAPE ) || ( cancel_hover && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        game->ingame_menu = false;
        game->savegame_menu = false;
    }
}


void loadgame_menu_update( game_t* game ) {
    cls( game->render );    

    center( game->render, game->render->font_name, "LOAD GAME", 160, 6, game->render->color_opt );

    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game->render, &mouse_x, &mouse_y );

    bool cancel_hover = mouse_y < 16 && mouse_x > 295;
    if( cancel_hover ) {
        box( game->render, 308, 0, 11, 10, game->render->color_opt );
    }
    cancel_icon( game->render, 309, 1, cancel_hover ? game->render->color_background : game->render->color_opt );    

    int hover_index = 0;
    for( int y = 0; y < 3; ++y ) {
        for( int x = 0; x < 3; ++x ) {
            savegame_t* savegame = &game->savegames[ x + y * 3 ];
            bool valid_slot = savegame->thumb_width && savegame->thumb_height;
            int xp = 31 + x * 96;
            int yp = 19 + y * 72;
            bool hover = mouse_x >= xp && mouse_x < xp + 78 && mouse_y >= yp && mouse_y < yp + 60;
            if( hover ) {
                hover_index = x + y * 3 + 1;
            }
            if( !valid_slot ) {
                frame( game->render, 32 + x * 96, 20 + y * 72, 75, 57, game->render->color_background, hover ? game->render->color_opt : game->render->color_disabled );
            }
            char str[ 2 ] = { 0, 0 };
            *str = '1' + x + y * 3;
            text( game->render, game->render->font_opt, str, xp - 8, yp + 3, game->render->color_opt );

            if( valid_slot ) {
                if( savegame->thumb ) {
                    draw_savegame_thumb( game->render, 32 + x * 96, 20 + y * 72, savegame->thumb, savegame->thumb_width, savegame->thumb_height, game->render->color_background, hover ? game->render->color_opt : game->render->color_disabled );
                } else if( savegame->thumb_width && savegame->thumb_height ){
                    draw_savegame_bitmap( game->render, 32 + x * 96, 20 + y * 72, x + y * 3, savegame->thumb_width, savegame->thumb_height, game->render->color_background, hover ? game->render->color_opt : game->render->color_disabled );
                }

                char datetime[ 20 ] = "";
                strcat( datetime, savegame->date );
                strcat( datetime, "  " );
                strcat( datetime, savegame->time );
                center( game->render, game->render->font_txt, datetime, xp + 39, yp + 60, game->render->color_txt );
            }
       }
    }

    if( was_key_pressed( game, APP_KEY_1 ) || ( hover_index == 1 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 1 );
    }

    if( was_key_pressed( game, APP_KEY_2 ) || ( hover_index == 2 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 2 );
    }

    if( was_key_pressed( game, APP_KEY_3 ) || ( hover_index == 3 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 3 );
    }

    if( was_key_pressed( game, APP_KEY_4 ) || ( hover_index == 4 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 4 );
    }

    if( was_key_pressed( game, APP_KEY_5 ) || ( hover_index == 5 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 5 );
    }

    if( was_key_pressed( game, APP_KEY_6 ) || ( hover_index == 6 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 6 );
    }

    if( was_key_pressed( game, APP_KEY_7 ) || ( hover_index == 7 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 7 );
    }

    if( was_key_pressed( game, APP_KEY_8 ) || ( hover_index == 8 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 8 );
    }

    if( was_key_pressed( game, APP_KEY_9 ) || ( hover_index == 9 && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        load_game( game, 9 );
    }

    if( was_key_pressed( game, APP_KEY_ESCAPE ) || ( cancel_hover && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        game->ingame_menu = false;
        game->loadgame_menu = false;
    }
}


bool button( render_t* render, int x, int y, int w, int h, pixelfont_t* font, char const* text, int background, int color, bool selected, int mouse_x, int mouse_y ) {
    bool hover = !selected && mouse_x >= x && mouse_y >= y && mouse_x <= x + w && mouse_y <= y + h;
    if( selected ) {
        frame2( render, x, y, w, h, hover ? color : background, hover ? background : color );
    } else {
        frame( render, x, y, w, h, hover ? color : background, hover ? background : color );
    }
    center( render, font, text, x + w / 2, y + 1, hover ? background : color );
    return hover;
}


void settings_menu_update( game_t* game ) {
    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game->render, &mouse_x, &mouse_y );

    frame( game->render, 99, 39, 124, 164, game->render->color_background, game->render->color_opt );

    int name_height = game->render->font_name->height;
    int opt_height = game->render->font_opt->height;
    scale_for_resolution_inverse( game->render, &name_height, &opt_height );

    int ypos = 50;
    center( game->render, game->render->font_name, "SETTINGS", 160, ypos, game->render->color_opt );
    ypos += name_height + 10;

    text( game->render, game->render->font_name, "SOUND", 110, ypos, game->render->color_opt );
    ypos += name_height + 2;

    int w = 110 / 4 - 5;
    int xpos = 110;
    bool sound_off = button( game->render, xpos, ypos, w, opt_height + 2, game->render->font_opt, "OFF", game->render->color_background, game->render->color_opt, game->sound_level == 0, mouse_x, mouse_y );
    xpos += w + 5;
    bool sound_low = button( game->render, xpos, ypos, w, opt_height + 2, game->render->font_opt, "LOW", game->render->color_background, game->render->color_opt, game->sound_level == 1, mouse_x, mouse_y );
    xpos += w + 5;
    bool sound_mid = button( game->render, xpos, ypos, w, opt_height + 2, game->render->font_opt, "MID", game->render->color_background, game->render->color_opt, game->sound_level == 2, mouse_x, mouse_y );
    xpos += w + 5;
    bool sound_max = button( game->render, xpos, ypos, w, opt_height + 2, game->render->font_opt, "MAX", game->render->color_background, game->render->color_opt, game->sound_level == 3, mouse_x, mouse_y );

    ypos += opt_height + 2 + 10;

    text( game->render, game->render->font_name, "SCREEN MODE", 110, ypos, game->render->color_opt );
    ypos += name_height + 2;

    w = 107 / 2 - 5;
    xpos = 110;
    bool screen_window = button( game->render, xpos, ypos, w, opt_height + 2, game->render->font_opt, "WINDOW", game->render->color_background, game->render->color_opt, !game->fullscreen, mouse_x, mouse_y );
    xpos += w + 5;
    bool screen_full = button( game->render, xpos, ypos, w, opt_height + 2, game->render->font_opt, "FULL", game->render->color_background, game->render->color_opt, game->fullscreen, mouse_x, mouse_y );

    ypos += opt_height + 2 + 10;

    text( game->render, game->render->font_name, "DISPLAY FILTER", 110, ypos, game->render->color_opt );
    ypos += name_height + 2;

    char const* filters[ 4 ] = { NULL };
    for( int i = 0; i < game->yarn->globals.display_filters->count; ++i ) {
        yarn_display_filter_t filter = game->yarn->globals.display_filters->items[ i ];
        switch( filter ) {
            case YARN_DISPLAY_FILTER_NONE: {
                filters[ i ] = "OFF";
            } break;
            case YARN_DISPLAY_FILTER_TV: {
                filters[ i ] = "TV";
            } break;
            case YARN_DISPLAY_FILTER_PC: {
                filters[ i ] = "PC";
            } break;
            case YARN_DISPLAY_FILTER_LITE: {
                filters[ i ] = "LITE";
            } break;
        }
    }

    bool filter_hover[ 4 ] = { false };
    w = 110 / 4 - 4;
    xpos = 110 + ( w + 4 ) * game->display_filter_index;
    filter_hover[ game->display_filter_index ] = button( game->render, xpos, ypos, w, opt_height + 2, game->render->font_opt, filters[ game->display_filter_index ], game->render->color_background, game->render->color_opt, true, mouse_x, mouse_y );
    xpos = 110;
    for( int i = 0; i < 4; ++i ) {
        if( filters[ i ] && i != game->display_filter_index ) {
            filter_hover[ i ] = button( game->render, xpos, ypos, w, opt_height + 2, game->render->font_opt, filters[ i ], game->render->color_background, game->render->color_opt, false, mouse_x, mouse_y );
        }
        xpos += w + 4;
    }

    ypos += opt_height + 2 + 15;

    bool done_hover = mouse_x >= 100 && mouse_y <= 224 && mouse_y >= ypos - 2 && mouse_y <= ypos + name_height;
    if( done_hover ) {
        box( game->render, 110, ypos - 2, 100, name_height + 4, done_hover ? game->render->color_opt : game->render->color_background );
    }
    center( game->render, game->render->font_name, "DONE", 160, ypos, done_hover ? game->render->color_background : game->render->color_opt );
    
    if( was_key_pressed( game, APP_KEY_ESCAPE ) || ( done_hover && was_key_pressed( game, APP_KEY_LBUTTON ) ) ) {
        game->settings_menu = false;
    }
    if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
        if( sound_off ) {
            game->sound_level = 0;
            audiosys_master_volume_set( game->audiosys, 0.0f );
        }
        if( sound_low ) {
            game->sound_level = 1;
            audiosys_master_volume_set( game->audiosys, 0.33f );
        }
        if( sound_mid ) {
            game->sound_level = 2;
            audiosys_master_volume_set( game->audiosys, 0.66f );
        }
        if( sound_max ) {
            game->sound_level = 3;
            audiosys_master_volume_set( game->audiosys, 1.0f );
        }
        if( screen_window ) {
            game->fullscreen = false;
        }
        if( screen_full ) {
            game->fullscreen = true;
        }
        for( int i = 0; i < 4; ++i ) {
            if( filter_hover[ i ] ) {
                game->display_filter_index = i;
            }
        }
    }
}


void ingame_menu_update( game_t* game ) {
    if( game->savegame_menu ) {
        savegame_menu_update( game );
        return;
    }
    if( game->loadgame_menu ) {
        loadgame_menu_update( game );
        return;
    }
    if( game->settings_menu ) {
        settings_menu_update( game );
        return;
    }
    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game->render, &mouse_x, &mouse_y );

    int spacing = game->render->font_name->height * 2;
    scale_for_resolution_inverse( game->render, &spacing, NULL );
    int ypos = 40 + ( 160 - ( 6 * spacing ) ) / 2;
    int option = ( mouse_y - ypos ) / spacing;
    if( option > 5 || mouse_y < ypos ) {
        option = -1;
    }

    bool clicked = was_key_pressed( game, APP_KEY_LBUTTON );
    if( was_key_pressed( game, APP_KEY_ESCAPE ) || was_key_pressed( game, APP_KEY_1 ) || ( clicked && option == 0 ) ) {
        game->ingame_menu = false;
    } else if( was_key_pressed( game, APP_KEY_2 ) || ( clicked && option == 1 ) ) {
        game->savegame_menu = true;
        load_savegames( game );
    } else if( was_key_pressed( game, APP_KEY_3 ) || ( clicked && option == 2 ) ) {
        game->loadgame_menu = true;
        load_savegames( game );
    } else if( was_key_pressed( game, APP_KEY_4 ) || ( clicked && option == 3 ) ) {
        game->settings_menu = true;
    } else if( was_key_pressed( game, APP_KEY_5 ) || ( clicked && option == 4 ) ) {
        game->restart_requested = true;    
        game->ingame_menu = false;
    } else if( was_key_pressed( game, APP_KEY_6 ) || ( clicked && option == 5 ) ) {
        game->exit_dialog = true;
    }

    int color_opt = game->exit_dialog ? game->render->color_disabled : game->render->color_opt;

    frame( game->render, 99, 39, 124, 164, game->render->color_background, color_opt );
    if( option >= 0 ) {
        box( game->render, 110, ypos + ( option  ) * spacing + spacing / 6, 100, spacing - spacing / 3, color_opt );
    }
    int offs = -spacing + spacing / 4;
    center( game->render, game->render->font_name, "RESUME", 160, offs + ( ypos+=spacing ), option == 0 ? game->render->color_background : color_opt );
    center( game->render, game->render->font_name, "SAVE GAME", 160, offs + ( ypos+=spacing ), option == 1 ? game->render->color_background : color_opt );
    center( game->render, game->render->font_name, "LOAD GAME", 160, offs + ( ypos+=spacing ), option == 2 ? game->render->color_background : color_opt );
    center( game->render, game->render->font_name, "SETTINGS", 160, offs + ( ypos+=spacing ), option == 3 ? game->render->color_background : color_opt );
    center( game->render, game->render->font_name, "RESTART", 160, offs + ( ypos+=spacing ), option == 4 ? game->render->color_background : color_opt );
    center( game->render, game->render->font_name, "QUIT", 160, offs + ( ypos+=spacing ), option == 5 ? game->render->color_background : color_opt );
}


void exit_dialog_update( game_t* game ) {
    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game->render, &mouse_x, &mouse_y );

    frame( game->render, 120, 95, 80, 50, game->render->color_background, game->render->color_opt );
    center_wrap( game->render, game->render->font_txt, "Are you sure you want to quit?", 160, 100, game->render->color_opt, 80 );

    int height = game->render->font_opt->height;
    scale_for_resolution_inverse( game->render, &height, NULL );
    height += 2;

    int ypos = 145 - height - 7;

    bool no = false;
    bool yes = false;
    if( mouse_y >= ypos && mouse_y <= ypos + height ) {
        if( mouse_x >= 130 && mouse_x <= 130 + 25 ) {
            no = true;
        } else if( mouse_x >= 165 && mouse_x <= 165 + 25 ) {
            yes = true;
        }
    }

    frame( game->render, 130, ypos, 25, height, no ? game->render->color_opt : game->render->color_background, 
        no ? game->render->color_background : game->render->color_opt );
    center_wrap( game->render, game->render->font_opt, "No", 142, ypos + 1, 
        no ? game->render->color_background : game->render->color_opt, 25 );

    frame2( game->render, 165, ypos, 25, height, yes ? game->render->color_opt : game->render->color_background, 
        yes ? game->render->color_background : game->render->color_opt );
    center_wrap( game->render, game->render->font_opt, "Yes", 177, ypos + 1, 
        yes ? game->render->color_background : game->render->color_opt, 25 );

    bool clicked = was_key_pressed( game, APP_KEY_LBUTTON );
    if( was_key_pressed( game, APP_KEY_ESCAPE ) || ( clicked && no ) ) {
        game->exit_dialog = false;
    } else if( was_key_pressed( game, APP_KEY_RETURN ) || ( clicked && yes ) ) {
        game->exit_dialog = false;
        game->exit_confirmed = true;
    }
}


// boot
gamestate_t boot_init( game_t* game ) {
    cls( game->render );
    return GAMESTATE_NO_CHANGE;
}


gamestate_t boot_update( game_t* game ) {
    (void) game;
    #ifdef __wasm__       
        cls( game->render );
        if( game->blink_visible ) {
            center( game->render, game->yarn->assets.font_name, "CLICK TO START", 160, 120 - game->yarn->assets.font_name->height / 2, game->render->color_name );
        }
        if( !was_key_pressed( game, APP_KEY_LBUTTON ) ) {
            return GAMESTATE_NO_CHANGE;
        }
    #endif

    if( game->state.current_screen >= 0 ) {
        return GAMESTATE_SCREEN;
    } else if( game->state.current_location >= 0 ) {
        return GAMESTATE_LOCATION;
    } else if( game->state.current_dialog >= 0 ) {
        return GAMESTATE_DIALOG;
    } else {
        return GAMESTATE_EXIT;
    }
    return GAMESTATE_NO_CHANGE;
}


// screen
gamestate_t screen_init( game_t* game ) {
    stack_entry_t entry;
    entry.type = STACK_ENTRY_SCREEN;
    entry.index = game->state.current_screen;
    array_add( game->state.section_stack, &entry );

    game->queued_screen = -1;
    game->queued_location = -1;
    game->queued_dialog = -1;

    yarn_t* yarn = game->yarn;
    yarn_screen_t* screen = &yarn->screens->items[ game->state.current_screen ];

    // mus: amb: snd:
    do_audio( game, screen->audio );

    // act:
    do_actions( game, screen->act );
    return GAMESTATE_NO_CHANGE;
}


gamestate_t screen_update( game_t* game ) {
    cls( game->render );

    yarn_t* yarn = game->yarn;

    if( game->state.current_screen < 0 && game->state.current_dialog >= 0 ) {
        return GAMESTATE_DIALOG;
    }

    if( game->state.current_screen < 0 && game->state.current_location >= 0 ) {
        return GAMESTATE_LOCATION;
    }

    if( game->state.current_screen < 0  ) return GAMESTATE_TERMINATE;

    yarn_screen_t* screen = &yarn->screens->items[ game->state.current_screen ];

    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game->render, &mouse_x, &mouse_y );

    bool menu_hover = mouse_y < 15 && mouse_x > 285;
    if( menu_hover ) {
        box( game->render, 308, 0, 10, 6, game->render->color_opt );
    }
    menu_icon( game->render, 309, 1, menu_hover ? game->render->color_background : game->render->color_opt );    

    if( game->queued_dialog >= 0 && ( ( was_key_pressed( game, APP_KEY_LBUTTON ) && !menu_hover ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
        game->state.current_screen = -1;
        if( game->state.current_dialog >= 0 ) {
            game->state.current_dialog = game->queued_dialog;
            game->disable_transition = true;
            return GAMESTATE_DIALOG;
        } else {
            game->state.current_dialog = game->queued_dialog;
            return GAMESTATE_DIALOG;
        }
    } else if( game->queued_location >= 0 && ( ( was_key_pressed( game, APP_KEY_LBUTTON ) && !menu_hover ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
        game->state.current_screen = -1;
        if( game->state.current_location >= 0 ) {
            game->state.current_location = game->queued_location;
            game->disable_transition = true;
            return GAMESTATE_LOCATION;
        } else {
            game->state.current_location = game->queued_location;
            return GAMESTATE_LOCATION;
        }
    } else if( game->queued_screen >= 0 && ( was_key_pressed( game, APP_KEY_LBUTTON ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
        game->state.current_screen = game->queued_screen;
        game->state.current_location = -1;
        game->state.current_dialog = -1;
        return GAMESTATE_SCREEN;
    }


    // scr:
    for( int i = 0; i < screen->scr->count; ++i ) {
        if( test_cond( game, &screen->scr->items[ i ].cond ) )  {
            game->state.current_image = screen->scr->items[ i ].scr_index;
        }
    }
    if( game->state.current_image >= 0 ) {
        draw( game->render, game->state.current_image, 0, 0 );
    }

    if( menu_hover && was_key_pressed( game, APP_KEY_LBUTTON ) ) {
        enter_menu( game );
        return GAMESTATE_NO_CHANGE;
    }


    return GAMESTATE_NO_CHANGE;
}


// location
gamestate_t location_init( game_t* game ) {
    stack_entry_t entry;
    entry.type = STACK_ENTRY_LOCATION;
    entry.index = game->state.current_location;
    array_add( game->state.section_stack, &entry );

    game->queued_screen = -1;
    game->queued_location = -1;
    game->queued_dialog = -1;
    game->limit = -30.0f;

    yarn_t* yarn = game->yarn;
    yarn_location_t* location = &yarn->locations->items[ game->state.current_location ];

    // mus: amb: snd:
    do_audio( game, location->audio );

    // act:
    do_actions( game, location->act );

    return GAMESTATE_NO_CHANGE;
}


gamestate_t location_update( game_t* game ) {
    cls( game->render );

    yarn_t* yarn = game->yarn;

    if( game->state.current_location < 0 && game->state.current_dialog >= 0 ) {
        return GAMESTATE_DIALOG;
    }

    if( game->state.current_location < 0  ) return GAMESTATE_TERMINATE;

    yarn_location_t* location = &yarn->locations->items[ game->state.current_location ];

    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game->render, &mouse_x, &mouse_y );

    // background_location:
    if( yarn->globals.background_location >= 0 ) {
        draw( game->render, yarn->globals.background_location, 0, 0 );
    }

    bool menu_hover = mouse_y < 15 && mouse_x > 285;
    if( menu_hover ) {
        box( game->render, 308, 0, 10, 6, game->render->color_opt );
    }
    menu_icon( game->render, 309, 1, menu_hover ? game->render->color_background : game->render->color_opt );    

    // txt:
    string txt = "";
    for( int i = 0; i < location->txt->count; ++i ) {
        if( !test_cond( game, &location->txt->items[ i ].cond ) ) {
            continue;
        }
        txt = cstr_cat( txt, cstr_cat( location->txt->items[ i ].text, "\n" ) );
    }

    if( game->yarn->globals.location_print_speed == 0 || game->limit >= strlen( txt ) ) {
        if( game->queued_dialog >= 0 && ( ( was_key_pressed( game, APP_KEY_LBUTTON ) && !menu_hover ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
            game->state.current_location = -1;
            if( game->state.current_dialog >= 0 ) {
                game->state.current_dialog = game->queued_dialog;
                game->disable_transition = true;
                return GAMESTATE_DIALOG;
            } else {
                game->state.current_dialog = game->queued_dialog;
                return GAMESTATE_DIALOG;
            }
        } else if( game->queued_screen >= 0 && ( ( was_key_pressed( game, APP_KEY_LBUTTON ) && !menu_hover ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
            game->state.current_location = -1;
            if( game->state.current_screen >= 0 ) {
                game->state.current_screen = game->queued_screen;
                game->disable_transition = true;
                return GAMESTATE_SCREEN;
            } else {
                game->state.current_screen = game->queued_screen;
                return GAMESTATE_SCREEN;
            }
        } else if( game->queued_location >= 0 && ( ( was_key_pressed( game, APP_KEY_LBUTTON ) && !menu_hover ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
            game->state.current_location = game->queued_location;
            game->state.current_dialog = -1;
            game->state.current_screen = -1;
            return GAMESTATE_LOCATION;
        }
    }

    // img:
    for( int i = 0; i < location->img->count; ++i ) {
        if( test_cond( game, &location->img->items[ i ].cond ) )  {
            game->state.current_image = location->img->items[ i ].image_index;
        }
    }
    if( game->state.current_image >= 0 ) {
        draw( game->render, game->state.current_image, 64, 10 );
    }

    // txt:
    game->limit += game->delta_time * game->yarn->globals.location_print_speed;
    if( game->yarn->globals.location_print_speed == 0 ) {
        game->limit = strlen( txt );
    }

    int hmargin = game->yarn->globals.hmargin_txt;
    int vmargin = game->yarn->globals.vmargin_txt;
    wrap_limit( game->render, game->render->font_txt, txt, 5 + hmargin, 146 + vmargin, game->render->color_txt, 310 - hmargin * 2, game->limit < 0.0f ? 0 : (int)game->limit );

    if( was_key_pressed( game, APP_KEY_LBUTTON) || was_key_pressed( game, APP_KEY_SPACE ) )  {
        game->limit = (float) strlen( txt );
    }

    // opt:
    int opt = -1;
    if( game->queued_dialog < 0 && game->queued_location < 0 && game->queued_screen < 0) {
        if( was_key_pressed( game, APP_KEY_1 ) ) opt = 0;
        if( was_key_pressed( game, APP_KEY_2 ) ) opt = 1;
        if( was_key_pressed( game, APP_KEY_3 ) ) opt = 2;
        if( was_key_pressed( game, APP_KEY_4 ) ) opt = 3;

        int c = 0;
        for( int i = 0; i < location->opt->count; ++i ) {
            if( !test_cond( game, &location->opt->items[ i ].cond ) ) {
                continue;
            }
            hmargin = game->yarn->globals.hmargin_opt;
            vmargin = game->yarn->globals.vmargin_opt;
            int ypos = 197 + vmargin + font_height( game->render, game->render->font_opt->height ) * c;
            pixelfont_bounds_t b = text( game->render, game->render->font_opt, location->opt->items[ i ].text, 5 + hmargin, ypos, game->render->color_opt );
            if( mouse_x >= hmargin && mouse_x <= 320 - hmargin && mouse_y >= ypos && mouse_y < ypos + b.height ) {
                box( game->render, 4 + hmargin, ypos - 1, 315 - hmargin * 2, b.height + 1, game->render->color_opt );
                text( game->render, game->render->font_opt, location->opt->items[ i ].text, 5 + hmargin, ypos, game->render->color_background );
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
        int color = game->render->color_disabled;
        bool enabled = false;
        for( int j = 0; j < location->use->count; ++j ) {
            if( !test_cond( game, &location->use->items[ j ].cond ) ) {
                continue;
            }
            for( int k = 0; k < location->use->items[ j ].item_indices->count; ++k ) {
                if( game->state.items->items[ i ] == location->use->items[ j ].item_indices->items[ k ] ) {
                    if( game->queued_dialog < 0 && game->queued_location < 0 && game->queued_screen ) {
                        color = game->render->color_use;
                        if( game->state.first_chr_or_use && !game->blink_visible ) {
                            color = game->render->color_background;
                        }
                        enabled = true;
                    }
                }
            }
        }
        hmargin = game->yarn->globals.hmargin_use;
        vmargin = game->yarn->globals.vmargin_use;
        int ypos = 4 + vmargin + ( ( 117 - ( game->state.items->count * font_height( game->render, game->render->font_use->height ) ) ) / 2 ) + c * font_height( game->render, game->render->font_use->height );
        pixelfont_bounds_t b = center( game->render, game->render->font_use, usetxt, 287 + hmargin, ypos, color );
        if( enabled && mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x > 259 + hmargin ) {
            box( game->render, 260 + hmargin, ypos - 1, 56, b.height + 1, game->render->color_use );
            center( game->render, game->render->font_use, usetxt, 287 + hmargin, ypos, game->render->color_background );
            if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                use = c;
            }
        }
        ++c;
    }

    if( c == 0 ) {
        hmargin = game->yarn->globals.hmargin_use;
        vmargin = game->yarn->globals.vmargin_use;
        int ypos = 4 + vmargin + ( ( 117 - ( 2 * font_height( game->render, game->render->font_use->height ) ) ) / 2 );
        center_wrap( game->render,  game->render->font_use, yarn->globals.nothing_text, 287 + hmargin, ypos, game->render->color_disabled, 56 );
    }


    // chr:
    int chr_count = 0;
    for( int i = 0; i < location->chr->count; ++i ) {
        if( !test_cond( game, &location->chr->items[ i ].cond ) ) {
            continue;
        }
        ++chr_count;
    }
    for( int i = 0; i < game->state.chars->count; ++i ) {
        bool found = false;
        for( int j = 0; j < location->chr->count; ++j ) {
            if( game->state.chars->items[ i ] == location->chr->items[ j ].chr_indices->items[ 0 ] && test_cond( game, &location->chr->items[ j ].cond ) ) {
                found = true;
                break;
            }
        }

        if( !found ) {
            ++chr_count;
        }
    }
    
    int chr = -1;
    c = 0;
    for( int i = 0; i < location->chr->count; ++i ) {
        if( !test_cond( game, &location->chr->items[ i ].cond ) ) {
            continue;
        }
        hmargin = game->yarn->globals.hmargin_chr;
        vmargin = game->yarn->globals.vmargin_chr;
        int ypos = 4 + vmargin + ( ( 117 - ( chr_count * font_height( game->render, game->render->font_chr->height ) ) ) / 2 ) + c * font_height( game->render, game->render->font_chr->height );
        int color = game->render->color_chr;
        if( game->queued_dialog >= 0 || game->queued_location >= 0 || game->queued_screen >= 0) {
            color = game->render->color_disabled;
        } else if( game->state.first_chr_or_use && !game->blink_visible ) {
            color = game->render->color_background;
        }


        pixelfont_bounds_t b = center( game->render,  game->render->font_chr, game->yarn->characters->items[ location->chr->items[ i ].chr_indices->items[ 0 ] ].short_name, 32 + hmargin, ypos, color );
        if( game->queued_dialog < 0 && game->queued_location < 0 && game->queued_screen < 0 ) {
            if( mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x < hmargin + 60 ) {
                box( game->render, 4, ypos - 1, 56, b.height + 1, game->render->color_chr );
                center( game->render,  game->render->font_chr, game->yarn->characters->items[ location->chr->items[ i ].chr_indices->items[ 0 ] ].short_name, 32 + hmargin, ypos, game->render->color_background );
                if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                    chr = c;
                }
            }
        }
        ++c;
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
            hmargin = game->yarn->globals.hmargin_chr;
            vmargin = game->yarn->globals.vmargin_chr;
            int ypos = 4 + vmargin + ( ( 117 - ( chr_count * font_height( game->render, game->render->font_chr->height ) ) ) / 2 ) + c * font_height( game->render, game->render->font_chr->height );
            int color = game->render->color_disabled;
            pixelfont_bounds_t b = center( game->render,  game->render->font_chr, game->yarn->characters->items[ game->state.chars->items[ i ] ].short_name, 32 + hmargin, ypos, color );
            ++c;
        }
    }

    if( c == 0 ) {
        hmargin = game->yarn->globals.hmargin_use;
        vmargin = game->yarn->globals.vmargin_use;
        int ypos = 4 + vmargin + ( ( 117 - ( 2 * font_height( game->render, game->render->font_chr->height ) ) ) / 2 );
        center_wrap( game->render,  game->render->font_chr, yarn->globals.alone_text, 32 + hmargin, ypos, game->render->color_disabled, 56 );
    }

    if( menu_hover && was_key_pressed( game, APP_KEY_LBUTTON ) ) {
        enter_menu( game );
        return GAMESTATE_NO_CHANGE;
    }

    if( game->queued_dialog < 0 && game->queued_location < 0 && game->queued_screen < 0 ) {
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
                game->state.first_chr_or_use = false;
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
                            game->state.first_chr_or_use = false;
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
        } else if( game->queued_screen >= 0 ) {
            game->state.current_location = -1;
            if( game->state.current_screen >= 0 ) {
                game->state.current_screen = game->queued_screen;
                game->disable_transition = true;
                return GAMESTATE_SCREEN;
            } else {
                game->state.current_screen = game->queued_screen;
                return GAMESTATE_SCREEN;
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
    entry.type = STACK_ENTRY_DIALOG;
    entry.index = game->state.current_dialog;
    array_add( game->state.section_stack, &entry );

    game->queued_screen = -1;
    game->queued_location = -1;
    game->queued_dialog = -1;
    game->limit = -30.0f;

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
        game->limit = 0.0f;
    }

    return GAMESTATE_NO_CHANGE;
}


gamestate_t dialog_update( game_t* game ) {
    if( game->state.current_dialog < 0 ) {
        return GAMESTATE_TERMINATE;
    }

    cls( game->render );
    yarn_t* yarn = game->yarn;
    yarn_dialog_t* dialog = &yarn->dialogs->items[ game->state.current_dialog ];

    int mouse_x = input_get_mouse_x( game->input );
    int mouse_y = input_get_mouse_y( game->input );
    scale_for_resolution_inverse( game->render, &mouse_x, &mouse_y );

    // background_dialog:
    if( yarn->globals.background_dialog >= 0 ) {
        draw( game->render, yarn->globals.background_dialog, 0, 0 );
    }

    bool menu_hover = mouse_y < 15 && mouse_x > 285;
    if( menu_hover ) {
        box( game->render, 308, 0, 10, 6, game->render->color_opt );
    }
    menu_icon( game->render, 309, 1, menu_hover ? game->render->color_background : game->render->color_opt );    

    
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
                int hmargin = game->yarn->globals.hmargin_dialog;
                int vmargin = game->yarn->globals.vmargin_dialog;   
                //pixelfont_bounds_t bounds = text_bounds( game->render, game->render->font_dialog, txt );
                //if( bounds.width < 310 ) {
                //    center_limit( game->render, game->render->font_dialog, txt, 160, 136, game->render->color_dialog, game->limit < 0.0f ? 0 : (int)game->limit );
                //} else {
                    wrap_limit( game->render, game->render->font_dialog, txt, 5 + hmargin, 136 + vmargin, game->render->color_dialog, 310 - hmargin * 2, game->limit < 0.0f ? 0 : (int)game->limit );
                //}                    
            } else {
                int hmargin = game->yarn->globals.hmargin_response;
                int vmargin = game->yarn->globals.vmargin_response;    
                wrap_limit( game->render, game->render->font_response, txt, 5 + hmargin, 197 + vmargin, game->render->color_response, 310 - hmargin * 2, game->limit < 0.0f ? 0 : (int)game->limit );
            }
        }
        ++phrase_count;
    }

    game->limit += game->delta_time * game->yarn->globals.dialog_print_speed;
    if( game->yarn->globals.dialog_print_speed == 0 ) {
        game->limit = game->dialog.phrase_len + 1;
    }
    if( game->limit > game->dialog.phrase_len && ( ( was_key_pressed( game, APP_KEY_LBUTTON) && !menu_hover ) || was_key_pressed( game, APP_KEY_SPACE ) ) ) {
        if( game->dialog.phrase_index < phrase_count - 1 ) {
            ++game->dialog.phrase_index;
            game->limit = -30.0f;
        } else if( game->dialog.enable_options == 0 ) {
            game->dialog.enable_options = 1;
        }
    } else if( ( was_key_pressed( game, APP_KEY_LBUTTON) && !menu_hover ) || was_key_pressed( game, APP_KEY_SPACE ) )  {
        game->limit = (float) game->dialog.phrase_len;
        game->dialog.enable_options = 0;
    }

    if( game->dialog.enable_options == 1 && !was_key_pressed( game, APP_KEY_LBUTTON ) && !was_key_pressed( game, APP_KEY_SPACE ) ) {
        game->dialog.enable_options = 2;
    }

    if( game->dialog.chr_index >= 0 ) {
        yarn_character_t* character = &game->yarn->characters->items[ game->dialog.chr_index ];
        if( character->face_index >= 0 ) {
            draw( game->render, character->face_index, 104, 18 );
        }
        int hmargin = game->yarn->globals.hmargin_name;
        int vmargin = game->yarn->globals.vmargin_name;
        center( game->render, game->render->font_name, character->name, 160 + hmargin, 6 + vmargin, game->render->color_name );
    }
    
    // say:
    int say = -1;
    if( game->dialog.enable_options == 2 ) {
        if( game->queued_dialog < 0 && game->queued_location < 0 && game->queued_screen < 0 ) {
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
            int hmargin = game->yarn->globals.hmargin_say;
            int vmargin = game->yarn->globals.vmargin_say;
            int ypos = 197 + vmargin + font_height( game->render, game->render->font_say->height ) * c;
            pixelfont_bounds_t b = text( game->render, game->render->font_say, dialog->say->items[ i ].text, 5 + hmargin, ypos, game->render->color_say );
            if( game->queued_dialog < 0 && game->queued_location < 0 && game->queued_screen < 0 ) {
                if( mouse_x >= hmargin && mouse_x <= 320 - hmargin && mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x < 277 ) {
                    box( game->render, 4 + hmargin, ypos - 1, 315 - hmargin * 2, b.height + 1, game->render->color_say );
                    text( game->render, game->render->font_say, dialog->say->items[ i ].text, 5 + hmargin, ypos, game->render->color_background );
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
        int color = game->render->color_disabled;
        bool enabled = false;
        for( int j = 0; j < dialog->use->count; ++j ) {
            if( !test_cond( game, &dialog->use->items[ j ].cond ) ) {
                continue;
            }
            for( int k = 0; k < dialog->use->items[ j ].item_indices->count; ++k ) {
                if( game->state.items->items[ i ] == dialog->use->items[ j ].item_indices->items[ k ] ) {
                    if( game->dialog.enable_options == 2 ) {
                        if( game->queued_dialog < 0 && game->queued_location < 0 && game->queued_screen < 0 ) {
                            color = game->render->color_use;
                            enabled = true;
                        }
                    }
                }
            }
        }
        int hmargin = game->yarn->globals.hmargin_use;
        int vmargin = game->yarn->globals.vmargin_use;   
        int ypos = 4 + vmargin + ( ( 117 - ( game->state.items->count * font_height( game->render, game->render->font_use->height ) ) ) / 2 ) + c * font_height( game->render, game->render->font_use->height );
        pixelfont_bounds_t b = center( game->render, game->render->font_use, txt, 287 + hmargin, ypos, color );
        if( enabled && mouse_y >= ypos && mouse_y < ypos + b.height && mouse_x > 259 + hmargin ) {
            box( game->render, 260 + hmargin, ypos - 1, 56, b.height + 1, game->render->color_use );
            center( game->render, game->render->font_use, txt, 287 + hmargin, ypos, game->render->color_background );
            if( was_key_pressed( game, APP_KEY_LBUTTON ) ) {
                use = c;
            }
        }
        ++c;
    }

    if( c == 0 ) {
        int hmargin = game->yarn->globals.hmargin_use;
        int vmargin = game->yarn->globals.vmargin_use;   
        int ypos = 4 + vmargin + ( ( 117 - ( 2 * font_height( game->render, game->render->font_use->height ) ) ) / 2 );
        center_wrap( game->render,  game->render->font_use, yarn->globals.nothing_text, 287 + hmargin, ypos, game->render->color_disabled, 56 );
    }

    if( menu_hover && was_key_pressed( game, APP_KEY_LBUTTON ) ) {
        enter_menu( game );
        return GAMESTATE_NO_CHANGE;
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
    else if( game->queued_screen >= 0 && game->dialog.enable_options == 2  ) {
        game->state.current_screen = game->queued_screen;
        game->state.current_dialog = -1;
        return GAMESTATE_SCREEN;
    }
    else if( game->queued_location >= 0 && game->dialog.enable_options == 2  ) {
        game->state.current_location = game->queued_location;
        game->state.current_dialog = -1;
        return GAMESTATE_LOCATION;
    }

    if( game->dialog.enable_options == 2 ) {

        if( game->queued_dialog < 0 && game->queued_location < 0 && game->queued_screen < 0 ) {
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
            } else if( game->queued_screen >= 0 ) {
                game->state.current_screen = game->queued_screen;
                game->state.current_dialog = -1;
                return GAMESTATE_SCREEN;
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
    draw_screenshot( game->render );
    return GAMESTATE_NO_CHANGE;
}


gamestate_t exit_update( game_t* game ) {
    draw_screenshot( game->render ); 
    if( was_key_pressed( game, APP_KEY_LBUTTON ) || was_key_pressed( game, APP_KEY_SPACE ) || was_key_pressed( game, APP_KEY_ESCAPE ) ) {
        return GAMESTATE_TERMINATE;
    }
    return GAMESTATE_NO_CHANGE;
}


gamestate_t terminate_init( game_t* game ) {
    (void) game;
    draw_screenshot( game->render ); 
    return GAMESTATE_NO_CHANGE;
}


gamestate_t terminate_update( game_t* game ) {
    draw_screenshot( game->render ); 
    game->exit_flag = true;
    return GAMESTATE_NO_CHANGE;
}
