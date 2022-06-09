typedef struct input_t {
	app_t* app_;
	bool prev_[ 256 ];
	bool curr_[ 256 ];
	int mouse_x_;
	int mouse_y_;
} input_t;


void input_init( struct input_t* input, app_t* app ) {
	input->app_ = app;
	for( int i = 0; i < 256; ++i ) {
		input->prev_[ i ] = false;
		input->curr_[ i ] = false;
	}
}


void input_update(  struct input_t* input, crtemu_pc_t* crtemu ) {
	for( int i = 0; i < 256; ++i ) {
		input->prev_[ i ] = input->curr_[ i ];
		input->curr_[ i ] = false;
	}

	app_input_t in = app_input( input->app_ );
		
	for( int i = 0; i < in.count; ++i ) {
		if( in.events[ i ].type == APP_INPUT_MOUSE_MOVE ) {
			input->mouse_x_ = in.events[ i ].data.mouse_pos.x;
			input->mouse_y_ = in.events[ i ].data.mouse_pos.y;
            if( crtemu ) {
                crtemu_pc_coordinates_window_to_bitmap( crtemu, /*384*/320, /*288*/200, &input->mouse_x_, &input->mouse_y_ );
                //mouse_x_ -= 32;
                //mouse_y_ -= 44;
            } else {
				app_coordinates_window_to_bitmap( input->app_, 320, 200, &input->mouse_x_, &input->mouse_y_ );
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
	return input->mouse_x_;
}


int input_get_mouse_y( struct input_t* input ) {
	return input->mouse_y_;
}
