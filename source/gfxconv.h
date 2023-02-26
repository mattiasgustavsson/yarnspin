
#undef clamp
#undef max
#undef min


int fround( float x ) {
    return x < 0.0f ? (int)( x - 0.5f ) : (int)( x + 0.5f );
}


float clamp( float x, float lo, float hi ) {
    return x < lo ? lo : x > hi ? hi : x;
}


float max( float x, float y ) {
    return x > y ? x : y;
}


float min( float x, float y ) {
    return x < y ? x : y;
}

float lerp( float a, float b, float t ) {
    return a  + ( b - a ) * t;
}


img_rgba_t img_rgba( float r, float g, float b, float a ) {
    img_rgba_t rgba;
    rgba.r = r;
    rgba.g = g;
    rgba.b = b;
    rgba.a = a;
    return rgba;
}


img_rgba_t img_rgba_add( img_rgba_t a, img_rgba_t b ) {
    img_rgba_t rgba;
    rgba.r = a.r + b.r;
    rgba.g = a.g + b.g;
    rgba.b = a.b + b.b;
    rgba.a = a.a + b.a;
    return rgba;
}


img_rgba_t img_rgba_div( img_rgba_t a, img_rgba_t b ) {
    img_rgba_t rgba;
    rgba.r = a.r / b.r;
    rgba.g = a.g / b.g;
    rgba.b = a.b / b.b;
    rgba.a = a.a / b.a;
    return rgba;
}


img_rgba_t img_rgba_mul( img_rgba_t a, img_rgba_t b ) {
    img_rgba_t rgba;
    rgba.r = a.r * b.r;
    rgba.g = a.g * b.g;
    rgba.b = a.b * b.b;
    rgba.a = a.a * b.a;
    return rgba;
}


img_rgba_t img_rgba_saturate( img_rgba_t a ) {
    img_rgba_t rgba;
    rgba.r = clamp( a.r, 0.0f, 1.0f );
    rgba.g = clamp( a.g, 0.0f, 1.0f );
    rgba.b = clamp( a.b, 0.0f, 1.0f );
    rgba.a = clamp( a.a, 0.0f, 1.0f );
    return rgba;
}


pixelfont_t* generate_pixel_font( uint8_t* ttf_data ) {
    stbtt_fontinfo font;
    stbtt_InitFont( &font, ttf_data, stbtt_GetFontOffsetForIndex( ttf_data, 0) );

    int size = 0;
    for( int i = 1; i < 32; ++i ) {
        float scale = stbtt_ScaleForPixelHeight( &font, (float) i );
        int w, h;
        PIXELFONT_U8* bitmap = stbtt_GetGlyphBitmap( &font, scale, scale, 'A', &w, &h, 0, 0 );
        int empty = 1;
        int antialiased = 0;
        for( int j = 0; j < w * h; ++j ) {
            if( bitmap[ j ] > 0 ) {
                empty = 0;
                if( bitmap[ j ] < 255 ) { antialiased = 1; break; }
            }
        }
        stbtt_FreeBitmap( bitmap, 0 );
        if( !empty && !antialiased ) { size = i; break; }
    }

    if( !size ) {
        printf( "Not a pixel font (size detection failed)\n" );
        return NULL;
    }

    float scale = stbtt_ScaleForPixelHeight( &font, (float) size );

    int ascent, descent;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, 0 );
    ascent = (int)( scale * ascent );
    descent = (int)( scale * descent );

    int x0, y0, x1, y1;
    stbtt_GetFontBoundingBox( &font, &x0, &y0, &x1, &y1 );
    x0 = (int)( scale * x0 );
    x1 = (int)( scale * x1 );
    y0 = (int)( scale * y0 );
    y1 = (int)( scale * y1 );

    int line_spacing = ascent - descent + 1; // TODO: verify this

    for( int c = 0; c < 256; ++c ) {
        int gi = stbtt_FindGlyphIndex( &font, c );
        if( gi > 0 && gi < font.numGlyphs ) {
            if( !stbtt_IsGlyphEmpty( &font, gi ) ) {
                int ix0, iy0, ix1, iy1;
                stbtt_GetGlyphBitmapBox(&font, gi, scale, scale, &ix0, &iy0, &ix1, &iy1);
                iy0 += ascent;
                iy1 += ascent;
                if( ix0 < x0 ) x0 = ix0;
                if( iy0 < y0 ) y0 = iy0;
                if( ix1 > x1 ) x1 = ix1;
                if( iy1 > y1 ) y1 = iy1;
            }
        }
    }

    pixelfont_builder_t* builder = pixelfont_builder_create( y1 - y0 + 1, ascent, line_spacing, 0 );

    PIXELFONT_U8 dummy;
    pixelfont_builder_glyph( builder, 0, 0, &dummy, 0, 0 );
    for( int c = 1; c < 256; ++c ) {
        int gi = stbtt_FindGlyphIndex( &font, c );
        if( gi > 0 && gi < font.numGlyphs ) {
            int advance;
            int left;
            stbtt_GetGlyphHMetrics( &font, gi, &advance, &left );
            advance = (int)( scale * advance );
            left = (int)( scale * left );
            advance -= left;

            if( !stbtt_IsGlyphEmpty( &font, gi ) ) {
                int ix0, iy0, ix1, iy1;
                stbtt_GetGlyphBitmapBox(&font, gi, scale, scale, &ix0, &iy0, &ix1, &iy1);
                int w, h, xo, yo;
                PIXELFONT_U8* bitmap = stbtt_GetGlyphBitmap( &font, scale, scale, gi, &w, &h, &xo, &yo );
                PIXELFONT_U8* temp_bmp = (PIXELFONT_U8*) malloc( (size_t) ( y1 - y0 + 1 ) * w  );
                memset( temp_bmp, 0, (size_t) ( y1 - y0 + 1 ) * w );
                int top = ascent + yo;
                top = top < 0 ? 0 : top;
                PIXELFONT_U8* out = temp_bmp + top * w;
                for( int y = 0; y < h; ++y )
                    for( int x = 0; x < w; ++x )
                        *out++ = bitmap[ x + y * w ] ? 1U : 0U; // font pixel
                pixelfont_builder_glyph( builder, c, w, temp_bmp, left, advance );
                if( bitmap ) stbtt_FreeBitmap( bitmap, NULL );
                if( temp_bmp ) free( temp_bmp );
            } else if( advance || left ) {
                pixelfont_builder_glyph( builder, c, 0, &dummy, left, advance );
            }

            for( int k = 0; k < 256; ++k ) {
                int kern = stbtt_GetCodepointKernAdvance( &font, c, k );
                kern = (int)( scale * kern );
                if( kern ) pixelfont_builder_kerning( builder, c, k, kern );
            }
        }
    }

    pixelfont_t* builtfont = pixelfont_builder_font( builder );
    pixelfont_t* pixelfont = (pixelfont_t*) malloc( (size_t) builtfont->size_in_bytes );
    memcpy( pixelfont, builtfont, builtfont->size_in_bytes );
    pixelfont_builder_destroy( builder );
    return pixelfont;
}


