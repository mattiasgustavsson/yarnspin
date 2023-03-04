#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

// Need to do this before anything else is included, to get proper filenames in memory leak reporting
#if defined( _WIN32 ) && defined( _DEBUG )
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

// Yarnspin version for file formats. Increment by one for each release
#define YARNSPIN_VERSION 1

// C standard lib includes
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Library includes
#include "libs/app.h"
#include "libs/array.h"
#include "libs/buffer.h"
#include "libs/crtemu.h"
#include "libs/crtemu_pc.h"
#include "libs/cstr.h"
#include "libs/dir.h"
#include "libs/frametimer.h"
#include "libs/file.h"
#include "libs/img.h"
#include "libs/ini.h"
#include "libs/paldither.h"
#include "libs/palettize.h"
#include "libs/palrle.h"
#include "libs/qoi.h"
#include "libs/stb_image.h"
#include "libs/stb_image_resize.h"
#include "libs/stb_image_write.h"
#include "libs/stb_truetype.h"
#include "libs/sysfont.h"
#include "libs/thread.h"

#define PIXELFONT_COLOR PIXELFONT_U8
#define PIXELFONT_FUNC_NAME pixelfont_blit
#include "libs/pixelfont.h"

#undef PIXELFONT_COLOR
#undef PIXELFONT_FUNC_NAME
#define PIXELFONT_COLOR PIXELFONT_U32
#define PIXELFONT_FUNC_NAME pixelfont_blit_rgb
#include "libs/pixelfont.h"


// Version number stored in the file .cache\VERSION, read at start of program
int g_cache_version = 0;


// forward declares for helper functions placed at the end of this file

char const* cextname( char const* path );
char const* cbasename( char const* path );

void create_path( char const* path, int pos );
int file_more_recent( char const* source_path, char const* output_path );
int file_exists( char const* filename );
int folder_exists( char const* filename );
char const* get_executable_filename( void );

void* compress_lzma( void* data, size_t size, size_t* out_size );
size_t decompress_lzma( void* compressed_data, size_t compressed_size, void* buffer, size_t size );


// automatic memory management helper stuff

#include "memmgr.h"
static struct memmgr_t g_memmgr = { 0 };

void array_deleter( void* context, void* ptr ) { (void) context; internal_array_destroy( (struct internal_array_t*) ptr ); }
#define managed_array( type ) ARRAY_CAST( memmgr_add( &g_memmgr, array_create( type ), NULL, array_deleter ) )

void palrle_deleter( void* context, void* ptr ) { (void) context; palrle_free( (palrle_data_t*) ptr, NULL ); }
#define manage_palrle( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, palrle_deleter ) )

void paldither_deleter( void* context, void* ptr ) { (void) context; paldither_palette_destroy( (paldither_palette_t*) ptr, NULL ); }
#define manage_paldither( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, paldither_deleter ) )

void pixelfont_deleter( void* context, void* ptr ) { (void) context; free( ptr ); }
#define manage_pixelfont( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, pixelfont_deleter ) )

void alloc_deleter( void* context, void* ptr ) { (void) context; free( ptr ); }
#define manage_alloc( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, alloc_deleter ) )


// helper defines
typedef cstr_t string;
#define array(type) array_t(type)
#define array_param(type) array_param_t(type)
#define ARRAY_COUNT( x ) ( sizeof( x ) / sizeof( *(x) ) )


// yarnspin files
#include "gfxconv.h"
#include "yarn.h"
#include "input.h"
#include "game.h"


