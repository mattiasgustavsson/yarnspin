

typedef struct render_t {
    yarn_t* yarn;
    uint8_t* screen;
    int screen_width;
    int screen_height;
    uint8_t* screenshot;
    uint32_t* screenshot_rgb;
    int color_background;
    int color_disabled;
    int color_txt;
    int color_opt;
    int color_dialog;
    int color_say;
    int color_response;
    int color_chr;
    int color_use;
    int color_name;
    pixelfont_t* font_txt;
    pixelfont_t* font_opt;
    pixelfont_t* font_dialog;
    pixelfont_t* font_say;
    pixelfont_t* font_response;
    pixelfont_t* font_chr;
    pixelfont_t* font_use;
    pixelfont_t* font_name;
    int* bitmap_width;
    int* bitmap_height;
    GLuint* textures;
    GLuint fontmap_txt;
    GLuint fontmap_opt;
    GLuint fontmap_dialog;
    GLuint fontmap_say;
    GLuint fontmap_response;
    GLuint fontmap_chr;
    GLuint fontmap_use;
    GLuint fontmap_name;
    GLuint frametexture;
    GLuint framebuffer;
    GLuint shader;
    GLuint font_shader;
    GLuint vertexbuffer;
    GLuint white_tex;
    GLuint savegame_tex[10];
} render_t;


