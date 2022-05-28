#define _CRT_NONSTDC_NO_DEPRECATE 
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <string.h>

#include "libs/app.h"


int app_proc( app_t* app, void* user_data ) {
    (void) user_data;
    static APP_U32 canvas[ 320 * 200 ];
    memset( canvas, 0xC0, sizeof( canvas ) );
    app_screenmode( app, APP_SCREENMODE_WINDOW );

    while( app_yield( app ) != APP_STATE_EXIT_REQUESTED ) {
        int x = rand() % 320;
        int y = rand() % 200;
        APP_U32 color = rand() | ( (APP_U32) rand() << 16 );
        canvas[ x + y * 320 ] = color;
        app_present( app, canvas, 320, 200, 0xffffff, 0x000000 );
    }
    return 0;
}


int main( int argc, char** argv ) {
    (void) argc, (void ) argv;
    return app_run( app_proc, NULL, NULL, NULL, NULL );
}


// pass-through so the program will build with either /SUBSYSTEM:WINDOWS or /SUBSYSTEM:CONSOLE
#pragma comment( lib, "user32")
#pragma comment( lib, "gdi32")
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
