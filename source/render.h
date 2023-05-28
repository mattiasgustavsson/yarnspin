typedef struct rgbimage_t {
    uint32_t width;
    uint32_t height;
    uint32_t pixels[ 1 ];
} rgbimage_t;


typedef struct render_t {
    yarn_t* yarn;
    uint8_t* screen;
    uint32_t* screen_rgb;
    int screen_width;
    int screen_height;
    uint8_t* screenshot;
    uint32_t* screenshot_rgb;
    int color_background;
    int color_disabled;
    int color_txt;
    int color_opt;
    int color_chr;
    int color_use;
    int color_name;
    int color_facebg;
    pixelfont_t* font_txt;
    pixelfont_t* font_opt;
    pixelfont_t* font_chr;
    pixelfont_t* font_use;
    pixelfont_t* font_name;
    rgbimage_t** rgbimages;
} render_t;


uint32_t blend_rgb( uint32_t color1, uint32_t color2, uint8_t alpha ) {
    uint64_t c1 = (uint64_t) color1;
    uint64_t c2 = (uint64_t) color2;
    uint64_t a = (uint64_t)( alpha );
    // bit magic to alpha blend R G B with single mul
    c1 = ( c1 | ( c1 << 24 ) ) & 0x00ff00ff00ffull;
    c2 = ( c2 | ( c2 << 24 ) ) & 0x00ff00ff00ffull;
    uint64_t o = ( ( ( ( c2 - c1 ) * a ) >> 8 ) + c1 ) & 0x00ff00ff00ffull; 
    return (uint32_t) ( o | ( o >> 24 ) );
}


void render_init( render_t* render, yarn_t* yarn, uint8_t* screen, uint32_t* screen_rgb, int width, int height ) {
    memset( render, 0, sizeof( *render ) );

    render->yarn = yarn;

    render->screen = screen;
    render->screen_rgb = screen_rgb;
    render->screen_width = width;
    render->screen_height = height;
    render->screenshot = render->screen ? (uint8_t*) manage_alloc( malloc( width * height * sizeof( uint8_t ) ) ) : NULL;
    render->screenshot_rgb = render->screen_rgb ? (uint32_t*) manage_alloc( malloc( width * height * sizeof( uint32_t ) ) ) : NULL;

    int darkest_index = 0;
    int darkest_luma = 65536;
    int brightest_index = 0;
    int brightest_luma = 0;
    int facebg_index = 0;
    int facebg_luma = 65536;
    int disabled_index = 0;
    int disabled_luma = 65536;

    int facebg_lumaref = 54 * 0x28 + 182 * 0x28 + 19 * 0x28;
    int disabled_lumaref = 54 * 0x70 + 182 * 0x70 + 19 * 0x70;
    for( int i = 0; i < yarn->assets.palette_count; ++i ) {
        int c = (int)( yarn->assets.palette[ i ] & 0x00ffffff );
        int r = c & 0xff;
        int g = ( c >> 8 ) & 0xff;
        int b = ( c >> 16 ) & 0xff;
        int l = 54 * r + 182 * g + 19 * b;
        if( l <= darkest_luma ) { darkest_luma = l; darkest_index = i; }
        if( l >= brightest_luma ) { brightest_luma = l; brightest_index = i; }
        int coldiff = ( abs( r - g ) + abs( g - b ) + abs( r - b ) ) * 64;
        if( abs( l - facebg_lumaref ) + coldiff <= facebg_luma ) { facebg_luma = abs( l - facebg_lumaref ) + coldiff; facebg_index = i; }
        if( abs( l - disabled_lumaref ) + coldiff <= disabled_luma ) { disabled_luma = abs( l - disabled_lumaref ) + coldiff; disabled_index = i; }
    }

    if( disabled_index == darkest_index ) {
        disabled_luma = 65536;
        for( int i = 0; i < yarn->assets.palette_count; ++i ) {
            if( i == darkest_index ) continue;
            int c = (int)( yarn->assets.palette[ i ] & 0x00ffffff );
            int r = c & 0xff;
            int g = ( c >> 8 ) & 0xff;
            int b = ( c >> 16 ) & 0xff;
            int l = 54 * r + 182 * g + 19 * b;
            if( l <= disabled_luma ) { disabled_luma = l; disabled_index = i; }
        }
    }

    render->font_txt = yarn->assets.font_description;
    render->font_opt = yarn->assets.font_options;
    render->font_chr = yarn->assets.font_characters;
    render->font_use = yarn->assets.font_items;
    render->font_name = yarn->assets.font_name;
    render->color_background = yarn->globals.color_background;
    render->color_disabled = yarn->globals.color_disabled;
    render->color_txt = yarn->globals.color_txt;
    render->color_opt = yarn->globals.color_opt;
    render->color_chr = yarn->globals.color_chr;
    render->color_use = yarn->globals.color_use;
    render->color_name = yarn->globals.color_name;
    render->color_facebg = yarn->globals.color_facebg;

    if( render->color_background < 0 ) render->color_background = (uint8_t)darkest_index;
    if( render->color_disabled < 0 ) render->color_disabled = (uint8_t)disabled_index;
    if( render->color_txt < 0 ) render->color_txt = (uint8_t)brightest_index;
    if( render->color_opt < 0 ) render->color_opt = (uint8_t)brightest_index;
    if( render->color_chr < 0 ) render->color_chr = (uint8_t)brightest_index;
    if( render->color_use < 0 ) render->color_use = (uint8_t)brightest_index;
    if( render->color_name < 0 ) render->color_name = (uint8_t)brightest_index;
    if( render->color_facebg < 0 ) render->color_facebg = (uint8_t)facebg_index;

    if( render->yarn->globals.colormode != YARN_COLORMODE_PALETTE ) {
        render->rgbimages = (rgbimage_t**)manage_alloc( malloc( sizeof( rgbimage_t* ) * render->yarn->assets.bitmaps->count ) );
        for( int i = 0; i < render->yarn->assets.bitmaps->count; ++i ) {
            bool jpeg = render->yarn->globals.colormode == YARN_COLORMODE_RGB && render->yarn->globals.resolution == YARN_RESOLUTION_FULL;
            qoi_data_t* qoi = (qoi_data_t*)render->yarn->assets.bitmaps->items[ i ];
            if( !jpeg ) {        
                qoi_desc desc;
                uint32_t* pixels = (uint32_t*)qoi_decode( qoi->data, qoi->size, &desc, 4 ); 
                rgbimage_t* image = (rgbimage_t*)manage_alloc( malloc( sizeof( rgbimage_t) + ( desc.width * desc.height - 1 ) * sizeof( uint32_t ) ) );
                image->width = desc.width;
                image->height = desc.height;
                uint32_t bgcolor = render->yarn->assets.palette[ render->color_background ];
                for( int i = 0; i < image->width * image->height; ++i ) {
                    uint32_t c = pixels[ i ];
                    uint8_t a = (uint8_t)( c >> 24 );
                    image->pixels[ i ] = blend_rgb( bgcolor, c, a );
                }
                render->rgbimages[ i ] = image;
                free( pixels );
            } else {
                int width, height, c;
                stbi_uc* pixels = stbi_load_from_memory( qoi->data, qoi->size, &width, &height, &c, 4 );
                rgbimage_t* image = (rgbimage_t*)manage_alloc( malloc( sizeof( rgbimage_t) + ( width * height - 1 ) * sizeof( uint32_t ) ) );
                image->width = width;
                image->height = height;
                memcpy( image->pixels, pixels, width * height * sizeof( uint32_t ) );
                render->rgbimages[ i ] = image;
                free( pixels );
            }
        }
    }
}