bool render_init( render_t* render, yarn_t* yarn, uint8_t* screen, int width, int height ) {
    memset( render, 0, sizeof( *render ) );

    render->yarn = yarn;

    render->screen = screen;
    render->screen_width = width;
    render->screen_height = height;
    render->screenshot = render->screen ? (uint8_t*) manage_alloc( malloc( width * height * sizeof( uint8_t ) ) ) : NULL;
    render->screenshot_rgb = !render->screen ? (uint32_t*) manage_alloc( malloc( width * height * sizeof( uint32_t ) ) ) : NULL;

    render->font_txt = yarn->assets.font_txt;
    render->font_opt = yarn->assets.font_opt;
    render->font_dialog = yarn->assets.font_dialog;
    render->font_say = yarn->assets.font_say;
    render->font_response = yarn->assets.font_response;
    render->font_chr = yarn->assets.font_chr;
    render->font_use = yarn->assets.font_use;
    render->font_name = yarn->assets.font_name;
    render->color_background = yarn->globals.color_background;
    render->color_disabled = yarn->globals.color_disabled;
    render->color_txt = yarn->globals.color_txt;
    render->color_opt = yarn->globals.color_opt;
    render->color_dialog = yarn->globals.color_dialog;
    render->color_say = yarn->globals.color_say;
    render->color_response = yarn->globals.color_response;
    render->color_chr = yarn->globals.color_chr;
    render->color_use = yarn->globals.color_use;
    render->color_name = yarn->globals.color_name;

    if( render->yarn->globals.colormode == YARN_COLORMODE_PALETTE ) {
        int darkest_index = 0;
        int darkest_luma = 65536;
        int brightest_index = 0;
        int brightest_luma = 0;
        int disabled_index = 0;
        int disabled_luma = 65536;

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

        if( render->color_background < 0 ) render->color_background = (uint8_t)darkest_index;
        if( render->color_disabled < 0 ) render->color_disabled = (uint8_t)disabled_index;
        if( render->color_txt < 0 ) render->color_txt = (uint8_t)brightest_index;
        if( render->color_opt < 0 ) render->color_opt = (uint8_t)brightest_index;
        if( render->color_dialog < 0 ) render->color_dialog = (uint8_t)brightest_index;
        if( render->color_response < 0 ) render->color_response = (uint8_t)brightest_index;
        if( render->color_say < 0 ) render->color_say = (uint8_t)brightest_index;
        if( render->color_chr < 0 ) render->color_chr = (uint8_t)brightest_index;
        if( render->color_use < 0 ) render->color_use = (uint8_t)brightest_index;
        if( render->color_name < 0 ) render->color_name = (uint8_t)brightest_index;
    }

    if( render->yarn->globals.colormode != YARN_COLORMODE_PALETTE ) {
        render->textures = (GLuint*)manage_alloc( malloc( sizeof( GLuint ) * render->yarn->assets.bitmaps->count ) );
        render->bitmap_width = (int*)manage_alloc( malloc( sizeof( int ) * render->yarn->assets.bitmaps->count ) );
        render->bitmap_height = (int*)manage_alloc( malloc( sizeof( int ) * render->yarn->assets.bitmaps->count ) );
        for( int i = 0; i < render->yarn->assets.bitmaps->count; ++i ) {
            bool jpeg = render->yarn->globals.colormode == YARN_COLORMODE_RGB && render->yarn->globals.resolution == YARN_RESOLUTION_FULL;
            uint32_t* pixels = NULL;
            int width = 0;
            int height = 0;
            qoi_data_t* qoi = (qoi_data_t*)render->yarn->assets.bitmaps->items[ i ];
            if( !jpeg ) {
                qoi_desc desc;
                pixels = (uint32_t*)qoi_decode( qoi->data, qoi->size, &desc, 4 );
                width = desc.width;
                height = desc.height;
            } else {
                int w, h, c;
                uint32_t* px = (uint32_t*)stbi_load_from_memory( qoi->data, qoi->size, &w, &h, &c, 4 );
                h /= 2;
                pixels = (uint32_t*)malloc( w * h * sizeof( uint32_t ) );
                for( int j = 0; j < w * h; ++j ) {
                    uint32_t c = px[ j ] & 0x00ffffff;
                    uint32_t a = ( px[ w * h + j ] & 0xff ) << 24;
                    pixels[ j ] = c | a;
                }
                free( px );
                width = w;
                height = h;
            }

            render->bitmap_width[ i ] = width;
            render->bitmap_height[ i ] = height;

            glGenTextures( 1, &render->textures[ i ] );
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, render->textures[ i ] );

            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

            glBindTexture( GL_TEXTURE_2D, 0 );
            free( pixels );
        }

        glActiveTexture( GL_TEXTURE0 );

        glGenTextures( 1, &render->fontmap_txt );
        glBindTexture( GL_TEXTURE_2D, render->fontmap_txt );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, ( (bitmapfont_t*)( render->font_txt ) )->width, ( (bitmapfont_t*)( render->font_txt ) )->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ( (bitmapfont_t*)( render->font_txt ) )->pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        render->font_txt->size_in_bytes = render->fontmap_txt;

        glGenTextures( 1, &render->fontmap_opt );
        glBindTexture( GL_TEXTURE_2D, render->fontmap_opt );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, ( (bitmapfont_t*)( render->font_opt ) )->width, ( (bitmapfont_t*)( render->font_opt ) )->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ( (bitmapfont_t*)( render->font_opt ) )->pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        render->font_opt->size_in_bytes = render->fontmap_opt;

        glGenTextures( 1, &render->fontmap_dialog );
        glBindTexture( GL_TEXTURE_2D, render->fontmap_dialog );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, ( (bitmapfont_t*)( render->font_dialog ) )->width, ( (bitmapfont_t*)( render->font_dialog ) )->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ( (bitmapfont_t*)( render->font_dialog ) )->pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        render->font_dialog->size_in_bytes = render->fontmap_dialog;

        glGenTextures( 1, &render->fontmap_say );
        glBindTexture( GL_TEXTURE_2D, render->fontmap_say );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, ( (bitmapfont_t*)( render->font_say ) )->width, ( (bitmapfont_t*)( render->font_say ) )->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ( (bitmapfont_t*)( render->font_say ) )->pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        render->font_say->size_in_bytes = render->fontmap_say;

        glGenTextures( 1, &render->fontmap_response );
        glBindTexture( GL_TEXTURE_2D, render->fontmap_response );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, ( (bitmapfont_t*)( render->font_response ) )->width, ( (bitmapfont_t*)( render->font_response ) )->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ( (bitmapfont_t*)( render->font_response ) )->pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        render->font_response->size_in_bytes = render->fontmap_response;

        glGenTextures( 1, &render->fontmap_chr );
        glBindTexture( GL_TEXTURE_2D, render->fontmap_chr );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, ( (bitmapfont_t*)( render->font_chr ) )->width, ( (bitmapfont_t*)( render->font_chr ) )->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ( (bitmapfont_t*)( render->font_chr ) )->pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        render->font_chr->size_in_bytes = render->fontmap_chr;

        glGenTextures( 1, &render->fontmap_use );
        glBindTexture( GL_TEXTURE_2D, render->fontmap_use );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, ( (bitmapfont_t*)( render->font_use ) )->width, ( (bitmapfont_t*)( render->font_use ) )->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ( (bitmapfont_t*)( render->font_use ) )->pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        render->font_use->size_in_bytes = render->fontmap_use;

        glGenTextures( 1, &render->fontmap_name );
        glBindTexture( GL_TEXTURE_2D, render->fontmap_name );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, ( (bitmapfont_t*)( render->font_name ) )->width, ( (bitmapfont_t*)( render->font_name ) )->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ( (bitmapfont_t*)( render->font_name ) )->pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        render->font_name->size_in_bytes = render->fontmap_name;

        glBindTexture( GL_TEXTURE_2D, 0 );

        glGenTextures( 1, &render->frametexture );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->frametexture );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

        glBindTexture( GL_TEXTURE_2D, 0 );


        glGenFramebuffers( 1, &render->framebuffer );
        glBindFramebuffer( GL_FRAMEBUFFER, render->framebuffer );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render->frametexture, 0 );
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );

        char const* vs_source =
        #ifdef __wasm__
            "precision highp float;\n"
        #else
            "#version 120\n"
        #endif
            "attribute vec4 pos;"
            "varying vec2 uv;"
            ""
            "void main( void ) {"
            "    gl_Position = vec4( pos.xy, 0.0, 1.0 );"
            "    uv = pos.zw;"
            "}"
            ;

        char const* fs_source =
        #ifdef __wasm__
            "precision highp float;\n"
        #else
            "#version 120\n"
        #endif
            "varying vec2 uv;"
            ""
            "uniform sampler2D tex0;"
            "uniform vec4 col;"
            ""
            "void main(void) {"
            "    gl_FragColor = texture2D( tex0, uv ) * col;"
            "}"
            ;

        char const* fs_font_source =
        #ifdef __wasm__
            "precision highp float;\n"
        #else
            "#version 120\n"
        #endif
            "varying vec2 uv;"
            ""
            "uniform sampler2D tex0;"
            "uniform vec4 col;"
            ""
            "void main(void) {"
            "    float a = texture2D( tex0, uv ).r;"
            "    gl_FragColor = vec4( a, a, a, a ) * col;"
            "}"
            ;

        char error_message[ 1024 ];

        {
        GLuint vs = glCreateShader( GL_VERTEX_SHADER );
        glShaderSource( vs, 1, (char const**) &vs_source, NULL );
        glCompileShader( vs );
        GLint vs_compiled;
        glGetShaderiv( vs, GL_COMPILE_STATUS, &vs_compiled );
        if( !vs_compiled ) {
            char const* prefix = "Vertex Shader Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            glGetShaderiv( vs, GL_INFO_LOG_LENGTH, &len );
            glGetShaderInfoLog( vs, (GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            printf( "%s\n", error_message );
            return false;
        }

        GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
        glShaderSource( fs, 1, (char const**) &fs_source, NULL );
        glCompileShader( fs );
        GLint fs_compiled;
        glGetShaderiv( fs, GL_COMPILE_STATUS, &fs_compiled );
        if( !fs_compiled ) {
            char const* prefix = "Fragment Shader Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            glGetShaderiv( vs, GL_INFO_LOG_LENGTH, &len );
            glGetShaderInfoLog( fs, (GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            printf( "%s\n", error_message );
            return false;
        }

        GLuint prg = glCreateProgram();
        glAttachShader( prg, fs );
        glAttachShader( prg, vs );
        glBindAttribLocation( prg, 0, "pos" );
        glLinkProgram( prg );

        GLint linked;
        glGetProgramiv( prg, GL_LINK_STATUS, &linked );
        if( !linked ) {
            char const* prefix = "Shader Link Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            glGetShaderiv( vs, GL_INFO_LOG_LENGTH, &len );
            glGetShaderInfoLog( prg, (GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            printf( "%s\n", error_message );
            return false;
        }

        render->shader = prg;
        }

        {
        GLuint vs = glCreateShader( GL_VERTEX_SHADER );
        glShaderSource( vs, 1, (char const**) &vs_source, NULL );
        glCompileShader( vs );
        GLint vs_compiled;
        glGetShaderiv( vs, GL_COMPILE_STATUS, &vs_compiled );
        if( !vs_compiled ) {
            char const* prefix = "Vertex Shader Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            glGetShaderiv( vs, GL_INFO_LOG_LENGTH, &len );
            glGetShaderInfoLog( vs, (GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            printf( "%s\n", error_message );
            return false;
        }

        GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
        glShaderSource( fs, 1, (char const**) &fs_font_source, NULL );
        glCompileShader( fs );
        GLint fs_compiled;
        glGetShaderiv( fs, GL_COMPILE_STATUS, &fs_compiled );
        if( !fs_compiled ) {
            char const* prefix = "Fragment Shader Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            glGetShaderiv( vs, GL_INFO_LOG_LENGTH, &len );
            glGetShaderInfoLog( fs, (GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            printf( "%s\n", error_message );
            return false;
        }


        GLuint prg = glCreateProgram();
        glAttachShader( prg, fs );
        glAttachShader( prg, vs );
        glBindAttribLocation( prg, 0, "pos" );
        glLinkProgram( prg );

        GLint linked;
        glGetProgramiv( prg, GL_LINK_STATUS, &linked );
        if( !linked ) {
            char const* prefix = "Shader Link Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            glGetShaderiv( vs, GL_INFO_LOG_LENGTH, &len );
            glGetShaderInfoLog( prg, (GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            printf( "%s\n", error_message );
            return false;
        }

        render->font_shader = prg;
        }

        glGenBuffers( 1, &render->vertexbuffer );
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );

        glDisable( GL_DEPTH_TEST );

        glGenTextures( 1, &render->white_tex );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->white_tex );
        uint32_t pixel = 0xffffffff;
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
        glBindTexture( GL_TEXTURE_2D, 0 );

        for( int i = 0; i < 10; ++i ) {
            glGenTextures( 1, &render->savegame_tex[ i ] );
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, render->savegame_tex[ i ] );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            glBindTexture( GL_TEXTURE_2D, 0 );
        }
    }

    return true;
}


void font_blit_rgb( render_t* render, bitmapfont_t const* font, int x, int y, char const* text, uint32_t color,
    int width, int height, pixelfont_align_t align, int wrap_width, int hspacing,
    int vspacing,  int limit, pixelfont_bounds_t* bounds ) {

    static GLfloat vertices[ 6 * 4 * 256 ];
    int numverts = 0;

    float xp = x;
    float yp = y;
    float max_x = x;
    int count = 0;
    char const* str = text;
    while( *str ) {
        int line_char_count = 0;
        float line_width = 0;
        int last_space_char_count = 0;
        float last_space_width = 0;
        char const* tstr = str;
        while( *tstr != '\n' && *tstr != '\0' && ( wrap_width <= 0 || line_width <= wrap_width  ) ) {
            if( *tstr <= ' ' ) {
                last_space_char_count = line_char_count;
                last_space_width = line_width;
            }
            float advance = font->glyphs[ (uint8_t)( *tstr - ' ' ) ].advance;
            line_width += advance;
            line_width += hspacing;
            ++tstr;
            ++line_char_count;
        }

        int skip_space = 0;
        if( wrap_width > 0 && line_width > wrap_width ) {
            if( last_space_char_count > 0 ) line_char_count = last_space_char_count;
            line_width = last_space_width;
            skip_space = 1;
        }

        if( wrap_width > 0 ) {
            if( align == PIXELFONT_ALIGN_RIGHT ) x += wrap_width - line_width;
            if( align == PIXELFONT_ALIGN_CENTER ) x += ( wrap_width - line_width ) / 2;
        } else {
            if( align == PIXELFONT_ALIGN_RIGHT ) x -= line_width;
            if( align == PIXELFONT_ALIGN_CENTER ) x -= line_width / 2;
        }

        for( int c = 0; c < line_char_count; ++c ) {
            if( *str < ' ' ) continue;
            uint8_t idx = (uint8_t)( *str - ' ' );
            float advance = font->glyphs[ idx ].advance;
            float rect_x1 = font->glyphs[ idx ].x1;
            float rect_y1 = font->glyphs[ idx ].y1;
            float rect_x2 = font->glyphs[ idx ].x2;
            float rect_y2 = font->glyphs[ idx ].y2;
            float rect_u1 = font->glyphs[ idx ].u1;
            float rect_v1 = font->glyphs[ idx ].v1;
            float rect_u2 = font->glyphs[ idx ].u2;
            float rect_v2 = font->glyphs[ idx ].v2;
            if( limit < 0 || count < limit ) {
                float x1 = ( x + rect_x1 ) / (float)width;
                float y1 = ( y + rect_y1 ) / (float)height;
                float x2 = ( x + rect_x2 ) / (float)width;
                float y2 = ( y + rect_y2 ) / (float)height;

                float u1 = rect_u1;
                float v1 = rect_v1;
                float u2 = rect_u2;
                float v2 = rect_v2;

                GLfloat verts[] = {
                    2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, u1, v1,
                    2.0f * x2 - 1.0f, 2.0f * y1 - 1.0f, u2, v1,
                    2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, u2, v2,

                    2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, u1, v1,
                    2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, u2, v2,
                    2.0f * x1 - 1.0f, 2.0f * y2 - 1.0f, u1, v2,
                };

                memcpy( vertices + numverts * 6 * 4, verts, 6 * 4 * sizeof( GLfloat ) ) ;
                numverts++;
                if( numverts >= 256 ) {
                    if( render ) {
                        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
                        glEnableVertexAttribArray( 0 );
                        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
                        glBufferData( GL_ARRAY_BUFFER, 6 * 4 * sizeof( GLfloat ) * numverts, vertices, GL_STATIC_DRAW );

                        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
                        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
                        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
                        float b = ( ( color       ) & 0xff ) / 255.0f;
                        glUseProgram( render->font_shader );
                        glActiveTexture( GL_TEXTURE0 );
                        glBindTexture( GL_TEXTURE_2D, (GLuint) font->size_in_bytes );
                        glUniform1i( glGetUniformLocation( render->font_shader, "tex0" ), 0 );
                        glUniform4f( glGetUniformLocation( render->font_shader, "col" ), r, g, b, a );
                        glDrawArrays( GL_TRIANGLES, 0, 6 * numverts );
                        glBindTexture( GL_TEXTURE_2D, 0 );
                    }
                    numverts = 0;
                }
            }

            x += advance;
            x += hspacing;
            ++str;
            ++count;
        }

        max_x = x > max_x ? x : max_x;
        x = xp;
        if( *str ) {
            y += font->line_spacing + vspacing;
        } else {
            y += font->font_height;
        }
        if( *str == '\n' ) ++str;
        if( *str && skip_space && *str <= ' ' ) ++str;
    }

    if( render ) {
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
        glBufferData( GL_ARRAY_BUFFER, 6 * 4 * sizeof( GLfloat ) * numverts, vertices, GL_STATIC_DRAW );

        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glUseProgram( render->font_shader );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, (GLuint) font->size_in_bytes );
        glUniform1i( glGetUniformLocation( render->font_shader, "tex0" ), 0 );
        glUniform4f( glGetUniformLocation( render->font_shader, "col" ), r, g, b, a );
        glDrawArrays( GL_TRIANGLES, 0, 6 * numverts );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }

    if( bounds ) {
        bounds->width = wrap_width > 0 ? wrap_width : ( max_x - xp );
        bounds->height = y - yp;
    }
}


void cls( render_t* render ) {
    if( render->screen ) {
        memset( render->screen, render->color_background, (size_t) render->screen_width * render->screen_height );
    } else {
        uint32_t color = render->color_background;
        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glClearColor( r, g, b, a );
        glClear( GL_COLOR_BUFFER_BIT );
    }
}


void render_new_frame( render_t* render, int hborder, int vborder ) {
    if( !render->screen ) {
        glBindFramebuffer( GL_FRAMEBUFFER, render->framebuffer );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0 );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->frametexture );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, render->screen_width + hborder * 2, render->screen_height + vborder * 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
        glBindTexture( GL_TEXTURE_2D, 0 );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render->frametexture, 0 );
        glViewport( hborder, vborder, render->screen_width, render->screen_height);
        glEnable( GL_BLEND );
        glBlendFuncSeparate( GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO );
    }
}


void render_bind_framebuffer( render_t* render, int width, int height ) {
    if( !render->screen ) {
        glDisable( GL_BLEND );
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        glViewport(0, 0, width, height);
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->frametexture);
    }
}


void render_present( render_t* render, int width, int height ) {
    
    if( render->screen ) {
        return;
    }

    glFlush();
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    glViewport(0, 0, width, height);
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    float view_width = (float)width;
    float view_height = (float)height;

    glDisable( GL_BLEND );

    float x1, y1, x2, y2;

    if( render->yarn->globals.resolution == YARN_RESOLUTION_FULL || render->yarn->globals.resolution == YARN_RESOLUTION_HIGH ) {
        float hscale = view_width / (float) render->screen_width;
        float vscale = view_height / (float) render->screen_height;
        float pixel_scale = hscale < vscale ? hscale : vscale;

        float hborder = ( view_width - pixel_scale * render->screen_width ) / 2.0f;
        float vborder = ( view_height - pixel_scale * render->screen_height ) / 2.0f;
        x1 = hborder;
        y1 = vborder;
        x2 = x1 + pixel_scale * render->screen_width;
        y2 = y1 + pixel_scale * render->screen_height;
    } else {
        int hscale = view_width / render->screen_width;
        int vscale = view_height / render->screen_height;
        int pixel_scale = pixel_scale = hscale < vscale ? hscale : vscale;
        pixel_scale = pixel_scale < 1 ? 1 : pixel_scale;

        int hborder = ( view_width - pixel_scale * render->screen_width ) / 2;
        int vborder = ( view_height - pixel_scale * render->screen_height ) / 2;
        x1 = (float) hborder;
        y1 = (float) vborder;
        x2 = x1 + (float) ( pixel_scale * render->screen_width );
        y2 = y1 + (float) ( pixel_scale * render->screen_height );
    }

    x1 = ( x1 / view_width );
    x2 = ( x2 / view_width );
    y1 = ( y1 / view_height );
    y2 = ( y2 / view_height );

    GLfloat vertices[] = {
        2.0f * x1 - 1.0f, -2.0f * y1 + 1.0f, 0.0f, 0.0f,
        2.0f * x2 - 1.0f, -2.0f * y1 + 1.0f, 1.0f, 0.0f,
        2.0f * x2 - 1.0f, -2.0f * y2 + 1.0f, 1.0f, 1.0f,
        2.0f * x1 - 1.0f, -2.0f * y2 + 1.0f, 0.0f, 1.0f,
    };
    glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
    glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

    uint32_t color = 0xffffffff;
    float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
    float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
    float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
    float b = ( ( color       ) & 0xff ) / 255.0f;
    glUseProgram( render->shader );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, render->frametexture);
    glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
    glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
    glActiveTexture( GL_TEXTURE0 );
    if( render->yarn->globals.resolution == YARN_RESOLUTION_FULL || render->yarn->globals.resolution == YARN_RESOLUTION_HIGH ) {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    } else {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    }
    glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
    glBindTexture( GL_TEXTURE_2D, 0 );    
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
        float width = render->screen_width;
        float height = render->screen_height;
        float x1 = x / width;
        float y1 = y / height;
        float x2 = x1 + render->bitmap_width[ bitmap_index ] / width;
        float y2 = y1 + render->bitmap_height[ bitmap_index ] / height;

        GLfloat vertices[] = {
            2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
            2.0f * x2 - 1.0f, 2.0f * y1 - 1.0f, 1.0f, 0.0f,
            2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
            2.0f * x1 - 1.0f, 2.0f * y2 - 1.0f, 0.0f, 1.0f,
        };
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
        glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

        uint32_t color = 0xffffffff;
        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glUseProgram( render->shader );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->textures[ bitmap_index ] );
        glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
        glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
}


