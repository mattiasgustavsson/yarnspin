typedef struct input_t {
    app_t* app_;
    bool prev_[ 256 ];
    bool curr_[ 256 ];
    int mouse_x;
    int mouse_y;
} input_t;


void input_init( struct input_t* input, app_t* app ) {
    input->app_ = app;
    for( int i = 0; i < 256; ++i ) {
        input->prev_[ i ] = false;
        input->curr_[ i ] = false;
    }
}


void input_update(  struct input_t* input, int screen_width, int screen_height, crtemu_lite_t* crtemu_lite, crtemu_pc_t* crtemu_pc, crtemu_t* crtemu ) {
    for( int i = 0; i < 256; ++i ) {
        input->prev_[ i ] = input->curr_[ i ];
        input->curr_[ i ] = false;
    }

    app_input_t in = app_input( input->app_ );

    for( int i = 0; i < in.count; ++i ) {
        if( in.events[ i ].type == APP_INPUT_MOUSE_MOVE ) {
            input->mouse_x = in.events[ i ].data.mouse_pos.x;
            input->mouse_y = in.events[ i ].data.mouse_pos.y;
            if( crtemu_lite ) {
                crtemu_lite_coordinates_window_to_bitmap( crtemu_lite, screen_width, screen_height, &input->mouse_x, &input->mouse_y );
            } else if( crtemu_pc ) {
                crtemu_pc_coordinates_window_to_bitmap( crtemu_pc, screen_width, screen_height, &input->mouse_x, &input->mouse_y );
            } else if( crtemu ) {
                int border_x = 22;
                int border_y = 33;
                if( screen_width == 480 ) {
                    border_x = 33;
                    border_y = 48;
                } else if( screen_width == 640 ) {
                    border_x = 44;
                    border_y = 66;
                }
                crtemu_coordinates_window_to_bitmap( crtemu, screen_width + border_x * 2, screen_height + border_y * 2, &input->mouse_x, &input->mouse_y );
                input->mouse_x -= border_x;
                input->mouse_y -= border_y;
            } else {
                app_coordinates_window_to_bitmap( input->app_, screen_width, screen_height, &input->mouse_x, &input->mouse_y );
            }
        } else if( in.events[ i ].type == APP_INPUT_KEY_DOWN ) {
            input->curr_[ in.events[ i ].data.key ] = true;
        }
    }
}


bool input_was_key_pressed( struct input_t* input, int key ) {
    return input->curr_[ key ] && !input->prev_[ key ];
}


bool input_was_key_released( struct input_t* input, int key ) {
    return input->prev_[ key ] && !input->curr_[ key ];
}


bool input_is_key_down( struct input_t* input, int key ) {
    return input->curr_[ key ];
}


int input_get_mouse_x( struct input_t* input ) {
    return input->mouse_x;
}


int input_get_mouse_y( struct input_t* input ) {
    return input->mouse_y;
}