int generate_palette( uint32_t* image, int width, int height, uint32_t palette[ 256 ] )  {
    int count = 0;

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            uint32_t pixel = image[ x + y * width ];
            pixel = ( pixel & 0x00ffffff ) | 0xff000000;
            for( int i = 0; i < count; ++i ) {
                if( palette[ i ] == pixel )
                    goto skip;
            }
            if( count >= 256 ) {
                return palettize_generate_palette_xbgr32( image, width, height, palette, 256, NULL );
            }
            palette[ count ++ ] = pixel;
        skip:
            ;
        }
    }

    return count;
}


int palette_from_file( char const* filename, uint32_t palette[ 256 ] ) {
    int w, h, c;
    stbi_uc* p = stbi_load( filename, &w, &h, &c, 4 );
    if( !p ) {
        printf( "Failed to load '%s' as an image\n", filename );
        return false;
    }

    int count = generate_palette( (uint32_t*) p, w, h, palette );
    stbi_image_free( p );

    if( !count ) {
        printf( "The file '%s; is not a valid palette image\n", filename );
        return 0;
    }
    return count;
}


void set_pixel( img_t* image, int x, int y, img_rgba_t pixel ) {
    if( x < image->width && y < image->height ) {
        image->pixels[ x + y * image->width ] = pixel;
    }
}


img_rgba_t get_pixel( img_t* image, int x, int y ) {
    if( x >= 0 && x < image->width && y >= 0 && y < image->height ) {
        img_rgba_t rgb = image->pixels[ x + y * image->width ];
        return img_rgba( rgb.r, rgb.g, rgb.b, rgb.a );
    }

    return img_rgba( 0.0f, 0.0f, 0.0f, 0.0f );
}


img_t duplicate( img_t const* image ) {
    img_t result = img_create( image->width, image->height );
    memcpy( result.pixels, image->pixels, sizeof( *result.pixels ) * image->width * image->height );
    return result;
}



void sobel( img_t* img, float r ) {
    int width = img->width;
    int height = img->height;
    img_t out = img_create( width, height );

    float weight_h[3][3] = {
        { -1,  0,  1 },
        { -2,  0,  2 },
        { -1,  0,  1 },
    };
    float weight_v[3][3] = {
        { -1, -2, -1 },
        {  0,  0,  0 },
        {  1,  2,  1 },
    };

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            float sum_h[3] = { 0, 0, 0 };
            float sum_v[3] = { 0, 0, 0 };
            for( int j = -1; j <= 1; ++j ) {
                for( int i = -1; i <= 1; ++i ) {
                    img_rgba_t smp = img_sample_clamp( img, x + i * r, y + j * r );
                    float rgb[4];
                    rgb[0] = smp.r;
                    rgb[1] = smp.g;
                    rgb[2] = smp.b;
                    rgb[3] = smp.a;
                    for( int k = 0; k < 3; ++k ) {
                        float h = weight_h[ j + 1 ][ i + 1 ] * rgb[ k ];
                        sum_h[ k ] += h * rgb[3];
                        float v = weight_v[ j + 1 ][ i + 1 ] * rgb[ k ];
                        sum_v[ k ] += v * rgb[3];
                    }
                }
            }
            float final_val[3];
            for( int k = 0; k < 3; ++k ) {
                float result = ( sum_h[k] * sum_h[k] + sum_v[k] * sum_v[k] ) / 2.0f;
                result = 1.0f - clamp( result, 0.0f, 1.0f );
                final_val[ k] = result;
            }
            set_pixel( &out, x, y, img_rgba( final_val[ 0 ], final_val[ 1 ], final_val[ 2 ], 1.0f ) );
        }
    }
    img_free( img );
    *img = out;
}