// main game loop and setup
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
        int aspect_width = (int)( ( scrheight * 4 ) / 3 );
        int aspect_height = (int)( ( scrwidth * 3 ) / 4 );
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
    app_interpolation( app, yarn->globals.resolution >= YARN_RESOLUTION_FULL || ( yarn->globals.resolution == YARN_RESOLUTION_HIGH && yarn->globals.colormode == YARN_COLORMODE_RGB ) ? APP_INTERPOLATION_LINEAR : APP_INTERPOLATION_NONE );
    app_screenmode( app, fullscreen ? APP_SCREENMODE_FULLSCREEN : APP_SCREENMODE_WINDOW );
    app_title( app, yarn->globals.title );

    int frame_pc_width = 0;
    int frame_pc_height = 0;
    CRTEMU_PC_U32* frame_pc_pixels = NULL;
    if( yarn->assets.frame_pc ) {
        int c;
        frame_pc_pixels = (CRTEMU_PC_U32*) stbi_load_from_memory( (stbi_uc*) yarn->assets.frame_pc,
            yarn->assets.frame_pc_size, &frame_pc_width, &frame_pc_height, &c, 4 );
    }

    int frame_tv_width = 0;
    int frame_tv_height = 0;
    CRTEMU_U32* frame_tv_pixels = NULL;
    if( yarn->assets.frame_tv ) {
        int c;
        frame_tv_pixels = (CRTEMU_U32*) stbi_load_from_memory( (stbi_uc*) yarn->assets.frame_tv,
            yarn->assets.frame_tv_size, &frame_tv_width, &frame_tv_height, &c, 4 );
    }

    int display_filter_index = 0;
    yarn_display_filter_t display_filter = yarn->globals.display_filters->items[ display_filter_index ];

    frametimer_t* frametimer = frametimer_create( NULL );
    frametimer_lock_rate( frametimer, 60 );

    crtemu_pc_t* crtemu_pc = NULL;
    crtemu_t* crtemu = NULL;

    int widths[] = { 320, 480, 640, 1440 };
    int heights[] = { 240, 360, 480, 1080 };
    int screen_width = widths[ yarn->globals.resolution ];
    int screen_height = heights[ yarn->globals.resolution ];
    
    uint8_t* canvas = NULL;
    uint32_t* canvas_rgb = NULL;
    uint32_t* screen = (uint32_t*)malloc( ( 1440 + (int)( 44 * 4.5f ) ) * ( 1080 + (int)(66 * 4.5 ) ) * sizeof( uint32_t ) );
    memset( screen, 0, ( 1440 + (int)( 44 * 4.5f ) ) * ( 1080 + (int)( 66 * 4.5 ) ) * sizeof( uint32_t ) );

    // run game
    input_t input;
    input_init( &input, app );
    game_t game;
    if( yarn->globals.colormode == YARN_COLORMODE_PALETTE ) {
        canvas = (uint8_t*)malloc( screen_width * screen_height * sizeof( uint8_t ) );
        memset( canvas, 0, screen_width * screen_height * sizeof( uint8_t ) );
        game_init( &game, yarn, &input, canvas, NULL, screen_width, screen_height );
    } else {
        canvas_rgb = (uint32_t*)malloc( screen_width * screen_height * sizeof( uint32_t ) );
        memset( canvas_rgb, 0, screen_width * screen_height * sizeof( uint32_t ) );
        game_init( &game, yarn, &input, NULL, canvas_rgb, screen_width, screen_height );
    }

    // main loop
    APP_U64 time = 0;
    while( app_yield( app ) != APP_STATE_EXIT_REQUESTED && !game.exit_flag ) {
        frametimer_update( frametimer );
        input_update( &input, screen_width, screen_height, crtemu_pc, crtemu );
        game_update( &game );

        if( input_was_key_pressed( &input, APP_KEY_F11 ) ) {
            fullscreen = !fullscreen;
            app_screenmode( app, fullscreen ? APP_SCREENMODE_FULLSCREEN : APP_SCREENMODE_WINDOW );
        }

        if( input_was_key_pressed( &input, APP_KEY_F9 ) ) {
            display_filter_index = ( display_filter_index + 1 ) % yarn->globals.display_filters->count;
            display_filter = yarn->globals.display_filters->items[ display_filter_index ];
        }

        APP_U32 transition = (APP_U32)( ( 255 * abs( game.transition_counter ) / 10 ) );
        APP_U32 fade = transition << 16 | transition << 8 | transition;
        uint32_t bg = yarn->assets.palette[ game.color_background ];
        #define RGBMUL32( a, b) \
            ( ( ( ( ( ( (a) >> 16U ) & 0xffU ) * ( ( (b) >> 16U ) & 0xffU ) ) >> 8U ) << 16U ) | \
                ( ( ( ( ( (a) >> 8U  ) & 0xffU ) * ( ( (b) >> 8U  ) & 0xffU ) ) >> 8U ) << 8U  ) | \
                ( ( ( ( ( (a)        ) & 0xffU ) * ( ( (b)        ) & 0xffU ) ) >> 8U )        ) )
        bg = RGBMUL32( fade, bg );

        time += 1000000 / 60;

        if( display_filter == YARN_DISPLAY_FILTER_PC && crtemu_pc == NULL ) {
            if( crtemu ) {
                crtemu_destroy( crtemu );
                crtemu = NULL;
            }
            crtemu_pc = crtemu_pc_create( NULL );
            if( crtemu_pc && frame_pc_pixels ) {
                crtemu_pc_frame( crtemu_pc, frame_pc_pixels, frame_pc_width, frame_pc_height );
            }
        }

        if( display_filter == YARN_DISPLAY_FILTER_TV && crtemu == NULL ) {
            if( crtemu_pc ) {
                crtemu_pc_destroy( crtemu_pc );
                crtemu_pc = NULL;
            }
            memset( screen, 0, ( 1440 + (int)( 22 * 4.5f ) ) * ( 1080 + (int)( 33 * 4.5 ) ) * sizeof( uint32_t ) );
            crtemu = crtemu_create( NULL );
            if( crtemu && frame_tv_pixels ) {
                crtemu_frame( crtemu, frame_tv_pixels, frame_tv_width, frame_tv_height );
            }
        }

        if( display_filter == YARN_DISPLAY_FILTER_NONE && crtemu_pc != NULL ) {
                crtemu_pc_destroy( crtemu_pc );
                crtemu_pc = NULL;
        }

        if( display_filter == YARN_DISPLAY_FILTER_NONE && crtemu != NULL ) {
                crtemu_destroy( crtemu );
                crtemu = NULL;
                memset( screen, 0, ( 1440 + (int)( 22 * 4.5f ) ) * ( 1080 + (int)( 33 * 4.5 ) ) * sizeof( uint32_t ) );
        }

        if( crtemu_pc ) {
            if( canvas ) {
                for( int i = 0; i < screen_width * screen_height; ++i ) {
                    screen[ i ] = yarn->assets.palette[ canvas[ i ] ];
                }
                crtemu_pc_present( crtemu_pc, time, screen, screen_width, screen_height, fade, bg );
            } else {
                crtemu_pc_present( crtemu_pc, time, canvas_rgb, screen_width, screen_height, fade, bg );
            }
            app_present( app, NULL, 1, 1, 0xffffff, 0x000000 );
        } else if( crtemu ) {
            int offset_x = 22;
            int offset_y = 33;
            scale_for_resolution( &game, &offset_x, &offset_y );
            if( canvas ) {
                for( int y = 0; y < screen_height; ++y ) {
                    for( int x = 0; x < screen_width; ++x ) {
                        screen[ ( offset_x + x ) + ( offset_y + y ) * ( screen_width + 2 * offset_x ) ] = yarn->assets.palette[ canvas[ x + y * screen_width ] ];
                    }
                }
            } else {
                for( int y = 0; y < screen_height; ++y ) {
                    for( int x = 0; x < screen_width; ++x ) {
                        screen[ ( offset_x + x ) + ( offset_y + y ) * ( screen_width + 2 * offset_x ) ] = canvas_rgb[ x + y * screen_width ];
                    }
                }
            }
            crtemu_present( crtemu, time, screen, screen_width + 2 * offset_x, screen_height + 2 * offset_y, fade, bg );
            app_present( app, NULL, 1, 1, 0xffffff, 0x000000 );
        } else {
            if( canvas ) {
                for( int i = 0; i < screen_width * screen_height; ++i ) {
                    screen[ i ] = yarn->assets.palette[ canvas[ i ] ];
                }
                app_present( app, screen, screen_width, screen_height, fade, bg );
            } else {
                app_present( app, canvas_rgb, screen_width, screen_height, fade, bg );
            }
        }
    }

    frametimer_destroy( frametimer );
    if( crtemu_pc ) {
        crtemu_pc_destroy( crtemu_pc );
    }
    if( crtemu ) {
        crtemu_destroy( crtemu );
    }

    if( canvas ) {
        free( canvas );
    }
    if( canvas_rgb ) {
        free( canvas_rgb );
    }
    free( screen );

    if( frame_tv_pixels ) {
        stbi_image_free( frame_tv_pixels );
    }

    if( frame_pc_pixels ) {
        stbi_image_free( frame_pc_pixels );
    }
    memmgr_clear( &g_memmgr );
    return 0;
}


