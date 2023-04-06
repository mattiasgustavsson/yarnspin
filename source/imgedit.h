
typedef struct imgedit_image_t {
    cstr_t name;
    cstr_t filename;
    int orig_width;
    int orig_height;
    uint32_t* orig_pixels;
    int sized_width;
    int sized_height;
    uint32_t* sized_pixels;
    uint32_t* processed;
    uint32_t* prev_processed;
    bool use_individual_settings;
    process_settings_t settings;
} imgedit_image_t;


void imgedit_resize( imgedit_image_t* image ) {
    int width = image->orig_width;
    int height = image->orig_height;
    int new_width = image->sized_width;
    int new_height = image->sized_height;
    if( new_width < width && new_height < height) {
        float horiz = new_width / (float) width;
        float vert = new_height / (float) height;
        float scale = max( horiz, vert );
        int scaled_width = fround( scale * width );
        int scaled_height = fround( scale * height );
        uint32_t* scaled = (uint32_t*) malloc(  scaled_width * scaled_height * sizeof( uint32_t ) );
        stbir_resize_uint8( (unsigned char*) image->orig_pixels, width, height, width * sizeof( uint32_t ), 
            (unsigned char* )scaled,  scaled_width, scaled_height, scaled_width * sizeof( uint32_t ), 4 );
        int hcrop = ( scaled_width - new_width );
        int vcrop = ( scaled_height - new_height );
        int xofs = hcrop / 2;
        int yofs = vcrop / 2;
        uint32_t* cropped = (uint32_t*) malloc( new_width * new_height * sizeof( uint32_t ) );
        for( int y = 0; y < new_height; ++y ) {
            memcpy( cropped + y * new_width, scaled + xofs + ( y + yofs ) * scaled_width, new_width * sizeof( uint32_t ) );
        }
        free( scaled );
        image->sized_pixels = cropped;
    } else if( new_width == width && new_height == height) {
        uint32_t* cropped = (uint32_t*) malloc( new_width * new_height * sizeof( uint32_t ) );
        memcpy( cropped, image->orig_pixels, new_width * new_height * sizeof( uint32_t ) );
        image->sized_pixels = cropped;
    } else {
        printf( "Image too small\n" );
    }
}

void imgedit_load_img( imgedit_image_t* img ) {
    if( img->orig_pixels && img->sized_pixels ) {
        return;
    }

    int w, h, c;
    stbi_uc* pixels = stbi_load( img->filename, &w, &h, &c, 4 );
    if( pixels ) {
        img->orig_width = w;
        img->orig_height = h;
        img->orig_pixels = (uint32_t*) malloc( w * h * sizeof( uint32_t ) );
        memcpy( img->orig_pixels, pixels, w * h * sizeof( uint32_t ) );
        stbi_image_free( pixels );

        cstr_t ini_filename = cstr_cat( img->filename, ".ini" );
        img->use_individual_settings = load_settings( &img->settings, ini_filename );
    } else {
        printf( "Failed to load image '%s'\n", img->filename );
    }
}


void imgedit_list_images( array_param(imgedit_image_t)* images, cstr_t folder, float resolution_scale ) {
    dir_t* dir = dir_open( folder );
    if( !dir ) {
        printf( "Could not find '%s' folder\n", folder );
        return;
    }

    bool mode_faces = cstr_is_equal( folder, "faces" );

    for( dir_entry_t* d = dir_read( dir ); d != NULL; d = dir_read( dir ) ) {
        if( dir_is_file( d ) ) {
            cstr_t name = cstr( dir_name( d ) );
            if( cstr_is_equal( name, "settings.ini" ) ) {
                continue;
            }
            if( strrchr( name, '.' ) == NULL || stricmp( strrchr( name, '.' ), ".ini" ) == 0 ) {
                continue;
            }
            cstr_t filename = cstr_cat( cstr_cat( folder, "/" ), name ); // TODO: cstr_join
            imgedit_image_t img = { NULL };
            img.name = cstr( cbasename( name ) );
            img.filename = filename;
            img.orig_width = 0;
            img.orig_height = 0;
            img.orig_pixels = NULL;
            img.sized_width = (int)( ( mode_faces ? 112 : 192 ) * resolution_scale ) ;
            img.sized_height = (int)( ( mode_faces ? 112 : 128 ) * resolution_scale );
            img.sized_pixels = NULL;
            img.processed = NULL;
            img.prev_processed = NULL;
            array_add( images, &img );
        }
    }
    dir_close( dir );
}



void imgedit_list_palettes( array_param(cstr_t)* palettes ) {
    dir_t* dir = dir_open( "palettes" );
    if( !dir ) {
        printf( "Could not find palettes folder\n" );
        return;
    }

    for( dir_entry_t* d = dir_read( dir ); d != NULL; d = dir_read( dir ) ) {
        if( dir_is_file( d ) ) {
            cstr_t name = cstr( dir_name( d ) );
            cstr_t filename = cstr_cat( "palettes/" , name ); 
            int w, h, c;
            stbi_uc* img = stbi_load( filename, &w, &h, &c, 4 );
            if( img && w > 0 && h > 0 ) {
                stbi_image_free( img );
                array_add( palettes, &name );
            }
        }
    }
    dir_close( dir );
}


#if defined( _WIN32 ) && !( defined( __clang__ ) || defined( __TINYC__ ) )
__forceinline
#endif
uint32_t imgedit_blend( uint32_t color1, uint32_t color2, uint8_t alpha ) {
    uint64_t c1 = (uint64_t) color1;
    uint64_t c2 = (uint64_t) color2;
    uint64_t a = (uint64_t)( alpha );
    // bit magic to alpha blend R G B with single mul
    c1 = ( c1 | ( c1 << 24 ) ) & 0x00ff00ff00ffull;
    c2 = ( c2 | ( c2 << 24 ) ) & 0x00ff00ff00ffull;
    uint64_t o = ( ( ( ( c2 - c1 ) * a ) >> 8 ) + c1 ) & 0x00ff00ff00ffull; 
    return (uint32_t) ( o | ( o >> 24 ) );
}


void imgedit_blit( uint32_t* pixels, int w, int h, int xp, int yp, int scale, uint32_t* screen, int screen_width, int screen_height ) {
    int clipy = yp < 0 ? -yp / scale : 0; 
    int cliph = h - ( ( ( yp + scale * h ) - ( screen_height - 10 ) ) / scale );
    cliph = cliph < 0 ? 0 : cliph;
    cliph = cliph > h ? h : cliph;
    int clipx = xp < 0 ? -xp / scale : 0; 
    int clipw = w - ( ( ( xp + scale * w ) - ( screen_width ) ) / scale );
    clipw = clipw < 0 ? 0 : clipw;
    clipw = clipw > w ? w : clipw;

    if( scale == 1 ) {
        for( int y = clipy; y < cliph; ++y ) {
            for( int x = clipx; x < clipw; ++x ) {
                int xo = xp + x;
                int yo = yp + y;
                uint32_t c = pixels[ x + y * w ];
                uint32_t a = c >> 24;
                if( a < 255 ) {
                    c = imgedit_blend( screen[ xo + 0 + ( yo + 0 ) * screen_width ], c, a );
                }
                if( a > 0 ) {
                    screen[ xo + 0 + ( yo + 0 ) * screen_width ] = c;
                }
            }
        }
    } else if( scale == 2 ) {
        for( int y = clipy; y < cliph; ++y ) {
            for( int x = clipx; x < clipw; ++x ) {
                int xo = xp + x * 2;
                int yo = yp + y * 2;
                uint32_t c = pixels[ x + y * w ];
                uint32_t a = c >> 24;
                if( a < 255 ) {
                    c = imgedit_blend( screen[ xo + 0 + ( yo + 0 ) * screen_width ], c, a );
                }
                if( a > 0 ) {
                    screen[ xo + 0 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 0 + ( yo + 1 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 1 ) * screen_width ] = c;
                }
            }
        }
    } else if( scale == 3 ) {
        for( int y = clipy; y < cliph; ++y ) {
            for( int x = clipx; x < clipw; ++x ) {
                int xo = xp + x * 3;
                int yo = yp + y * 3;
                uint32_t c = pixels[ x + y * w ];
                uint32_t a = c >> 24;
                if( a < 255 ) {
                    c = imgedit_blend( screen[ xo + 0 + ( yo + 0 ) * screen_width ], c, a );
                }
                if( a > 0 ) {
                    screen[ xo + 0 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 2 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 0 + ( yo + 1 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 1 ) * screen_width ] = c;
                    screen[ xo + 2 + ( yo + 1 ) * screen_width ] = c;
                    screen[ xo + 0 + ( yo + 2 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 2 ) * screen_width ] = c;
                    screen[ xo + 2 + ( yo + 2 ) * screen_width ] = c;
                }
            }
        }
    } else if( scale == 4 ) {
        for( int y = clipy; y < cliph; ++y ) {
            for( int x = clipx; x < clipw; ++x ) {
                int xo = xp + x * 4;
                int yo = yp + y * 4;
                uint32_t c = pixels[ x + y * w ];
                uint32_t a = c >> 24;
                if( a < 255 ) {
                    c = imgedit_blend( screen[ xo + 0 + ( yo + 0 ) * screen_width ], c, a );
                }
                if( a > 0 ) {
                    screen[ xo + 0 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 2 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 3 + ( yo + 0 ) * screen_width ] = c;
                    screen[ xo + 0 + ( yo + 1 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 1 ) * screen_width ] = c;
                    screen[ xo + 2 + ( yo + 1 ) * screen_width ] = c;
                    screen[ xo + 3 + ( yo + 1) * screen_width ] = c;
                    screen[ xo + 0 + ( yo + 2 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 2 ) * screen_width ] = c;
                    screen[ xo + 2 + ( yo + 2 ) * screen_width ] = c;
                    screen[ xo + 3 + ( yo + 2 ) * screen_width ] = c;
                    screen[ xo + 0 + ( yo + 3 ) * screen_width ] = c;
                    screen[ xo + 1 + ( yo + 3 ) * screen_width ] = c;
                    screen[ xo + 2 + ( yo + 3 ) * screen_width ] = c;
                    screen[ xo + 3 + ( yo + 3 ) * screen_width ] = c;
                }
            }
        }
    } else {
        for( int y = 0; y < h; ++y ) {
            for( int x = 0; x < w; ++x ) {
                for( int sy = 0; sy < scale; ++sy ) {
                    for( int sx = 0; sx < scale; ++sx ) {
                        int xo = xp + x * scale + sx;
                        int yo = yp + y * scale + sy;
                        if( xo >= 0 && yo >= 0 && xo < screen_width && yo < screen_height ) {
                            uint32_t c = pixels[ x + y * w ];
                            uint32_t a = c >> 24;
                            screen[ xo + yo * screen_width ] = imgedit_blend( screen[ xo + yo * screen_width ], c, a );
                        }
                    }
                }
            }
        }
    }
}


