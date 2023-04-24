#ifdef __wasm__
    #define YARNSPIN_RUNTIME_ONLY
#endif

#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

// Need to do this before anything else is included, to get proper filenames in memory leak reporting
#if defined( _WIN32 ) && defined( _DEBUG )
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

// Yarnspin version for file formats. Increment by one for each release
#define YARNSPIN_VERSION 2

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
#include "libs/audiosys.h"
#include "libs/buffer.h"
#include "libs/crtemu.h"
#include "libs/cstr.h"
#include "libs/frametimer.h"
#include "libs/palrle.h"
#include "libs/qoi.h"
#include "libs/qoa.h"
#include "libs/rnd.h"
#include "libs/stb_image.h"
#include "libs/stb_image_resize.h"
#include "libs/stb_image_write.h"
#include "libs/thread.h"

#define PIXELFONT_COLOR PIXELFONT_U8
#define PIXELFONT_FUNC_NAME pixelfont_blit
#include "libs/pixelfont.h"

#undef PIXELFONT_COLOR
#undef PIXELFONT_FUNC_NAME
#define PIXELFONT_COLOR PIXELFONT_U32
#define PIXELFONT_FUNC_NAME pixelfont_blit_rgb
#include "libs/pixelfont.h"

// Tools-only includes
#ifndef YARNSPIN_RUNTIME_ONLY
    #include "libs/dir.h"
    #include "libs/dr_flac.h"
    #include "libs/dr_mp3.h"
    #include "libs/dr_wav.h"
    #include "libs/file.h"
    #include "libs/img.h"
    #include "libs/ini.h"
    #include "libs/paldither.h"
    #include "libs/palettize.h"
    #include "libs/samplerate.h"
    #include "libs/stb_truetype.h"
    #include "libs/stb_vorbis.h"
    #include "libs/sysfont.h"
#endif

// Version number stored in the file .cache\VERSION, read at start of program
int g_cache_version = 0;


// forward declares for helper functions placed at the end of this file

#ifndef YARNSPIN_RUNTIME_ONLY
    char const* cextname( char const* path );
    char const* cbasename( char const* path );

    void delete_file( char const* filename );
    void create_path( char const* path, int pos );
    int file_more_recent( char const* source_path, char const* output_path );
    int file_exists( char const* filename );
    int folder_exists( char const* filename );

    void* compress_lzma( void* data, size_t size, size_t* out_size );
#endif

size_t decompress_lzma( void* compressed_data, size_t compressed_size, void* buffer, size_t size );
char const* get_executable_filename( void );

bool save_data( char const* name, void* data, size_t size );
void* load_data( char const* name, size_t* out_size );

// automatic memory management helper stuff

#include "memmgr.h"
static struct memmgr_t g_memmgr = { 0 };

void array_deleter( void* context, void* ptr ) { (void) context; internal_array_destroy( (struct internal_array_t*) ptr ); }
#define managed_array( type ) ARRAY_CAST( memmgr_add( &g_memmgr, array_create( type ), NULL, array_deleter ) )

void palrle_deleter( void* context, void* ptr ) { (void) context; palrle_free( (palrle_data_t*) ptr, NULL ); }
#define manage_palrle( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, palrle_deleter ) )

#ifndef YARNSPIN_RUNTIME_ONLY
    void paldither_deleter( void* context, void* ptr ) { (void) context; paldither_palette_destroy( (paldither_palette_t*) ptr, NULL ); }
    #define manage_paldither( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, paldither_deleter ) )
#endif

void pixelfont_deleter( void* context, void* ptr ) { (void) context; free( ptr ); }
#define manage_pixelfont( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, pixelfont_deleter ) )

void alloc_deleter( void* context, void* ptr ) { (void) context; free( ptr ); }
#define manage_alloc( instance ) ARRAY_CAST( memmgr_add( &g_memmgr, instance, NULL, alloc_deleter ) )


// helper defines
typedef cstr_t string;
#define array(type) array_t(type)
#define array_param(type) array_param_t(type)
#define ARRAY_COUNT( x ) ( sizeof( x ) / sizeof( *(x) ) )


typedef struct qoi_data_t {
    uint32_t size;
    uint8_t data[ 1 ];
} qoi_data_t;


typedef struct qoa_data_t {
    uint32_t size;
    uint8_t data[ 1 ];
} qoa_data_t;


// yarnspin files
#ifndef YARNSPIN_RUNTIME_ONLY
    #include "gfxconv.h"
    #include "audioconv.h"
#endif
#include "yarn.h"
#include "input.h"
#include "game.h"


thread_mutex_t g_sound_mutex;

