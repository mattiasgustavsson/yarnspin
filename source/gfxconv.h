
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


bitmapfont_t* generate_bitmap_font( uint8_t* ttf_data, int font_size, int padding ) {
    stbtt_fontinfo font;
    if( !stbtt_InitFont( &font, ttf_data, stbtt_GetFontOffsetForIndex( ttf_data, 0) ) ) {
        return NULL;
    }

    int size = font_size;
    if( size == 0 ) {
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
    }

    if( size == 0 ) {
        size = 12;
    }

    font_size = size;

    float scale = stbtt_ScaleForPixelHeight( &font, (float) font_size );

    int ascent, descent;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, 0 );
    ascent = (int)( scale * ascent );
    descent = (int)( scale * descent );

    int width = 128;
    int height = 128;
    uint8_t* pixels = (uint8_t*) malloc( sizeof( uint8_t ) * width * height );
    stbtt_packedchar chardata[ 224 ];
    bool retry = true;
    while( retry && width <= 8192 && height <= 8192 ) {
        memset( pixels, 0, sizeof( uint8_t ) * width * height );
        stbtt_pack_context spc;
        stbtt_PackBegin( &spc, pixels, width, height, 0, padding, NULL );
        stbtt_PackSetOversampling( &spc, 1, 1 );
        stbtt_pack_range range;
        range.font_size = font_size;
        range.first_unicode_codepoint_in_range = ' ';
        int latin1_chars[] = {
            0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,
            0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,
            0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,0x20AC,0x0081,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0x02C6,0x2030,0x0160,0x2039,0x0152,0x008D,0x017D,0x008F,0x0090,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x02DC,0x2122,0x0161,
            0x203A,0x0153,0x009D,0x017E,0x0178,0x00A0,0x00A1,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,0x00A8,0x00A9,0x00AA,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,0x00B8,0x00B9,0x00BA,
            0x00BB,0x00BC,0x00BD,0x00BE,0x00BF,0x00C0,0x00C1,0x00C2,0x00C3,0x00C4,0x00C5,0x00C6,0x00C7,0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,0x00D0,0x00D1,0x00D2,0x00D3,0x00D4,0x00D5,0x00D6,0x00D7,0x00D8,0x00D9,0x00DA,
            0x00DB,0x00DC,0x00DD,0x00DE,0x00DF,0x00E0,0x00E1,0x00E2,0x00E3,0x00E4,0x00E5,0x00E6,0x00E7,0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,0x00F0,0x00F1,0x00F2,0x00F3,0x00F4,0x00F5,0x00F6,0x00F7,0x00F8,0x00F9,0x00FA,
            0x00FB,0x00FC,0x00FD,0x00FE,0x00FF,
        };
        range.array_of_unicode_codepoints = latin1_chars;
        range.num_chars = 224;
        range.chardata_for_range = chardata;
        int res = stbtt_PackFontRanges( &spc, ttf_data, 0, &range, 1 );    
        
        stbtt_PackEnd( &spc );
        if( res ) {
            retry = false;
        } else {
            free( pixels );
            if( width <= height ) {
                width *= 2;
            } else {
                height *= 2;
            }
            pixels = (uint8_t*) malloc( sizeof( uint8_t ) * width * height );
        }
    }

    if( retry ) {
        free( pixels );
        return NULL;
    }

    bitmapfont_t* bitmapfont = (bitmapfont_t*) malloc( sizeof( bitmapfont_t ) + sizeof( uint8_t ) * ( width * height - 1 ) );
    bitmapfont->size_in_bytes = sizeof( bitmapfont_t ) + sizeof( uint8_t ) * ( width * height - 1 );
    bitmapfont->u8font_height = (uint8_t)font_size + 1;
    bitmapfont->u8line_spacing = (uint8_t)(ascent - descent + 1);
    bitmapfont->width = width;
    bitmapfont->height = height;
    memcpy( bitmapfont->pixels, pixels, sizeof( uint8_t ) * width * height );
    free( pixels );
    bitmapfont->font_height = font_size + 1;
    bitmapfont->line_spacing = ascent - descent + 1;
    for( int i = 0; i < 224; ++i ) {
        float xp = 0.0f;
        float yp = ascent;
        stbtt_aligned_quad quad;
        stbtt_GetPackedQuad( chardata, width, height, i, &xp, &yp, &quad, 0 );
        bitmapfont->glyphs[ i ].advance = xp;
        bitmapfont->glyphs[ i ].x1 = quad.x0;
        bitmapfont->glyphs[ i ].y1 = quad.y0;
        bitmapfont->glyphs[ i ].x2 = quad.x1;
        bitmapfont->glyphs[ i ].y2 = quad.y1;
        bitmapfont->glyphs[ i ].u1 = quad.s0;
        bitmapfont->glyphs[ i ].v1 = quad.t0;
        bitmapfont->glyphs[ i ].u2 = quad.s1;
        bitmapfont->glyphs[ i ].v2 = quad.t1;
    }

    return bitmapfont;
}