typedef struct imgedit_slider_t {
    bool active;
    int start_x;
    float start_value;
} imgedit_slider_t;


typedef struct imgedit_dropdown_t {
    bool active;
} imgedit_dropdown_t;

typedef struct imgedit_panel_t {
    bool show_processed;
    bool preview_changes;
    int scale;
    int scroll;
    process_settings_t settings;
    process_settings_t pending_settings;
    imgedit_slider_t sliders[ 8 ];
    process_settings_t undo[ 1024 ];
    int undo_index;
    int undo_max;
} imgedit_panel_t;


typedef enum imgedit_mode_t {
    IMGEDIT_MODE_IMAGES,
    IMGEDIT_MODE_FACES,
    IMGEDIT_MODE_SINGLE,

    IMGEDIT_MODECOUNT,
} imgedit_mode_t;


typedef struct imgedit_t {
    int resolution;

    array_param(cstr_t)* palettes;
    int selected_palette;
    paldither_palette_t* palette;
    size_t palette_size;

    array_param(imgedit_image_t)* images;
    array_param(imgedit_image_t)* faces;
    
    int screen_width;
    int screen_height;
    uint32_t* screen;

    int panel_height;
    imgedit_mode_t mode;
    imgedit_panel_t panels[ IMGEDIT_MODECOUNT ];
    imgedit_dropdown_t dropdowns[ 1 ];

    int first_image;
    int single_image;
    int single_face;

    bool converting_palette;
    bool cancel_last_image;
    thread_atomic_int_t exit_process_thread;
    thread_mutex_t mutex;
} imgedit_t;


void imgedit_save_settings( process_settings_t settings, char const* filename ) {
    ini_t* ini = ini_create( NULL );
    ini_property_add( ini, INI_GLOBAL_SECTION, "use_portrait_processor", -1, settings.use_portrait_processor ? "true" : "false", -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "bayer_dither", -1, settings.bayer_dither ? "true" : "false", -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "brightness", -1, cstr_float( settings.brightness ), -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "contrast", -1, cstr_float( settings.contrast ), -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "saturation", -1, cstr_float( settings.saturation ), -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "auto_contrast", -1, cstr_float( settings.auto_contrast ), -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "sharpen_radius", -1, cstr_float( settings.sharpen_radius ), -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "sharpen_strength", -1, cstr_float( settings.sharpen_strength ), -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "vignette_size", -1, cstr_float( settings.vignette_size ), -1 );
    ini_property_add( ini, INI_GLOBAL_SECTION, "vignette_opacity", -1, cstr_float( settings.vignette_opacity ), -1 );
    static char data[ 512 ];
    int size = ini_save( ini, data, (int) sizeof( data ) );
    ini_destroy( ini );
    file_save_data( data, size, filename, FILE_MODE_TEXT );
}