#ifndef __wasm__
    #include "imgedit.h"
    void threads_init( void );
#endif



    int main( int argc, char** argv ) {
    (void) argc, (void ) argv;

    // Enable windows memory leak detection (will report leaks in the Output window)
    #if defined( _WIN32 ) && defined( _DEBUG )
        int flag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ); // Get current flag
        flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
        _CrtSetDbgFlag( flag ); // Set flag to the new value
        //_CrtSetBreakAlloc( 0 ); // Can be manually commented back in to break at a certain allocation
    #endif

    // if -i or --images parameter were specified, run image editor
    #ifndef __wasm__
        if( argc == 2 && ( strcmp( argv[ 1 ], "-i" ) == 0 || strcmp( argv[ 1 ], "--images" ) == 0 ) ) {
            threads_init();
            int resolution = 0;
            int prev_resolution = resolution;
            int result = EXIT_SUCCESS;
            for( ; ; ) {
                result = app_run( imgedit_proc, &resolution, NULL, NULL, NULL );
                if( prev_resolution == resolution ) {
                    break;
                }
                prev_resolution = resolution;
            } 
            return result;
        }
    #endif

    #ifndef __wasm__
        // compile yarn
        if( folder_exists( "scripts" ) ) {
            file_t* version_file = file_load( ".cache/VERSION", FILE_MODE_TEXT, NULL );
            if( version_file ) {
                g_cache_version = atoi( (char const*) version_file->data );
                file_destroy( version_file );
            }
            buffer_t* compiled_yarn = yarn_compile( "." );
            if( !compiled_yarn ) {
                printf( "Failed to compile game file\n" );
                return EXIT_FAILURE;
            }

            printf( "Compressing yarn file\n" );
            size_t size;
            void* data = compress_lzma( buffer_data( compiled_yarn ), buffer_size( compiled_yarn ), &size );
            uint32_t original_size = (uint32_t) buffer_size( compiled_yarn );
            buffer_destroy( compiled_yarn );

            FILE* fp = fopen( "yarnspin.dat", "wb" );
            char header[] = "YARNSPIN";
            uint32_t version = YARNSPIN_VERSION;
            uint32_t size_in_bytes = (uint32_t)( size + 2 * strlen( header ) + sizeof( uint32_t ) * 5 );
            fwrite( header, 1, strlen( header ), fp );
            fwrite( &version, 1, sizeof( version ), fp );
            fwrite( &size_in_bytes, 1, sizeof( size_in_bytes ), fp );
            fwrite( &original_size, 1, sizeof( original_size ), fp );
            fwrite( data, 1, size, fp );
            fwrite( &size_in_bytes, 1, sizeof( size_in_bytes ), fp );
            fwrite( &version, 1, sizeof( version ), fp );
            fwrite( header, 1, strlen( header ), fp );
            fclose( fp );
            free( data );
            printf( "yarnspin.dat written\n" );

            char version_string[ 16 ];
            sprintf( version_string, "%d", (int) YARNSPIN_VERSION );
            file_save_data( version_string, strlen( version_string ), ".cache/VERSION", FILE_MODE_TEXT );
        }
    #endif

    // load and decompress yarn data
    buffer_t* decompressed_yarn = buffer_create();

    #ifndef __wasm__
    if( file_exists( "yarnspin.dat" ) ) {
    #else
    {
    #endif
        // load from external data file if it exists
        buffer_t* loaded_yarn = buffer_load( "yarnspin.dat" );
        if( !loaded_yarn ) {
            printf( "Failed to load yarnspin.dat\n" );
            return EXIT_FAILURE;
        }
        char header[ 8 ];
        buffer_read_i8( loaded_yarn, header, 8 );
        if( strncmp( header, "YARNSPIN", 8 ) != 0 ) {
            printf( "The file yarnspin.dat is not a valid Yarnspin game data file\n" );
            return EXIT_FAILURE;
        }
        uint32_t version = 0;
        buffer_read_u32( loaded_yarn, &version, 1 );
        if( version != YARNSPIN_VERSION ) {
            printf( "The file yarnspin.dat is for a different Yarnspin version\n" );
            return EXIT_FAILURE;
        }
        uint32_t size_in_bytes;
        buffer_read_u32( loaded_yarn, &size_in_bytes, 1 );
        uint32_t uncompressed_size;
        buffer_read_u32( loaded_yarn, &uncompressed_size, 1 );
        buffer_resize( decompressed_yarn, uncompressed_size );
        void* data = (void*) ( ( (uintptr_t)buffer_data( loaded_yarn ) ) + buffer_position( loaded_yarn ) );
        size_t size = buffer_size( loaded_yarn ) - buffer_position( loaded_yarn );
        size_t decompressed_size = decompress_lzma( data, size, buffer_data( decompressed_yarn ), uncompressed_size );
        buffer_destroy( loaded_yarn );
        if( decompressed_size != uncompressed_size ) {
            buffer_destroy( decompressed_yarn );
            printf( "Failed to decompress game file\n" );
            return EXIT_FAILURE;
        }
    #ifndef __wasm__
    } else {
        // load from end of executable, if no external data file is present
        char const* filename = get_executable_filename();
        if( !filename ) {
            printf( "Could not determine executable file\n" );
            return EXIT_FAILURE;
        }
        FILE* fp = fopen( filename, "rb" );
        if( !fp ) {
            printf( "Could not open game data file\n" );
            return EXIT_FAILURE;
        }
        fseek( fp, -8, SEEK_END );
        char header[ 8 ];
        fread( header, 1, 8, fp );
        if( strncmp( header, "YARNSPIN", 8 ) != 0 ) {
            printf( "No yarnspin.dat game data can be loaded\n" );
            return EXIT_FAILURE;
        }
        fseek( fp, -( 8 + (int) sizeof( uint32_t ) ), SEEK_END );
        uint32_t version = 0;
        fread( &version, 1, sizeof( version ), fp );
        if( version != YARNSPIN_VERSION ) {
            printf( "The game data file embedded in the EXE is for a different Yarnspin version\n" );
            return EXIT_FAILURE;
        }
        fseek( fp, -( 8 + 2 * (int) sizeof( uint32_t ) ), SEEK_END );
        uint32_t size_in_bytes = 0;
        fread( &size_in_bytes, 1, sizeof( size_in_bytes ), fp );
        fseek( fp, -(int)size_in_bytes, SEEK_END );
        fseek( fp, 8 + sizeof( uint32_t ) * 2, SEEK_CUR );

        uint32_t uncompressed_size;
        fread( &uncompressed_size, 1, sizeof( uncompressed_size ), fp );
        buffer_resize( decompressed_yarn, uncompressed_size );
        size_t size = size_in_bytes - ( 8 + sizeof( uint32_t ) * 3 );
        void* data = malloc( size );
        fread( data, 1, size, fp );
        fclose( fp );
        size_t decompressed_size = decompress_lzma( data, size, buffer_data( decompressed_yarn ), uncompressed_size );
        free( data );
        if( decompressed_size != uncompressed_size ) {
            buffer_destroy( decompressed_yarn );
            printf( "Failed to decompress game file\n" );
            return EXIT_FAILURE;
        }
    #endif
    }

    // if -c or --compile parameter were specified, don't run the game, just exit after compiling the yarn
    if( argc == 2 && ( strcmp( argv[ 1 ], "-c" ) == 0 || strcmp( argv[ 1 ], "--compile" ) == 0 ) ) {
        return EXIT_SUCCESS;
    }

    // load yarn
    yarn_t yarn;
    yarn_load( decompressed_yarn, &yarn );
    buffer_destroy( decompressed_yarn );

    return app_run( app_proc, &yarn, NULL, NULL, NULL );
}


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