void vignette( img_t* img, float power, float opcacity ) {
    int width = img->width;
    int height = img->height;
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            float u = (float) x / (float) width;
            float v = (float) y / (float) height;
            u *= 1.0f - (float) y / (float) height;
            v *= 1.0f - (float) x / (float) width;
            float vig = u * v * 15.0f;
            vig = (float)pow( (double)vig, (double)power );
            img_rgba_t c = img->pixels[ x + y * width ];
            img->pixels[ x + y * width ] = img_rgba_lerp( c, img_rgba_mul( c, img_rgba( vig, vig, vig, 1.0f ) ), opcacity );
        }
    }
}


img_rgba_t sample_average( img_t* img, int x0, int y0, int x1, int y1 ) {
    img_rgba_t pixel=img_rgba( 0.0f, 0.0f, 0.0f, 0.0f );
    int count = 0;
    for( int y = y0; y < y1; ++y ) {
        for( int x = x0; x < x1; ++x ) {
            pixel = img_rgba_add( pixel, get_pixel( img, x, y ) );
            ++count;
        }
    }
    return img_rgba_div( pixel, img_rgba( (float) count, (float) count, (float) count, (float) count ) );
}


img_t resize_image( img_t img, int new_width, int new_height ) {
    if( new_width < img.width && new_height < img.height) {
        float horiz = new_width / (float) img.width;
        float vert = new_height / (float) img.height;
        float scale = max( horiz, vert );
        int scaled_width = fround( scale * img.width );
        int scaled_height = fround( scale * img.height );

        img_t temp = img_create( scaled_width, scaled_height );
        stbir_resize_float( (float*)img.pixels, img.width, img.height, sizeof( float ) * 4 * img.width, 
            (float*)temp.pixels, temp.width, temp.height, sizeof( float ) * 4 * temp.width, 4 );

        return temp;
    }
    return duplicate( &img );
}


img_rgba_t sample_border( img_t* image, float p_x, float p_y ) {
    float f_x = (float)floor( (double) p_x );
    float f_y = (float)floor( (double) p_y );
    float d_x = p_x - f_x;
    float d_y = p_y - f_y;

    int x1 = (int) ( f_x );
    int y1 = (int) ( f_y );
    int x2 = (int) ( f_x + 1.0f );
    int y2 = (int) ( f_y + 1.0f );

    img_rgba_t c1 = img_rgba( 0.0f, 0.0f, 0.0f, 0.0f);
    img_rgba_t c2 = img_rgba( 0.0f, 0.0f, 0.0f, 0.0f);
    img_rgba_t c3 = img_rgba( 0.0f, 0.0f, 0.0f, 0.0f);
    img_rgba_t c4 = img_rgba( 0.0f, 0.0f, 0.0f, 0.0f);

    if( x1 >= 0 && x1 < (int) image->width && y1 >= 0 && y1 < (int) image->height )  {
        c1 = get_pixel( image, x1, y1 );
    }
    if( x2 >= 0 && x2 < (int) image->width && y1 >= 0 && y1 < (int) image->height ) {
        c2 = get_pixel( image, x2, y1 );
    }
    if( x1 >= 0 && x1 < (int) image->width && y2 >= 0 && y2 < (int) image->height ) {
        c3 = get_pixel( image, x1, y2 );
    }
    if( x2 >= 0 && x2 < (int) image->width && y2 >= 0 && y2 < (int) image->height ) {
        c4 = get_pixel( image, x2, y2 );
    }

    return img_rgba_lerp( img_rgba_lerp( c1, c2, d_x ), img_rgba_lerp( c3, c4, d_x ), d_y );
}


img_t crop( img_t img, int new_width, int new_height ) {
    int hcrop = ( img.width - new_width );
    int vcrop = ( img.height - new_height );
    int xofs = hcrop / 2;
    int yofs = vcrop / 2;

    img_t temp = img_create( new_width, new_height );
    for( int y = 0; y < temp.height; ++y ) {
        for( int x = 0; x < temp.width; ++x ) {
            img_rgba_t pixel = sample_border( &img, (float) x + xofs, (float) y + yofs );
            set_pixel( &temp, x, y, pixel );
        }
    }

    return temp;
}


void auto_contrast( img_t* img, float strength ) {
    int histogram[ 256 ] = { 0 };
    int histogram_total = 0;
    for( int i = 0; i < img->width * img->height; ++i ) {
        img_rgba_t c = img->pixels[ i ];
        int r = (int)( ( c.r * 255.0f ) );
        int g = (int)( ( c.g * 255.0f ) );
        int b = (int)( ( c.b * 255.0f ) );
        int a = (int)( ( c.a * 255.0f ) );
        if( a > 0 ) {
            int luma = ( ( 54 * r + 183 * g + 19 * b ) >> 8 );
            luma = luma < 0 ? 0 : luma > 255 ? 255 : luma;
            ++histogram[ luma ];
            ++histogram_total;
        }
    }

    int64_t dark = 255;
    int64_t bright = 0;
    int dark_count = ( 3 * histogram_total ) / 100; // 0.3%
    int bright_count = dark_count;
    for( int i = 0; i < 256; ++i ) {
        if( dark_count > 0 ) {
            dark_count -= histogram[ i ];
            if( dark_count <= 0 ) dark = i;
        }
        if( bright_count > 0 ) {
            bright_count -= histogram[ 255 - i ];
            if( bright_count <= 0 )
                bright = 255 - i;
        }
    }

    if( bright - dark <= 0 ) return;
    float range = ( ( bright - dark ) / 255.0f );
    float offset = dark / 255.0f;
    for( int i = 0; i < img->width * img->height; ++i ) {
        img_rgba_t p = img->pixels[ i ];
        p.r = ( p.r - offset ) / range;
        p.g = ( p.g - offset ) / range;
        p.b = ( p.b - offset ) / range;
        img->pixels[ i ] = img_rgba_lerp( img->pixels[ i ], p, strength );
    }
}


