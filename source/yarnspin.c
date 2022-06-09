#define _CRT_NONSTDC_NO_DEPRECATE 
#define _CRT_SECURE_NO_WARNINGS

#if defined( _WIN32 ) && defined( _DEBUG )
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "libs/app.h"
#include "libs/array.h"
#include "libs/buffer.h"
#include "libs/crtemu_pc.h"
#include "libs/cstr.h"
#include "libs/dir.h"
#include "libs/frametimer.h"
#include "libs/file.h"
#include "libs/file_util.h"
#include "libs/img.h"
#include "libs/paldither.h"
#include "libs/palrle.h"
#include "libs/pixelfont.h"
#include "libs/stb_image.h"
#include "libs/stb_image_write.h"
#include "libs/stb_truetype.h"

#include "memmgr.h"

static struct memmgr_t g_memmgr = { 0 };
void array_deleter( void* context, void* ptr ) { (void) context; internal_array_destroy( (struct internal_array_t*) ptr ); }
#define managed_array( type ) ARRAY_CAST( memmgr_add( &g_memmgr, array_create( type ), NULL, array_deleter ) )

void palrle_deleter( void* context, void* ptr ) { (void) context; palrle_free( (palrle_data_t*) ptr, NULL ); }
#define manage_palrle( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, palrle_deleter ) )

void paldither_deleter( void* context, void* ptr ) { (void) context; paldither_palette_destroy( (paldither_palette_t*) ptr ); }
#define manage_paldither( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, paldither_deleter ) )

void pixelfont_deleter( void* context, void* ptr ) { (void) context; free( ptr ); }
#define manage_pixelfont( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, pixelfont_deleter ) )

typedef cstr_t string;
#define array(type) array_t(type)
#define array_param(type) array_param_t(type)
#define ARRAY_COUNT( x ) ( sizeof( x ) / sizeof( *(x) ) )

#include "gfxconv.h"
#include "yarn.h"

#include "input.h"
#include "game.h"


int app_proc( app_t* app, void* user_data ) {
    yarn_t* yarn = (yarn_t*) user_data;

    // position window centered on main display
    app_displays_t displays = app_displays( app );
    if( displays.count > 0 ) {
        // find main display
        int disp = 0;
        for( int i = 0; i < displays.count; ++i ) {
            if( displays.displays[ i ].x == 0 && displays.displays[ i ].y == 0 ) {
                disp = i;
                break;
            }
        }
        // calculate aspect locked width/height
        int scrwidth = displays.displays[ disp ].width - 80;
        int scrheight = displays.displays[ disp ].height - 80;
        int aspect_width = (int)( ( scrheight * 4.25f ) / 3 );
        int aspect_height = (int)( ( scrwidth * 3 ) / 4.25f );
        int target_width, target_height;
        if( aspect_height <= scrheight ) {
            target_width = scrwidth;
            target_height = aspect_height;
        } else {
            target_width = aspect_width;
            target_height = scrheight;
        }
        // set window size and position
        int x = displays.displays[ disp ].x + ( displays.displays[ disp ].width - target_width ) / 2;
        int y = displays.displays[ disp ].y + ( displays.displays[ disp ].height - target_height ) / 2;
        int w = target_width;
        int h = target_height;
        app_window_pos( app, x, y );
        app_window_size( app, w, h );
    }
    
    #ifndef __wasm__
        bool fullscreen = true;
    #else
        bool fullscreen = false;
    #endif    
    app_interpolation( app, APP_INTERPOLATION_NONE );
    app_screenmode( app, fullscreen ? APP_SCREENMODE_FULLSCREEN : APP_SCREENMODE_WINDOW );
    app_title( app, "Yarnspin" );

    crtemu_pc_t* crtemu = crtemu_pc_create( NULL );
    int w, h, c;
    stbi_uc* crtframe = stbi_load( "images/crtframe.png", &w, &h, &c, 4 );
    crtemu_pc_frame( crtemu, (CRTEMU_PC_U32*) crtframe, w, h );
    stbi_image_free( crtframe );

    frametimer_t* frametimer = frametimer_create( NULL );
    frametimer_lock_rate( frametimer, 60 );

    static uint8_t canvas[ 320 * 200 ];
    memset( canvas, 0, sizeof( canvas) );

    // run game
    input_t input;
    input_init( &input, app );
    game_state_t game_state;
    game_state_init( &game_state );
    game_t game;
    game_init( &game, &game_state, yarn, &input, canvas, 320, 200 );

    // main loop
    bool exit_flag = false;
    while( app_yield( app ) != APP_STATE_EXIT_REQUESTED && !exit_flag) {
        frametimer_update( frametimer );
        input_update( &input, crtemu );
        exit_flag = game_update( &game );

        if( input_was_key_pressed( &input, APP_KEY_F11 ) ) {
            fullscreen = !fullscreen;
            app_screenmode( app, fullscreen ? APP_SCREENMODE_FULLSCREEN : APP_SCREENMODE_WINDOW );
        }

        static uint32_t screen[ 320 * 200 ];
        for( int i = 0; i < 320 * 200; ++i) {
            screen[ i ] = yarn->assets.palette[ canvas[ i ] ];
        }
        crtemu_pc_present( crtemu, 0, screen, 320, 200, 0xffffff, 0x000000 );
        app_present( app, NULL, 1, 1, 0xffffff, 0x000000 );
    }

    frametimer_destroy( frametimer );
    crtemu_pc_destroy( crtemu );
    memmgr_clear( &g_memmgr );
    return 0;
}