#define CRTEMU_IMPLEMENTATION
#include "libs/crtemu.h"

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

#define IMG_IMPLEMENTATION
#include "libs/img.h"

#define INI_IMPLEMENTATION
#include "libs/ini.h"

#define LZMA_IMPLEMENTATION
#include "libs/lzma.h"

#define PALDITHER_IMPLEMENTATION
#include "libs/paldither.h"

#define PALETTIZE_IMPLEMENTATION
#include "libs/palettize.h"

#define PALRLE_IMPLEMENTATION
#include "libs/palrle.h"

#define PIXELFONT_IMPLEMENTATION
#define PIXELFONT_BUILDER_IMPLEMENTATION
#undef PIXELFONT_COLOR
#undef PIXELFONT_FUNC_NAME
#define PIXELFONT_COLOR PIXELFONT_U8
#define PIXELFONT_FUNC_NAME pixelfont_blit
#include "libs/pixelfont.h"

#define PIXELFONT_IMPLEMENTATION
#define PIXELFONT_COLOR PIXELFONT_U32
#define PIXELFONT_FUNC_NAME pixelfont_blit_rgb
uint32_t pixelfont_blend( uint32_t color1, uint32_t color2, uint8_t alpha )	{
    uint64_t c1 = (uint64_t) color1;
    uint64_t c2 = (uint64_t) color2;
    uint64_t a = (uint64_t)( alpha );
    // bit magic to alpha blend R G B with single mul
    c1 = ( c1 | ( c1 << 24 ) ) & 0x00ff00ff00ffull;
    c2 = ( c2 | ( c2 << 24 ) ) & 0x00ff00ff00ffull;
    uint64_t o = ( ( ( ( c2 - c1 ) * a ) >> 8 ) + c1 ) & 0x00ff00ff00ffull; 
    return (uint32_t) ( o | ( o >> 24 ) );
}
#undef PIXELFONT_PIXEL_FUNC
#define PIXELFONT_PIXEL_FUNC( dst, fnt, col ) *dst = pixelfont_blend( *dst, col, fnt );
#include "libs/pixelfont.h"