unsigned char transparency_dither[ 4 * 4 * 7 ] = {
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,

    0,0,0,0,
    0,0,0,1,
    0,1,0,0,
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

    1,1,0,1,
    0,1,1,1,
    1,1,1,1,
    1,1,1,1,

    1,1,1,1,
    1,1,1,1,
    1,1,1,1,
    1,1,1,1,
};



typedef struct process_settings_t {
    bool use_portrait_processor;
    bool bayer_dither;
    float brightness;
    float contrast;
    float saturation;
    float auto_contrast;
    float sharpen_radius;
    float sharpen_strength;
    float vignette_size;
    float vignette_opacity;
} process_settings_t;


bool load_settings( process_settings_t* settings, char const* filename ) {
    file_t* file = file_load( filename, FILE_MODE_TEXT, NULL );
    if( !file ) {
        return false;
    }

    ini_t* ini = ini_load( file->data, NULL );
    file_destroy( file );
    if( !ini ) {
        return false;
    }
    memset( settings, 0, sizeof( *settings ) );
    
    int use_portrait_processor = ini_find_property( ini, INI_GLOBAL_SECTION, "use_portrait_processor", -1 );
    int bayer_dither = ini_find_property( ini, INI_GLOBAL_SECTION, "bayer_dither", -1 );
    int brightness = ini_find_property( ini, INI_GLOBAL_SECTION, "brightness", -1 );
    int contrast = ini_find_property( ini, INI_GLOBAL_SECTION, "contrast", -1 );
    int saturation = ini_find_property( ini, INI_GLOBAL_SECTION, "saturation", -1 );
    int auto_contrast = ini_find_property( ini, INI_GLOBAL_SECTION, "auto_contrast", -1 );
    int sharpen_radius = ini_find_property( ini, INI_GLOBAL_SECTION, "sharpen_radius", -1 );
    int sharpen_strength = ini_find_property( ini, INI_GLOBAL_SECTION, "sharpen_strength", -1 );
    int vignette_size = ini_find_property( ini, INI_GLOBAL_SECTION, "vignette_size", -1 );
    int vignette_opacity = ini_find_property( ini, INI_GLOBAL_SECTION, "vignette_opacity", -1 );

    if( use_portrait_processor != INI_NOT_FOUND )
        settings->use_portrait_processor = cstr_is_equal( ini_property_value( ini, INI_GLOBAL_SECTION, use_portrait_processor ), "true" );
   
    if( bayer_dither != INI_NOT_FOUND )
        settings->bayer_dither = cstr_is_equal( ini_property_value( ini, INI_GLOBAL_SECTION, bayer_dither ), "true" );
    
    if( brightness != INI_NOT_FOUND )
        settings->brightness = (float) atof( ini_property_value( ini, INI_GLOBAL_SECTION, brightness ) );
    
    if( contrast != INI_NOT_FOUND )
        settings->contrast = (float) atof( ini_property_value( ini, INI_GLOBAL_SECTION, contrast ) );
    
    if( saturation != INI_NOT_FOUND )
        settings->saturation = (float) atof( ini_property_value( ini, INI_GLOBAL_SECTION, saturation ) );
    
    if( auto_contrast != INI_NOT_FOUND )
        settings->auto_contrast = (float) atof( ini_property_value( ini, INI_GLOBAL_SECTION, auto_contrast ) );
    
    if( sharpen_radius != INI_NOT_FOUND )
        settings->sharpen_radius = (float) atof( ini_property_value( ini, INI_GLOBAL_SECTION, sharpen_radius ) );
    
    if( sharpen_strength != INI_NOT_FOUND )
        settings->sharpen_strength = (float) atof( ini_property_value( ini, INI_GLOBAL_SECTION, sharpen_strength ) );
    
    if( vignette_size != INI_NOT_FOUND )
        settings->vignette_size = (float) atof( ini_property_value( ini, INI_GLOBAL_SECTION, vignette_size ) );
    
    if( vignette_opacity != INI_NOT_FOUND )
        settings->vignette_opacity = (float) atof( ini_property_value( ini, INI_GLOBAL_SECTION, vignette_opacity ) );

    ini_destroy( ini );
    return true;
}