bitmapfont_t* bitmap_font_from_pixel_font( pixelfont_t* pixelfont ) {
    stbrp_rect rects[ 224 ];
    int count = 0;
    for( int i = 0; i < 224; ++i ) {
        if( pixelfont->offsets[ i + 32 ] ) {
            PIXELFONT_U8 const* glyph = pixelfont->glyphs + pixelfont->offsets[ i + 32 ];
            int w = glyph[ 1 ];
            int h = pixelfont->height;
            stbrp_rect* rect = &rects[ count++ ];
            rect->id = i;
            rect->w = w + 2;
            rect->h = h + 2;
        }
    }
    stbrp_context context;
    stbrp_node nodes[ 256 ];
    int width = 128;
    int height = 128;
    bool retry = true;
    while( retry && width <= 8192 && height <= 8192 ) {
        stbrp_init_target( &context, width, height, nodes, 256 );
        if( stbrp_pack_rects( &context, rects, count) ) {
            retry = false;
        } else {
            width *= 2;
            height *= 2;
        }
    }

    if( retry ) {
        return NULL;
    }


    bitmapfont_t* bitmapfont = (bitmapfont_t*) malloc( sizeof( bitmapfont_t ) + sizeof( uint8_t ) * ( width * height - 1 ) );
    bitmapfont->size_in_bytes = sizeof( bitmapfont_t ) + sizeof( uint8_t ) * ( width * height - 1 );
    bitmapfont->u8font_height = (uint8_t)pixelfont->height;
    bitmapfont->u8line_spacing = (uint8_t)pixelfont->line_spacing;
    bitmapfont->width = width;
    bitmapfont->height = height;
    memset( bitmapfont->glyphs, 0, sizeof( bitmapfont->glyphs ) );
    memset( bitmapfont->pixels, 0, sizeof( uint8_t ) * width * height );
    bitmapfont->font_height = pixelfont->height;
    bitmapfont->line_spacing = pixelfont->line_spacing;
    for( int i = 0; i < count; ++i ) {
        stbrp_rect* r = &rects[ i ];
        PIXELFONT_U8 const* glyph = pixelfont->glyphs + pixelfont->offsets[ r->id + 32 ];
        int preadvance = (PIXELFONT_I8) *glyph++;
        int w = *glyph++;
        int h = pixelfont->height;
        for( int y = 0; y < h; ++y ) {
            for( int x = 0; x < w; ++x ) {
                bitmapfont->pixels[ ( r->x + x + 1 ) + ( r->y + y + 1 ) * width ] = *glyph++;
            }
        }
        bitmapfont->glyphs[ r->id ].advance = (float)( preadvance +  (PIXELFONT_I8) *glyph++ );
        bitmapfont->glyphs[ r->id ].x1 = 0 - preadvance;
        bitmapfont->glyphs[ r->id ].y1 = 0;
        bitmapfont->glyphs[ r->id ].x2 = r->w - 2;
        bitmapfont->glyphs[ r->id ].y2 = r->h - 2;
        bitmapfont->glyphs[ r->id ].u1 = ( r->x + 1 ) / (float) width;
        bitmapfont->glyphs[ r->id ].v1 = ( r->y + 1 ) / (float) height;
        bitmapfont->glyphs[ r->id ].u2 = ( r->x + 1 + r->w - 2 ) / (float) width;
        bitmapfont->glyphs[ r->id ].v2 = ( r->y + 1 + r->h - 2 ) / (float) height;
    }
    return bitmapfont;
}


