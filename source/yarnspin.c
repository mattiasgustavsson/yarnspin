#define _CRT_NONSTDC_NO_DEPRECATE 
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <string.h>

#include "libs/app.h"
#include "libs/crtemu_pc.h"
#include "libs/frametimer.h"
#include "libs/stb_image.h"


int app_proc( app_t* app, void* user_data ) {
    (void) user_data;

    
    crtemu_pc_t* crtemu = crtemu_pc_create( NULL );
    int w, h, c;
    stbi_uc* crtframe = stbi_load( "images/crtframe.png", &w, &h, &c, 4 );
    crtemu_pc_frame( crtemu, (CRTEMU_PC_U32*) crtframe, w, h );
    stbi_image_free( crtframe );

    frametimer_t* frametimer = frametimer_create( NULL );
    frametimer_lock_rate( frametimer, 60 );

    static APP_U32 canvas[ 320 * 200 ];
    memset( canvas, 0xC0, sizeof( canvas ) );
    app_screenmode( app, APP_SCREENMODE_WINDOW );

    while( app_yield( app ) != APP_STATE_EXIT_REQUESTED ) {
        frametimer_update( frametimer );
        int x = rand() % 320;
        int y = rand() % 200;
        APP_U32 color = rand() | ( (APP_U32) rand() << 16 );
        canvas[ x + y * 320 ] = color;
        crtemu_pc_present( crtemu, 0, canvas, 320, 200, 0xffffff, 0x000000 );
        app_present( app, NULL, 1, 1, 0xffffff, 0x000000 );
    }
    frametimer_destroy( frametimer );
    crtemu_pc_destroy( crtemu );
    return 0;
}


int main( int argc, char** argv ) {
    (void) argc, (void ) argv;
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

#define CRTEMU_PC_IMPLEMENTATION
#include "libs/crtemu_pc.h"

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