int imgedit_process_thread( void* user_data ) {
    imgedit_t* imgedit = (imgedit_t*) user_data;
    thread_timer_t timer;
    thread_timer_init( &timer );
    thread_mutex_lock( &imgedit->mutex );
    int images_count = array_count( imgedit->images );
    int images_capacity = images_count;
    imgedit_image_t* images = (imgedit_image_t*) malloc( sizeof( imgedit_image_t ) * images_count );
    int faces_count = array_count( imgedit->faces );
    int faces_capacity = faces_count;
    imgedit_image_t* faces = (imgedit_image_t*) malloc( sizeof( imgedit_image_t ) * faces_count );
    int selected_palette = imgedit->selected_palette;
    paldither_palette_t* palette = selected_palette == 0 || selected_palette == 1 ? NULL : (paldither_palette_t*) malloc( imgedit->palette_size );
    if( selected_palette > 1 ) memcpy( palette, imgedit->palette, imgedit->palette_size );
    float scale_factors[] = { 1.0f, 1.25f, 1.5f, 2.0f, 4.5f };
    float resolution_scale = scale_factors[ imgedit->resolution ];
    while( thread_atomic_int_load( &imgedit->exit_process_thread ) == 0 ) {
        bool show_processed = imgedit->panels[ imgedit->mode ].show_processed;
        bool mode_faces = imgedit->mode == IMGEDIT_MODE_FACES;
        int first_image = imgedit->first_image;
        int single_image = imgedit->single_image;
        int single_face = imgedit->single_face;

        images_count = array_count( imgedit->images );
        if( images_count > images_capacity ) {
            images = (imgedit_image_t*) realloc( images, sizeof( imgedit_image_t ) * images_count );
        }
        memcpy( images, array_item( imgedit->images, 0 ), sizeof( imgedit_image_t ) * images_count );
        faces_count = array_count( imgedit->faces );
        if( faces_count > faces_capacity ) {
            faces = (imgedit_image_t*) realloc( faces, sizeof( imgedit_image_t ) * faces_count );
        }
        memcpy( faces, array_item( imgedit->faces, 0 ), sizeof( imgedit_image_t ) * faces_count );
        
        int new_selected_palette = imgedit->selected_palette;
        if( selected_palette != new_selected_palette ) {
            selected_palette = new_selected_palette;
            cstr_t* name = array_item( imgedit->palettes, selected_palette );
            cstr_t palette_filename = cstr_cat( "palettes/", *name );
            imgedit->converting_palette = true;

            thread_mutex_unlock( &imgedit->mutex );
            thread_yield();
            
            size_t palette_size = 0;
            paldither_palette_t* new_palette = selected_palette == 0 || selected_palette == 1 ? NULL : convert_palette( palette_filename, &palette_size );
            

            thread_mutex_lock( &imgedit->mutex );            
            imgedit->converting_palette = false;
            if( imgedit->palette ) {
                paldither_palette_destroy( imgedit->palette, NULL );
            }
            imgedit->palette = new_palette;
            imgedit->palette_size = palette_size;
            if( new_palette ) {
                palette = (paldither_palette_t*) malloc( imgedit->palette_size );
                memcpy( palette, imgedit->palette, imgedit->palette_size );
            } else {
                palette = NULL;
            }
            continue;
        }


        process_settings_t settings[ IMGEDIT_MODECOUNT ];
        for( int i = 0; i < IMGEDIT_MODECOUNT; ++i ) {
            settings[ i ] = imgedit->panels[ i ].settings;
        }   

        imgedit->cancel_last_image = false;
        thread_mutex_unlock( &imgedit->mutex );
        thread_yield();

        bool is_image = !mode_faces;
        imgedit_image_t* imglist = mode_faces ? faces : images;
        int imglist_count = mode_faces ? faces_count : images_count;

        bool found = false;
        imgedit_image_t image = { NULL };
        if( single_image >= 0 ) {
            if( !imglist[ single_image ].orig_pixels || !imglist[ single_image ].sized_pixels || !imglist[ single_image ].processed ) {
                image = imglist[ single_image ];
                found = true;
            }
        } else if( single_face >= 0 ) {
            if( !imglist[ single_face ].orig_pixels || !imglist[ single_face ].sized_pixels || !imglist[ single_face ].processed ) {
                image = imglist[ single_face ];
                found = true;
            }
        } else {       
            for( int i = 0; i < imglist_count; ++i ) {
                int idx = ( i + first_image ) % imglist_count;
                if( show_processed ) {
                    if( !imglist[ idx ].orig_pixels || !imglist[ idx ].processed ) {
                        image = imglist[ idx ];
                        found = true;
                        break;
                    }
                } else {
                    if( !imglist[ idx ].orig_pixels || !imglist[ idx ].sized_pixels ) {
                        image = imglist[ idx ];
                        found = true;
                        break;
                    }
                }
            }

            if( !found ) {
                for( int i = 0; i < imglist_count; ++i ) {
                    int idx = ( i + first_image ) % imglist_count;
                    if( !show_processed ) {
                        if( !imglist[ idx ].orig_pixels || !imglist[ idx ].processed ) {
                            image = imglist[ idx ];
                            found = true;
                            break;
                        }
                    } else {
                        if( !imglist[ idx ].orig_pixels || !imglist[ idx ].sized_pixels ) {
                            image = imglist[ idx ];
                            found = true;
                            break;
                        }
                    }
                }
            }

            if( !found ) {
                is_image = mode_faces;
                imglist = !mode_faces ? faces : images;
                imglist_count = !mode_faces ? faces_count : images_count;

                for( int i = 0; i < imglist_count; ++i ) {
                    int idx = ( i + first_image ) % imglist_count;
                    if( !imglist[ idx ].orig_pixels || !imglist[ idx ].sized_pixels || !imglist[ idx ].processed ) {
                        image = imglist[ idx ];
                        found = true;
                        break;
                    }
                }
            }
        }

        if( found ) {
            if( !image.orig_pixels ) {
                imgedit_load_img( &image );
            }
            if( ( !show_processed || image.processed ) && !image.sized_pixels ) {
                imgedit_resize( &image );
            } else if( !image.processed ) {
                int outw =image.sized_width;
                int outh =image.sized_height;
                uint8_t* pixels = (uint8_t*) malloc( 2 * outw * outh );

                uint32_t* img = (uint32_t*) malloc( sizeof( uint32_t ) * image.orig_width *image.orig_height );
                memcpy( img, image.orig_pixels, sizeof( uint32_t ) * image.orig_width *image.orig_height );
                if( is_image ) {
                    if( settings[ single_image >= 0 ? IMGEDIT_MODE_SINGLE : IMGEDIT_MODE_IMAGES ].use_portrait_processor ) {
                        process_face( img, image.orig_width, image.orig_height, pixels, outw, outh, palette, image.use_individual_settings ? &image.settings : &settings[ IMGEDIT_MODE_IMAGES ] );
                    } else {
                        process_image( img, image.orig_width, image.orig_height, pixels, outw, outh, palette, image.use_individual_settings ? &image.settings : &settings[ IMGEDIT_MODE_IMAGES ], resolution_scale );
                    }
                } else {
                    if( settings[ single_face >= -0 ? IMGEDIT_MODE_SINGLE :IMGEDIT_MODE_FACES ].use_portrait_processor ) {
                        process_face( img, image.orig_width, image.orig_height, pixels, outw, outh, palette, image.use_individual_settings ? &image.settings : &settings[ IMGEDIT_MODE_FACES ] );
                    } else {
                        process_image( img, image.orig_width, image.orig_height, pixels, outw, outh, palette, image.use_individual_settings ? &image.settings : &settings[ IMGEDIT_MODE_FACES ], resolution_scale );
                    }
                }

                if( selected_palette == 1 ) {
                    dither_rgb9( img, outw, outh, settings[ single_image >= 0 || single_face >= 0 ? IMGEDIT_MODE_SINGLE : is_image ? IMGEDIT_MODE_IMAGES : IMGEDIT_MODE_FACES ].bayer_dither );
                }

                image.processed = (uint32_t*) malloc( sizeof( uint32_t ) * outw * outh ); 
                if( palette ) {
                    for( int y = 0; y < outh; ++y ) {
                        for( int x = 0; x < outw; ++x ) {
                            uint32_t c = img[ x + outw * y ] & 0xff000000;
                            c = c | ( palette->colortable[ pixels[ x + outw * y ] ] & 0x00ffffff );
                            image.processed[ x + outw * y ] = c;
                        }
                    }
                } else {
                    for( int y = 0; y < outh; ++y ) {
                        for( int x = 0; x < outw; ++x ) {
                            image.processed[ x + outw * y ] = img[ x + outw * y ];
                        }
                    }
                }
                free( img );
                free( pixels );
            }
        } else {
            thread_timer_wait( &timer, 1000000000 );
            thread_mutex_lock( &imgedit->mutex );
            continue;
        }


        thread_timer_wait( &timer, 1000000 );
        thread_mutex_lock( &imgedit->mutex );

        array(imgedit_image_t)* list = ARRAY_CAST( imgedit->faces );
        if( is_image ) {
            list = ARRAY_CAST( imgedit->images );
        }
        if( single_image >= 0 ) {
            image.prev_processed = list->items[ single_image ].prev_processed;
            list->items[ single_image ] = image;
        } else if( single_face >= 0 ) {
            image.prev_processed = list->items[ single_face ].prev_processed;
            list->items[ single_face ] = image;
        } else {
            for( int i = 0; i < list->count; ++i ) {
                if( cstr_is_equal( list->items[ i ].filename, image.filename ) ) {
                    if( list->items[ i ].processed == NULL && image.processed != NULL && imgedit->cancel_last_image ) {
                        free( image.processed );
                        image.processed = NULL;
                        break;
                    }
                    image.prev_processed = list->items[ i ].prev_processed;
                    list->items[ i ] = image;
                    break;
                }
            }
        }
    }
    thread_mutex_unlock( &imgedit->mutex );
    thread_timer_term( &timer );
    free( palette );
    free( images );
    free( faces );
    return 0;
}


void imgedit_invalidate_image( imgedit_image_t* image ) {
    if( image->processed ) {
        if( image->prev_processed ) {
            free( image->prev_processed );
            image->prev_processed = NULL;
        }
        image->prev_processed = image->processed;
    }
    image->processed = NULL;
}


void imgedit_invalidate( imgedit_t* imgedit, imgedit_mode_t mode ) {
    array(imgedit_image_t)* imglist = ARRAY_CAST( imgedit->images );
    if( mode == IMGEDIT_MODE_FACES ) {
        imglist = ARRAY_CAST( imgedit->faces );
    }

    for( int i = 0; i < imglist->count; ++i ) {
        imgedit_image_t* image = &imglist->items[ i ];
        if( image->processed ) {
            if( image->prev_processed ) {
                free( image->prev_processed );
                image->prev_processed = NULL;
            }
            image->prev_processed = image->processed;
        }
        image->processed = NULL;
    }

    imgedit->cancel_last_image = true;
}


void imgedit_hline( imgedit_t* imgedit, int x, int y, int len, uint32_t color ) {
	if( y < 0 || y >= imgedit->screen_height ) {
        return;
    }
	if( x < 0 ) { 
        len += x; 
        x = 0; 
    }
	if( x + len >= imgedit->screen_width ) {
        len = imgedit->screen_width - x - 1;
    }
	uint32_t* scr = imgedit->screen + y * imgedit->screen_width + x;
	uint32_t* end = scr + len;
	while( scr <= end ) {
        *scr = color;
        ++scr;
    }
}


void imgedit_text( imgedit_t* imgedit, int x, int y, char const* text, uint32_t color ) {
    sysfont_9x16_u32( imgedit->screen, imgedit->screen_width, imgedit->screen_height, x, y, text, color );
}


void imgedit_line( imgedit_t* imgedit, int x1, int y1, int x2, int y2, uint32_t color ) {
	int dx = x2 - x1;
	dx = dx < 0 ? -dx : dx;
	int sx = x1 < x2 ? 1 : -1;
	int dy = y2 - y1;
	dy = dy < 0 ? -dy : dy;
	int sy = y1 < y2 ? 1 : -1; 
	int err = ( dx > dy ? dx : -dy ) / 2;	 
	int x = x1;
	int y = y1;
	while( x != x2 || y != y2 ) {
        if( x >= 0 && x < imgedit->screen_width && y >= 0 && y < imgedit->screen_height ) {
		    imgedit->screen[ x + y * imgedit->screen_width ] = color;
        }
		int e2 = err;
		if( e2 > -dx ) { 
            err -= dy; 
            x += sx; 
        }
		if( e2 < dy ) { 
            err += dx; 
            y += sy; 
        }
	}
    if( x >= 0 && x < imgedit->screen_width && y >= 0 && y < imgedit->screen_height ) {
		imgedit->screen[ x + y * imgedit->screen_width ] = color;
    }
}


void imgedit_box( imgedit_t* imgedit, int x, int y, int w, int h, uint32_t color ) {
    if( y < 0 ) { 
        h += y;
        y = 0;
    }
    if( y + h >= imgedit->screen_height ) {
        h = imgedit->screen_height - y;
    }
	for( int i = y; i < y + h; ++i ) {
		imgedit_hline( imgedit, x, i, w, color );
	}
}


void imgedit_rect( imgedit_t* imgedit, int x, int y, int w, int h, uint32_t color ) {
	imgedit_hline( imgedit, x, y, w, color );
	imgedit_hline( imgedit, x, y + h, w, color );
	imgedit_line( imgedit, x, y, x, y + h, color  );
	imgedit_line( imgedit, x + w, y, x + w, y + h, color  );
}