#define QOI_IMPLEMENTATION
#include "libs/qoi.h"

#pragma warning( push )
#pragma warning( disable: 4255 )
#pragma warning( disable: 4668 )
#define STB_IMAGE_IMPLEMENTATION
#if defined( _WIN32 ) && ( defined( __clang__ ) || defined( __TINYC__ ) )
    #define STBI_NO_SIMD
#endif
#include "libs/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION
#pragma warning( pop )

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "libs/stb_image_resize.h"

#pragma warning( push )
#pragma warning( disable: 4204 )
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"
#pragma warning( pop )

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_RASTERIZER_VERSION 1
#include "libs/stb_truetype.h"

#pragma warning( push )
#pragma warning( disable: 4456 )
#pragma warning( disable: 4457 )
#define SYSFONT_IMPLEMENTATION
#include "libs/sysfont.h"
#pragma warning( pop )

#ifndef __wasm__
    #if defined( __TINYC__ )
        typedef struct _RTL_CONDITION_VARIABLE { PVOID Ptr; } RTL_CONDITION_VARIABLE, *PRTL_CONDITION_VARIABLE;
        typedef RTL_CONDITION_VARIABLE CONDITION_VARIABLE, *PCONDITION_VARIABLE;
        static VOID (*InitializeConditionVariable)( PCONDITION_VARIABLE );
        static VOID (*WakeConditionVariable)( PCONDITION_VARIABLE );
        static BOOL (*SleepConditionVariableCS)( PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD );
    #endif

        
    #define THREAD_IMPLEMENTATION
    #include "libs/thread.h"

    void threads_init( void ) {
        #if defined( __TINYC__ )
            HMODULE kernel = LoadLibrary( "kernel32" );
            InitializeConditionVariable = GetProcAddress( kernel, "InitializeConditionVariable");
            WakeConditionVariable = GetProcAddress( kernel, "WakeConditionVariable");
            SleepConditionVariableCS = GetProcAddress( kernel, "SleepConditionVariableCS");
        #endif
    }