void cls( render_t* render ) {
    if( render->screen ) {
        memset( render->screen, render->color_background, (size_t) render->screen_width * render->screen_height );
    } else {
        uint32_t c = render->yarn->assets.palette[ render->color_background ];
        for( int i = 0; i < render->screen_width * render->screen_height ; ++i ) {
            render->screen_rgb[ i ] = c;
        }
    }
}


void scale_for_resolution( render_t* render, int* x, int* y ) {
    float scale_factors[] = { 1.0f, 1.25f, 1.5f, 2.0f, 4.5f };
    if( x ) {
        *x = (int)( *x * scale_factors[ (int) render->yarn->globals.resolution ] );
    }
    if( y ) {
        *y = (int)( *y * scale_factors[ (int) render->yarn->globals.resolution ] );
    }
}


void scale_for_resolution_inverse( render_t* render, int* x, int* y ) {
    float scale_factors[] = { 1.0f, 1.25f, 1.5f, 2.0f, 4.5f };
    if( x ) {
        *x = (int)( *x / scale_factors[ (int) render->yarn->globals.resolution ] );
    }
    if( y ) {
        *y = (int)( *y / scale_factors[ (int) render->yarn->globals.resolution ] );
    }
}


void draw( render_t* render, int bitmap_index, int x, int y ) {
    scale_for_resolution( render, &x, &y );
    if( render->screen ) {
        palrle_blit( render->yarn->assets.bitmaps->items[ bitmap_index ], x, y, render->screen, render->screen_width, render->screen_height );
    } else {
        rgbimage_t* image = render->rgbimages[ bitmap_index ];
        for( int i = 0; i < image->height; ++i ) {
            memcpy( render->screen_rgb + x + ( y +i ) * render->screen_width, image->pixels + i * image->width, image->width * sizeof( uint32_t ) );
        }
    }
}


void draw_raw( render_t* render, int x, int y, uint8_t* pixels, int w, int h ) {
    scale_for_resolution( render, &x, &y );
    if( render->screen ) {
        for( int i = 0; i < h; ++i ) {
            memcpy( render->screen + x + ( y + i ) * render->screen_width, pixels + i * w, w * sizeof( uint8_t ) );
        }
    }
}