void imgedit_putpixel( imgedit_t* imgedit, int x, int y, uint32_t color ) {
    if( x >= 0 && x < imgedit->screen_width && y >= 0 && y < imgedit->screen_height ) {
		imgedit->screen[ x + y * imgedit->screen_width ] = color;
    }
}


void imgedit_circle( imgedit_t* imgedit, int x, int y, int r, uint32_t color ) {
	int f = 1 - r;
	int dx = 0;
	int dy = -2 * r;
	int ix = 0;
	int iy = r;
 
	imgedit_putpixel( imgedit, x, y + r, color );
	imgedit_putpixel( imgedit, x, y - r, color );
	imgedit_putpixel( imgedit, x + r, y, color );
	imgedit_putpixel( imgedit, x - r, y, color );
 
	while( ix < iy )  {
		if( f >= 0 ) {
			--iy;
			dy += 2;
			f += dy;
		}
		++ix;
		dx += 2;
		f += dx + 1;    

		imgedit_putpixel( imgedit, x + ix, y + iy, color );
		imgedit_putpixel( imgedit, x - ix, y + iy, color );
		imgedit_putpixel( imgedit, x + ix, y - iy, color );
		imgedit_putpixel( imgedit, x - ix, y - iy, color );
		imgedit_putpixel( imgedit, x + iy, y + ix, color );
		imgedit_putpixel( imgedit, x - iy, y + ix, color );
		imgedit_putpixel( imgedit, x + iy, y - ix, color );
		imgedit_putpixel( imgedit, x - iy, y - ix, color );
	}
}


void imgedit_disc( imgedit_t* imgedit, int x, int y, int r, uint32_t color ) {       
	int f = 1 - r;
	int dx = 0;
	int dy = -2 * r;
	int ix = 0;
	int iy = r;
 
	while( ix <= iy )  {
		imgedit_hline( imgedit, x - iy, y + ix, 2 * iy, color );
		imgedit_hline( imgedit, x - iy, y - ix, 2 * iy, color );
		if( f >= 0 ) {
			imgedit_hline( imgedit, x - ix, y + iy, 2 * ix, color );
			imgedit_hline( imgedit, x - ix, y - iy, 2 * ix, color );

			--iy;
			dy += 2;
			f += dy;
		}
		++ix;
		dx += 2;
		f += dx + 1;    
	}
}

typedef struct imgedit_input_t {
    bool clicked;
    int mouse_x;
    int mouse_y;
    bool lbutton_pressed;
    bool lbutton;
    int lbutton_x;
    int lbutton_y;
    float scroll_delta;
    bool esc;
} imgedit_input_t;


void imgedit_images( imgedit_t* imgedit, imgedit_input_t const* input ) {
    float scroll_delta = input->scroll_delta;

    array(imgedit_image_t)* imglist = ARRAY_CAST( imgedit->images );
    if( imgedit->mode == IMGEDIT_MODE_FACES ) {
        imglist = ARRAY_CAST( imgedit->faces );
    }

    float scale_factors[] = { 1.0f, 1.25f, 1.5f, 2.0f, 4.5f };
    float resolution_scale = scale_factors[ imgedit->resolution ];
    int w = (int)( ( imgedit->mode == IMGEDIT_MODE_FACES ? 112 : 192 ) * resolution_scale );
    int h = (int)( ( imgedit->mode == IMGEDIT_MODE_FACES ? 112 : 128 ) * resolution_scale );

    imgedit->panels[ imgedit->mode ].scroll += (int)( scroll_delta * 100.0f );
    int hcount = imgedit->screen_width / ( ( w + 1 ) * imgedit->panels[ imgedit->mode ].scale );
    hcount = hcount <= 0 ? 1 : hcount; 
    int vcount = ( imglist->count + hcount - 1  ) / hcount;
    int max_scroll = vcount * ( ( h + 1 )* imgedit->panels[ imgedit->mode ].scale ) - ( imgedit->screen_height - imgedit->panel_height ) + 8;
    imgedit->panels[ imgedit->mode ].scroll = imgedit->panels[ imgedit->mode ].scroll > 0 ? imgedit->panels[ imgedit->mode ].scroll : 0;
    imgedit->panels[ imgedit->mode ].scroll = imgedit->panels[ imgedit->mode ].scroll <= max_scroll ? imgedit->panels[ imgedit->mode ].scroll : max_scroll;

    int screen_width = imgedit->screen_width;
    int screen_height = imgedit->screen_height - imgedit->panel_height;
    uint32_t* screen = imgedit->screen + screen_width * imgedit->panel_height;

    for( int y = 0; y < screen_height; ++y ) {
        for( int x = 0; x < screen_width; ++x ) {
            int c = ( ( x / ( 10 * imgedit->panels[ imgedit->mode ].scale ) ) + ( ( y - imgedit->panels[ imgedit->mode ].scroll + max_scroll ) / ( 10 * imgedit->panels[ imgedit->mode ].scale ) ) ) % 2;
            screen[ x + y * screen_width ] = 0x505050 + c * 0x101010;
        }
    }

    int xp = 0;
    int yp = -imgedit->panels[ imgedit->mode ].scroll;
    int first_image = -1;
    for( int i = 0; i < imglist->count; ++i ) {
        imgedit_image_t* image = &imglist->items[ i ];
        if( xp + image->sized_width * imgedit->panels[ imgedit->mode ].scale >= screen_width ) {
            xp = 0;
            yp += ( image->sized_height + 1 ) * imgedit->panels[ imgedit->mode ].scale;
            if( yp >= screen_height ) {
                break;
            }
        }      
        if( yp + image->sized_height * imgedit->panels[ imgedit->mode ].scale >= 0 ) {
            if( first_image == -1 ) {
                first_image = i;
            }
            if( imgedit->panels[ imgedit->mode ].show_processed ) {
                if( image->processed ) {
                    imgedit_blit( image->processed, image->sized_width, image->sized_height, xp, yp, imgedit->panels[ imgedit->mode ].scale, screen, screen_width, screen_height );
                } else if( image->prev_processed ) {
                    imgedit_blit( image->prev_processed, image->sized_width, image->sized_height, xp, yp, imgedit->panels[ imgedit->mode ].scale, screen, screen_width, screen_height );
                }
            } else {
                if( image->sized_pixels ) {
                    imgedit_blit( image->sized_pixels, image->sized_width, image->sized_height, xp, yp, imgedit->panels[ imgedit->mode ].scale, screen, screen_width, screen_height );
                }
            }

            if( image->use_individual_settings ) {
                imgedit_rect( imgedit, xp, ( yp + imgedit->panel_height ), image->sized_width * imgedit->panels[ imgedit->mode ].scale - 1, image->sized_height * imgedit->panels[ imgedit->mode ].scale - 1, 0xff00ff00 );
                imgedit_rect( imgedit, xp + 1, ( yp + imgedit->panel_height ) + 1, image->sized_width * imgedit->panels[ imgedit->mode ].scale - 3, image->sized_height * imgedit->panels[ imgedit->mode ].scale - 3, 0xff00ff00 );
            }

            int mouse_x = input->mouse_x;
            int mouse_y = input->mouse_y;
            if( mouse_x >= xp && mouse_y >= ( yp + imgedit->panel_height ) && mouse_y > imgedit->panel_height && mouse_x < xp + image->sized_width * imgedit->panels[ imgedit->mode ].scale && mouse_y < ( yp + imgedit->panel_height ) + image->sized_height * imgedit->panels[ imgedit->mode ].scale ) {
                imgedit_rect( imgedit, xp, ( yp + imgedit->panel_height ), image->sized_width * imgedit->panels[ imgedit->mode ].scale - 1, image->sized_height * imgedit->panels[ imgedit->mode ].scale - 1, 0xff00ffff );
                imgedit_rect( imgedit, xp + 1, ( yp + imgedit->panel_height ) + 1, image->sized_width * imgedit->panels[ imgedit->mode ].scale - 3, image->sized_height * imgedit->panels[ imgedit->mode ].scale - 3, 0xff00ffff );
                for( int y = -1; y < 2; ++y ) {
                    for( int x = -1; x < 2; ++x ) {
                        imgedit_text( imgedit, xp + ( x == 0 ? 2 : x ) + ( image->sized_width * imgedit->panels[ imgedit->mode ].scale / 2 ) - ( ( strlen( image->name ) * 9 ) / 2 ), ( yp + imgedit->panel_height ) + ( y == 0 ? 2 : y )  + 2, image->name, 0xff000000 );
                    }
                }
                imgedit_text( imgedit, xp + 0 + ( image->sized_width * imgedit->panels[ imgedit->mode ].scale / 2 ) - ( ( strlen( image->name ) * 9 ) / 2 ), ( yp + imgedit->panel_height ) + 0 + 2, image->name, 0xff00ffff );
                if( input->clicked ) {
                    if( imgedit->mode == IMGEDIT_MODE_IMAGES ) {
                        imgedit->single_image = i;
                        imgedit->panels[ IMGEDIT_MODE_SINGLE ].settings = imgedit->panels[ IMGEDIT_MODE_IMAGES ].settings;
                    } else if( imgedit->mode == IMGEDIT_MODE_FACES ) {
                        imgedit->single_face = i;
                        imgedit->panels[ IMGEDIT_MODE_SINGLE ].settings = imgedit->panels[ IMGEDIT_MODE_FACES ].settings;
                    }
                    if( image->use_individual_settings ) {
                        imgedit->panels[ IMGEDIT_MODE_SINGLE ].settings = image->settings;
                    }
                    imgedit->panels[ IMGEDIT_MODE_SINGLE ].pending_settings = imgedit->panels[ IMGEDIT_MODE_SINGLE ].settings;
                }
            }
        }
        xp += ( image->sized_width + 1 ) * imgedit->panels[ imgedit->mode ].scale;
    }

    imgedit->first_image = first_image < 0 ? 0 : first_image;
}