void draw_screenshot( render_t* render ) {
    if( render->screen ) {
        memcpy( render->screen, render->screenshot, sizeof( uint8_t ) * render->screen_width * render->screen_height );
    } else {
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->savegame_tex[ 0 ] );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, render->screen_width, render->screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, render->screenshot_rgb );
        glBindTexture( GL_TEXTURE_2D, 0 );
        float width = render->screen_width;
        float height = render->screen_height;
        float x1 = 0.0f;
        float y1 = 0.0f;
        float x2 = 1.0f;
        float y2 = 1.0f;

        GLfloat vertices[] = {
            2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
            2.0f * x2 - 1.0f, 2.0f * y1 - 1.0f, 1.0f, 0.0f,
            2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
            2.0f * x1 - 1.0f, 2.0f * y2 - 1.0f, 0.0f, 1.0f,
        };
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
        glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

        uint32_t color = 0xffffffff;
        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glUseProgram( render->shader );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->savegame_tex[ 0 ] );
        glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
        glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
}


void draw_savegame_thumb( render_t* render, int x, int y, uint8_t* pixels, int w, int h, int bg, int fg ) {
    scale_for_resolution( render, &x, &y );
    int s = render->screen_width >= 1280 ? 3 : render->screen_width >= 640 ? 2 : 1;
    x -= 2 * s;
    y -= 2 * s;
    w += 4 * s;
    h += 4 * s;
    for( int i = 0; i < 3; ++i ) {
        for( int iy = 0; iy <= h; ++iy ) {
            for( int ix = 0; ix <= w; ++ix ) {
                int xp = x + ix;
                int yp = y + iy;
                if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                    render->screen[ xp + yp * render->screen_width ] = (uint8_t) bg;
                }
            }
        }
        x += 1 * s;
        y += 1 * s;
        w -= 2 * s;
        h -= 2 * s;
        int t = bg;
        bg = fg;
        fg = t;
    }
    x -= 1 * s;
    y -= 1 * s;
    w += 2 * s;
    h += 2 * s;
    for( int i = 0; i < h; ++i ) {
        memcpy( render->screen + x + ( y + i ) * render->screen_width, pixels + i * w, w * sizeof( uint8_t ) );
    }
}