void process_image( uint32_t* image, int width, int height, uint8_t* output, int outw, int outh, paldither_palette_t* palette, process_settings_t* settings ) {
    if( palette && width == outw && height == outh ) {
        bool all_colors_match = true;
        for( int i = 0; i < width * height; ++i ) {
            uint32_t c = image[ i ] & 0xffffff;
            bool color_found = false;
            for( int j = 0; j < palette->color_count; ++j ) {
                if( c == ( palette->colortable[ j ] & 0xffffff ) ) {
                    color_found = true;
                    output[ i ] = (uint8_t) j;
                    break;
                }
            }
            if( !color_found ) {
                all_colors_match = false;
                break;
            }
        }
        if( all_colors_match ) {
            return;
        }
    }


    img_t img = img_from_abgr32( image, width, height );    
    if( img.width != outw || img.height != outh ) {
        img_t sized_img = resize_image( img, outw, outh );
        img_t cropped_img = crop( sized_img, outw, outh );
        img_free( &sized_img );
        img_free( &img );
        img = cropped_img;
    }

    if( settings ) {
        auto_contrast( &img, settings->auto_contrast );
        img_adjust_saturation( &img, lerp( -0.5f, 0.5f, settings->saturation )  );
        img_adjust_contrast( &img, lerp( 0.1f, 1.9f, settings->contrast ) );
        img_adjust_brightness( &img, lerp( -0.5f, 0.5f, settings->brightness ) );
        vignette( &img, settings->vignette_size * 1.5f, settings->vignette_opacity );
        img_sharpen( &img, settings->sharpen_radius, settings->sharpen_strength );
    } else {
        auto_contrast( &img, 1.0f );
        img_sharpen( &img, 0.15f, 1.0f );
    } 


    img_adjust_contrast( &img, 1.2f );
    img_adjust_saturation( &img, 0.15f );

    for( int y = 0; y < img.height; ++y ) {
        for( int x = 0; x < img.width; ++x )     {
            img_rgba_t c = get_pixel( &img, x, y );
            float w = 1.0f - ( 1.0f - c.a ) * ( 1.0f - c.a );
            c.a = w;
            c.r = c.r - ( 1.0f - c.a );
            c.g = c.g - ( 1.0f - c.a );
            c.b = c.b - ( 1.0f - c.a );
            set_pixel( &img, x, y, c );
        }
    }

    uint32_t* out = image;
    img_to_argb32( &img, out );
    img_free( &img );

    if( palette ) {
        paldither_type_t dither = PALDITHER_TYPE_DEFAULT;
        if( settings && settings->bayer_dither ) {
            dither = PALDITHER_TYPE_BAYER;
        }
        paldither_palettize( out, outw, outh, palette, dither, output );

        for( int y = 0; y < outh; ++y ) {
            for( int x = 0; x < outw; ++x )  {
                uint32_t c = palette->colortable[ output[ x + outw * y ] ];
                uint32_t b = c & 0xffffff;
                uint32_t a = ( out[ x + outw * y ] ) >> 24;
                c = ( c & 0xffffff ) | 0xff000000;
                if( a < 4 ) {
                    c = b;
                } else if ( a > 256 - 128 ) {
                    c = c;
                } else {
                    a = ( a - 4 );
                    a = ( a * 5 ) / ( 256 - 4 - 128 ) + 2;
                    if( a < 2 ) a = 2;
                    if( a > 6 ) a = 6;
                    unsigned char d = transparency_dither[ 4 * 4 * a + ( x & 3 ) + ( y & 3 ) * 4 ];
                    if( d == 0 )
                        c = b;
                    else
                        c = c;
                }
                out[ x + outw * y ] = c;
            }
        }
    }
}