void imgedit_single_image( imgedit_t* imgedit, imgedit_input_t const* input ) {
    if( input->esc ) {
        imgedit->single_image = -1;
        imgedit->single_face = -1;
        return;
    }

    int img_index = imgedit->single_image;
    array(imgedit_image_t)* imglist = ARRAY_CAST( imgedit->images );
    if( imgedit->single_face >= 0 ) {
        img_index = imgedit->single_face;
        imglist = ARRAY_CAST( imgedit->faces );
    }

    int screen_width = imgedit->screen_width;
    int screen_height = imgedit->screen_height - imgedit->panel_height;
    uint32_t* screen = imgedit->screen + screen_width * imgedit->panel_height;

    for( int y = 0; y < screen_height; ++y ) {
        for( int x = 0; x < screen_width; ++x ) {
            int c = ( ( x / ( 10 * imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale ) ) + ( ( y ) / ( 10 * imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale ) ) ) % 2;
            screen[ x + y * screen_width ] = 0x202020 + c * 0x080808;
        }
    }

    imgedit_image_t* image = &imglist->items[ img_index ];
    int xp = ( screen_width - image->sized_width * imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale ) / 2; 
    int yp = 20;
    if( imgedit->panels[ IMGEDIT_MODE_SINGLE ].show_processed ) {
        if( image->processed ) {
            imgedit_blit( image->processed, image->sized_width, image->sized_height, xp, yp, imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale, screen, screen_width, screen_height );
        } else if( image->prev_processed ) {
            imgedit_blit( image->prev_processed, image->sized_width, image->sized_height, xp, yp, imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale, screen, screen_width, screen_height );
        }
    } else {
        if( image->sized_pixels ) {
            imgedit_blit( image->sized_pixels, image->sized_width, image->sized_height, xp, yp, imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale, screen, screen_width, screen_height );
        }
    }
    
    for( int y = -1; y < 2; ++y ) {
        for( int x = -1; x < 2; ++x ) {
            imgedit_text( imgedit, xp + ( x == 0 ? 2 : x ) + ( image->sized_width * imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale / 2 ) - ( ( strlen( image->filename ) * 9 ) / 2 ), ( yp + imgedit->panel_height ) + image->sized_height * imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale + 10 + ( y == 0 ? 2 : y )  + 2, image->filename, 0xff000000 );
        }
    }
    imgedit_text( imgedit, xp + 0 + ( image->sized_width * imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale / 2 ) - ( ( strlen( image->filename ) * 9 ) / 2 ), ( yp + imgedit->panel_height ) + image->sized_height * imgedit->panels[ IMGEDIT_MODE_SINGLE ].scale + 10 + 2, image->filename, 0xffffffff );

    uint32_t col = 0xffffffff;
    if( input->mouse_x >= xp - 50 && input->mouse_y >= imgedit->panel_height + 10 && input->mouse_x <= xp && input->mouse_y <= imgedit->panel_height + 50 ) {
        col = 0xff00ffff;
        if( input->clicked ) {
            imgedit->single_image = -1;
            imgedit->single_face = -1;
            return;
        }
    }

    imgedit_line( imgedit, xp - 50 + 10, 20 + imgedit->panel_height, xp - 50 + 30, 40 + imgedit->panel_height, col );
    imgedit_line( imgedit, xp - 50 + 11, 20 + imgedit->panel_height, xp - 50 + 31, 40 + imgedit->panel_height, col );
    imgedit_line( imgedit, xp - 50 + 30, 20 + imgedit->panel_height, xp - 50 + 10, 40 + imgedit->panel_height, col );
    imgedit_line( imgedit, xp - 50 + 31, 20 + imgedit->panel_height, xp - 50 + 11, 40 + imgedit->panel_height, col );
}


bool imgedit_button( imgedit_t* imgedit, int x, int y, char const* text, bool enabled, imgedit_input_t* input ) {
    bool result = false;
    int width = 19 + (int) strlen( text ) * 9;
    int height = 24;

    bool hover = false;
    if( enabled ) {
        if( input->mouse_x >= x && input->mouse_x <= x + width && input->mouse_y >= y && input->mouse_y <= y + height ) {
            hover = true;
            if( input->lbutton ) {
                if( !( input->lbutton_x >= x && input->lbutton_x <= x + width && input->lbutton_y >= y && input->lbutton_y <= y + height  ) ) {
                    hover = false;
                }
            }
        }
    }

    uint32_t color = hover ? 0xff00ffff : enabled ? 0xffffffff : 0xffa0a0a0;
    imgedit_rect( imgedit, x, y, width, height, color );
    if( hover && input->lbutton ) {
        imgedit_rect( imgedit, x + 1, y + 1, width - 2, height - 2, color );
    }
    imgedit_text( imgedit, x + 10, y + 4, text, color );

    if( hover && input->clicked ) {
        result = true;
    }

    return result;
}


bool imgedit_checkbox( imgedit_t* imgedit, int x, int y, char const* text, bool checked, imgedit_input_t* input ) {
    int width = 20 + (int) strlen( text ) * 9;
    int height = 18;

    bool hover = false;
    if( input->mouse_x >= x && input->mouse_x <= x + width && input->mouse_y >= y - 2 && input->mouse_y <= y + height - 2 ) {
        hover = true;
        if( input->clicked ) {
            checked = !checked;
        }
    }

    uint32_t color = hover ? 0xff00ffff : 0xffffffff;
    imgedit_rect( imgedit, x, y, 12, 12, color );
    imgedit_text( imgedit, x + 20, y, text, color );
    if( checked ) {
        imgedit_line( imgedit, x + 2, y + 2, x - 2 + 12, y - 2 + 12, color );
        imgedit_line( imgedit, x - 2 + 12, y + 2, x + 2, y - 2 + 12, color );
    }
    return checked;
}


int imgedit_radiobuttons( imgedit_t* imgedit, int x, int y, char const* items[], int count, int selected, imgedit_input_t* input ) {
    for( int i = 0; i < count; ++i ) {
        int width = 20 + (int) strlen( items[ i ] ) * 9;
        int height = 20;

        bool hover = false;
        if( input->mouse_x >= x && input->mouse_x <= x + width && input->mouse_y >= y - 2 && input->mouse_y <= y + height - 2 ) {
            hover = true;
            if( input->clicked ) {
                selected = i;
            }
        }

        uint32_t color = hover ? 0xff00ffff : 0xffffffff;
        imgedit_circle( imgedit, x + 6, y + 6, 7, color );
        imgedit_text( imgedit, x + 20, y, items[ i ], color );
        if( selected == i ) {
            imgedit_disc( imgedit, x + 6, y + 6, 4, color );
        }
        y += height;
    }
    return selected;
}


float imgedit_slider( imgedit_t* imgedit, int x, int y, int width, char const* label, imgedit_slider_t* state, float value, imgedit_input_t* input ) {
    int len = (int) strlen( label );
    
    bool hover = false;
    if( state->active ) {
        float dx = ( input->mouse_x - state->start_x ) / (float)width;
        value = state->start_value + dx;
        value = value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;

        if( !input->lbutton ) {
            state->active = false;
        }    
    } else {
        int px = x + (int)( width * value );
        int py = y + 7;
        int r = 10;
        if( input->mouse_x >= px - r && input->mouse_x <= px + r && input->mouse_y >= py - r && input->mouse_y <= py + r ) {
            hover = true;
            if( input->lbutton_pressed ) {
                state->active = true;
                state->start_value = value;
                state->start_x = input->lbutton_x;
            }
        }
    }

    uint32_t color = state->active ? 0xffffff00 : hover ? 0xff00ffff : 0xffffffff;
    imgedit_line( imgedit, x, y + 7, x + width, y + 7, color );
    imgedit_disc( imgedit, x + (int)( width * value ), y + 7, 5, color );
    imgedit_text( imgedit, x - ( len * 9 + 10 ), y, label, color );
    return value;
}