void draw_savegame_bitmap( render_t* render, int x, int y, int savegame_index, int w, int h, int bg, int fg ) {
    scale_for_resolution( render, &x, &y );
    int s = render->screen_width >= 1280 ? 3 : render->screen_width >= 640 ? 2 : 1;
    x -= 2 * s;
    y -= 2 * s;
    w += 4 * s;
    h += 4 * s;
    for( int i = 0; i < 3; ++i ) {
        float width = render->screen_width;
        float height = render->screen_height;
        float x1 = x / width;
        float y1 = y / height;
        float x2 = x1 + w / width;
        float y2 = y1 + h / height;

        GLfloat vertices[] = {
            2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
            2.0f * x2 - 1.0f, 2.0f * y1 - 1.0f, 1.0f, 0.0f,
            2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
            2.0f * x1 - 1.0f, 2.0f * y2 - 1.0f, 0.0f, 1.0f,
        };
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
        glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

        uint32_t color = bg;
        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glUseProgram( render->shader );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->white_tex );
        glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
        glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        glBindTexture( GL_TEXTURE_2D, 0 );
        x += 1 * s;
        y += 1 * s;
        w -= 2 * s;
        h -= 2 * s;
        int t = bg;
        bg = fg;
        fg = t;
    }
    x -= 1 * s;
    y -= 1 * s;
    w += 2 * s;
    h += 2 * s;
    float width = render->screen_width;
    float height = render->screen_height;
    float x1 = x / width;
    float y1 = y / height;
    float x2 = x1 + w / width;
    float y2 = y1 + h / height;

    GLfloat vertices[] = {
        2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
        2.0f * x2 - 1.0f, 2.0f * y1 - 1.0f, 1.0f, 0.0f,
        2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
        2.0f * x1 - 1.0f, 2.0f * y2 - 1.0f, 0.0f, 1.0f,
    };
    glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
    glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

    uint32_t color = 0xffffffff;
    float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
    float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
    float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
    float b = ( ( color       ) & 0xff ) / 255.0f;
    glUseProgram( render->shader );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, render->savegame_tex[ savegame_index ] );
    glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
    glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
    glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
    glBindTexture( GL_TEXTURE_2D, 0 );
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
        float width = render->screen_width;
        float height = render->screen_height;
        float x1 = x / width;
        float y1 = y / height;
        float x2 = x1 + w / width;
        float y2 = y1 + h / height;

        GLfloat vertices[] = {
            2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
            2.0f * x2 - 1.0f, 2.0f * y1 - 1.0f, 1.0f, 0.0f,
            2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
            2.0f * x1 - 1.0f, 2.0f * y2 - 1.0f, 0.0f, 1.0f,
        };
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
        glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

        uint32_t color = c;
        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glUseProgram( render->shader );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->white_tex );
        glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
        glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
}