void draw_raw_rgb( render_t* render, int x, int y, uint32_t* pixels, int w, int h ) {
    scale_for_resolution( render, &x, &y );
    if( render->screen_rgb ) {
        for( int i = 0; i < h; ++i ) {
            memcpy( render->screen_rgb + x + ( y + i ) * render->screen_width, pixels + i * w, w * sizeof( uint32_t ) );
        }
    }
}

void box( render_t* render, int x, int y, int w, int h, int c ) {
    scale_for_resolution( render, &x, &y );
    scale_for_resolution( render, &w, &h );
    if( render->screen ) {
        for( int iy = 0; iy < h; ++iy ) {
            for( int ix = 0; ix < w; ++ix ) {
                int xp = x + ix;
                int yp = y + iy;
                if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                    render->screen[ xp + yp * render->screen_width ] = (uint8_t) c;
                }
            }
        }
    } else {
        for( int iy = 0; iy < h; ++iy ) {
            for( int ix = 0; ix < w; ++ix ) {
                int xp = x + ix;
                int yp = y + iy;
                if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                    render->screen_rgb[ xp + yp * render->screen_width ] = render->yarn->assets.palette[ c ];
                }
            }
        }
    }
}


void menu_icon( render_t* render, int x, int y, int c ) {
    scale_for_resolution( render, &x, &y );
    int s = 8;
    scale_for_resolution( render, &s, NULL );
    int h = s / 2;
    if( render->screen ) {
        for( int iy = 0; iy < h; ++iy ) {
            for( int ix = 0; ix < s; ++ix ) {
                int xp = x + ix;
                int yp = y + iy;
                if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                    render->screen[ xp + yp * render->screen_width ] = (uint8_t) c;
                }
            }
            ++x;
            s -= 2;
        }
    } else {
        for( int iy = 0; iy < h; ++iy ) {
            for( int ix = 0; ix < s; ++ix ) {
                int xp = x + ix;
                int yp = y + iy;
                if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                    render->screen_rgb[ xp + yp * render->screen_width ] = render->yarn->assets.palette[ c ];
                }
            }
            ++x;
            s -= 2;
        }
    }
}



pixelfont_bounds_t center( render_t* render, pixelfont_t* font, string str, int x, int y, int color ) {
    scale_for_resolution( render, &x, &y );
    pixelfont_bounds_t bounds;
    if( render->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, render->screen, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_CENTER, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    } else {
        pixelfont_blit_rgb( font, x, y, str, render->yarn->assets.palette[ color ], render->screen_rgb, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_CENTER, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    }
    scale_for_resolution_inverse( render, &bounds.width, &bounds.height );
    return bounds;
}


pixelfont_bounds_t center_wrap( render_t* render, pixelfont_t* font, string str, int x, int y, int color, int wrap_width ) {
    scale_for_resolution( render, &x, &y );
    scale_for_resolution( render, &wrap_width, NULL );
    pixelfont_bounds_t bounds;
    x -= wrap_width / 2;
    if( render->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, render->screen, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_CENTER, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            &bounds );
    } else {
        pixelfont_blit_rgb( font, x, y, str, render->yarn->assets.palette[ color ], render->screen_rgb, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_CENTER, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            &bounds );
    }
    scale_for_resolution_inverse( render, &bounds.width, &bounds.height );
    return bounds;
}


pixelfont_bounds_t text( render_t* render, pixelfont_t* font, string str, int x, int y, int color ) {
    scale_for_resolution( render, &x, &y );
    pixelfont_bounds_t bounds;
    if( render->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, render->screen, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    } else {
        pixelfont_blit_rgb( font, x, y, str, render->yarn->assets.palette[ color ], render->screen_rgb, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    }
    scale_for_resolution_inverse( render, &bounds.width, &bounds.height );
    return bounds;
}


void wrap( render_t* render, pixelfont_t* font, string str, int x, int y, int color, int wrap_width ) {
    scale_for_resolution( render, &x, &y );
    scale_for_resolution( render, &wrap_width, NULL );
    if( render->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, render->screen, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            NULL );
    } else {
        pixelfont_blit_rgb( font, x, y, str, render->yarn->assets.palette[ color ], render->screen_rgb, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            NULL );
    }
}


void wrap_limit( render_t* render, pixelfont_t* font, string str, int x, int y, int color, int wrap_width, int limit ) {
    scale_for_resolution( render, &x, &y );
    scale_for_resolution( render, &wrap_width, NULL );
    if( render->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, render->screen, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, limit, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF,
            PIXELFONT_UNDERLINE_OFF, NULL );
    } else {
        pixelfont_blit_rgb( font, x, y, str, render->yarn->assets.palette[ color ], render->screen_rgb, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, limit, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF,
            PIXELFONT_UNDERLINE_OFF, NULL );
    }
}


int font_height( render_t* render, int height ) {
    scale_for_resolution_inverse( render, &height, NULL );
    return height;
}