int imgedit_tabs( imgedit_t* imgedit, int x, int y, int w, int h, char const* items[], int count, int selected, imgedit_input_t* input ) {
    imgedit_box( imgedit, x, y, w, h, 0xff303030 );
    imgedit_line( imgedit, x, y + h, x + w, y + h, 0xffffffff );
    x += 20;
    for( int i = 0; i < count; ++i ) {
        int width = 20 + (int) strlen( items[ i ] ) * 9 + 20;
        int height = 22;

        bool hover = false;
        if( input->mouse_x >= x && input->mouse_x <= x + width && input->mouse_y >= y - 2 && input->mouse_y <= y + height + 2 ) {
            hover = true;
            if( input->clicked ) {
                selected = i;
            }
        }

        uint32_t color = hover ? 0xff00ffff : ( i == selected ? 0xffffffff : 0xffa0a0a0 );
        imgedit_rect( imgedit, x, y + h - height, width - 1, height -( i == selected ? 0 : 1 ), i == selected ? 0xffffffff : 0xffa0a0a0);
        imgedit_box( imgedit, x + 1, y + h - height + 1, width - 3, height - ( i == selected ? 0 : 1 ), i == selected ? 0xff706860 : 0xff505050 );
        imgedit_text( imgedit, x + 20, y + h - height + 3, items[ i ], color );
        x += width;
    }
    return selected;
}


int imgedit_dropdown( imgedit_t* imgedit, int x, int y, char const* items[], int count, imgedit_dropdown_t* state, int selected, bool enabled, imgedit_input_t* input ) {
    int maxlen = 0;
    for( int i = 0; i < count; ++i ) {
        int len = (int) strlen( items[ i ] );
        maxlen = len > maxlen ? len : maxlen;
    }

    int maxlines = ( imgedit->screen_height - y - 22 - 6 ) / 16;
    int columns = ( count + maxlines - 1 ) / maxlines;
    int lines = ( count + columns - 1 ) / columns;

    int width = 20 + maxlen * 9 + 20;
    int height = 20;

    bool hover = false;
    if( enabled && !state->active ) {
        if( input->mouse_x >= x && input->mouse_x <= x + width && input->mouse_y >= y - 2 && input->mouse_y <= y + height - 2 ) {
            hover = true;
        }
    }

    uint32_t color = hover ? 0xff00ffff : 0xffffffff;
    imgedit_box( imgedit, x, y, width, height, 0xff505050 );
    imgedit_rect( imgedit, x, y, width, height, enabled ? 0xffc0c0c0 : 0xff909090 );
    imgedit_text( imgedit, x + 10, y + 2, items[ selected ], enabled ? color : 0xffa0a0a0 );
    imgedit_rect( imgedit, x + width - 20, y, 20, height, enabled ? 0xffc0c0c0 : 0xff909090 );
    imgedit_line( imgedit, x + width - 20 + 4, y + 8, x + width - 20 + 10, 16, enabled ? color : 0xffa0a0a0 );
    imgedit_line( imgedit, x + width - 20 + 16, y + 8, x + width - 20 + 10, 16, enabled ? color : 0xffa0a0a0 );
    if( enabled && state->active ) {
        imgedit_box( imgedit, x, y + 20, width + ( columns - 1 ) * ( maxlen * 9 + 20 ), 16 * lines + 6, 0xff505050 );
        imgedit_rect( imgedit, x, y + 20, width + ( columns - 1 ) * ( maxlen * 9 + 20 ), 16 * lines + 6, 0xffc0c0c0 );
        int line = 0;
        int xp = x;
        int yp = y;
        for( int i = 0; i < count; ++i ) {
            bool item_hover = false;
            if( input->mouse_x >= xp + 5 && input->mouse_x <= xp + maxlen * 9 + 10 && input->mouse_y >= yp + 22 && input->mouse_y <= yp + 22 + 15 ) {
                item_hover = true;
                if( input->clicked ) {
                    selected = i;
                }
            }
            if( item_hover ) {
                imgedit_box( imgedit, xp + 5, yp + 22, maxlen * 9 + 10, 15, 0xff707070 );
            }
            imgedit_text( imgedit, xp + 10, yp + 22, items[ i ], item_hover ? 0xff00ffff : 0xffffffff );
            yp += 16;
            ++line;
            if( line >= lines ) {
                line = 0;
                yp = y;
                xp += maxlen * 9 + 20;
            }
        }

    }
    if( enabled ) {
        if( state->active ) {
            if( input->clicked ) {
                state->active = false;
            }
        } else if( hover ) {
            if( input->clicked ) {
                state->active = true;
            }
        }
    }
        
    return selected;
}


void imgedit_add_undo_state( imgedit_panel_t* panel, process_settings_t state ) {
    if( panel->undo_index > 0 ) {
        process_settings_t undo = panel->undo[ panel->undo_index - 1 ];
        bool different = false;
        different |= state.use_portrait_processor != undo.use_portrait_processor;
        different |= state.bayer_dither != undo.bayer_dither;
        different |= state.brightness != undo.brightness;
        different |= state.contrast != undo.contrast;
        different |= state.saturation != undo.saturation;
        different |= state.auto_contrast != undo.auto_contrast;
        different |= state.sharpen_radius != undo.sharpen_radius;
        different |= state.sharpen_strength != undo.sharpen_strength;
        different |= state.vignette_size != undo.vignette_size;
        different |= state.vignette_opacity != undo.vignette_opacity;
        if( !different ) {
            return;
        }
    }

    if( panel->undo_index >= sizeof( panel->undo ) / sizeof( *panel->undo ) ) {
        memmove( panel->undo, panel->undo + 1, sizeof( panel->undo ) - sizeof( *panel->undo ) );
        --panel->undo_index;
    }
    panel->undo[ panel->undo_index++ ] = panel->settings;
    panel->undo_max = panel->undo_index;
}


void imgedit_undo( imgedit_t* imgedit ) {
    imgedit_panel_t* panel = &imgedit->panels[ imgedit->mode ];
    if( panel->undo_index > 0 ) {
        panel->pending_settings = panel->undo[ --panel->undo_index ];
        panel->settings = panel->pending_settings;
        imgedit_invalidate( imgedit, imgedit->mode );
    }
}


void imgedit_redo( imgedit_t* imgedit ) {
    imgedit_panel_t* panel = &imgedit->panels[ imgedit->mode ];
    if( panel->undo_index < panel->undo_max ) {
        panel->pending_settings = panel->undo[ panel->undo_index++ ];
        panel->settings = panel->pending_settings;
        imgedit_invalidate( imgedit, imgedit->mode );
    }
}