void frame( render_t* render, int x, int y, int w, int h, int bg, int fg ) {
    scale_for_resolution( render, &x, &y );
    scale_for_resolution( render, &w, &h );
    int s = render->screen_width >= 1280 ? 3 : render->screen_width >= 640 ? 2 : 1;
    x -= 2 * s;
    y -= 2 * s;
    w += 4 * s;
    h += 4 * s;
    if( render->screen ) {
        for( int i = 0; i < 3; ++i ) {
            for( int iy = 0; iy <= h; ++iy ) {
                for( int ix = 0; ix <= w; ++ix ) {
                    int xp = x + ix;
                    int yp = y + iy;
                    if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                        render->screen[ xp + yp * render->screen_width ] = (uint8_t) bg;
                    }
                }
            }
            x += 1 * s;
            y += 1 * s;
            w -= 2 * s;
            h -= 2 * s;
            int t = bg;
            bg = fg;
            fg = t;
        }
    } else {
        for( int i = 0; i < 3; ++i ) {
            float width = render->screen_width;
            float height = render->screen_height;
            float x1 = x / width;
            float y1 = y / height;
            float x2 = x1 + w / width;
            float y2 = y1 + h / height;

            GLfloat vertices[] = {
                2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
                2.0f * x2 - 1.0f, 2.0f * y1 - 1.0f, 1.0f, 0.0f,
                2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
                2.0f * x1 - 1.0f, 2.0f * y2 - 1.0f, 0.0f, 1.0f,
            };
            glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
            glEnableVertexAttribArray( 0 );
            glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
            glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

            uint32_t color = bg;
            float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
            float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
            float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
            float b = ( ( color       ) & 0xff ) / 255.0f;
            glUseProgram( render->shader );
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, render->white_tex );
            glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
            glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
            glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
            glBindTexture( GL_TEXTURE_2D, 0 );
            x += 1 * s;
            y += 1 * s;
            w -= 2 * s;
            h -= 2 * s;
            int t = bg;
            bg = fg;
            fg = t;
        }
    }
}