void process_face( uint32_t* image, int width, int height, uint8_t* output, int outw, int outh, paldither_palette_t* palette, process_settings_t* settings ) {
    if( palette && width == outw && height == outh ) {
        bool all_colors_match = true;
        for( int i = 0; i < width * height; ++i ) {
            uint32_t c = image[ i ] & 0xffffff;
            bool color_found = false;
            for( int j = 0; j < palette->color_count; ++j ) {
                if( c == ( palette->colortable[ j ] & 0xffffff ) ) {
                    color_found = true;
                    output[ i ] = (uint8_t) j;
                    break;
                }
            }
            if( !color_found ) {
                all_colors_match = false;
                break;
            }
        }
        if( all_colors_match ) {
            return;
        }
    }
    
    img_t img = img_from_abgr32( image, width, height );

    if( img.width != outw || img.height != outh ) {
        img_t sized_img = resize_image( img, outw, outh );
        img_t cropped_img = crop( sized_img, outw, outh );
        img_free( &sized_img );
        img_free( &img );
        img = cropped_img;
    }

    if( settings ) {
        auto_contrast( &img, settings->auto_contrast );
        img_adjust_saturation( &img, lerp( -0.5f, 0.5f, settings->saturation )  );
        img_adjust_contrast( &img, lerp( 0.1f, 1.9f, settings->contrast ) );
        img_adjust_brightness( &img, lerp( -0.5f, 0.5f, settings->brightness ) );
        vignette( &img, settings->vignette_size * 1.5f, settings->vignette_opacity );
        img_sharpen( &img, settings->sharpen_radius, settings->sharpen_strength );
    } 

    int extrahigh = 0;
    int extrahigh_low = 0;
    int high = 0;
    int low = 0;
    for( int y = 0; y < img.height; ++y ) {
        for( int x = 0; x < img.width; ++x ) {
            img_rgba_t c = get_pixel( &img, x, y );
            if( c.a < 1.0f ) continue;
            float l = c.r * 0.2126f + c.g * 0.7152f + c.b * 0.0722f;
            if( x > img.width / 5 && x < img.width - img.width / 5 && y > img.height / 5 && y < img.height - img.height / 5 ) {
                if( l > 0.63f )
                    ++extrahigh;
                else if( l > 0.54f )
                    ++extrahigh_low;
            }
            if( x > img.width / 3 && x < img.width - img.width / 3 && y > img.height / 3 && y < img.height - img.height / 3 ) {
                if( l > 0.25f ) ++high;
                if( l > 0.05f && l < 0.2f ) ++low;
            }
        }
    }


    img_t sobel_img = duplicate( &img );
    sobel( &sobel_img, 0.6f );

    img_t sobel_img2 = duplicate( &img );
    sobel( &sobel_img2, 1.1f );

    img_t sobel_img3 = duplicate( &img );
    sobel( &sobel_img3, 1.5f );

    for( int y = 0; y < img.height; ++y ) {
        for( int x = 0; x < img.width; ++x ) {
            img_rgba_t c = get_pixel( &img, x, y );
            c.r = c.r - ( 1.0f - c.a ) * 0.2f;
            c.g = c.g - ( 1.0f - c.a ) * 0.2f;
            c.b = c.b - ( 1.0f - c.a ) * 0.2f;
            set_pixel( &img, x, y, c );
        }
    }

    for( int y = 0; y < img.height; ++y ) for( int x = 0; x < img.width; ++x ) set_pixel( &img, x, y, img_rgba_saturate( get_pixel( &img, x, y ) ) );

    img_t orig = duplicate( &img );

    img_sharpen( &img, 0.25f, 0.6f );
    for( int y = 0; y < img.height; ++y ) for( int x = 0; x < img.width; ++x ) set_pixel( &img, x, y, img_rgba_saturate( get_pixel( &img, x, y ) ) );

    img_sharpen( &img, 0.1f, 0.8f );
    for( int y = 0; y < img.height; ++y ) for( int x = 0; x < img.width; ++x ) set_pixel( &img, x, y, img_rgba_saturate( get_pixel( &img, x, y ) ) );

    for( int y = 0; y < img.height; ++y ) {
        for( int x = 0; x < img.width; ++x )     {
            img_rgba_t c = get_pixel( &img, x, y );
            img_rgba_t o = get_pixel( &orig, x, y );
            img_rgba_t s1 = get_pixel( &sobel_img, x, y );
            img_rgba_t s2 = get_pixel( &sobel_img2, x, y );
            img_rgba_t s3 = get_pixel( &sobel_img3, x, y );
            float ol = 1.0f - ( o.r * 0.2126f + o.g * 0.7152f + o.b * 0.0722f );
            float l1 = min( s1.r, min( s1.g, s1.b ) );
            float l2 = min( s2.r, min( s2.g, s2.b ) );
            float l3 = min( s3.r, min( s3.g, s3.b ) );
            float l = min( l1, min( l2, l3 ) );
            ol = clamp( 1.0f - (float) pow( (double) ol * 1.5f, 8.0 ), 0.0f, 1.0f );
            l = 1 - clamp( ( 1 - l - 0.01f ) * 4.0f, 0.0f, 1.0f );
            l = (float)pow( (double)l, 10.0 );
            l = l * ol *  c.a * c.a;
            set_pixel( &sobel_img3, x, y, img_rgba( l, l, l, 1 ) );
            img_rgba_t t = img_rgba_lerp( img_rgba_mul( o, img_rgba( 1.025f, 1.025f, 1.025f, 1.025f ) ), c, 1.0f - l );
            c.r = t.r;
            c.g = t.g;
            c.b = t.b;
            set_pixel( &img, x, y, c );
        }
    }
    for( int y = 0; y < img.height; ++y ) for( int x = 0; x < img.width; ++x ) set_pixel( &img, x, y, img_rgba_saturate( get_pixel( &img, x, y ) ) );



    if( low > high ) extrahigh = extrahigh_low = 0;
    float lighten = low > high ? 0.1f : 0.0f;
    float darken = extrahigh > extrahigh_low ? 0.1f : 0.0f;

    img_adjust_saturation( &img, 0.11f  - lighten * 0.3f  );
    for( int y = 0; y < img.height; ++y ) for( int x = 0; x < img.width; ++x ) set_pixel( &img, x, y, img_rgba_saturate( get_pixel( &img, x, y ) ) );


    img_adjust_brightness( &img, 0.2f + lighten * 0.7f - darken * 0.7f );
    img_adjust_contrast( &img, 1.4f + lighten * 5.0f );
    img_adjust_brightness( &img, -0.18f );
    for( int y = 0; y < img.height; ++y ) for( int x = 0; x < img.width; ++x ) set_pixel( &img, x, y, img_rgba_saturate( get_pixel( &img, x, y ) ) );


    for( int y = 0; y < img.height; ++y ) {
        for( int x = 0; x < img.width; ++x )     {
            img_rgba_t c = get_pixel( &img, x, y );
            float w = 1.0f - ( 1.0f - c.a ) * ( 1.0f - c.a );
            c.a = w;;
            c.r = c.r - ( 1.0f - c.a );
            c.g = c.g - ( 1.0f - c.a );
            c.b = c.b - ( 1.0f - c.a );
            set_pixel( &img, x, y, c );
        }
    }

    for( int y = 0; y < img.height; ++y ) {
        for( int x = 0; x < img.width; ++x ) {
            img_rgba_t s1 = get_pixel( &sobel_img, x, y );
            float l1 = s1.r * 0.2126f + s1.g * 0.7152f + s1.b* 0.0722f;
            l1 *= l1;

            img_rgba_t c = get_pixel( &img, x, y );
            img_rgba_t o = get_pixel( &orig, x, y );
            img_rgba_t s = get_pixel( &sobel_img2, x, y );

            float l = s.r * 0.2126f + s.g * 0.7152f + s.b* 0.0722f;
            l = min( s.r, min( s.g, s.b ) );
            float luma = c.r * 0.2126f + c.g * 0.7152f + c.b* 0.0722f;
            float alpha = l * 0.5f + 0.5f;
            alpha += l1 * 0.5f;
            img_rgba_t rgb = c;
            rgb = img_rgba_lerp( img_rgba_mul( o, img_rgba( luma, luma, luma, luma ) ), rgb, clamp( alpha, 0.0f, 1.0f ) );
            c.r = rgb.r - 0.07f * ( 1.0f - l );
            c.g = rgb.g - 0.07f * ( 1.0f - l );
            c.b = rgb.b - 0.07f * ( 1.0f - l );
            set_pixel( &img, x, y, c );
        }
    }
    for( int y = 0; y < img.height; ++y ) for( int x = 0; x < img.width; ++x ) set_pixel( &img, x, y, img_rgba_saturate( get_pixel( &img, x, y ) ) );


    for( int y = 0; y < img.height; ++y ) {
        for( int x = 0; x < img.width; ++x ) {
            img_rgba_t c = get_pixel( &img, x, y );
            img_rgba_t s = get_pixel( &sobel_img, x, y );
            float l = s.r * 0.2126f + s.g * 0.7152f + s.b* 0.0722f;
            l = min( s.r, min( s.g, s.b ) );
            l *= l;
            //float luma = c.r * 0.2126f + c.g * 0.7152f + c.b* 0.0722f;
            c.r = c.r - 0.25f * ( 1.0f - l );
            c.g = c.g - 0.25f * ( 1.0f - l );
            c.b = c.b - 0.25f * ( 1.0f - l );
            set_pixel( &img, x, y, c );
        }
    }
    for( int y = 0; y < img.height; ++y ) for( int x = 0; x < img.width; ++x ) set_pixel( &img, x, y, img_rgba_saturate( get_pixel( &img, x, y ) ) );


    uint32_t* out = image;
    img_to_argb32( &img, out );
    img_free( &img );

    img_free( &orig );
    img_free( &sobel_img );
    img_free( &sobel_img2 );
    img_free( &sobel_img3 );

    if( palette ) {
        paldither_type_t dither = PALDITHER_TYPE_DEFAULT;
        if( settings && settings->bayer_dither ) {
            dither = PALDITHER_TYPE_BAYER;
        }
        paldither_palettize( out, outw, outh, palette, dither, output );


        for( int y = 0; y < outh; ++y ) {
            for( int x = 0; x < outw; ++x )  {
                uint32_t c = palette->colortable[ output[ x + outw * y ] ];
                uint32_t b = c & 0xffffff;
                uint32_t a = ( out[ x + outw * y ] ) >> 24;
                c = ( c & 0xffffff ) | 0xff000000;
                if( a < 4 ) {
                    c = b;
                } else if ( a > 256 - 128 ) {
                    c = c;
                } else {
                    a = ( a - 4 );
                    a = ( a * 5 ) / ( 256 - 4 - 128 ) + 2;
                    if( a < 2 ) a = 2;
                    if( a > 6 ) a = 6;
                    unsigned char d = transparency_dither[ 4 * 4 * a + ( x & 3 ) + ( y & 3 ) * 4 ];
                    if( d == 0 )
                        c = b;
                    else
                        c = c;
                }
                out[ x + outw * y ] = c;
            }
        }
    }
}