void imgedit_panel( imgedit_t* imgedit, imgedit_input_t* input ) {
    imgedit_panel_t* panel = &imgedit->panels[ imgedit->mode ];
    imgedit_image_t* image = NULL;
    if( imgedit->single_image >= 0 || imgedit->single_face >= 0 ) {
        panel = &imgedit->panels[ IMGEDIT_MODE_SINGLE ];
        array(imgedit_image_t)* imglist = ARRAY_CAST( imgedit->images );
        image = &imglist->items[ imgedit->single_image ];
        if( imgedit->single_face >= 0 ) {
            imglist = ARRAY_CAST( imgedit->faces );
            image = &imglist->items[ imgedit->single_face ];
        }
    }

    imgedit_input_t dropdown_input = *input;
    if( imgedit->dropdowns[ 0 ].active ) {
        input->mouse_x = 0;
        input->mouse_y = 0;
        input->clicked = false;
    }

    imgedit_box( imgedit, 0, 0, imgedit->screen_width, imgedit->panel_height, 0xff706860 );

    if( imgedit->single_image < 0 && imgedit->single_face < 0 ) {
        char const* tabs[] = { "Images", "Faces" };
        int prev_mode = imgedit->mode;
        imgedit->mode = (imgedit_mode_t) imgedit_tabs( imgedit, 0, 0, imgedit->screen_width, 26, tabs, sizeof( tabs ) / sizeof( *tabs ), imgedit->mode, input );
        if( imgedit->mode != prev_mode ) {
            imgedit->single_image = -1;
            imgedit->single_face = -1;
        }
    } else {
        char const* tabs[ 1 ];
        tabs[ 0 ] = image->name;
        imgedit_tabs( imgedit, 0, 0, imgedit->screen_width, 26, tabs, sizeof( tabs ) / sizeof( *tabs ), 0, input );
    }

    panel->show_processed = imgedit_checkbox( imgedit, 10, 40, "Show processed", panel->show_processed, input );     

    panel->pending_settings.use_portrait_processor = imgedit_checkbox( imgedit, 10, 60, "Use portrait processor", panel->pending_settings.use_portrait_processor, input ); 
    panel->pending_settings.bayer_dither = imgedit_checkbox( imgedit, 10, 80, "Use bayer dithering", panel->pending_settings.bayer_dither, input ); 
    
    panel->preview_changes = imgedit_checkbox( imgedit, 10, 100, "Auto-apply changes", panel->preview_changes, input ); 

    char const* scales[] = { "100%", "200%", "300%", "400%", };
    panel->scale = 1 + imgedit_radiobuttons( imgedit, 250, 40, scales, sizeof( scales ) / sizeof( *scales ), panel->scale - 1, input );

    panel->pending_settings.brightness = imgedit_slider( imgedit, 460, 40, 250, "Brightness", &panel->sliders[ 0 ], panel->pending_settings.brightness, input );
    panel->pending_settings.contrast = imgedit_slider( imgedit, 460, 60, 250, "Contrast", &panel->sliders[ 1 ], panel->pending_settings.contrast, input );
    panel->pending_settings.saturation = imgedit_slider( imgedit, 460, 80, 250, "Saturation", &panel->sliders[ 2 ], panel->pending_settings.saturation, input );
    panel->pending_settings.auto_contrast = imgedit_slider( imgedit, 460, 100, 250, "Auto Contrast", &panel->sliders[ 3 ], panel->pending_settings.auto_contrast, input );

    panel->pending_settings.sharpen_radius = imgedit_slider( imgedit, 900, 40, 250, "Sharpen Radius", &panel->sliders[ 4 ], panel->pending_settings.sharpen_radius, input );
    panel->pending_settings.sharpen_strength = imgedit_slider( imgedit, 900, 60, 250, "Sharpen Strength", &panel->sliders[ 5 ], panel->pending_settings.sharpen_strength, input );
    panel->pending_settings.vignette_size = imgedit_slider( imgedit, 900, 80, 250, "Vignette Scale", &panel->sliders[ 6 ], panel->pending_settings.vignette_size, input );
    panel->pending_settings.vignette_opacity = imgedit_slider( imgedit, 900, 100, 250, "Vignette Strength", &panel->sliders[ 7 ], panel->pending_settings.vignette_opacity, input );

    char const* resolutions[] = { "Retro", "Low", "Medium", "High", };
    imgedit->resolution = imgedit_radiobuttons( imgedit, 1180, 40, resolutions, sizeof( resolutions ) / sizeof( *resolutions ), imgedit->resolution, input );
   
    int new_selected_palette = imgedit->selected_palette;
    if( imgedit->single_image < 0 && imgedit->single_face < 0 ) {
        imgedit_text( imgedit, 230, 5, "Palette", 0xffffffff );
        int new_selected_palette = imgedit_dropdown( imgedit, 300, 2, array_item( imgedit->palettes, 0 ), array_count( imgedit->palettes ), &imgedit->dropdowns[ 0 ], imgedit->selected_palette, !imgedit->converting_palette, &dropdown_input );     
        if( imgedit->converting_palette ) {
            static int c = 0;
            ++c;
            char const* paltxt[] = { "Generating palettte LUT", "Generating palettte LUT.","Generating palettte LUT..","Generating palettte LUT..." };
            imgedit_text( imgedit, 540, 6, paltxt[ ( c / 8 ) % 4 ], 0xff40ff40 );
        }
    }

    bool apply = imgedit_button( imgedit, 10, 125, "Apply", !panel->preview_changes, input );

    if( imgedit_button( imgedit, 90, 125, "Reset", true, input ) ) {
        if( imgedit->single_image < 0 && imgedit->single_face < 0 ) {
            panel->pending_settings.use_portrait_processor = ( imgedit->mode == IMGEDIT_MODE_FACES );
            panel->pending_settings.bayer_dither = false;
            panel->pending_settings.brightness = 0.5f;
            panel->pending_settings.contrast = 0.5f;
            panel->pending_settings.saturation = 0.5f;
            panel->pending_settings.auto_contrast = ( imgedit->mode == IMGEDIT_MODE_FACES ) ? 0.0f : 1.0f;
            panel->pending_settings.sharpen_radius = ( imgedit->mode == IMGEDIT_MODE_FACES ) ? 0.0f : 0.15f;
            panel->pending_settings.sharpen_strength = ( imgedit->mode == IMGEDIT_MODE_FACES ) ? 0.0f : 1.0f;
            panel->pending_settings.vignette_size = 0.0f;
            panel->pending_settings.vignette_opacity = 0.0f;
        } else {
            image->use_individual_settings = false;
            panel->settings = imgedit->single_image >= 0 ? imgedit->panels[ IMGEDIT_MODE_IMAGES ].settings : imgedit->panels[ IMGEDIT_MODE_FACES ].settings;
            panel->pending_settings = panel->settings;
            cstr_t ini_filename = cstr_cat( image->filename, ".ini"  );
            delete_file( ini_filename );
            imgedit_invalidate_image( image );
        }
    }

    bool changes = false;
    changes |= panel->pending_settings.use_portrait_processor != panel->settings.use_portrait_processor;
    changes |= panel->pending_settings.bayer_dither != panel->settings.bayer_dither;
    changes |= panel->pending_settings.brightness != panel->settings.brightness;
    changes |= panel->pending_settings.contrast != panel->settings.contrast;
    changes |= panel->pending_settings.saturation != panel->settings.saturation;
    changes |= panel->pending_settings.auto_contrast != panel->settings.auto_contrast;
    changes |= panel->pending_settings.sharpen_radius != panel->settings.sharpen_radius;
    changes |= panel->pending_settings.sharpen_strength != panel->settings.sharpen_strength;
    changes |= panel->pending_settings.vignette_size != panel->settings.vignette_size;
    changes |= panel->pending_settings.vignette_opacity != panel->settings.vignette_opacity;
    if( changes && ( panel->preview_changes || apply ) ) {
        process_settings_t undo = panel->pending_settings;
        if( panel->sliders[ 0 ].active ) undo.brightness = panel->sliders[ 0 ].start_value;
        if( panel->sliders[ 1 ].active ) undo.contrast = panel->sliders[ 1 ].start_value;
        if( panel->sliders[ 2 ].active ) undo.saturation = panel->sliders[ 2 ].start_value;
        if( panel->sliders[ 3 ].active ) undo.auto_contrast = panel->sliders[ 3 ].start_value;
        if( panel->sliders[ 4 ].active ) undo.sharpen_radius = panel->sliders[ 4 ].start_value;
        if( panel->sliders[ 5 ].active ) undo.sharpen_strength = panel->sliders[ 5 ].start_value;
        if( panel->sliders[ 6 ].active ) undo.vignette_size = panel->sliders[ 6 ].start_value;
        if( panel->sliders[ 7 ].active ) undo.vignette_opacity = panel->sliders[ 7 ].start_value;
        imgedit_add_undo_state( panel, undo );
        panel->settings = panel->pending_settings;
        if( imgedit->single_image >= 0 || imgedit->single_face >= 0 ) {
            cstr_t ini_filename = cstr_cat( image->filename, ".ini"  );
            imgedit_save_settings( panel->settings, ini_filename );
            image->settings = panel->settings;
            image->use_individual_settings = true;
            imgedit_invalidate_image( image );
        } else {
            imgedit_invalidate( imgedit, imgedit->mode );
            imgedit_save_settings( panel->settings, imgedit->mode == IMGEDIT_MODE_IMAGES ? "images/settings.ini" : "faces/settings.ini" );
        }
    }
    if( new_selected_palette != imgedit->selected_palette ) {
        imgedit_invalidate( imgedit, IMGEDIT_MODE_IMAGES );
        imgedit_invalidate( imgedit, IMGEDIT_MODE_FACES );
        imgedit->selected_palette = new_selected_palette;   
    }
}