#endif

#include <sys/stat.h>

void makedir( char const* path ) {
    #ifdef _WIN32
        CreateDirectoryA( path, NULL );
    #else
        mkdir( path, S_IRWXU );
    #endif
}


void create_path( char const* path, int pos ) {
    pos = cstr_find( path, "/", pos );
    if( pos < 0 ) {
        return;
    }
    char const* dir = cstr_mid( path, 0, pos );
    makedir( dir );
    create_path( path, pos + 1 );
}


time_t file_last_changed( char const* filename ) {
    if( filename ) {
        struct stat result;
        int ret = stat( filename, &result );
        if( ret == 0 ) {
            return result.st_mtime;
        }
    }
    return 0;
}


int file_more_recent( char const* source_path,  char const* output_path  ) {
    return file_last_changed( source_path ) > file_last_changed( output_path );
}


int file_exists( char const* filename ) {
    struct stat result;
    int ret = stat( filename, &result );
    if( ret == 0 ) {
        return result.st_mode & S_IFREG;
    }

    return 0;
}

int folder_exists( char const* filename ) {
    struct stat result;
    int ret = stat( filename, &result );
    if( ret == 0 ) {
        return result.st_mode & S_IFDIR;
    }

    return 0;
}

#ifndef _WIN32
    #include <strings.h>
