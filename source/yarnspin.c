#define _CRT_NONSTDC_NO_DEPRECATE 
#define _CRT_SECURE_NO_WARNINGS

#if defined( _WIN32 ) && defined( _DEBUG )
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
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
#include "libs/stb_image.h"

#include "memmgr.h"
static struct memmgr_t g_memmgr = { 0 };
void array_deleter( void* context, void* ptr ) { (void) context; internal_array_destroy( (struct internal_array_t*) ptr ); }
#define managed_array( type ) ARRAY_CAST( memmgr_add( &g_memmgr, array_create( type ), NULL, array_deleter ) )


typedef cstr_t string;
#define array(type) array_t(type)
#define array_param(type) array_param_t(type)
#define ARRAY_COUNT( x ) ( sizeof( x ) / sizeof( *(x) ) )

#include "yarn.h"

APP_U32 blend( APP_U32 x, APP_U32 y, APP_U32 a ) {
    APP_U32 xr = ( x ) & 0xff;
    APP_U32 xg = ( x >> 8 ) & 0xff;
    APP_U32 xb = ( x >> 16 ) & 0xff;
    APP_U32 yr = ( y ) & 0xff;
    APP_U32 yg = ( y >> 8 ) & 0xff;
    APP_U32 yb = ( y >> 16 ) & 0xff;
    APP_U32 ia = 255 - ( a & 0xff );
    APP_U32 r = ( xr * ia + yr * a ) >> 8;
    APP_U32 g = ( xg * ia + yg * a ) >> 8;
    APP_U32 b = ( xb * ia + yb * a ) >> 8;
    return r | ( g << 8 ) | ( b << 16 );
}


int app_proc( app_t* app, void* user_data ) {
    (void) user_data;

    // compile yarn
	buffer_t* compiled_yarn = yarn_compile( "." );
	if( !compiled_yarn ) {
		printf( "Failed to compile game file\n" );
		return EXIT_FAILURE;
	}    
    buffer_save( compiled_yarn, "game.yarn" );

    // load yarn
    buffer_position_set( compiled_yarn, 0 );
    yarn_t yarn;
    yarn_load( compiled_yarn, &yarn );
    buffer_destroy( compiled_yarn );    

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
    
    app_screenmode( app, APP_SCREENMODE_WINDOW );
    app_title( app, "Yarnspin" );

    crtemu_pc_t* crtemu = crtemu_pc_create( NULL );
    int w, h, c;
    stbi_uc* crtframe = stbi_load( "images/crtframe.png", &w, &h, &c, 4 );
    crtemu_pc_frame( crtemu, (CRTEMU_PC_U32*) crtframe, w, h );
    stbi_image_free( crtframe );

    frametimer_t* frametimer = frametimer_create( NULL );
    frametimer_lock_rate( frametimer, 60 );

    static APP_U32 canvas[ 320 * 200 ];
    memset( canvas, 0, sizeof( canvas) );

    // display yarnspin logo
    APP_U32* logo = (APP_U32*) stbi_load( "images/yarnspin_logo.png", &w, &h, &c, 4 );
    for( int i = 0; i < 320 * 200; ++i ) {
        canvas[ i ] = blend( canvas[ i ], logo[ i ], logo[ i ] >> 24 );
    }
    stbi_image_free( logo );


    // main loop
    while( app_yield( app ) != APP_STATE_EXIT_REQUESTED ) {
        frametimer_update( frametimer );
        crtemu_pc_present( crtemu, 0, canvas, 320, 200, 0xffffff, 0x000000 );
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
//		_CrtSetBreakAlloc( 0 ); // Can be manually commented back in to break at a certain allocation
	#endif

    return app_run( app_proc, NULL, NULL, NULL, NULL );
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

#define FRAMETIMER_IMPLEMENTATION
#include "libs/frametimer.h"

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