int imgedit_proc( app_t* app, void* user_data ) {       

    // Load imgedit state
    int ini_resolution = 1;
    cstr_t ini_palette = NULL;
    file_t* state_file = file_load( ".cache/imgedit.ini", FILE_MODE_TEXT, NULL );
    if( state_file ) {
        ini_t* state_ini = ini_load( state_file->data, NULL );
        if( state_ini ) {
            int prop_resolution = ini_find_property( state_ini, INI_GLOBAL_SECTION, "resolution", -1 );
            int prop_palette = ini_find_property( state_ini, INI_GLOBAL_SECTION, "palette", -1 );

            if( prop_resolution != INI_NOT_FOUND ) {
                ini_resolution = atoi( ini_property_value( state_ini, INI_GLOBAL_SECTION, prop_resolution ) );
            }

            if( prop_palette != INI_NOT_FOUND ) {
                ini_palette = cstr( ini_property_value( state_ini, INI_GLOBAL_SECTION, prop_palette ) );
            }
            ini_destroy( state_ini );
        }
        file_destroy( state_file );
    }

    file_t* version_file = file_load( ".cache/VERSION", FILE_MODE_TEXT, NULL );
    if( version_file ) {
        g_cache_version = atoi( (char const*) version_file->data );
        file_destroy( version_file );
    }

    app_interpolation( app, APP_INTERPOLATION_NONE );
    app_screenmode( app, APP_SCREENMODE_WINDOW );
    app_title( app, "Yarnspin Image Editor" );

    imgedit_t imgedit = { 0 };
    imgedit.single_image = -1;
    imgedit.single_face = -1;
    imgedit.resolution = ini_resolution;

    imgedit.palettes = array_create( cstr_t );
    cstr_t palette_none = cstr( "None (RGB mode)" );
    array_add( imgedit.palettes, &palette_none );
    cstr_t palette_none9 = cstr( "None (9-bit RGB)" );
    array_add( imgedit.palettes, &palette_none9 );
    imgedit_list_palettes( imgedit.palettes );

    float scale_factors[] = { 1.0f, 1.25f, 1.5f, 2.0f, 4.5f };
    float resolution_scale = scale_factors[ imgedit.resolution ];

    imgedit.images = array_create( imgedit_image_t );
    imgedit_list_images( imgedit.images, "images", resolution_scale );

    imgedit.faces = array_create( imgedit_image_t );
    imgedit_list_images( imgedit.faces, "faces", resolution_scale );

    if( array_count( imgedit.images ) == 0 && array_count( imgedit.faces ) == 0 ) {
        printf( "No image files found in folders 'images' or 'faces'\n" );
        return EXIT_FAILURE;
    }

    // Use first palette in list.. 
    imgedit.selected_palette = 0; 
    // ...unless the palette in the ini file is in the list
    for( int i = 0; i < array_count( imgedit.palettes ); ++i ) { 
        cstr_t filename = "";
        array_get( imgedit.palettes, i, &filename );
        if( cstr_is_equal( filename, ini_palette ) ) {
            imgedit.selected_palette = i;
            break;
        } 
    }
    cstr_t palette_filename;
    array_get( imgedit.palettes, imgedit.selected_palette, &palette_filename );

    imgedit.palette = imgedit.selected_palette == 0 || imgedit.selected_palette == 1 ? NULL : convert_palette( cstr_cat( "palettes/", palette_filename ), &imgedit.palette_size );

    imgedit.screen_width = app_window_width( app );
    imgedit.screen_height = app_window_height( app );
    imgedit.screen_width = imgedit.screen_width < 200 ? 200 : imgedit.screen_width;
    imgedit.screen = (uint32_t*) malloc( sizeof( uint32_t ) * imgedit.screen_width * imgedit.screen_height );

    imgedit.mode = IMGEDIT_MODE_IMAGES;
    for( int i = 0; i < IMGEDIT_MODECOUNT; ++i ) {
        imgedit.panels[ i ].show_processed = false;
        imgedit.panels[ i ].preview_changes = true;
        imgedit.panels[ i ].scale = i == IMGEDIT_MODE_SINGLE ? 3 : 2;
        imgedit.panels[ i ].scroll = 0;
        imgedit.panels[ i ].settings.use_portrait_processor = ( i == IMGEDIT_MODE_FACES );
        imgedit.panels[ i ].settings.brightness = 0.5f;
        imgedit.panels[ i ].settings.contrast = 0.5f;
        imgedit.panels[ i ].settings.saturation = 0.5f;
        imgedit.panels[ i ].settings.auto_contrast = ( i == IMGEDIT_MODE_FACES ) ? 0.0f : 1.0f;
        imgedit.panels[ i ].settings.sharpen_radius = ( i == IMGEDIT_MODE_FACES ) ? 0.0f : 0.15f;
        imgedit.panels[ i ].settings.sharpen_strength = ( i == IMGEDIT_MODE_FACES ) ? 0.0f : 1.0f;
        imgedit.panels[ i ].settings.vignette_size = 0.0f;
        imgedit.panels[ i ].settings.vignette_opacity = 0.0f;
        imgedit.panels[ i ].pending_settings = imgedit.panels[ i ].settings;
    }
    
    load_settings( &imgedit.panels[ IMGEDIT_MODE_FACES ].settings, "faces/settings.ini" );
    load_settings( &imgedit.panels[ IMGEDIT_MODE_IMAGES ].settings, "images/settings.ini" );
    
    imgedit.panels[ IMGEDIT_MODE_FACES ].pending_settings = imgedit.panels[ IMGEDIT_MODE_FACES ].settings;
    imgedit.panels[ IMGEDIT_MODE_IMAGES ].pending_settings = imgedit.panels[ IMGEDIT_MODE_IMAGES ].settings;

    imgedit.panel_height = 160;

    thread_atomic_int_store( &imgedit.exit_process_thread, 0 );
    thread_mutex_init( &imgedit.mutex );
    thread_ptr_t process_thread = thread_create( imgedit_process_thread, &imgedit, THREAD_STACK_SIZE_DEFAULT );

    int prev_resolution = imgedit.resolution;
    bool shift = false;
    bool ctrl = false;
    bool lbutton = false;
    int lbutton_x = 0;
    int lbutton_y = 0;
    while( app_yield( app ) != APP_STATE_EXIT_REQUESTED ) {

        int new_width = app_window_width( app ) < 200 ? 200 : app_window_width( app );
        int new_height = app_window_height( app );
        if( new_width != imgedit.screen_width || new_height != imgedit.screen_height ) {
            imgedit.screen_width = new_width;
            imgedit.screen_height = new_height;            
            free( imgedit.screen );
            imgedit.screen = (uint32_t*) malloc( sizeof( uint32_t ) * imgedit.screen_width * imgedit.screen_height );
            
        }

        app_input_t input = app_input( app );

        thread_mutex_lock( &imgedit.mutex );
        
        imgedit_input_t imgedit_input = { false };
        imgedit_input.mouse_x = app_pointer_x( app );
        imgedit_input.mouse_y = app_pointer_y( app );

        for( int i = 0; i < input.count; ++i ) {
            app_input_event_t* event = &input.events[ i ];
            if( event->type == APP_INPUT_KEY_DOWN ) {
                if( event->data.key == APP_KEY_LBUTTON ) {
                    imgedit_input.lbutton_pressed = true;
                    lbutton = true;
                    lbutton_x = imgedit_input.mouse_x;
                    lbutton_y = imgedit_input.mouse_y;
                }
                if( event->data.key == APP_KEY_ESCAPE ) {
                    imgedit_input.esc = true;
                }
                if( event->data.key == APP_KEY_SHIFT ) {
                    shift = true;
                }
                if( event->data.key == APP_KEY_CONTROL ) {
                    ctrl = true;
                }
                if( !shift && ctrl && event->data.key == APP_KEY_Z ) {
                    imgedit_undo( &imgedit );
                }
                if( shift && ctrl && event->data.key == APP_KEY_Z ) {
                    imgedit_redo( &imgedit );
                }
                if( !shift && ctrl && event->data.key == APP_KEY_Y ) {
                    imgedit_redo( &imgedit );
                }
            }
            if( event->type == APP_INPUT_KEY_UP ) {
                if( event->data.key == APP_KEY_LBUTTON ) {
                    lbutton = false;
                    imgedit_input.clicked = true;
                }
                if( event->data.key == APP_KEY_SHIFT ) {
                    shift = false;
                }
                if( event->data.key == APP_KEY_CONTROL ) {
                    ctrl = false;
                }
            }
            if( event->type == APP_INPUT_SCROLL_WHEEL ) {
                imgedit_input.scroll_delta -= event->data.wheel_delta;
            }
        }
        imgedit_input.lbutton = lbutton;
        imgedit_input.lbutton_x = lbutton_x;
        imgedit_input.lbutton_y = lbutton_y;

        if( imgedit.single_image >= 0 || imgedit.single_face >= 0 ) {
            imgedit_single_image( &imgedit, &imgedit_input );
        } else {
            imgedit_images( &imgedit, &imgedit_input );
        }

        imgedit_panel( &imgedit, &imgedit_input );


        thread_mutex_unlock( &imgedit.mutex );
      
        app_present( app, imgedit.screen, imgedit.screen_width, imgedit.screen_height, 0xffffff, 0x000000 );
        
        if( prev_resolution != imgedit.resolution ) {
            break;
        }
    }

    thread_atomic_int_store( &imgedit.exit_process_thread, 1 );
    thread_join( process_thread );
    thread_destroy( process_thread );
    thread_mutex_term( &imgedit.mutex );

    // save imgedit state
    //cstr( "None (RGB mode)" )
    ini_t* save_ini = ini_create( NULL );
    ini_property_add( save_ini, INI_GLOBAL_SECTION, "resolution", 0, cstr_int( imgedit.resolution ), 0 );
    cstr_t selected_palette = "";
    array_get( imgedit.palettes, imgedit.selected_palette, &selected_palette );
    ini_property_add( save_ini, INI_GLOBAL_SECTION, "palette", 0, selected_palette, 0 );
    int ini_size = ini_save( save_ini, NULL, 0 );
    file_t* ini_save_file = file_create( ini_size, NULL );
    ini_save( save_ini, ini_save_file->data, ini_save_file->size );
    file_save( ini_save_file, ".cache/imgedit.ini", FILE_MODE_TEXT );
    ini_destroy( save_ini );
    file_destroy( ini_save_file );

    int run_again = ini_resolution != imgedit.resolution;

    for( int i = 0; i < array_count( imgedit.images ); ++i ) {
        imgedit_image_t img;
        array_get( imgedit.images, i, &img );
        if( img.orig_pixels ) {
            free( img.orig_pixels );
        }
        if( img.sized_pixels ) {
            free( img.sized_pixels );
        }
        if( img.processed ) {
            free( img.processed );
        }
        if( img.prev_processed ) {
            free( img.prev_processed );
        }
    }

    for( int i = 0; i < array_count( imgedit.faces ); ++i ) {
        imgedit_image_t img;
        array_get( imgedit.faces, i, &img );
        if( img.orig_pixels ) {
            free( img.orig_pixels );
        }
        if( img.sized_pixels ) {
            free( img.sized_pixels );
        }
        if( img.processed ) {
            free( img.processed );
        }
        if( img.prev_processed ) {
            free( img.prev_processed );
        }
    }

    array_destroy( imgedit.images );
    array_destroy( imgedit.faces );
    paldither_palette_destroy( imgedit.palette, NULL );
    free( imgedit.screen );
    memmgr_clear( &g_memmgr );
    return run_again ? -1 : EXIT_SUCCESS;
}