#endif


char const* cextname( char const* path ) {
    static char result[ 1024 ];
    strcpy( result, "" );

    if( path ) {
        char const* lastForwardSlash = strrchr( path, '/' );
        char const* lastBackSlash = strrchr( path, '\\' );

        char const* name = 0;
        char const* ext = 0;

        if( !lastBackSlash && !lastForwardSlash ) {
            name = path;
        } else if( !lastBackSlash ) {
            name = lastForwardSlash + 1;
        } else if( !lastForwardSlash ) {
            name = lastBackSlash + 1;
        } else if( lastForwardSlash > lastBackSlash ) {
            name = lastForwardSlash + 1;
        } else {
            name = lastBackSlash + 1;
        }

        ext = strrchr( name, '.' );

        if( ext && !( ext[ 0 ] == '.' && ext[ 1 ] == 0 ) ) {
            strncpy( result, ext, sizeof( result ) );
        }
    }

    return result;
}


char const* cbasename( char const* path ) {
    char const* extension = cextname( path );

    static char result[ 1024 ];
    strcpy( result, "" );

    if( path ) {
        char const* lastForwardSlash = strrchr( path, '/' );
        char const* lastBackSlash = strrchr( path, '\\' );

        char const* name = 0;

        if( !lastBackSlash && !lastForwardSlash ) {
            name = path;
        } else if( !lastBackSlash ) {
            name = lastForwardSlash + 1;
        } else if( !lastForwardSlash ) {
            name = lastBackSlash + 1;
        } else if( lastForwardSlash > lastBackSlash ) {
            name = lastForwardSlash + 1;
        } else {
            name = lastBackSlash + 1;
        }

        strncpy( result, name, sizeof( result ) );

        if( extension ) {
            size_t extlen = strlen( extension );
            size_t reslen = strlen( result );

            if( reslen >= extlen ) {
                #if _WIN32
                if( stricmp( result + (reslen - extlen), extension ) == 0 ) {
                #else
                if( strcasecmp( result + (reslen - extlen), extension ) == 0 ) {
                #endif
                    result[ reslen - extlen ] = 0;
                }
            }

        }
    }

    return result;
}



void* compress_lzma( void* data, size_t size, size_t* out_size ) {

    size_t size_props_count = 5;
    size_t compression_header_size = ( sizeof( uint16_t ) * 2 + size_props_count * sizeof( unsigned char ) );
    void* compressed_data = malloc( size + compression_header_size );
    uint16_t* version = (uint16_t*) compressed_data;
    *version = 19;
    uint16_t* props_count = version + 1;
    unsigned char* props = (unsigned char*)( props_count + 1 );
    unsigned char* compression_buffer = props + size_props_count;
    size_t compressed_size = size;

    // TODO: configurable compression rate vs speed?
    int result = LzmaCompress( compression_buffer, &compressed_size, (unsigned char*) data, size,
        props, &size_props_count, -1, 0, -1, -1, -1, -1, NULL, NULL );
    *props_count = (uint16_t)size_props_count;
    *out_size = compressed_size + compression_header_size;

    if( result == SZ_OK )
        return compressed_data;

    free( compressed_data );
    return NULL;
}


size_t decompress_lzma( void* compressed_data, size_t compressed_size, void* buffer, size_t size ) {
    uint16_t* props = (uint16_t*)compressed_data;
    //assert( *props == 19 );
    ++props; compressed_size -= 2;
    size_t props_count = *props;
    ++props; compressed_size -= 2;
    compressed_size -= props_count;
    int ret = LzmaUncompress( (unsigned char*) buffer, &size, ( (unsigned char*) props ) + props_count,
        &compressed_size, (unsigned char*) props, props_count );
    if( ret != SZ_OK ) {
        return 0;
    }
    return size;
}
 
#ifndef _WIN32
    #include <unistd.h>
#endif

char const* get_executable_filename( void ) {
    static char filename[ 1024 ];
    #ifdef _WIN32
        DWORD sz = GetModuleFileNameA( NULL, filename, (DWORD) sizeof( filename ) );
        if( sz <= 0 || sz >= sizeof( filename ) ) {
            return NULL;
        }
    #else
        size_t sz = readlink( "/proc/self/exe", filename, sizeof( filename ) );
        if( sz <= 0 || sz >= sizeof( filename ) ) {
            return NULL;
        }
    #endif
    return filename;
}


void ensure_console_open( void ) {
    #ifdef _WIN32
        #ifdef __TINYC__
           HMODULE dll = LoadLibrary( "kernel32" );
           HWND (*WINAPI GetConsoleWindow)(void) = GetProcAddress( dll, "GetConsoleWindow");
           BOOL (*WINAPI AttachConsole)(DWORD) = GetProcAddress( dll, "AttachConsole");
        #endif
       if( GetConsoleWindow() == NULL) {
            if( AttachConsole( ATTACH_PARENT_PROCESS ) ) {
                freopen( "CONOUT$", "w", stdout );
                freopen( "CONOUT$", "w", stderr );
                SetFocus( GetConsoleWindow() );
            }
        }
    #endif
}


// pass-through so the program will build with either /SUBSYSTEM:WINDOWS or /SUBSYSTEM:CONSOLE
#if defined( _WIN32 )
    #ifdef __cplusplus
        extern "C" int __stdcall WinMain( struct HINSTANCE__*, struct HINSTANCE__*, char*, int ) {
            ensure_console_open();
            return main( __argc, __argv );
        }
    #else
        struct HINSTANCE__;
        int __stdcall WinMain( struct HINSTANCE__* a, struct HINSTANCE__* b, char* c, int d ) {
            (void) a, b, c, d; 
            ensure_console_open();
            return main( __argc, __argv );
        }
    #endif
#endif