paldither_palette_t* convert_palette( string palette_filename, size_t* palette_size ) {
    uint32_t colortable[ 256 ];
    int colortable_count = palette_from_file( palette_filename, colortable );
    if( colortable_count <= 0 ) return NULL;

    string palette_lookup_file = cstr_cat( cstr_cat( ".cache/palettes/", cbasename( palette_filename ) ), ".plut" );

    paldither_palette_t* ditherpal = NULL;
    if( file_exists( palette_lookup_file ) && g_cache_version == YARNSPIN_VERSION && !file_more_recent( palette_filename, palette_lookup_file ) ) {
        file_t* file = file_load( palette_lookup_file, FILE_MODE_BINARY, NULL );
        if( file ) {
            ditherpal = paldither_palette_create_from_data( file->data, file->size, NULL );
            if( palette_size ) {
                *palette_size = file->size;
            }
            file_destroy( file );
        }
    }

    if( !ditherpal ) {
        size_t pal_size;
        ditherpal = paldither_palette_create( colortable, colortable_count, &pal_size, NULL );
        if( palette_size ) {
            *palette_size = pal_size;
        }
        create_path( palette_lookup_file, 0 );
        file_save_data( &ditherpal->color_count, pal_size, palette_lookup_file, FILE_MODE_BINARY );
    }

    return ditherpal;
}