void frame2( render_t* render, int x, int y, int w, int h, int bg, int fg ) {
    scale_for_resolution( render, &x, &y );
    scale_for_resolution( render, &w, &h );
    int s = render->screen_width >= 1280 ? 6 : render->screen_width >= 640 ? 4 : 2;
    x -= 1 * s + ( 1 * ( s / 2 ) );
    y -= 1 * s + ( 1 * ( s / 2 ) );
    w += 2 * s + ( 2 * ( s / 2 ) );
    h += 2 * s + ( 2 * ( s / 2 ) );
    if( render->screen ) {
        for( int i = 0; i < 3; ++i ) {
            for( int iy = 0; iy <= h; ++iy ) {
                for( int ix = 0; ix <= w; ++ix ) {
                    int xp = x + ix;
                    int yp = y + iy;
                    if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                        render->screen[ xp + yp * render->screen_width ] = (uint8_t) bg;
                    }
                }
            }
            x += 1 * ( i == 0 ? s / 2 : s );
            y += 1 * ( i == 0 ? s / 2 : s );
            w -= 2 * ( i == 0 ? s / 2 : s );
            h -= 2 * ( i == 0 ? s / 2 : s );
            int t = bg;
            bg = fg;
            fg = t;
        }
    } else {
        for( int i = 0; i < 3; ++i ) {
            float width = render->screen_width;
            float height = render->screen_height;
            float x1 = x / width;
            float y1 = y / height;
            float x2 = x1 + w / width;
            float y2 = y1 + h / height;

            GLfloat vertices[] = {
                2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
                2.0f * x2 - 1.0f, 2.0f * y1 - 1.0f, 1.0f, 0.0f,
                2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
                2.0f * x1 - 1.0f, 2.0f * y2 - 1.0f, 0.0f, 1.0f,
            };
            glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
            glEnableVertexAttribArray( 0 );
            glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
            glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

            uint32_t color = bg;
            float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
            float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
            float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
            float b = ( ( color       ) & 0xff ) / 255.0f;
            glUseProgram( render->shader );
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, render->white_tex );
            glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
            glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
            glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
            glBindTexture( GL_TEXTURE_2D, 0 );
            x += 1 * ( i == 0 ? s / 2 : s );
            y += 1 * ( i == 0 ? s / 2 : s );
            w -= 2 * ( i == 0 ? s / 2 : s );
            h -= 2 * ( i == 0 ? s / 2 : s );
            int t = bg;
            bg = fg;
            fg = t;
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
        float width = render->screen_width;
        float height = render->screen_height;
        float x1 = ( x - 1 ) / width;
        float y1 = y / height;
        float x2 = x1 + ( s + 1 ) / width;
        float y2 = y1;
        float x3 = x1 + ( ( s + 1 ) / 2.0f ) / width;
        float y3 = y1 + h / height;
        float x4 = x1 + ( 1 + ( s + 1 ) / 2.0f ) / width;
        float y4 = y1 + h / height;

        GLfloat vertices[] = {
            2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
            2.0f * x3 - 1.0f, 2.0f * y3 - 1.0f, 1.0f, 0.0f,
            2.0f * x4 - 1.0f, 2.0f * y4 - 1.0f, 1.0f, 1.0f,
            2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
        };
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
        glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

        uint32_t color = c;
        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glUseProgram( render->shader );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->white_tex );
        glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
        glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
}