pixelfont_t* generate_pixel_font( uint8_t* ttf_data, int font_size ) {
    stbtt_fontinfo font;
    if( !stbtt_InitFont( &font, ttf_data, stbtt_GetFontOffsetForIndex( ttf_data, 0) ) ) {
        return NULL;
    }

    int size = font_size;
    if( size == 0 ) {
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
    }

    if( size == 0 ) {
        size = 12;
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
                for( int y = 0; y < h; ++y ) {
                    for( int x = 0; x < w; ++x ) {
                        *out++ = bitmap[ x + y * w ] ; // font pixel
                    }
                }
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



void sobel( img_t* img, float r, float scale ) {
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
                result = 1.0f - clamp( result * scale, 0.0f, 1.0f );
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
    if( new_width != img.width && new_height != img.height) {
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


void process_image( uint32_t* image, int width, int height, uint8_t* output, int outw, int outh, paldither_palette_t* palette, process_settings_t* settings, float resolution_scale ) {
    if( width == outw && height == outh ) {
        if( palette ) {
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
        } else {
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
        img_adjust_contrast( &img, lerp( 0.1f, 2.1f, settings->contrast ) );
        img_adjust_brightness( &img, lerp( -0.5f, 0.5f, settings->brightness ) );
        vignette( &img, settings->vignette_size * 1.5f, settings->vignette_opacity );
        float sharpen_strength = settings->sharpen_strength * 2.0f;
        if( sharpen_strength > 1.0f ) {
            img_sharpen( &img, settings->sharpen_radius * resolution_scale, 1.0f );
            sharpen_strength -= 1.0f;
        }
        img_sharpen( &img, settings->sharpen_radius * resolution_scale , sharpen_strength );
    } else {
        if( resolution_scale < 4.0f ) {
            auto_contrast( &img, 0.25f / resolution_scale );
            img_sharpen( &img, 0.15f * resolution_scale, 1.0f );
        }
    } 

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
    sobel( &sobel_img, 0.6f, 1.0f );

    img_t sobel_img2 = duplicate( &img );
    sobel( &sobel_img2, 1.1f, 1.0f );

    img_t sobel_img3 = duplicate( &img );
    sobel( &sobel_img3, 1.5f, 1.0f );

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


palrle_data_t* convert_bitmap( string image_filename, int width, int height, string palette_filename, paldither_palette_t* palette, float resolution_scale ) {
    string ini_filename = cstr_cat( image_filename, ".ini" ); 

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
        file_more_recent( is_face ? "faces/settings.ini" : "images/settings.ini", processed_filename ) || 
        ( file_exists( ini_filename ) && file_more_recent( ini_filename, processed_filename ) ) ) {

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
        if( file_exists( ini_filename ) ) {
            load_settings( &settings, ini_filename );
        }

        bool use_portrait_processor = false;
        if( have_settings ) use_portrait_processor = settings.use_portrait_processor;
        if( use_portrait_processor ) {
            process_face( (uint32_t*) img, w, h, pixels, outw, outh, palette, have_settings ? &settings : NULL );
        } else {
            process_image( (uint32_t*) img, w, h, pixels, outw, outh, palette, have_settings ? &settings : NULL, resolution_scale );
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


unsigned char bayer_dither_pattern[ 4 * 4 * 17 ] = {
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


unsigned char custom_dither_pattern[ 4 * 4 * 7 ] =  {

	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,

    0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,

	1,0,1,0,
	0,0,0,0,
	1,0,1,0,
	0,0,0,0,

	1,0,1,0,
	0,1,0,0,
	1,0,1,0,
	0,0,0,1,

	1,0,1,0,
	0,1,0,1,
	1,0,1,0,
	0,1,0,1,

	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,

    1,1,1,1,
	1,1,1,1,
	1,1,1,1,
	1,1,1,1,
};

void dither_rgb9( uint32_t* pixels, int width, int height, bool use_bayer_dither ) {
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            uint32_t src = pixels[ x + y * width ];
            uint32_t sr = src & 0xff;
            uint32_t sg = ( src >> 8 ) & 0xff;
            uint32_t sb = ( src >> 16 ) & 0xff;
            uint32_t a = ( src >> 24 ) & 0xff;
            uint32_t dr = ( ( ( ( sr * 8 )  ) / 256 ) * 256 ) / 8 ;
            uint32_t dg = ( ( ( ( sg * 8 )  ) / 256 ) * 256 ) / 8 ;
            uint32_t db = ( ( ( ( sb * 8 )  ) / 256 ) * 256 ) / 8 ;
            uint32_t br = dr + 32;
            uint32_t bg = dg + 32;
            uint32_t bb = db + 32;
            dr = dr > 255 ? 255 : dr;
            dg = dg > 255 ? 255 : dg;
            db = db > 255 ? 255 : db;
            br = br > 255 ? 255 : br;
            bg = bg > 255 ? 255 : bg;
            bb = bb > 255 ? 255 : bb;

            int diff_r = sr - dr;
            int diff_g = sg - dg;
            int diff_b = sb - db;

            if( use_bayer_dither ) {
                dr = bayer_dither_pattern[ ( ( diff_r * 17 ) / 32 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? br : dr;
                dg = bayer_dither_pattern[ ( ( diff_g * 17 ) / 32 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? bg : dg;
                db = bayer_dither_pattern[ ( ( diff_b * 17 ) / 32 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? bb : db;
            } else {
                dr = custom_dither_pattern[ ( ( diff_r * 7 ) / 32 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? br : dr;
                dg = custom_dither_pattern[ ( ( diff_g * 7 ) / 32 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? bg : dg;
                db = custom_dither_pattern[ ( ( diff_b * 7 ) / 32 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? bb : db;
            }

            uint32_t dst = ( a << 24 ) | ( db << 16 ) | ( dg << 8 ) | dr;
            pixels[ x + y * width ] = dst;
        }
    }
}


void dither_rgb8( uint32_t* pixels, int width, int height, bool use_bayer_dither, float resolution_scale ) {
    img_t sobel_img = img_from_abgr32( pixels, width, height );
    float radius = resolution_scale / 3.0f;
    sobel( &sobel_img, radius, 0.5f );
    
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            uint32_t src = pixels[ x + y * width ];
            uint32_t sr = src & 0xff;
            uint32_t sg = ( src >> 8 ) & 0xff;
            uint32_t sb = ( src >> 16 ) & 0xff;
            uint32_t a = ( src >> 24 ) & 0xff;
            uint32_t dr = ( ( ( ( sr * 6 )  ) / 256 ) * 256 ) / 6 ;
            uint32_t dg = ( ( ( ( sg * 7 )  ) / 256 ) * 256 ) / 7 ;
            uint32_t db = ( ( ( ( sb * 6 )  ) / 256 ) * 256 ) / 6 ;
            uint32_t br = dr + 42;
            uint32_t bg = dg + 36;
            uint32_t bb = db + 42;
            dr = dr > 255 ? 255 : dr;
            dg = dg > 255 ? 255 : dg;
            db = db > 255 ? 255 : db;
            br = br > 255 ? 255 : br;
            bg = bg > 255 ? 255 : bg;
            bb = bb > 255 ? 255 : bb;

            int diff_r = sr - dr;
            int diff_g = sg - dg;
            int diff_b = sb - db;

            if( sobel_img.pixels[ x + y * width ].r < 0.75f || sobel_img.pixels[ x + y * width ].g < 0.75f || sobel_img.pixels[ x + y * width ].b < 0.75f) {
                diff_r = diff_r < 21 ? 0 : 41;
                diff_g = diff_g < 28 ? 0 : 35;
                diff_b = diff_b < 21 ? 0 : 41;
            }

            if( use_bayer_dither ) {
                dr = bayer_dither_pattern[ ( ( diff_r * 17 ) / 43 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? br : dr;
                dg = bayer_dither_pattern[ ( ( diff_g * 17 ) / 37 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? bg : dg;
                db = bayer_dither_pattern[ ( ( diff_b * 17 ) / 43 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? bb : db;
            } else {
                dr = custom_dither_pattern[ ( ( diff_r * 7 ) / 43 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? br : dr;
                dg = custom_dither_pattern[ ( ( diff_g * 7 ) / 37 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? bg : dg;
                db = custom_dither_pattern[ ( ( diff_b * 7 ) / 43 ) * 4 * 4 + ( x % 4 ) + ( y % 4 ) * 4 ] ? bb : db;
            }
            //dr = (uint32_t)( 255.0f * sobel_img.pixels[ x + y * width ].r );
            //dg = (uint32_t)( 255.0f * sobel_img.pixels[ x + y * width ].g );
            //db = (uint32_t)( 255.0f * sobel_img.pixels[ x + y * width ].b );

            uint32_t dst = ( a << 24 ) | ( db << 16 ) | ( dg << 8 ) | dr;
            pixels[ x + y * width ] = dst;
        }
    }
    img_free( &sobel_img );
}


void atarist_palettize( PALDITHER_U32* abgr, int width, int height, paldither_palette_t const* palette,
    paldither_type_t dither_type, PALDITHER_U8* output );

void atari_st( uint32_t* pixels, int width, int height ) {

    for( int y = 0; y < height; ++y ) {
        uint32_t colors[ 1600 ];
        for( int x = 0; x < width; ++x ) {
            uint32_t src = pixels[ x + y * width ];
            uint32_t sr = src & 0xff;
            uint32_t sg = ( src >> 8 ) & 0xff;
            uint32_t sb = ( src >> 16 ) & 0xff;
            uint32_t dr = ( ( ( ( sr * 8 )  ) / 256 ) * 256 ) / 8 ;
            uint32_t dg = ( ( ( ( sg * 8 )  ) / 256 ) * 256 ) / 8 ;
            uint32_t db = ( ( ( ( sb * 8 )  ) / 256 ) * 256 ) / 8 ;
            uint32_t dst = ( 255 << 24 ) | ( db << 16 ) | ( dg << 8 ) | dr;
            colors[ x ] = dst;
        }
        static uint32_t palette[ 256 ];
        int count = palettize_generate_palette_xbgr32( colors, width, 1, palette, 16, NULL );
        if( count < 16 ) {
            for( int i = 17; i < 256; ++i ) {
                count = palettize_generate_palette_xbgr32( colors, width, 1, palette, i, NULL );
                if( count >= 16 ) {
                    if( count > 16 ) {
                        palettize_generate_palette_xbgr32( colors, width, 1, palette, i - 1, NULL );
                        count = 16;
                    }
                    break;
                }
            }
        }
        static uint32_t input[ 1600 * 4 ];
        memcpy( input, pixels + ( y & (~3) ) * width, sizeof( uint32_t ) * width * 4 );
        static uint8_t output[ 1600 * 4 ];
        paldither_palette_t* paldither = paldither_palette_create( palette, 16, NULL, NULL );
        atarist_palettize( input, width, 4, paldither, PALDITHER_TYPE_BAYER, output ); 
        for( int x = 0; x < width; ++x ) {
            pixels[ x + y * width ] = paldither->colortable[ output[ x + ( y & 3 ) * width ] ];
        }
        paldither_palette_destroy( paldither );
    }
}


qoi_data_t* convert_rgb( string image_filename, int width, int height, int bpp, float resolution_scale, bool jpeg ) {
    string ini_filename = cstr_cat( image_filename, ".ini" ); 

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

    string processed_filename = cstr_cat( processed_filename_no_ext, jpeg ? ".jpg" : ".qoi" );
    string intermediate_processed_filename = cstr_cat( processed_filename_no_ext, ".png" );

    if( !file_exists( processed_filename ) || !file_exists( intermediate_processed_filename ) ||
        g_cache_version != YARNSPIN_VERSION ||
        file_more_recent( cstr_cat( is_face ? "faces/" : "images/", image_filename ), processed_filename ) ||
        file_more_recent( cstr_cat( is_face ? "faces/" : "images/", image_filename ), intermediate_processed_filename ) ||
        file_more_recent( is_face ? "faces/settings.ini" : "images/settings.ini", processed_filename ) ||
        ( file_exists( ini_filename ) && file_more_recent( ini_filename, processed_filename ) ) ) {

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
        if( file_exists( ini_filename ) ) {
            load_settings( &settings, ini_filename );
        }

        bool use_portrait_processor = false;
        if( have_settings ) use_portrait_processor = settings.use_portrait_processor;
        if( use_portrait_processor ) {
            process_face( (uint32_t*) img, w, h, NULL, outw, outh, NULL, have_settings ? &settings : NULL );
        } else {
            process_image( (uint32_t*) img, w, h, NULL, outw, outh, NULL, have_settings ? &settings : NULL, resolution_scale );
        }

        if( bpp == 8 ) {
            dither_rgb8( (uint32_t*) img, outw, outh, have_settings ? settings.bayer_dither : false, resolution_scale );
        }
        if( bpp == 9 ) {
            dither_rgb9( (uint32_t*) img, outw, outh, have_settings ? settings.bayer_dither : false );
        }

        create_path( processed_filename, 0 );
        
        stbi_write_png( intermediate_processed_filename, outw, outh, 4, (stbi_uc*)img, 4 * outw );

        if( !jpeg ) {
            for( int i = 0; i < outw * outh; ++i ) {
                uint32_t a = ((uint32_t*)img)[ i ] >> 24;
                uint32_t b = ( ((uint32_t*)img)[ i ] >> 16 ) & 0xff;
                uint32_t g = ( ((uint32_t*)img)[ i ] >> 8 ) & 0xff;
                uint32_t r = ( ((uint32_t*)img)[ i ] >> 0 ) & 0xff;
                r = ( a * r ) / 255;
                g = ( a * g ) / 255;
                b = ( a * b ) / 255;
                ((uint32_t*)img)[ i ] = ( a << 24 ) | ( b << 16 ) | ( g << 8 ) | r;
            }
            qoi_desc desc;
            desc.width = outw;
            desc.height = outh;
            desc.channels = 4;
            desc.colorspace = 0;
            qoi_write( processed_filename, img, &desc );
        } else {
            uint32_t* px = (uint32_t*)malloc( outw * outh * 2 * sizeof( uint32_t ) );
            for( int i = 0; i < outw * outh; ++i ) {
                uint32_t a = ((uint32_t*)img)[ i ] >> 24;
                uint32_t b = ( ((uint32_t*)img)[ i ] >> 16 ) & 0xff;
                uint32_t g = ( ((uint32_t*)img)[ i ] >> 8 ) & 0xff;
                uint32_t r = ( ((uint32_t*)img)[ i ] >> 0 ) & 0xff;
                r = ( a * r ) / 255;
                g = ( a * g ) / 255;
                b = ( a * b ) / 255;
                px[ i ] = ( b << 16 ) | ( g << 8 ) | r;
                px[ outw * outh + i ] = 0xff000000 | ( a << 16 ) | ( a << 8 ) | a;
            }
            stbi_write_jpg( processed_filename, outw, outh * 2, 4, px, 80 );
            free( px );
        }
    }

    file_t* file = file_load( processed_filename, FILE_MODE_BINARY, 0 );
    qoi_data_t* qoi = (qoi_data_t*) malloc( sizeof( qoi_data_t ) + ( file->size - 1 ) );
    qoi->size = (uint32_t) file->size;
    memcpy( qoi->data, file->data, file->size );
    file_destroy( file );

    return qoi;
}


pixelfont_t* convert_font( string font_filename, int font_size, bool palette_mode ) {
    string processed_filename_no_ext = cstr_format( ".cache/processed/fonts/%d/%s_%s", font_size, 
        cstr( cbasename( font_filename ) ), cstr_mid( cextname( font_filename ), 1, 0 ) ) ;

    string processed_filename = cstr_cat( processed_filename_no_ext, palette_mode ? ".fnt" : ".bmf" );

    if( !file_exists( processed_filename ) || g_cache_version != YARNSPIN_VERSION ||
        file_more_recent( font_filename, processed_filename ) ) {

        file_t* font_file = file_load( font_filename, FILE_MODE_BINARY, 0 );
        if( !font_file ) {
            return NULL;
        }
        if( cstr_compare_nocase( cextname( font_filename ), ".fnt" ) == 0 ) {
            if( palette_mode ) {
                create_path( processed_filename, 0 );
                file_save( font_file, processed_filename, FILE_MODE_BINARY );
            } else {
                bitmapfont_t* font = bitmap_font_from_pixel_font( (pixelfont_t*) font_file->data );
                create_path( processed_filename, 0 );
                file_save_data( font, font->size_in_bytes, processed_filename, FILE_MODE_BINARY );
                free( font );
            }
        } else if( palette_mode ) {
            pixelfont_t* font = generate_pixel_font( (uint8_t*) font_file->data, font_size );
            create_path( processed_filename, 0 );
            file_save_data( font, font->size_in_bytes, processed_filename, FILE_MODE_BINARY );
            free( font );
        } else {
            if( cstr_compare_nocase( cextname( font_filename ), ".bmf" ) == 0 ) {
                create_path( processed_filename, 0 );
                file_save( font_file, processed_filename, FILE_MODE_BINARY );
            } else {
                bitmapfont_t* font = generate_bitmap_font( (uint8_t*) font_file->data, font_size, 1 );
                create_path( processed_filename, 0 );
                file_save_data( font, font->size_in_bytes, processed_filename, FILE_MODE_BINARY );
                free( font );
            }
        }
        file_destroy( font_file );
    }

    file_t* font_file = file_load( processed_filename, FILE_MODE_BINARY, 0 );
    if( !font_file ) {
        return NULL;
    }
    pixelfont_t* font = (pixelfont_t*) malloc( font_file->size );
    memcpy( font, font_file->data, font_file->size );
    file_destroy( font_file );
    return font;
}