palrle_data_t* convert_bitmap( string image_filename, int width, int height, string palette_filename, paldither_palette_t* palette ) {
    bool is_face = false;
    if( cstr_starts( image_filename, "faces/" ) ) {
        is_face = true;
        image_filename = cstr_mid( image_filename,  6, 0 );
    } else if( cstr_starts( image_filename, "images/" ) ) {
        is_face = false;
        image_filename = cstr_mid( image_filename,  7, 0 );
    } else {
        NULL;
    }

    string processed_filename_no_ext = cstr_format( ".cache/processed/%s/%s/%dx%d/%s_%s", is_face ? "faces" : "images",
        cstr( cbasename( palette_filename ) ), width, height,
        cstr( cbasename( image_filename ) ), cstr_mid( cextname( image_filename ), 1, 0 ) ) ;

    string processed_filename = cstr_cat( processed_filename_no_ext, ".bitmap" );
    string intermediate_processed_filename = cstr_cat( processed_filename_no_ext, ".png" );

    if( !file_exists( processed_filename ) || !file_exists( intermediate_processed_filename ) ||
        g_cache_version != YARNSPIN_VERSION ||
        file_more_recent( cstr_cat( is_face ? "faces/" : "images/", image_filename ), processed_filename ) ||
        file_more_recent( cstr_cat( is_face ? "faces/" : "images/", image_filename ), intermediate_processed_filename ) ||
        file_more_recent( is_face ? "faces/settings.ini" : "images/settings.ini", processed_filename ) ) {

        int w, h, c;
        stbi_uc* img = stbi_load( cstr_cat( is_face ? "faces/" : "images/", image_filename ), &w, &h, &c, 4 );
        if( !img ) {
            return NULL;
        }

        if( w * h < width * height ) {
            img = (stbi_uc*) realloc( img, width * height * 4 );
        }

        int outw = width;
        int outh = height;

        uint8_t* pixels = (uint8_t*) malloc( 2 * outw * outh );

        process_settings_t settings;
        bool have_settings = load_settings( &settings, is_face ? "faces/settings.ini" : "images/settings.ini" );

        bool use_portrait_processor = is_face;
        if( have_settings ) use_portrait_processor = settings.use_portrait_processor;
        if( use_portrait_processor ) {
            process_face( (uint32_t*) img, w, h, pixels, outw, outh, palette, have_settings ? &settings : NULL );
        } else {
            process_image( (uint32_t*) img, w, h, pixels, outw, outh, palette, have_settings ? &settings : NULL );
        }

        uint8_t* mask = pixels + outw * outh;
        for( int y = 0; y < outh; ++y ) {
            for( int x = 0; x < outw; ++x ) {
                mask[ x + outw * y ] = ( ( (uint32_t*) img )[ x + outw * y ] & 0xff000000 ) ? 0xff : 0x00;
            }
        }

        create_path( processed_filename, 0 );

        stbi_write_png( intermediate_processed_filename, outw, outh, 4, (stbi_uc*)img, 4 * outw );

        stbi_image_free( img );

        palrle_data_t* rle = palrle_encode_mask( pixels, mask, outw, outh, palette->colortable, palette->color_count, NULL );
        free( pixels );
        file_save_data( rle, rle->size, processed_filename, FILE_MODE_BINARY );
        return rle;
    }

    file_t* file = file_load( processed_filename, FILE_MODE_BINARY, 0 );
    palrle_data_t* rle = (palrle_data_t*) malloc( file->size );
    memcpy( rle, file->data, file->size );
    file_destroy( file );

    return rle;
}


uint32_t* convert_rgb( string image_filename, int width, int height ) {
    bool is_face = false;
    if( cstr_starts( image_filename, "faces/" ) ) {
        is_face = true;
        image_filename = cstr_mid( image_filename,  6, 0 );
    } else if( cstr_starts( image_filename, "images/" ) ) {
        is_face = false;
        image_filename = cstr_mid( image_filename,  7, 0 );
    } else {
        NULL;
    }

    string processed_filename_no_ext = cstr_format( ".cache/processed/%s/%dx%d/%s_%s", is_face ? "faces" : "images",
        width, height, cstr( cbasename( image_filename ) ), cstr_mid( cextname( image_filename ), 1, 0 ) ) ;

    string processed_filename = cstr_cat( processed_filename_no_ext, /* ".qoi"*/ ".png" );
    string intermediate_processed_filename = cstr_cat( processed_filename_no_ext, ".png" );

    if( !file_exists( processed_filename ) || !file_exists( intermediate_processed_filename ) ||
        g_cache_version != YARNSPIN_VERSION ||
        file_more_recent( cstr_cat( is_face ? "faces/" : "images/", image_filename ), processed_filename ) ||
        file_more_recent( cstr_cat( is_face ? "faces/" : "images/", image_filename ), intermediate_processed_filename ) ||
        file_more_recent( is_face ? "faces/settings.ini" : "images/settings.ini", processed_filename ) ) {

        int w, h, c;
        stbi_uc* img = stbi_load( cstr_cat( is_face ? "faces/" : "images/", image_filename ), &w, &h, &c, 4 );
        if( !img ) {
            return NULL;
        }

        if( w * h < width * height ) {
            img = (stbi_uc*) realloc( img, width * height * 4 );
        }

        int outw = width;
        int outh = height;

        process_settings_t settings;
        bool have_settings = load_settings( &settings, is_face ? "faces/settings.ini" : "images/settings.ini" );

        bool use_portrait_processor = is_face;
        if( have_settings ) use_portrait_processor = settings.use_portrait_processor;
        if( use_portrait_processor ) {
            process_face( (uint32_t*) img, w, h, NULL, outw, outh, NULL, have_settings ? &settings : NULL );
        } else {
            process_image( (uint32_t*) img, w, h, NULL, outw, outh, NULL, have_settings ? &settings : NULL );
        }

        create_path( processed_filename, 0 );

        stbi_write_png( intermediate_processed_filename, outw, outh, 4, (stbi_uc*)img, 4 * outw );

        return (uint32_t*)img;
    }

    int w, h, c;
    stbi_uc* img = stbi_load( processed_filename, &w, &h, &c, 4 );
    return (uint32_t*)img;
}


pixelfont_t* convert_font( string font_filename ) {
    // TODO: Save converted font to cache and don't regenerate if it already exists
    file_t* ttf = file_load( font_filename, FILE_MODE_BINARY, 0 );
    pixelfont_t* font = generate_pixel_font( (uint8_t*) ttf->data );
    file_destroy( ttf );
    return font;
}