void cancel_icon( render_t* render, int x, int y, int c ) {
    scale_for_resolution( render, &x, &y );
    int s = 8;
    int t = ( render->screen_width >= 1280 ? 5 : render->screen_width >= 480 ? 2 : 1 );
    scale_for_resolution( render, &s, NULL );
    int h = s;
    if( render->screen ) {
        for( int iy = 0; iy < h; ++iy ) {
            for( int ix = 0; ix <= t; ++ix ) {
                int xp = x + iy + ix;
                int yp = y + iy;
                if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                    render->screen[ xp + yp * render->screen_width ] = (uint8_t) c;
                }
                xp = x + s - iy + ix - 1;
                yp = y + iy;
                if( xp >= 0 && xp < render->screen_width && yp >= 0 && yp < render->screen_height ) {
                    render->screen[ xp + yp * render->screen_width ] = (uint8_t) c;
                }
            }
        }
    } else {
        t += 1;
        {
        float width = render->screen_width;
        float height = render->screen_height;
        float x1 = x / width;
        float y1 = y / height;
        float x2 = x1 + t / width;
        float y2 = y1;
        float x3 = x1 + s / width;
        float y3 = y1 + s / height;
        float x4 = x3 + t / width;
        float y4 = y3;

        GLfloat vertices[] = {
            2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
            2.0f * x3 - 1.0f, 2.0f * y3 - 1.0f, 1.0f, 0.0f,
            2.0f * x4 - 1.0f, 2.0f * y4 - 1.0f, 1.0f, 1.0f,
            2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
        };
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
        glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

        uint32_t color = c;
        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glUseProgram( render->shader );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->white_tex );
        glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
        glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        glBindTexture( GL_TEXTURE_2D, 0 );
        }
        {
        float width = render->screen_width;
        float height = render->screen_height;
        float x1 = ( x + s ) / width;
        float y1 = y / height;
        float x2 = x1 + t / width;
        float y2 = y1;
        float x3 = x1 - s / width;
        float y3 = y1 + s / height;
        float x4 = x3 + t / width;
        float y4 = y3;

        GLfloat vertices[] = {
            2.0f * x1 - 1.0f, 2.0f * y1 - 1.0f, 0.0f, 0.0f,
            2.0f * x3 - 1.0f, 2.0f * y3 - 1.0f, 1.0f, 0.0f,
            2.0f * x4 - 1.0f, 2.0f * y4 - 1.0f, 1.0f, 1.0f,
            2.0f * x2 - 1.0f, 2.0f * y2 - 1.0f, 1.0f, 1.0f,
        };
        glBindBuffer( GL_ARRAY_BUFFER, render->vertexbuffer );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( GLfloat ), 0 );
        glBufferData( GL_ARRAY_BUFFER, 4 * 4 * sizeof( GLfloat ), vertices, GL_STATIC_DRAW );

        uint32_t color = c;
        float a = ( ( color >> 24 ) & 0xff ) / 255.0f;
        float r = ( ( color >> 16 ) & 0xff ) / 255.0f;
        float g = ( ( color >> 8  ) & 0xff ) / 255.0f;
        float b = ( ( color       ) & 0xff ) / 255.0f;
        glUseProgram( render->shader );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, render->white_tex );
        glUniform1i( glGetUniformLocation( render->shader, "tex0" ), 0 );
        glUniform4f( glGetUniformLocation( render->shader, "col" ), r, g, b, a );
        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        glBindTexture( GL_TEXTURE_2D, 0 );
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
        font_blit_rgb( render, (bitmapfont_t*)font, x, y, str, color, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_CENTER, 0, 0, 0, -1, &bounds );
    }
    scale_for_resolution_inverse( render, &bounds.width, &bounds.height );
    return bounds;
}