int main( int argc, char** argv ) {
    (void) argc, (void ) argv;

	// Enable windows memory leak detection (will report leaks in the Output window)
    #if defined( _WIN32 ) && defined( _DEBUG )
		int flag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		_CrtSetDbgFlag( flag ); // Set flag to the new value
		//_CrtSetBreakAlloc( 0 ); // Can be manually commented back in to break at a certain allocation
	#endif

    #ifndef __wasm__
        // compile yarn
	    buffer_t* compiled_yarn = yarn_compile( "." );
	    if( !compiled_yarn ) {
		    printf( "Failed to compile game file\n" );
		    return EXIT_FAILURE;
	    }    
        buffer_save( compiled_yarn, "game.yarn" );
        buffer_destroy( compiled_yarn );   
    #endif

    // load yarn
    buffer_t* loaded_yarn = buffer_load( "game.yarn" );
    yarn_t yarn;
    yarn_load( loaded_yarn, &yarn );
    buffer_destroy( loaded_yarn );    

    return app_run( app_proc, &yarn, NULL, NULL, NULL );
}


// pass-through so the program will build with either /SUBSYSTEM:WINDOWS or /SUBSYSTEM:CONSOLE
#if defined( _WIN32 ) && !defined( __TINYC__ )
    #ifdef __cplusplus 
        extern "C" int __stdcall WinMain( struct HINSTANCE__*, struct HINSTANCE__*, char*, int ) { 
            return main( __argc, __argv ); 
        }
    #else
        struct HINSTANCE__;
        int __stdcall WinMain( struct HINSTANCE__* a, struct HINSTANCE__* b, char* c, int d ) { 
            (void) a, b, c, d; return main( __argc, __argv ); 
        }
    #endif
#endif

#define APP_IMPLEMENTATION
#ifdef _WIN32 
    #define APP_WINDOWS
#elif __wasm__
    #define APP_WASM
#else 
    #define APP_SDL
#endif
#define APP_LOG( ctx, level, message ) 
#include "libs/app.h"

#define ARRAY_IMPLEMENTATION
#include "libs/array.h"

#define BUFFER_IMPLEMENTATION
#include "libs/buffer.h"

#define CRTEMU_PC_IMPLEMENTATION
#include "libs/crtemu_pc.h"

#define CSTR_IMPLEMENTATION
#include "libs/cstr.h"

#define DIR_IMPLEMENTATION
#ifdef _WIN32
    #define DIR_WINDOWS
#else
    #define DIR_POSIX
#endif
#include "libs/dir.h"
        
#define FILE_IMPLEMENTATION
#include "libs/file.h"

#define FILE_UTIL_IMPLEMENTATION
#include "libs/file_util.h"

#define FRAMETIMER_IMPLEMENTATION
#include "libs/frametimer.h"

#define IMG_IMPLEMENTATION
#include "libs/img.h"

#define PALDITHER_IMPLEMENTATION
#include "libs/paldither.h"

#define PALRLE_IMPLEMENTATION
#include "libs/palrle.h"

#define PIXELFONT_IMPLEMENTATION
#define PIXELFONT_BUILDER_IMPLEMENTATION
#include "libs/pixelfont.h"

#pragma warning( push )
#pragma warning( disable: 4255 )
#pragma warning( disable: 4296 )
#pragma warning( disable: 4365 )
#pragma warning( disable: 4668 )
#define STB_IMAGE_IMPLEMENTATION
#if defined( _WIN32 ) && ( defined( __clang__ ) || defined( __TINYC__ ) )
    #define STBI_NO_SIMD
#endif
#include "libs/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable: 4204 )
#pragma warning( disable: 4244 ) // conversion from 'int' to 'short', possible loss of data
#pragma warning( disable: 4365 ) 
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"
#pragma warning( pop )

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"