void sound_callback( APP_S16* sample_pairs, int sample_pairs_count, void* user_data ) {
    thread_mutex_lock( &g_sound_mutex );
    
    audiosys_t* audiosys = (audiosys_t*) user_data;
    audiosys_render( audiosys, sample_pairs, sample_pairs_count );
   
    thread_mutex_unlock( &g_sound_mutex );
}


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
    CRTEMU_U32* frame_pc_pixels = NULL;
    if( yarn->assets.frame_pc ) {
        int c;
        frame_pc_pixels = (CRTEMU_U32*) stbi_load_from_memory( (stbi_uc*) yarn->assets.frame_pc,
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

    crtemu_t* crtemu_lite = NULL;
    crtemu_t* crtemu_pc = NULL;
    crtemu_t* crtemu_tv = NULL;

    int widths[] = { 320, 400, 480, 640, 1440 };
    int heights[] = { 240, 300, 360, 480, 1080 };
    int screen_width = widths[ yarn->globals.resolution ];
    int screen_height = heights[ yarn->globals.resolution ];
    
    uint8_t* canvas = NULL;
    uint32_t* canvas_rgb = NULL;
    uint32_t* screen = (uint32_t*)malloc( ( 1440 + (int)( 44 * 4.5f ) ) * ( 1080 + (int)(66 * 4.5 ) ) * sizeof( uint32_t ) );
    memset( screen, 0, ( 1440 + (int)( 44 * 4.5f ) ) * ( 1080 + (int)( 66 * 4.5 ) ) * sizeof( uint32_t ) );

    audiosys_t* audiosys = audiosys_create( AUDIOSYS_DEFAULT_VOICE_COUNT, NULL );

    input_t input;
    input_init( &input, app );
    
    rnd_pcg_t rnd;
    rnd_pcg_seed( &rnd, (RND_U32) app_time_count( app ) );
    
    // run game
    game_t game;
    if( yarn->globals.colormode == YARN_COLORMODE_PALETTE ) {
        canvas = (uint8_t*)malloc( screen_width * screen_height * sizeof( uint8_t ) );
        memset( canvas, 0, screen_width * screen_height * sizeof( uint8_t ) );
        game_init( &game, yarn, &input, audiosys, &rnd, canvas, NULL, screen_width, screen_height );
    } else {
        canvas_rgb = (uint32_t*)malloc( screen_width * screen_height * sizeof( uint32_t ) );
        memset( canvas_rgb, 0, screen_width * screen_height * sizeof( uint32_t ) );
        game_init( &game, yarn, &input, audiosys, &rnd, NULL, canvas_rgb, screen_width, screen_height );
    }

    thread_mutex_init( &g_sound_mutex );
    app_sound( app, 735 * 8, sound_callback, audiosys );

    // main loop
    APP_U64 time = 0;
    while( app_yield( app ) != APP_STATE_EXIT_REQUESTED && !game.exit_flag ) {
        frametimer_update( frametimer );
        input_update( &input, screen_width, screen_height, crtemu_lite, crtemu_pc, crtemu_tv );

        thread_mutex_lock( &g_sound_mutex );
        game_update( &game, frametimer_delta_time( frametimer ) );
        thread_mutex_unlock( &g_sound_mutex );


        if( game.yarn->is_debug ) {
            char const* dbgstr = "debug";
            pixelfont_bounds_t bounds;
            pixelfont_blit( game.yarn->assets.font_description, 0, 0, dbgstr, 0, NULL, game.screen_width, game.screen_height,
                    PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );            
            if( game.screen ) {
                pixelfont_blit( game.yarn->assets.font_description, game.screen_width - bounds.width - 1, game.screen_height - bounds.height, dbgstr, (uint8_t)game.color_disabled, game.screen, game.screen_width, game.screen_height,
                    PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, NULL );
            } else {
                pixelfont_blit_rgb( game.yarn->assets.font_description, game.screen_width - bounds.width - 1, game.screen_height - bounds.height, dbgstr, 0x404040, game.screen_rgb, game.screen_width, game.screen_height,
                    PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, NULL );
            }

        }

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

        if( display_filter == YARN_DISPLAY_FILTER_LITE && crtemu_lite == NULL ) {
            if( crtemu_tv ) {
                crtemu_destroy( crtemu_tv );
                crtemu_tv = NULL;
            }
            if( crtemu_pc ) {
                crtemu_destroy( crtemu_pc );
                crtemu_pc = NULL;
            }
            crtemu_lite = crtemu_create( CRTEMU_TYPE_LITE, NULL );
        }
        
        if( display_filter == YARN_DISPLAY_FILTER_PC && crtemu_pc == NULL ) {
            if( crtemu_lite ) {
                crtemu_destroy( crtemu_lite );
                crtemu_lite = NULL;
            }
            if( crtemu_tv ) {
                crtemu_destroy( crtemu_tv );
                crtemu_tv = NULL;
            }
            crtemu_pc = crtemu_create( CRTEMU_TYPE_PC, NULL );
            if( crtemu_pc && frame_pc_pixels ) {
                crtemu_frame( crtemu_pc, frame_pc_pixels, frame_pc_width, frame_pc_height );
            }
        }

        if( display_filter == YARN_DISPLAY_FILTER_TV && crtemu_tv == NULL ) {
            if( crtemu_lite ) {
                crtemu_destroy( crtemu_lite );
                crtemu_lite = NULL;
            }
            if( crtemu_pc ) {
                crtemu_destroy( crtemu_pc );
                crtemu_pc = NULL;
            }
            memset( screen, 0, ( 1440 + (int)( 22 * 4.5f ) ) * ( 1080 + (int)( 33 * 4.5 ) ) * sizeof( uint32_t ) );
            crtemu_tv = crtemu_create( CRTEMU_TYPE_TV, NULL );
            if( crtemu_tv && frame_tv_pixels ) {
                crtemu_frame( crtemu_tv, frame_tv_pixels, frame_tv_width, frame_tv_height );
            }
        }

        if( display_filter == YARN_DISPLAY_FILTER_NONE && crtemu_lite != NULL ) {
            crtemu_destroy( crtemu_lite );
            crtemu_lite = NULL;
        }

        if( display_filter == YARN_DISPLAY_FILTER_NONE && crtemu_pc != NULL ) {
                crtemu_destroy( crtemu_pc );
                crtemu_pc = NULL;
        }

        if( display_filter == YARN_DISPLAY_FILTER_NONE && crtemu_tv != NULL ) {
                crtemu_destroy( crtemu_tv );
                crtemu_tv = NULL;
                memset( screen, 0, ( 1440 + (int)( 22 * 4.5f ) ) * ( 1080 + (int)( 33 * 4.5 ) ) * sizeof( uint32_t ) );
        }

        if( crtemu_lite ) {
            if( canvas ) {
                for( int i = 0; i < screen_width * screen_height; ++i ) {
                    screen[ i ] = yarn->assets.palette[ canvas[ i ] ];
                }
                crtemu_present( crtemu_lite, time, screen, screen_width, screen_height, fade, 0x000000 );
            } else {
                crtemu_present( crtemu_lite, time, canvas_rgb, screen_width, screen_height, fade, 0x000000 );
            }
            app_present( app, NULL, 1, 1, 0xffffff, 0x000000 );
        } else if( crtemu_pc ) {
            if( canvas ) {
                for( int i = 0; i < screen_width * screen_height; ++i ) {
                    screen[ i ] = yarn->assets.palette[ canvas[ i ] ];
                }
                crtemu_present( crtemu_pc, time, screen, screen_width, screen_height, fade, 0x000000 );
            } else {
                crtemu_present( crtemu_pc, time, canvas_rgb, screen_width, screen_height, fade, 0x000000 );
            }
            app_present( app, NULL, 1, 1, 0xffffff, 0x000000 );
        } else if( crtemu_tv ) {
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
            crtemu_present( crtemu_tv, time, screen, screen_width + 2 * offset_x, screen_height + 2 * offset_y, fade, 0x000000 );
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

    app_sound( app, 0, NULL, NULL );
    thread_mutex_term( &g_sound_mutex );

    audiosys_destroy( audiosys );

    frametimer_destroy( frametimer );
    if( crtemu_lite ) {
        crtemu_destroy( crtemu_lite );
    }
    if( crtemu_pc ) {
        crtemu_destroy( crtemu_pc );
    }
    if( crtemu_tv ) {
        crtemu_destroy( crtemu_tv );
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


#ifndef YARNSPIN_RUNTIME_ONLY
    #include "imgedit.h"
    void threads_init( void );
#endif

#ifdef _WIN32
    void opengl_preinit( void );
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

    #ifdef _WIN32
        opengl_preinit();
    #endif

    // if -i or --images parameter were specified, run image editor
    #ifndef YARNSPIN_RUNTIME_ONLY
        if( argc == 2 && ( strcmp( argv[ 1 ], "-i" ) == 0 || strcmp( argv[ 1 ], "--images" ) == 0 ) ) {
            threads_init();
            int result = -1;
            while( result < 0 ) {
                result = app_run( imgedit_proc, NULL, NULL, NULL, NULL );
            } 
            return result;
        }
    #endif

    bool is_debug = false;
    #ifndef YARNSPIN_RUNTIME_ONLY
        bool no_compile = false;
        if( argc == 2 && ( strcmp( argv[ 1 ], "-r" ) == 0 || strcmp( argv[ 1 ], "--run" ) == 0 ) ) {
            no_compile = true;
        }
 
        if( argc == 2 && ( strcmp( argv[ 1 ], "-d" ) == 0 || strcmp( argv[ 1 ], "--debug" ) == 0 ) ) {
            is_debug = true;
        }

        // compile yarn
        if( folder_exists( "scripts" ) && !no_compile ) {
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

            size_t size = 0;
            void* data = NULL;
            if( !is_debug ) {
                printf( "Compressing yarn file\n" );
                data = compress_lzma( buffer_data( compiled_yarn ), buffer_size( compiled_yarn ), &size );
            }
            uint32_t original_size = (uint32_t) buffer_size( compiled_yarn );

            FILE* fp = fopen( "yarnspin.dat", "wb" );
            char header[] = "YARNSPIN";
            char headerdbg[] = "YARN_DBG";
            uint32_t version = YARNSPIN_VERSION;
            uint32_t size_in_bytes = (uint32_t)( size + 2 * strlen( is_debug ? headerdbg : header ) + sizeof( uint32_t ) * 5 );
            fwrite( is_debug ? headerdbg : header, 1, strlen( is_debug ? headerdbg : header ), fp );
            fwrite( &version, 1, sizeof( version ), fp );
            fwrite( &size_in_bytes, 1, sizeof( size_in_bytes ), fp );
            fwrite( &original_size, 1, sizeof( original_size ), fp );
            if( !is_debug ) {
                fwrite( data, 1, size, fp );
                free( data );
            } else {
                fwrite( buffer_data( compiled_yarn ), 1, buffer_size( compiled_yarn ), fp );
            }
            fwrite( &size_in_bytes, 1, sizeof( size_in_bytes ), fp );
            fwrite( &version, 1, sizeof( version ), fp );
            fwrite( is_debug ? headerdbg : header, 1, strlen( is_debug ? headerdbg : header ), fp );
            fclose( fp );
            printf( "yarnspin.dat written\n" );

            buffer_destroy( compiled_yarn );

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
        bool is_yarnspin = strncmp( header, "YARNSPIN", 8 ) == 0;
        bool is_yarn_dbg = strncmp( header, "YARN_DBG", 8 ) == 0;
        if( !is_yarnspin && !is_yarn_dbg ) {
            printf( "The file yarnspin.dat is not a valid Yarnspin game data file\n" );
            return EXIT_FAILURE;
        }
        if( is_yarn_dbg ) {
            is_debug = true;
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

        if( !is_yarn_dbg ) {
            size_t decompressed_size = decompress_lzma( data, size, buffer_data( decompressed_yarn ), uncompressed_size );
            buffer_destroy( loaded_yarn );
            if( decompressed_size != uncompressed_size ) {
                buffer_destroy( decompressed_yarn );
                printf( "Failed to decompress game file\n" );
                return EXIT_FAILURE;
            }
        } else {
            if( size < uncompressed_size ) {
                buffer_destroy( decompressed_yarn );
                buffer_destroy( loaded_yarn );
                printf( "Game file has invalid size\n" );
                return EXIT_FAILURE;
            }
            memcpy( buffer_data( decompressed_yarn ), data, uncompressed_size ); 
            buffer_destroy( loaded_yarn );
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
        bool is_yarnspin = strncmp( header, "YARNSPIN", 8 ) == 0;
        bool is_yarn_dbg = strncmp( header, "YARN_DBG", 8 ) == 0;
        if( !is_yarnspin && !is_yarn_dbg ) {
            printf( "No yarnspin.dat game data can be loaded\n" );
            return EXIT_FAILURE;
        }
        if( is_yarn_dbg ) {
            is_debug = true;
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

        if( !is_yarn_dbg ) {
            size_t decompressed_size = decompress_lzma( data, size, buffer_data( decompressed_yarn ), uncompressed_size );
            free( data );
            if( decompressed_size != uncompressed_size ) {
                buffer_destroy( decompressed_yarn );
                printf( "Failed to decompress game file\n" );
                return EXIT_FAILURE;
            }
        } else {
            if( size < uncompressed_size ) {
                buffer_destroy( decompressed_yarn );
                free( data );
                printf( "Game file has invalid size\n" );
                return EXIT_FAILURE;
            }
            memcpy( buffer_data( decompressed_yarn ), data, uncompressed_size ); 
                free( data );
        }
        
    #endif
    }

    // if -c or --compile parameter were specified, don't run the game, just exit after compiling the yarn
    if( argc == 2 && ( strcmp( argv[ 1 ], "-c" ) == 0 || strcmp( argv[ 1 ], "--compile" ) == 0 ) ) {
        return EXIT_SUCCESS;
    }

    // load yarn
    yarn_t yarn;
    yarn_load( decompressed_yarn, &yarn, is_debug );
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

#define AUDIOSYS_IMPLEMENTATION
#include "libs/audiosys.h"

#define BUFFER_IMPLEMENTATION
#include "libs/buffer.h"

#define CRTEMU_IMPLEMENTATION
#include "libs/crtemu.h"

#define CSTR_IMPLEMENTATION
#include "libs/cstr.h"

#define FRAMETIMER_IMPLEMENTATION
#include "libs/frametimer.h"

#define LZMA_IMPLEMENTATION
#include "libs/lzma.h"

#define PALRLE_IMPLEMENTATION
#include "libs/palrle.h"

#define PIXELFONT_IMPLEMENTATION
#ifndef YARNSPIN_RUNTIME_ONLY
    #define PIXELFONT_BUILDER_IMPLEMENTATION
#endif
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

#define QOA_IMPLEMENTATION
#include "libs/qoa.h"

#define RND_IMPLEMENTATION
#include "libs/rnd.h"

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

#ifndef YARNSPIN_RUNTIME_ONLY

#define DIR_IMPLEMENTATION
#ifdef _WIN32
    #define DIR_WINDOWS
#else
    #define DIR_POSIX
#endif
#include "libs/dir.h"

#define DR_FLAC_IMPLEMENTATION
#include "libs/dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#include "libs/dr_mp3.h"

#define DR_WAV_IMPLEMENTATION
#include "libs/dr_wav.h"

#define FILE_IMPLEMENTATION
#include "libs/file.h"

#define IMG_IMPLEMENTATION
#include "libs/img.h"

#define INI_IMPLEMENTATION
#include "libs/ini.h"

#define PALDITHER_IMPLEMENTATION
#include "libs/paldither.h"

#define PALETTIZE_IMPLEMENTATION
#include "libs/palettize.h"

#define SAMPLERATE_IMPLEMENTATION
#include "libs/samplerate.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_RASTERIZER_VERSION 1
#include "libs/stb_truetype.h"

#define STB_VORBIS_IMPLEMENTATION
#include "libs/stb_vorbis.h"

#pragma warning( push )
#pragma warning( disable: 4456 )
#pragma warning( disable: 4457 )
#define SYSFONT_IMPLEMENTATION
#include "libs/sysfont.h"
#pragma warning( pop )

#endif

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

void makedir( char const* path ) {
    #ifdef _WIN32
        CreateDirectoryA( path, NULL );
    #else
        mkdir( path, S_IRWXU );
    #endif
}


#ifndef YARNSPIN_RUNTIME_ONLY

void delete_file( char const* filename ) {
    #ifdef _WIN32
        DeleteFileA( filename );
    #else
        remove( filename );
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

#endif

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


#ifdef __wasm__

    char* base64enc( unsigned char const* data, size_t input_length, size_t* out_size ) {
        static const char encoding_table[] = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
            'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
            'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
            'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
            'w', 'x', 'y', 'z', '0', '1', '2', '3',
            '4', '5', '6', '7', '8', '9', '-', '_'
        };

        const int mod_table[] = { 0, 2, 1 };

        size_t output_length = 4 * ( ( input_length + 2 ) / 3 );

        char* encoded_data = (char*) malloc( output_length );

        if( encoded_data == NULL ) {
            return NULL;
        }

        for( int i = 0, j = 0; i < input_length; ) {

            uint32_t octet_a = i < input_length ? (unsigned char) data[ i++ ] : 0;
            uint32_t octet_b = i < input_length ? (unsigned char) data[ i++ ] : 0;
            uint32_t octet_c = i < input_length ? (unsigned char) data[ i++ ] : 0;

            uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

            encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];

        }

        for( int i = 0; i < mod_table[ input_length % 3 ]; ++i ) {
            encoded_data[ output_length - 1 - i ] = '=';
        }

        output_length = output_length - 2 + mod_table[ input_length % 3 ];
        encoded_data[ output_length ] = 0;
        if( out_size ) {
            *out_size = output_length;
        }
        return encoded_data;

    }

    void* base64dec( char const* data, size_t input_length, size_t* out_size ) {
        static const unsigned char decoding_table[256] = {
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,
            0x3d,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,
            0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x00,0x00,0x00,0x00,
            0x3f,0x00,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,
            0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
        };

        if( input_length % 4 != 0 ) {
            return NULL;
        }

        size_t output_length = input_length / 4 * 3;

        if( data[ input_length - 1 ] == '=') output_length--;
        if( data[ input_length - 2 ] == '=') output_length--;

        unsigned char* decoded_data = (unsigned char*)malloc( output_length + 1 );

        if( decoded_data == NULL ) {
            return NULL;
        }

        for( int i = 0, j = 0; i < input_length; ) {

            uint32_t sextet_a = data[ i ] == '=' ? 0 & i++ : decoding_table[ data[ i++ ] ];
            uint32_t sextet_b = data[ i ] == '=' ? 0 & i++ : decoding_table[ data[ i++ ] ];
            uint32_t sextet_c = data[ i ] == '=' ? 0 & i++ : decoding_table[ data[ i++ ] ];
            uint32_t sextet_d = data[ i ] == '=' ? 0 & i++ : decoding_table[ data[ i++ ] ];

            uint32_t triple = 
                  ( sextet_a << 3 * 6 )
                + ( sextet_b << 2 * 6 )
                + ( sextet_c << 1 * 6 )
                + ( sextet_d << 0 * 6 );

            if( j < output_length ) { decoded_data[ j++ ] = ( triple >> 2 * 8 ) & 0xFF; }
            if( j < output_length ) { decoded_data[ j++ ] = ( triple >> 1 * 8 ) & 0xFF; }
            if( j < output_length ) { decoded_data[ j++ ] = ( triple >> 0 * 8 ) & 0xFF; }
        }

        decoded_data[ output_length ] = 0;

        if( out_size ) {
            *out_size = output_length;
        }

        return decoded_data;

    };

    WAJIC(bool, web_storage_save, ( char const* name, char const* value ), {
        try {
            window.localStorage.setItem( MStrGet( name ), MStrGet( value ) );        
        } catch {
            return false;
        }
        return true;
    })
    
    WAJIC(char*, web_storage_load, ( char const* name ), {
        var value = window.localStorage.getItem( MStrGet( name ) );
        if( value == null ) {
            return null;
        }
        return MStrPut( value );
    })

    bool save_data( char const* name, void* data, size_t size ) {
        char* str = base64enc( data, size, NULL );
        web_storage_save( name, str );
        free( str );
        return false;
    }

    void* load_data( char const* name, size_t* out_size ) {        
        char* value = web_storage_load( name );
        if( !value ) {
            return NULL;
        }
        size_t size = 0;
        void* data = base64dec( value, strlen( value ), &size );
        free( value );
        if( data && out_size ) {
            *out_size = size;
        }
        return data;
    }

#else

    bool save_data( char const* name, void* data, size_t size ) {
        if( !folder_exists( "savegame" ) ) {
            makedir( "savegame" );
        }
        cstr_t filename = cstr_cat( "savegame/", name );
        FILE* fp = fopen( filename, "wb" );
        if( !fp ) {
            return false;
        }
        size_t size_written = fwrite( data, 1, size, fp );
        fclose( fp );
        if( size_written != size ) {
            return false;
        }
        return true;
    }


    void* load_data( char const* name, size_t* out_size ) {      
        if( !folder_exists( "savegame" ) ) {
            return NULL;
        }
        cstr_t filename = cstr_cat( "savegame/", name );
        FILE* fp = fopen( filename, "rb" );
        if( !fp ) {
            return NULL;
        }
        fseek( fp, 0, SEEK_END );
        size_t size = ftell( fp );
        fseek( fp, 0, SEEK_SET );
        if( size == 0 ) {
            fclose( fp );
            return NULL;
        }
        void* data = malloc( size + 1 );
        size_t size_read = fread( data, 1, size, fp );
        fclose( fp );
        if( size_read != size ) {
            free( data );
            return NULL;
        }
        ((char*)data)[ size ] = '\0';
        if( out_size ) {
            *out_size = size;
        }
        return data;
    }

#endif


    #ifdef _WIN32

    int opengl_preinit_thread_proc( void* user_data ) {
        (void) user_data;
        HDC dc = GetDC( NULL );
	    DescribePixelFormat( dc, 0, 0, NULL );
	    ReleaseDC( NULL, dc );
        return 0;
    }


    void opengl_preinit( void ) {
        thread_create( opengl_preinit_thread_proc, NULL, THREAD_STACK_SIZE_DEFAULT );
    }

#endif

#ifndef YARNSPIN_RUNTIME_ONLY

void atarist_palettize( PALDITHER_U32* abgr, int width, int height, paldither_palette_t const* palette,
    paldither_type_t dither_type, PALDITHER_U8* output )
    {
    unsigned char default_dither_pattern[ 4 * 4 * 11 ] =
        {
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,

        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,

        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,

        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,1,

        1,0,1,0,
        0,1,0,0,
        1,0,1,0,
        0,0,0,1,

        1,0,1,0,
        0,1,0,1,
        1,0,1,0,
        0,1,0,1,

        1,0,1,0,
        1,1,0,1,
        1,0,1,0,
        0,1,1,1,

        1,1,1,1,
        1,1,1,1,
        1,1,1,1,
        1,1,1,1,

        1,1,1,1,
        1,1,1,1,
        1,1,1,1,
        1,1,1,1,

        1,1,1,1,
        1,1,1,1,
        1,1,1,1,
        1,1,1,1,

        1,1,1,1,
        1,1,1,1,
        1,1,1,1,
        1,1,1,1,

       };

    unsigned char bayer_dither_pattern[ 4 * 4 * 17 ] =
        {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,

        1, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,

        1, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 0,

        1, 0, 1, 0,
        0, 0, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 0,

        1, 0, 1, 0,
        0, 0, 0, 0,
        1, 0, 1, 0,
        0, 0, 0, 0,

        1, 0, 1, 0,
        0, 1, 0, 0,
        1, 0, 1, 0,
        0, 0, 0, 0,

        1, 0, 1, 0,
        0, 1, 0, 0,
        1, 0, 1, 0,
        0, 0, 0, 1,

        1, 0, 1, 0,
        0, 1, 0, 1,
        1, 0, 1, 0,
        0, 0, 0, 1,

        1, 0, 1, 0,
        0, 1, 0, 1,
        1, 0, 1, 0,
        0, 1, 0, 1,

        1, 1, 1, 0,
        0, 1, 0, 1,
        1, 0, 1, 0,
        0, 1, 0, 1,

        1, 1, 1, 0,
        0, 1, 0, 1,
        1, 0, 1, 1,
        0, 1, 0, 1,

        1, 1, 1, 1,
        0, 1, 0, 1,
        1, 0, 1, 1,
        0, 1, 0, 1,

        1, 1, 1, 1,
        0, 1, 0, 1,
        1, 1, 1, 1,
        0, 1, 0, 1,

        1, 1, 1, 1,
        1, 1, 0, 1,
        1, 1, 1, 1,
        0, 1, 0, 1,

        1, 1, 1, 1,
        1, 1, 0, 1,
        1, 1, 1, 1,
        0, 1, 1, 1,

        1, 1, 1, 1,
        1, 1, 1, 1,
        1, 1, 1, 1,
        0, 1, 1, 1,

        1, 1, 1, 1,
        1, 1, 1, 1,
        1, 1, 1, 1,
        1, 1, 1, 1,
        };

    unsigned char none_dither_pattern[ 4 * 4 * 2 ] =
        {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        };


    uintptr_t ptr = (uintptr_t)( palette + 1 );

    int default_pal_mix_count = *(int*)ptr; ptr += sizeof( int );
    paldither_mix_t const* default_pal_mix = (paldither_mix_t*) ptr; ptr += default_pal_mix_count * sizeof( paldither_mix_t );
    int const* default_pal_map = (int*) ptr; ptr += 16 * 16 * 16 * sizeof( int );
    int default_pal_list_count = *(int*)ptr; ptr += sizeof( int );
    int const* default_pal_list = (int*) ptr; ptr += default_pal_list_count * sizeof( int );

    int bayer_pal_mix_count = *(int*)ptr; ptr += sizeof( int );
    paldither_mix_t const* bayer_pal_mix = (paldither_mix_t*) ptr; ptr += bayer_pal_mix_count * sizeof( paldither_mix_t );
    int const* bayer_pal_map = (int*) ptr; ptr += 16 * 16 * 16 * sizeof( int );
    int bayer_pal_list_count = *(int*)ptr; ptr += sizeof( int );
    int const* bayer_pal_list = (int*) ptr; ptr += bayer_pal_list_count * sizeof( int );

    int nodither_pal_mix_count = *(int*)ptr; ptr += sizeof( int );
    paldither_mix_t const* nodither_pal_mix = (paldither_mix_t*) ptr; ptr += nodither_pal_mix_count * sizeof( paldither_mix_t );
    int const* nodither_pal_map = (int*) ptr; ptr += 16 * 16 * 16 * sizeof( int );
    int nodither_pal_list_count = *(int*)ptr; ptr += sizeof( int );
    int const* nodither_pal_list = (int*) ptr; ptr += nodither_pal_list_count * sizeof( int );

    unsigned char* dither_pattern = default_dither_pattern;
    int pal_mix_count = default_pal_mix_count;
    paldither_mix_t const* pal_mix = default_pal_mix;
    int const* pal_map = default_pal_map;
    int const* pal_list = default_pal_list;

    if( dither_type == PALDITHER_TYPE_BAYER )
        {
        dither_pattern = bayer_dither_pattern;
        pal_mix_count = bayer_pal_mix_count;
        pal_mix = bayer_pal_mix;
        pal_map = bayer_pal_map;
        pal_list = bayer_pal_list;
        }
    else if( dither_type == PALDITHER_TYPE_NONE )
        {
        dither_pattern = none_dither_pattern;
        pal_mix_count = nodither_pal_mix_count;
        pal_mix = nodither_pal_mix;
        pal_map = nodither_pal_map;
        pal_list = nodither_pal_list;
        }

    for( int y = 0; y < height; ++y )
        {
        for( int x = 0; x < width; ++x )
            {
            PALDITHER_U32 color = abgr[ x + y * width ];
            int r = (int)( ( color & 0x000000ff ) );
            int g = (int)( ( color & 0x0000ff00 ) >> 8 );
            int b = (int)( ( color & 0x00ff0000 ) >> 16 );
            int l = (int)( ( 54 * r + 183 * g + 19 * b + 127 ) >> 8 );
            PALDITHER_ASSERT( l <= 0xff && l >= 0, "Value out of range" );

            paldither_mix_t const* best_mix = 0;
            int pal_index = pal_map[ ( r >> 4 ) * 16 * 16 + ( g >> 4 ) * 16 + ( b >> 4 ) ];
            int count = pal_list[ pal_index++ ];
            if( count != 0 )
                {
                int best_diff = 0x7FFFFFFF;
                int const* index = &pal_list[ pal_index ];
                for( int i = 0; i < count; ++i, ++index )
                    {
                    paldither_mix_t const* m = &pal_mix[ *index ];
                    int dr = r - m->r;
                    int dg = g - m->g;
                    int db = b - m->b;
                    int dl = l - m->l;
                    int d = ( ( ( dr*dr + dg*dg + db*db ) >> 1 ) + dl*dl) + ( m->d * 3 );
                    if( i == 0 || d < best_diff ) { best_diff = d; best_mix = m; }
                    }
                }
            else
                {
                int best_diff = 0x7FFFFFFF;
                paldither_mix_t const* m = pal_mix;
                for( int i = 0; i < pal_mix_count; ++i, ++m )
                    {
                    int dr = r - m->r;
                    int dg = g - m->g;
                    int db = b - m->b;
                    int dl = l - m->l;
                    int d = ( ( ( dr*dr + dg*dg + db*db ) >> 1 ) + dl*dl) + ( m->d * 3 );
                    if( i == 0 || d < best_diff ) { best_diff = d; best_mix = m; }
                    }
                }


            int ratio_max = dither_type == PALDITHER_TYPE_BAYER ? 17 : dither_type == PALDITHER_TYPE_DEFAULT ? 11 : 1;
            int index = dither_pattern[ best_mix->ratio * 4 * 4 + ( x & 3 ) + ( y & 3 ) * 4 ] ?
                best_mix->second : best_mix->first;

            int d = 0;
            {
                PALDITHER_U32 f = palette->colortable[ best_mix->first ];
                int fr = (int)( ( f & 0x000000ff ) );
                int fg = (int)( ( f & 0x0000ff00 ) >> 8 );
                int fb = (int)( ( f & 0x00ff0000 ) >> 16 );
                int fl = (int)( ( 54 * fr + 183 * fg + 19 * fb + 127 ) >> 8 );
                PALDITHER_U32 s = palette->colortable[ best_mix->second ];
                int sr = (int)( ( s & 0x000000ff ) );
                int sg = (int)( ( s & 0x0000ff00 ) >> 8 );
                int sb = (int)( ( s & 0x00ff0000 ) >> 16 );
                int sl = (int)( ( 54 * sr + 183 * sg + 19 * sb + 127 ) >> 8 );
                d = abs( fl - sl );
            }


            if( d > 48 ) {
                int best_diff = 0x7FFFFFFF;
                int best_index = 0;
                for( int i = 0; i < palette->color_count; ++i )
                    {
                    PALDITHER_U32 c = palette->colortable[ i ];
                    int cr = (int)( ( c & 0x000000ff ) );
                    int cg = (int)( ( c & 0x0000ff00 ) >> 8 );
                    int cb = (int)( ( c & 0x00ff0000 ) >> 16 );
                    int cl = (int)( ( 54 * cr + 183 * cg + 19 * cb + 127 ) >> 8 );
                    int dr = r - cr;
                    int dg = g - cg;
                    int db = b - cb;
                    int dl = l - cl;
                    int d = ( ( ( dr*dr + dg*dg + db*db ) >> 1 ) + dl*dl);
                    if( i == 0 || d < best_diff ) { best_diff = d; best_index = i; }
                    }
                index = best_index;
                }

            if( output) output[ x + y * width ] = (PALDITHER_U8) index;
            abgr[ x + y * width ] = ( abgr[ x + y * width ] & 0xff000000 ) | ( palette->colortable[ index ] & 0x00ffffff );
            }
        }
    }
    
    #endif