pixelfont_bounds_t center_limit( render_t* render, pixelfont_t* font, string str, int x, int y, int color, int limit ) {
    scale_for_resolution( render, &x, &y );
    pixelfont_bounds_t bounds;
    if( render->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, render->screen, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_CENTER, 0, 0, 0, limit, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    } else {
        font_blit_rgb( render, (bitmapfont_t*)font, x, y, str, color, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_CENTER, 0, 0, 0, limit, &bounds );
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
        font_blit_rgb( render, (bitmapfont_t*)font, x, y, str, color, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_CENTER, wrap_width, 0, 0, -1, 
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
        font_blit_rgb( render, (bitmapfont_t*)font, x, y, str, color, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, &bounds );
    }
    scale_for_resolution_inverse( render, &bounds.width, &bounds.height );
    return bounds;
}


void wrap( render_t* render, pixelfont_t* font, string str, int x, int y, int color, int wrap_width ) {
    if( cstr_starts( str, "You are standing" ) ) {
        render = render;
    }
    scale_for_resolution( render, &x, &y );
    scale_for_resolution( render, &wrap_width, NULL );
    if( render->screen ) {
        pixelfont_blit( font, x, y, str, (uint8_t)color, render->screen, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF,
            NULL );
    } else {
        font_blit_rgb( render, (bitmapfont_t*)font, x, y, str, color, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, -1, 
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
        font_blit_rgb( render, (bitmapfont_t*)font, x, y, str, color, render->screen_width, render->screen_height,
            PIXELFONT_ALIGN_LEFT, wrap_width, 0, 0, limit, NULL );
    }
}

pixelfont_bounds_t text_bounds( render_t* render, pixelfont_t* font, string str ) {
    pixelfont_bounds_t bounds;
    if( render->screen ) {
        pixelfont_blit( font, 0, 0, str, 0, NULL, 0, 0,
            PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, PIXELFONT_BOLD_OFF, PIXELFONT_ITALIC_OFF, PIXELFONT_UNDERLINE_OFF, &bounds );
    } else {
        font_blit_rgb( NULL, (bitmapfont_t*)font, 0, 0, str, 0, 0, 0,
            PIXELFONT_ALIGN_LEFT, 0, 0, 0, -1, &bounds );
    }
    scale_for_resolution_inverse( render, &bounds.width, &bounds.height );
    return bounds;
}



int font_height( render_t* render, int height ) {
    scale_for_resolution_inverse( render, &height, NULL );
    return height;
}


uint32_t* make_thumbnail_rgb( uint32_t* screenshot_rgb, int screen_width, int screen_height, int thumb_width, int thumb_height ) {
    uint32_t* thumb = (uint32_t*) malloc( thumb_width * thumb_height * sizeof( uint32_t ) * 2 );
    memset( thumb, 0, thumb_width * thumb_height * sizeof( uint32_t ) * 2 );

    stbir_resize_uint8( (unsigned char*) screenshot_rgb, screen_width, screen_height, screen_width * 4, 
        (unsigned char*)( thumb + thumb_width * thumb_height ), thumb_width, thumb_height, thumb_width * 4, 4 );
    
    float filter[ 9 ] = {
        -1.0f, -1.0f, -1.0f,
        -1.0f,  9.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
    };
    for( int y = 1; y < thumb_height - 1; ++y ) {
        for( int x = 1; x < thumb_width - 1; ++x ) {
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            for( int fx = -1; fx <= 1; ++fx ) {
                for( int fy = -1; fy <= 1; ++fy ) {
                    uint32_t c = thumb[ thumb_width * thumb_height + x + fx + ( y + fy ) * thumb_width ];
                    r += ( filter[ ( 1 + fx ) + ( 1 + fy ) * 3 ] ) * ( ( c & 0xff ) / 255.0f );
                    g += ( filter[ ( 1 + fx ) + ( 1 + fy ) * 3 ] ) * ( ( ( c >> 8 ) & 0xff ) / 255.0f );
                    b += ( filter[ ( 1 + fx ) + ( 1 + fy ) * 3 ] ) * ( ( ( c >> 16 ) & 0xff ) / 255.0f );
                }
            }
            uint32_t c = thumb[ thumb_width * thumb_height + x + y * thumb_width ];
            r = r * 0.25f + 0.75f * ( ( c & 0xff ) / 255.0f );
            g = g * 0.25f + 0.75f * ( ( ( c >> 8 ) & 0xff ) / 255.0f );
            b = b * 0.25f + 0.75f * ( ( ( c >> 16 ) & 0xff ) / 255.0f );
            r = r < 0.0f ? 0.0f : r > 1.0f ? 1.0f : r;
            g = g < 0.0f ? 0.0f : g > 1.0f ? 1.0f : g;
            b = b < 0.0f ? 0.0f : b > 1.0f ? 1.0f : b;
            uint32_t col = ( ( uint32_t)( r * 255.0f ) ) | ( ( ( uint32_t)( g * 255.0f ) ) << 8 ) | ( ( ( uint32_t)( b * 255.0f ) ) << 16 );
            thumb[ x + y * thumb_width ] = col | 0xff000000;
        }
    }
    return thumb;
}


uint8_t* make_thumbnail( uint8_t* screenshot, int screen_width, int screen_height, int thumb_width, int thumb_height ) {
    uint8_t* thumb = (uint8_t*) malloc( thumb_width * thumb_height * sizeof( uint8_t ) );
    memset( thumb, 0, thumb_width * thumb_height * sizeof( uint8_t ) );
    for( int y = 0; y < thumb_height; ++y ) {
        for( int x = 0; x < thumb_width; ++x ) {
            int fx = ( x * ( screen_width / (float) thumb_width ) );
            int fy = ( y * ( screen_height / (float) thumb_height ) );
            thumb[ x + y * thumb_width ] = screenshot[ fx + fy * screen_width ];
        }
    }
    return thumb;
}


void generate_savegame_texture( render_t* render, uint32_t* thumb_rgb, int thumb_width, int thumb_height, int savegame_index ) {
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, render->savegame_tex[ savegame_index ] );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, thumb_width, thumb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, thumb_rgb );
    glBindTexture( GL_TEXTURE_2D, 0 );
}
