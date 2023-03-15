/*
------------------------------------------------------------------------------
          Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

crtemu_lite.h - v0.1 - Cathode ray tube emulation shader for C/C++.

Do this:
    #define CRTEMU_LITE_IMPLEMENTATION
before you include this file in *one* C/C++ file to create the implementation.
*/


#ifndef crtemu_lite_h
#define crtemu_lite_h

#ifndef CRTEMU_LITE_U32
    #define CRTEMU_LITE_U32 unsigned int
#endif
#ifndef CRTEMU_LITE_U64
    #define CRTEMU_LITE_U64 unsigned long long
#endif

typedef struct crtemu_lite_t crtemu_lite_t;

crtemu_lite_t* crtemu_lite_create( void* memctx );

void crtemu_lite_destroy( crtemu_lite_t* crtemu_lite );

typedef struct crtemu_lite_config_t
    {
    float curvature, scanlines, shadow_mask, separation, ghosting, noise, flicker, vignette, distortion, aspect_lock,
        hpos, vpos, hsize, vsize, contrast, brightness, saturation, blur, degauss; // range -1.0f to 1.0f, default=0.0f
    } crtemu_lite_config_t;

void crtemu_lite_config(crtemu_lite_t* crtemu_lite, crtemu_lite_config_t const* config);

void crtemu_lite_frame( crtemu_lite_t* crtemu_lite, CRTEMU_LITE_U32* frame_abgr, int frame_width, int frame_height );

void crtemu_lite_present( crtemu_lite_t* crtemu_lite, CRTEMU_LITE_U64 time_us, CRTEMU_LITE_U32 const* pixels_xbgr, int width, int height,
    CRTEMU_LITE_U32 mod_xbgr, CRTEMU_LITE_U32 border_xbgr );

void crtemu_lite_coordinates_window_to_bitmap( crtemu_lite_t* crtemu_lite, int width, int height, int* x, int* y );
void crtemu_lite_coordinates_bitmap_to_window( crtemu_lite_t* crtemu_lite, int width, int height, int* x, int* y );

#endif /* crtemu_lite_h */

/*
----------------------
    IMPLEMENTATION
----------------------
*/
#ifdef CRTEMU_LITE_IMPLEMENTATION
#undef CRTEMU_LITE_IMPLEMENTATION

#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#include <stddef.h>
#include <string.h>

#ifndef CRTEMU_LITE_MALLOC
    #include <stdlib.h>
    #if defined(__cplusplus)
        #define CRTEMU_LITE_MALLOC( ctx, size ) ( ::malloc( size ) )
        #define CRTEMU_LITE_FREE( ctx, ptr ) ( ::free( ptr ) )
    #else
        #define CRTEMU_LITE_MALLOC( ctx, size ) ( malloc( size ) )
        #define CRTEMU_LITE_FREE( ctx, ptr ) ( free( ptr ) )
    #endif
#endif

#ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
    #ifndef CRTEMU_LITE_REPORT_ERROR
        #define _CRT_NONSTDC_NO_DEPRECATE
        #define _CRT_SECURE_NO_WARNINGS
        #include <stdio.h>
        #define CRTEMU_LITE_REPORT_ERROR( str ) printf( "%s", str )
    #endif
#endif

#ifndef _WIN32
    #define CRTEMU_LITE_SDL
#endif

#ifdef __wasm__
    #define CRTEMU_LITE_WEBGL
#endif

#ifndef CRTEMU_LITE_SDL

    #ifdef __cplusplus
    extern "C" {
    #endif

        __declspec(dllimport) struct HINSTANCE__* __stdcall LoadLibraryA( char const* lpLibFileName );
        __declspec(dllimport) int __stdcall FreeLibrary( struct HINSTANCE__* hModule );
        #if defined(_WIN64)
            typedef __int64 (__stdcall* CRTEMU_LITE_PROC)( void );
            __declspec(dllimport) CRTEMU_LITE_PROC __stdcall GetProcAddress( struct HINSTANCE__* hModule, char const* lpLibFileName );
        #else
            typedef __int32 (__stdcall* CRTEMU_LITE_PROC)( void );
            __declspec(dllimport) CRTEMU_LITE_PROC __stdcall GetProcAddress( struct HINSTANCE__* hModule, char const* lpLibFileName );
        #endif

    #ifdef __cplusplus
        }
    #endif

    #define CRTEMU_LITE_GLCALLTYPE __stdcall
    typedef unsigned int CRTEMU_LITE_GLuint;
    typedef int CRTEMU_LITE_GLsizei;
    typedef unsigned int CRTEMU_LITE_GLenum;
    typedef int CRTEMU_LITE_GLint;
    typedef float CRTEMU_LITE_GLfloat;
    typedef char CRTEMU_LITE_GLchar;
    typedef unsigned char CRTEMU_LITE_GLboolean;
    typedef size_t CRTEMU_LITE_GLsizeiptr;
    typedef unsigned int CRTEMU_LITE_GLbitfield;

    #define CRTEMU_LITE_GL_FLOAT 0x1406
    #define CRTEMU_LITE_GL_FALSE 0
    #define CRTEMU_LITE_GL_FRAGMENT_SHADER 0x8b30
    #define CRTEMU_LITE_GL_VERTEX_SHADER 0x8b31
    #define CRTEMU_LITE_GL_COMPILE_STATUS 0x8b81
    #define CRTEMU_LITE_GL_LINK_STATUS 0x8b82
    #define CRTEMU_LITE_GL_INFO_LOG_LENGTH 0x8b84
    #define CRTEMU_LITE_GL_ARRAY_BUFFER 0x8892
    #define CRTEMU_LITE_GL_TEXTURE_2D 0x0de1
    #define CRTEMU_LITE_GL_TEXTURE0 0x84c0
    #define CRTEMU_LITE_GL_TEXTURE1 0x84c1
    #define CRTEMU_LITE_GL_TEXTURE2 0x84c2
    #define CRTEMU_LITE_GL_TEXTURE3 0x84c3
    #define CRTEMU_LITE_GL_TEXTURE_MIN_FILTER 0x2801
    #define CRTEMU_LITE_GL_TEXTURE_MAG_FILTER 0x2800
    #define CRTEMU_LITE_GL_NEAREST 0x2600
    #define CRTEMU_LITE_GL_LINEAR 0x2601
    #define CRTEMU_LITE_GL_STATIC_DRAW 0x88e4
    #define CRTEMU_LITE_GL_RGBA 0x1908
    #define CRTEMU_LITE_GL_UNSIGNED_BYTE 0x1401
    #define CRTEMU_LITE_GL_COLOR_BUFFER_BIT 0x00004000
    #define CRTEMU_LITE_GL_TRIANGLE_FAN 0x0006
    #define CRTEMU_LITE_GL_FRAMEBUFFER 0x8d40
    #define CRTEMU_LITE_GL_VIEWPORT 0x0ba2
    #define CRTEMU_LITE_GL_RGB 0x1907
    #define CRTEMU_LITE_GL_COLOR_ATTACHMENT0 0x8ce0
    #define CRTEMU_LITE_GL_TEXTURE_WRAP_S 0x2802
    #define CRTEMU_LITE_GL_TEXTURE_WRAP_T 0x2803
    #define CRTEMU_LITE_GL_CLAMP_TO_BORDER 0x812D
    #define CRTEMU_LITE_GL_TEXTURE_BORDER_COLOR 0x1004

#else

    #ifndef CRTEMU_LITE_WEBGL
        #include <GL/glew.h>
        #include "SDL_opengl.h"
    #else
        #include <wajic_gl.h>
    #endif
    #define CRTEMU_LITE_GLCALLTYPE GLAPIENTRY

    typedef GLuint CRTEMU_LITE_GLuint;
    typedef GLsizei CRTEMU_LITE_GLsizei;
    typedef GLenum CRTEMU_LITE_GLenum;
    typedef GLint CRTEMU_LITE_GLint;
    typedef GLfloat CRTEMU_LITE_GLfloat;
    typedef GLchar CRTEMU_LITE_GLchar;
    typedef GLboolean CRTEMU_LITE_GLboolean;
    typedef GLsizeiptr CRTEMU_LITE_GLsizeiptr;
    typedef GLbitfield CRTEMU_LITE_GLbitfield;

     #define CRTEMU_LITE_GL_FLOAT GL_FLOAT
     #define CRTEMU_LITE_GL_FALSE GL_FALSE
     #define CRTEMU_LITE_GL_FRAGMENT_SHADER GL_FRAGMENT_SHADER
     #define CRTEMU_LITE_GL_VERTEX_SHADER GL_VERTEX_SHADER
     #define CRTEMU_LITE_GL_COMPILE_STATUS GL_COMPILE_STATUS
     #define CRTEMU_LITE_GL_LINK_STATUS GL_LINK_STATUS
     #define CRTEMU_LITE_GL_INFO_LOG_LENGTH GL_INFO_LOG_LENGTH
     #define CRTEMU_LITE_GL_ARRAY_BUFFER GL_ARRAY_BUFFER
     #define CRTEMU_LITE_GL_TEXTURE_2D GL_TEXTURE_2D
     #define CRTEMU_LITE_GL_TEXTURE0 GL_TEXTURE0
     #define CRTEMU_LITE_GL_TEXTURE1 GL_TEXTURE1
     #define CRTEMU_LITE_GL_TEXTURE2 GL_TEXTURE2
     #define CRTEMU_LITE_GL_TEXTURE3 GL_TEXTURE3
     #define CRTEMU_LITE_GL_TEXTURE_MIN_FILTER GL_TEXTURE_MIN_FILTER
     #define CRTEMU_LITE_GL_TEXTURE_MAG_FILTER GL_TEXTURE_MAG_FILTER
     #define CRTEMU_LITE_GL_NEAREST GL_NEAREST
     #define CRTEMU_LITE_GL_LINEAR GL_LINEAR
     #define CRTEMU_LITE_GL_STATIC_DRAW GL_STATIC_DRAW
     #define CRTEMU_LITE_GL_RGBA GL_RGBA
     #define CRTEMU_LITE_GL_UNSIGNED_BYTE GL_UNSIGNED_BYTE
     #define CRTEMU_LITE_GL_COLOR_BUFFER_BIT GL_COLOR_BUFFER_BIT
     #define CRTEMU_LITE_GL_TRIANGLE_FAN GL_TRIANGLE_FAN
     #define CRTEMU_LITE_GL_FRAMEBUFFER GL_FRAMEBUFFER
     #define CRTEMU_LITE_GL_VIEWPORT GL_VIEWPORT
     #define CRTEMU_LITE_GL_RGB GL_RGB
     #define CRTEMU_LITE_GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0
     #define CRTEMU_LITE_GL_TEXTURE_WRAP_S GL_TEXTURE_WRAP_S
     #define CRTEMU_LITE_GL_TEXTURE_WRAP_T GL_TEXTURE_WRAP_T
     #ifndef CRTEMU_LITE_WEBGL
         #define CRTEMU_LITE_GL_CLAMP_TO_BORDER GL_CLAMP_TO_BORDER
         #define CRTEMU_LITE_GL_TEXTURE_BORDER_COLOR GL_TEXTURE_BORDER_COLOR
     #else
         // WebGL does not support GL_CLAMP_TO_BORDER, we have to emulate
         // this behavior with code in the fragment shader
         #define CRTEMU_LITE_GL_CLAMP_TO_BORDER GL_CLAMP_TO_EDGE
     #endif
#endif


struct crtemu_lite_t
    {
    void* memctx;
    crtemu_lite_config_t config;

    CRTEMU_LITE_GLuint vertexbuffer;
    CRTEMU_LITE_GLuint backbuffer;

    CRTEMU_LITE_GLuint accumulatetexture_a;
    CRTEMU_LITE_GLuint accumulatetexture_b;
    CRTEMU_LITE_GLuint accumulatebuffer_a;
    CRTEMU_LITE_GLuint accumulatebuffer_b;

    CRTEMU_LITE_GLuint blurtexture_a;
    CRTEMU_LITE_GLuint blurtexture_b;
    CRTEMU_LITE_GLuint blurbuffer_a;
    CRTEMU_LITE_GLuint blurbuffer_b;

    CRTEMU_LITE_GLuint frametexture;
    float use_frame;

    CRTEMU_LITE_GLuint crt_shader;
    CRTEMU_LITE_GLuint blur_shader;
    CRTEMU_LITE_GLuint accumulate_shader;
    CRTEMU_LITE_GLuint blend_shader;
    CRTEMU_LITE_GLuint copy_shader;

    int last_present_width;
    int last_present_height;


    #ifndef CRTEMU_LITE_SDL
        struct HINSTANCE__* gl_dll;
        CRTEMU_LITE_PROC (CRTEMU_LITE_GLCALLTYPE *wglGetProcAddress) (char const* );
    #endif

    void (CRTEMU_LITE_GLCALLTYPE* TexParameterfv) (CRTEMU_LITE_GLenum target, CRTEMU_LITE_GLenum pname, CRTEMU_LITE_GLfloat const* params);
    void (CRTEMU_LITE_GLCALLTYPE* DeleteFramebuffers) (CRTEMU_LITE_GLsizei n, CRTEMU_LITE_GLuint const* framebuffers);
    void (CRTEMU_LITE_GLCALLTYPE* GetIntegerv) (CRTEMU_LITE_GLenum pname, CRTEMU_LITE_GLint *data);
    void (CRTEMU_LITE_GLCALLTYPE* GenFramebuffers) (CRTEMU_LITE_GLsizei n, CRTEMU_LITE_GLuint *framebuffers);
    void (CRTEMU_LITE_GLCALLTYPE* BindFramebuffer) (CRTEMU_LITE_GLenum target, CRTEMU_LITE_GLuint framebuffer);
    void (CRTEMU_LITE_GLCALLTYPE* Uniform1f) (CRTEMU_LITE_GLint location, CRTEMU_LITE_GLfloat v0);
    void (CRTEMU_LITE_GLCALLTYPE* Uniform2f) (CRTEMU_LITE_GLint location, CRTEMU_LITE_GLfloat v0, CRTEMU_LITE_GLfloat v1);
    void (CRTEMU_LITE_GLCALLTYPE* FramebufferTexture2D) (CRTEMU_LITE_GLenum target, CRTEMU_LITE_GLenum attachment, CRTEMU_LITE_GLenum textarget, CRTEMU_LITE_GLuint texture, CRTEMU_LITE_GLint level);
    CRTEMU_LITE_GLuint (CRTEMU_LITE_GLCALLTYPE* CreateShader) (CRTEMU_LITE_GLenum type);
    void (CRTEMU_LITE_GLCALLTYPE* ShaderSource) (CRTEMU_LITE_GLuint shader, CRTEMU_LITE_GLsizei count, CRTEMU_LITE_GLchar const* const* string, CRTEMU_LITE_GLint const* length);
    void (CRTEMU_LITE_GLCALLTYPE* CompileShader) (CRTEMU_LITE_GLuint shader);
    void (CRTEMU_LITE_GLCALLTYPE* GetShaderiv) (CRTEMU_LITE_GLuint shader, CRTEMU_LITE_GLenum pname, CRTEMU_LITE_GLint *params);
    CRTEMU_LITE_GLuint (CRTEMU_LITE_GLCALLTYPE* CreateProgram) (void);
    void (CRTEMU_LITE_GLCALLTYPE* AttachShader) (CRTEMU_LITE_GLuint program, CRTEMU_LITE_GLuint shader);
    void (CRTEMU_LITE_GLCALLTYPE* BindAttribLocation) (CRTEMU_LITE_GLuint program, CRTEMU_LITE_GLuint index, CRTEMU_LITE_GLchar const* name);
    void (CRTEMU_LITE_GLCALLTYPE* LinkProgram) (CRTEMU_LITE_GLuint program);
    void (CRTEMU_LITE_GLCALLTYPE* GetProgramiv) (CRTEMU_LITE_GLuint program, CRTEMU_LITE_GLenum pname, CRTEMU_LITE_GLint *params);
    void (CRTEMU_LITE_GLCALLTYPE* GenBuffers) (CRTEMU_LITE_GLsizei n, CRTEMU_LITE_GLuint *buffers);
    void (CRTEMU_LITE_GLCALLTYPE* BindBuffer) (CRTEMU_LITE_GLenum target, CRTEMU_LITE_GLuint buffer);
    void (CRTEMU_LITE_GLCALLTYPE* EnableVertexAttribArray) (CRTEMU_LITE_GLuint index);
    void (CRTEMU_LITE_GLCALLTYPE* VertexAttribPointer) (CRTEMU_LITE_GLuint index, CRTEMU_LITE_GLint size, CRTEMU_LITE_GLenum type, CRTEMU_LITE_GLboolean normalized, CRTEMU_LITE_GLsizei stride, void const* pointer);
    void (CRTEMU_LITE_GLCALLTYPE* GenTextures) (CRTEMU_LITE_GLsizei n, CRTEMU_LITE_GLuint* textures);
    void (CRTEMU_LITE_GLCALLTYPE* Enable) (CRTEMU_LITE_GLenum cap);
    void (CRTEMU_LITE_GLCALLTYPE* ActiveTexture) (CRTEMU_LITE_GLenum texture);
    void (CRTEMU_LITE_GLCALLTYPE* BindTexture) (CRTEMU_LITE_GLenum target, CRTEMU_LITE_GLuint texture);
    void (CRTEMU_LITE_GLCALLTYPE* TexParameteri) (CRTEMU_LITE_GLenum target, CRTEMU_LITE_GLenum pname, CRTEMU_LITE_GLint param);
    void (CRTEMU_LITE_GLCALLTYPE* DeleteBuffers) (CRTEMU_LITE_GLsizei n, CRTEMU_LITE_GLuint const* buffers);
    void (CRTEMU_LITE_GLCALLTYPE* DeleteTextures) (CRTEMU_LITE_GLsizei n, CRTEMU_LITE_GLuint const* textures);
    void (CRTEMU_LITE_GLCALLTYPE* BufferData) (CRTEMU_LITE_GLenum target, CRTEMU_LITE_GLsizeiptr size, void const *data, CRTEMU_LITE_GLenum usage);
    void (CRTEMU_LITE_GLCALLTYPE* UseProgram) (CRTEMU_LITE_GLuint program);
    void (CRTEMU_LITE_GLCALLTYPE* Uniform1i) (CRTEMU_LITE_GLint location, CRTEMU_LITE_GLint v0);
    void (CRTEMU_LITE_GLCALLTYPE* Uniform3f) (CRTEMU_LITE_GLint location, CRTEMU_LITE_GLfloat v0, CRTEMU_LITE_GLfloat v1, CRTEMU_LITE_GLfloat v2);
    CRTEMU_LITE_GLint (CRTEMU_LITE_GLCALLTYPE* GetUniformLocation) (CRTEMU_LITE_GLuint program, CRTEMU_LITE_GLchar const* name);
    void (CRTEMU_LITE_GLCALLTYPE* TexImage2D) (CRTEMU_LITE_GLenum target, CRTEMU_LITE_GLint level, CRTEMU_LITE_GLint internalformat, CRTEMU_LITE_GLsizei width, CRTEMU_LITE_GLsizei height, CRTEMU_LITE_GLint border, CRTEMU_LITE_GLenum format, CRTEMU_LITE_GLenum type, void const* pixels);
    void (CRTEMU_LITE_GLCALLTYPE* ClearColor) (CRTEMU_LITE_GLfloat red, CRTEMU_LITE_GLfloat green, CRTEMU_LITE_GLfloat blue, CRTEMU_LITE_GLfloat alpha);
    void (CRTEMU_LITE_GLCALLTYPE* Clear) (CRTEMU_LITE_GLbitfield mask);
    void (CRTEMU_LITE_GLCALLTYPE* DrawArrays) (CRTEMU_LITE_GLenum mode, CRTEMU_LITE_GLint first, CRTEMU_LITE_GLsizei count);
    void (CRTEMU_LITE_GLCALLTYPE* Viewport) (CRTEMU_LITE_GLint x, CRTEMU_LITE_GLint y, CRTEMU_LITE_GLsizei width, CRTEMU_LITE_GLsizei height);
    void (CRTEMU_LITE_GLCALLTYPE* DeleteShader) (CRTEMU_LITE_GLuint shader);
    void (CRTEMU_LITE_GLCALLTYPE* DeleteProgram) (CRTEMU_LITE_GLuint program);
    #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
        void (CRTEMU_LITE_GLCALLTYPE* GetShaderInfoLog) (CRTEMU_LITE_GLuint shader, CRTEMU_LITE_GLsizei bufSize, CRTEMU_LITE_GLsizei *length, CRTEMU_LITE_GLchar *infoLog);
    #endif
    };


static CRTEMU_LITE_GLuint crtemu_lite_internal_build_shader( crtemu_lite_t* crtemu_lite, char const* vs_source, char const* fs_source )
    {
    #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
        char error_message[ 1024 ];
    #endif

    CRTEMU_LITE_GLuint vs = crtemu_lite->CreateShader( CRTEMU_LITE_GL_VERTEX_SHADER );
    crtemu_lite->ShaderSource( vs, 1, (char const**) &vs_source, NULL );
    crtemu_lite->CompileShader( vs );
    CRTEMU_LITE_GLint vs_compiled;
    crtemu_lite->GetShaderiv( vs, CRTEMU_LITE_GL_COMPILE_STATUS, &vs_compiled );
    if( !vs_compiled )
        {
        #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
            char const* prefix = "Vertex Shader Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            crtemu_lite->GetShaderiv( vs, CRTEMU_LITE_GL_INFO_LOG_LENGTH, &len );
            crtemu_lite->GetShaderInfoLog( vs, (CRTEMU_LITE_GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            CRTEMU_LITE_REPORT_ERROR( error_message );
        #endif
        return 0;
        }

    CRTEMU_LITE_GLuint fs = crtemu_lite->CreateShader( CRTEMU_LITE_GL_FRAGMENT_SHADER );
    crtemu_lite->ShaderSource( fs, 1, (char const**) &fs_source, NULL );
    crtemu_lite->CompileShader( fs );
    CRTEMU_LITE_GLint fs_compiled;
    crtemu_lite->GetShaderiv( fs, CRTEMU_LITE_GL_COMPILE_STATUS, &fs_compiled );
    if( !fs_compiled )
        {
        #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
            char const* prefix = "Fragment Shader Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            crtemu_lite->GetShaderiv( vs, CRTEMU_LITE_GL_INFO_LOG_LENGTH, &len );
            crtemu_lite->GetShaderInfoLog( fs, (CRTEMU_LITE_GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            CRTEMU_LITE_REPORT_ERROR( error_message );
        #endif
        return 0;
        }


    CRTEMU_LITE_GLuint prg = crtemu_lite->CreateProgram();
    crtemu_lite->AttachShader( prg, fs );
    crtemu_lite->AttachShader( prg, vs );
    crtemu_lite->BindAttribLocation( prg, 0, "pos" );
    crtemu_lite->LinkProgram( prg );

    CRTEMU_LITE_GLint linked;
    crtemu_lite->GetProgramiv( prg, CRTEMU_LITE_GL_LINK_STATUS, &linked );
    if( !linked )
        {
        #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
            char const* prefix = "Shader Link Error: ";
            strcpy( error_message, prefix );
            int len = 0, written = 0;
            crtemu_lite->GetShaderiv( vs, CRTEMU_LITE_GL_INFO_LOG_LENGTH, &len );
            crtemu_lite->GetShaderInfoLog( prg, (CRTEMU_LITE_GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written,
                error_message + strlen( prefix ) );
            CRTEMU_LITE_REPORT_ERROR( error_message );
        #endif
        return 0;
        }

    return prg;
    }


crtemu_lite_t* crtemu_lite_create( void* memctx )
    {

    char const* vs_source =
        #ifdef CRTEMU_LITE_WEBGL
            "precision highp float;\n\n"
        #else
            "#version 120\n\n"
        #endif
        ""
        "attribute vec4 pos;"
        "varying vec2 uv;"
        ""
        "void main( void )"
        "    {"
        "    gl_Position = vec4( pos.xy, 0.0, 1.0 );"
        "    uv = pos.zw;"
        "    }";

    char const* crt_fs_source =
        #ifdef CRTEMU_LITE_WEBGL
            "precision highp float;\n\n"
        #else
            "#version 120\n\n"
        #endif
        "\n"
        "varying vec2 uv;\n"
        "\n"
        "uniform vec3 modulate;\n"
        "uniform vec2 resolution;\n"
        "uniform vec2 size;\n"
        "uniform float time;\n"
        "uniform sampler2D backbuffer;\n"
        "uniform sampler2D blurbuffer;\n"
        "uniform sampler2D frametexture;\n"
        "uniform float use_frame;\n"
        "\n"
       /* #ifdef CRTEMU_LITE_WEBGL
            // WebGL does not support GL_CLAMP_TO_BORDER so we overwrite texture2D
            // with this function which emulates the clamp-to-border behavior
            "vec4 texture2Dborder(sampler2D samp, vec2 tc)\n"
            "    {\n"
            "    float borderdist = .502-max(abs(.5-tc.x), abs(.5-tc.y));\n"
            "    float borderfade = clamp(borderdist * 400.0, 0.0, 1.0);\n"
            "    return texture2D( samp, tc ) * borderfade;\n"
            "    }\n"
            "#define texture2D texture2Dborder\n"
        #endif*/
        "vec3 tsample( sampler2D samp, vec2 tc )\n"
        "    {\n"
        "    vec3 s = pow( abs( texture2D( samp, vec2( tc.x, 1.0-tc.y ) ).rgb), vec3( 2.2 ) );\n"
        "    return s;\n"
        "    }\n"
        "\n"
        "vec3 filmic( vec3 LinearColor )\n"
        "    {\n"
        "    vec3 x = max( vec3(0.0), LinearColor-vec3(0.004));\n"
        "    return (x*(6.2*x+0.5))/(x*(6.2*x+1.7)+0.06);\n"
        "    }\n"
        "\n"
        "void main(void)\n"
        "   {\n"
        "    // Main color\n"
        "    vec3 col;\n"
        "    col = mix( tsample(backbuffer,uv ), tsample(frametexture,uv ), 0.45 );\n"
        "    col = 3.0*col + pow( col, vec3( 3.0 ) );\n"
        "    col += tsample(blurbuffer,uv )*0.3;\n"
        "\n"
        "    // Scanlines\n"
        "    float scans = clamp( 0.5-0.5*cos( uv.y * 6.28319 * (size.y ) ), 0.0, 1.0);\n"
        "    float s = pow(scans,1.3);\n"
        "    col = mix( col, col * vec3(s), 0.7 );\n"
        "\n"
        "    // Vertical lines (shadow mask)\n"
        "    col*=1.0-0.23*(clamp((mod(gl_FragCoord.xy.x, 3.0))/2.0,0.0,1.0));\n"
        "\n"
        "    // Vignette\n"
        "    float vig = (0.1 + 1.0*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y));\n"
        "    vig = 1.3*pow(vig,0.5);\n"
        "    col = mix( col, col*vig, 0.2 );\n"
        "\n"
        "    // Tone map\n"
        "    col = mix( pow( col, vec3(1.0 / 2.2) ), filmic( col ), 0.5 );\n"
        "\n"
        "    col*=modulate; \n"
        "    gl_FragColor = vec4( col, 1.0 );\n"
        "   }\n"
        "   \n"
        "";

    char const* blur_fs_source =
        #ifdef CRTEMU_LITE_WEBGL
            "precision highp float;\n\n"
        #else
            "#version 120\n\n"
        #endif
        ""
        "varying vec2 uv;"
        ""
        "uniform vec2 blur;"
        "uniform sampler2D texture;"
        ""
        "void main( void )"
        "    {"
        "    vec4 sum = texture2D( texture, uv ) * 0.2270270270;"
        "    sum += texture2D(texture, vec2( uv.x - 4.0 * blur.x, uv.y - 4.0 * blur.y ) ) * 0.0162162162;"
        "    sum += texture2D(texture, vec2( uv.x - 3.0 * blur.x, uv.y - 3.0 * blur.y ) ) * 0.0540540541;"
        "    sum += texture2D(texture, vec2( uv.x - 2.0 * blur.x, uv.y - 2.0 * blur.y ) ) * 0.1216216216;"
        "    sum += texture2D(texture, vec2( uv.x - 1.0 * blur.x, uv.y - 1.0 * blur.y ) ) * 0.1945945946;"
        "    sum += texture2D(texture, vec2( uv.x + 1.0 * blur.x, uv.y + 1.0 * blur.y ) ) * 0.1945945946;"
        "    sum += texture2D(texture, vec2( uv.x + 2.0 * blur.x, uv.y + 2.0 * blur.y ) ) * 0.1216216216;"
        "    sum += texture2D(texture, vec2( uv.x + 3.0 * blur.x, uv.y + 3.0 * blur.y ) ) * 0.0540540541;"
        "    sum += texture2D(texture, vec2( uv.x + 4.0 * blur.x, uv.y + 4.0 * blur.y ) ) * 0.0162162162;"
        "    gl_FragColor = sum;"
        "    }   "
        "";


    char const* accumulate_fs_source =
        #ifdef CRTEMU_LITE_WEBGL
            "precision highp float;\n\n"
        #else
            "#version 120\n\n"
        #endif
        ""
        "varying vec2 uv;"
        ""
        "uniform sampler2D tex0;"
        "uniform sampler2D tex1;"
        "uniform float modulate;"
        ""
        "void main( void )"
        "    {"
        "    vec4 a = texture2D( tex0, uv ) * vec4( modulate );"
        "    vec4 b = texture2D( tex1, uv );"
        ""
        "    gl_FragColor = max( a, b * 0.96 );"
        "    }   "
        "";

    char const* blend_fs_source =
        #ifdef CRTEMU_LITE_WEBGL
            "precision highp float;\n\n"
        #else
            "#version 120\n\n"
        #endif
        ""
        "varying vec2 uv;"
        ""
        "uniform sampler2D tex0;"
        "uniform sampler2D tex1;"
        "uniform float modulate;"
        ""
        "void main( void )"
        "    {"
        "    vec4 a = texture2D( tex0, uv ) * vec4( modulate );"
        "    vec4 b = texture2D( tex1, uv );"
        ""
        "    gl_FragColor = max( a, b * 0.2 );"
        "    }   "
        "";

    char const* copy_fs_source =
        #ifdef CRTEMU_LITE_WEBGL
            "precision highp float;\n\n"
        #else
            "#version 120\n\n"
        #endif
        ""
        "varying vec2 uv;"
        ""
        "uniform sampler2D tex0;"
        ""
        "void main( void )"
        "    {"
        "    gl_FragColor = texture2D( tex0, uv );"
        "    }   "
        "";

    crtemu_lite_t* crtemu_lite = (crtemu_lite_t*) CRTEMU_LITE_MALLOC( memctx, sizeof( crtemu_lite_t ) );
    memset( crtemu_lite, 0, sizeof( crtemu_lite_t ) );
    crtemu_lite->memctx = memctx;

    crtemu_lite->config.curvature = 0.0f;
    crtemu_lite->config.scanlines = 0.0f;
    crtemu_lite->config.shadow_mask = 0.0f;
    crtemu_lite->config.separation = 0.0f;
    crtemu_lite->config.ghosting = 0.0f;
    crtemu_lite->config.noise = 0.0f;
    crtemu_lite->config.flicker = 0.0f;
    crtemu_lite->config.vignette = 0.0f;
    crtemu_lite->config.distortion = 0.0f;
    crtemu_lite->config.aspect_lock = 0.0f;
    crtemu_lite->config.hpos = 0.0f;
    crtemu_lite->config.vpos = 0.0f;
    crtemu_lite->config.hsize = 0.0f;
    crtemu_lite->config.vsize = 0.0f;
    crtemu_lite->config.contrast = 0.0f;
    crtemu_lite->config.brightness = 0.0f;
    crtemu_lite->config.saturation = 0.0f;
    crtemu_lite->config.blur = 0.0f;
    crtemu_lite->config.degauss = 0.0f;

    crtemu_lite->use_frame = 0.0f;

    crtemu_lite->last_present_width = 0;
    crtemu_lite->last_present_height = 0;

    #ifndef CRTEMU_LITE_SDL

        crtemu_lite->gl_dll = LoadLibraryA( "opengl32.dll" );
        if( !crtemu_lite->gl_dll ) goto failed;

        crtemu_lite->wglGetProcAddress = (CRTEMU_LITE_PROC (CRTEMU_LITE_GLCALLTYPE*)(char const*)) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "wglGetProcAddress" );
        if( !crtemu_lite->gl_dll ) goto failed;

        // Attempt to bind opengl functions using GetProcAddress
        crtemu_lite->TexParameterfv = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLfloat const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glTexParameterfv" );
        crtemu_lite->DeleteFramebuffers = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glDeleteFramebuffers" );
        crtemu_lite->GetIntegerv = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint *) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glGetIntegerv" );
        crtemu_lite->GenFramebuffers = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint *) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glGenFramebuffers" );
        crtemu_lite->BindFramebuffer = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glBindFramebuffer" );
        crtemu_lite->Uniform1f = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLfloat) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glUniform1f" );
        crtemu_lite->Uniform2f = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glUniform2f" );
        crtemu_lite->FramebufferTexture2D = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLuint, CRTEMU_LITE_GLint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glFramebufferTexture2D" );
        crtemu_lite->CreateShader = ( CRTEMU_LITE_GLuint (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glCreateShader" );
        crtemu_lite->ShaderSource = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLchar const* const*, CRTEMU_LITE_GLint const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glShaderSource" );
        crtemu_lite->CompileShader = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glCompileShader" );
        crtemu_lite->GetShaderiv = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glGetShaderiv" );
        crtemu_lite->CreateProgram = ( CRTEMU_LITE_GLuint (CRTEMU_LITE_GLCALLTYPE*) (void) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glCreateProgram" );
        crtemu_lite->AttachShader = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glAttachShader" );
        crtemu_lite->BindAttribLocation = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLuint, CRTEMU_LITE_GLchar const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glBindAttribLocation" );
        crtemu_lite->LinkProgram = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glLinkProgram" );
        crtemu_lite->GetProgramiv = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glGetProgramiv" );
        crtemu_lite->GenBuffers = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glGenBuffers" );
        crtemu_lite->BindBuffer = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glBindBuffer" );
        crtemu_lite->EnableVertexAttribArray = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glEnableVertexAttribArray" );
        crtemu_lite->VertexAttribPointer = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLint, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLboolean, CRTEMU_LITE_GLsizei, void const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glVertexAttribPointer" );
        crtemu_lite->GenTextures = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glGenTextures" );
        crtemu_lite->Enable = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glEnable" );
        crtemu_lite->ActiveTexture = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glActiveTexture" );
        crtemu_lite->BindTexture = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glBindTexture" );
        crtemu_lite->TexParameteri = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glTexParameteri" );
        crtemu_lite->DeleteBuffers = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glDeleteBuffers" );
        crtemu_lite->DeleteTextures = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glDeleteTextures" );
        crtemu_lite->BufferData = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLsizeiptr, void const *, CRTEMU_LITE_GLenum) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glBufferData" );
        crtemu_lite->UseProgram = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glUseProgram" );
        crtemu_lite->Uniform1i = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glUniform1i" );
        crtemu_lite->Uniform3f = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glUniform3f" );
        crtemu_lite->GetUniformLocation = ( CRTEMU_LITE_GLint (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLchar const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glGetUniformLocation" );
        crtemu_lite->TexImage2D = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint, CRTEMU_LITE_GLint, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLint, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, void const*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glTexImage2D" );
        crtemu_lite->ClearColor = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glClearColor" );
        crtemu_lite->Clear = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLbitfield) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glClear" );
        crtemu_lite->DrawArrays = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint, CRTEMU_LITE_GLsizei) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glDrawArrays" );
        crtemu_lite->Viewport = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLint, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLsizei) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glViewport" );
        crtemu_lite->DeleteShader = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glDeleteShader" );
        crtemu_lite->DeleteProgram = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glDeleteProgram" );
        #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
            crtemu_lite->GetShaderInfoLog = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLsizei*, CRTEMU_LITE_GLchar*) ) (uintptr_t) GetProcAddress( crtemu_lite->gl_dll, "glGetShaderInfoLog" );
        #endif

        // Any opengl functions which didn't bind, try binding them using wglGetProcAddrss
        if( !crtemu_lite->TexParameterfv ) crtemu_lite->TexParameterfv = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLfloat const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glTexParameterfv" );
        if( !crtemu_lite->DeleteFramebuffers ) crtemu_lite->DeleteFramebuffers = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glDeleteFramebuffers" );
        if( !crtemu_lite->GetIntegerv ) crtemu_lite->GetIntegerv = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint *) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glGetIntegerv" );
        if( !crtemu_lite->GenFramebuffers ) crtemu_lite->GenFramebuffers = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint *) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glGenFramebuffers" );
        if( !crtemu_lite->BindFramebuffer ) crtemu_lite->BindFramebuffer = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glBindFramebuffer" );
        if( !crtemu_lite->Uniform1f ) crtemu_lite->Uniform1f = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLfloat) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glUniform1f" );
        if( !crtemu_lite->Uniform2f ) crtemu_lite->Uniform2f = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glUniform2f" );
        if( !crtemu_lite->FramebufferTexture2D ) crtemu_lite->FramebufferTexture2D = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLuint, CRTEMU_LITE_GLint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glFramebufferTexture2D" );
        if( !crtemu_lite->CreateShader ) crtemu_lite->CreateShader = ( CRTEMU_LITE_GLuint (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glCreateShader" );
        if( !crtemu_lite->ShaderSource ) crtemu_lite->ShaderSource = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLchar const* const*, CRTEMU_LITE_GLint const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glShaderSource" );
        if( !crtemu_lite->CompileShader ) crtemu_lite->CompileShader = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glCompileShader" );
        if( !crtemu_lite->GetShaderiv ) crtemu_lite->GetShaderiv = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glGetShaderiv" );
        if( !crtemu_lite->CreateProgram ) crtemu_lite->CreateProgram = ( CRTEMU_LITE_GLuint (CRTEMU_LITE_GLCALLTYPE*) (void) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glCreateProgram" );
        if( !crtemu_lite->AttachShader ) crtemu_lite->AttachShader = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glAttachShader" );
        if( !crtemu_lite->BindAttribLocation ) crtemu_lite->BindAttribLocation = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLuint, CRTEMU_LITE_GLchar const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glBindAttribLocation" );
        if( !crtemu_lite->LinkProgram ) crtemu_lite->LinkProgram = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glLinkProgram" );
        if( !crtemu_lite->GetProgramiv ) crtemu_lite->GetProgramiv = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glGetProgramiv" );
        if( !crtemu_lite->GenBuffers ) crtemu_lite->GenBuffers = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glGenBuffers" );
        if( !crtemu_lite->BindBuffer ) crtemu_lite->BindBuffer = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glBindBuffer" );
        if( !crtemu_lite->EnableVertexAttribArray ) crtemu_lite->EnableVertexAttribArray = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glEnableVertexAttribArray" );
        if( !crtemu_lite->VertexAttribPointer ) crtemu_lite->VertexAttribPointer = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLint, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLboolean, CRTEMU_LITE_GLsizei, void const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glVertexAttribPointer" );
        if( !crtemu_lite->GenTextures ) crtemu_lite->GenTextures = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glGenTextures" );
        if( !crtemu_lite->Enable ) crtemu_lite->Enable = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glEnable" );
        if( !crtemu_lite->ActiveTexture ) crtemu_lite->ActiveTexture = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glActiveTexture" );
        if( !crtemu_lite->BindTexture ) crtemu_lite->BindTexture = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glBindTexture" );
        if( !crtemu_lite->TexParameteri ) crtemu_lite->TexParameteri = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glTexParameteri" );
        if( !crtemu_lite->DeleteBuffers ) crtemu_lite->DeleteBuffers = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glDeleteBuffers" );
        if( !crtemu_lite->DeleteTextures ) crtemu_lite->DeleteTextures = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLuint const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glDeleteTextures" );
        if( !crtemu_lite->BufferData ) crtemu_lite->BufferData = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLsizeiptr, void const *, CRTEMU_LITE_GLenum) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glBufferData" );
        if( !crtemu_lite->UseProgram ) crtemu_lite->UseProgram = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glUseProgram" );
        if( !crtemu_lite->Uniform1i ) crtemu_lite->Uniform1i = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glUniform1i" );
        if( !crtemu_lite->Uniform3f ) crtemu_lite->Uniform3f = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glUniform3f" );
        if( !crtemu_lite->GetUniformLocation ) crtemu_lite->GetUniformLocation = ( CRTEMU_LITE_GLint (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLchar const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glGetUniformLocation" );
        if( !crtemu_lite->TexImage2D ) crtemu_lite->TexImage2D = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint, CRTEMU_LITE_GLint, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLint, CRTEMU_LITE_GLenum, CRTEMU_LITE_GLenum, void const*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glTexImage2D" );
        if( !crtemu_lite->ClearColor ) crtemu_lite->ClearColor = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat, CRTEMU_LITE_GLfloat) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glClearColor" );
        if( !crtemu_lite->Clear ) crtemu_lite->Clear = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLbitfield) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glClear" );
        if( !crtemu_lite->DrawArrays ) crtemu_lite->DrawArrays = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLenum, CRTEMU_LITE_GLint, CRTEMU_LITE_GLsizei) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glDrawArrays" );
        if( !crtemu_lite->Viewport ) crtemu_lite->Viewport = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLint, CRTEMU_LITE_GLint, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLsizei) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glViewport" );
        if( !crtemu_lite->DeleteShader ) crtemu_lite->DeleteShader = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glDeleteShader" );
        if( !crtemu_lite->DeleteProgram ) crtemu_lite->DeleteProgram = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glDeleteProgram" );
        #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
            if( !crtemu_lite->GetShaderInfoLog ) crtemu_lite->GetShaderInfoLog = ( void (CRTEMU_LITE_GLCALLTYPE*) (CRTEMU_LITE_GLuint, CRTEMU_LITE_GLsizei, CRTEMU_LITE_GLsizei*, CRTEMU_LITE_GLchar*) ) (uintptr_t) crtemu_lite->wglGetProcAddress( "glGetShaderInfoLog" );
        #endif

    #else

         crtemu_lite->TexParameterfv = glTexParameterfv;
         crtemu_lite->DeleteFramebuffers = glDeleteFramebuffers;
         crtemu_lite->GetIntegerv = glGetIntegerv;
         crtemu_lite->GenFramebuffers = glGenFramebuffers;
         crtemu_lite->BindFramebuffer = glBindFramebuffer;
         crtemu_lite->Uniform1f = glUniform1f;
         crtemu_lite->Uniform2f = glUniform2f;
         crtemu_lite->FramebufferTexture2D = glFramebufferTexture2D;
         crtemu_lite->CreateShader = glCreateShader;
         crtemu_lite->ShaderSource = glShaderSource;
         crtemu_lite->CompileShader = glCompileShader;
         crtemu_lite->GetShaderiv = glGetShaderiv;
         crtemu_lite->CreateProgram = glCreateProgram;
         crtemu_lite->AttachShader = glAttachShader;
         crtemu_lite->BindAttribLocation = glBindAttribLocation;
         crtemu_lite->LinkProgram = glLinkProgram;
         crtemu_lite->GetProgramiv = glGetProgramiv;
         crtemu_lite->GenBuffers = glGenBuffers;
         crtemu_lite->BindBuffer = glBindBuffer;
         crtemu_lite->EnableVertexAttribArray = glEnableVertexAttribArray;
         crtemu_lite->VertexAttribPointer = glVertexAttribPointer;
         crtemu_lite->GenTextures = glGenTextures;
         crtemu_lite->Enable = glEnable;
         crtemu_lite->ActiveTexture = glActiveTexture;
         crtemu_lite->BindTexture = glBindTexture;
         crtemu_lite->TexParameteri = glTexParameteri;
         crtemu_lite->DeleteBuffers = glDeleteBuffers;
         crtemu_lite->DeleteTextures = glDeleteTextures;
         crtemu_lite->BufferData = glBufferData;
         crtemu_lite->UseProgram = glUseProgram;
         crtemu_lite->Uniform1i = glUniform1i;
         crtemu_lite->Uniform3f = glUniform3f;
         crtemu_lite->GetUniformLocation = glGetUniformLocation;
         crtemu_lite->TexImage2D = glTexImage2D;
         crtemu_lite->ClearColor = glClearColor;
         crtemu_lite->Clear = glClear;
         crtemu_lite->DrawArrays = glDrawArrays;
         crtemu_lite->Viewport = glViewport;
         crtemu_lite->DeleteShader = glDeleteShader;
         crtemu_lite->DeleteProgram = glDeleteProgram;
         #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
            crtemu_lite->GetShaderInfoLog = glGetShaderInfoLog;
         #endif

    #endif

    // Report error if any gl function was not found.
    if( !crtemu_lite->TexParameterfv ) goto failed;
    if( !crtemu_lite->DeleteFramebuffers ) goto failed;
    if( !crtemu_lite->GetIntegerv ) goto failed;
    if( !crtemu_lite->GenFramebuffers ) goto failed;
    if( !crtemu_lite->BindFramebuffer ) goto failed;
    if( !crtemu_lite->Uniform1f ) goto failed;
    if( !crtemu_lite->Uniform2f ) goto failed;
    if( !crtemu_lite->FramebufferTexture2D ) goto failed;
    if( !crtemu_lite->CreateShader ) goto failed;
    if( !crtemu_lite->ShaderSource ) goto failed;
    if( !crtemu_lite->CompileShader ) goto failed;
    if( !crtemu_lite->GetShaderiv ) goto failed;
    if( !crtemu_lite->CreateProgram ) goto failed;
    if( !crtemu_lite->AttachShader ) goto failed;
    if( !crtemu_lite->BindAttribLocation ) goto failed;
    if( !crtemu_lite->LinkProgram ) goto failed;
    if( !crtemu_lite->GetProgramiv ) goto failed;
    if( !crtemu_lite->GenBuffers ) goto failed;
    if( !crtemu_lite->BindBuffer ) goto failed;
    if( !crtemu_lite->EnableVertexAttribArray ) goto failed;
    if( !crtemu_lite->VertexAttribPointer ) goto failed;
    if( !crtemu_lite->GenTextures ) goto failed;
    if( !crtemu_lite->Enable ) goto failed;
    if( !crtemu_lite->ActiveTexture ) goto failed;
    if( !crtemu_lite->BindTexture ) goto failed;
    if( !crtemu_lite->TexParameteri ) goto failed;
    if( !crtemu_lite->DeleteBuffers ) goto failed;
    if( !crtemu_lite->DeleteTextures ) goto failed;
    if( !crtemu_lite->BufferData ) goto failed;
    if( !crtemu_lite->UseProgram ) goto failed;
    if( !crtemu_lite->Uniform1i ) goto failed;
    if( !crtemu_lite->Uniform3f ) goto failed;
    if( !crtemu_lite->GetUniformLocation ) goto failed;
    if( !crtemu_lite->TexImage2D ) goto failed;
    if( !crtemu_lite->ClearColor ) goto failed;
    if( !crtemu_lite->Clear ) goto failed;
    if( !crtemu_lite->DrawArrays ) goto failed;
    if( !crtemu_lite->Viewport ) goto failed;
    if( !crtemu_lite->DeleteShader ) goto failed;
    if( !crtemu_lite->DeleteProgram ) goto failed;
    #ifdef CRTEMU_LITE_REPORT_SHADER_ERRORS
        if( !crtemu_lite->GetShaderInfoLog ) goto failed;
    #endif

    crtemu_lite->crt_shader = crtemu_lite_internal_build_shader( crtemu_lite, vs_source, crt_fs_source );
    if( crtemu_lite->crt_shader == 0 ) goto failed;

    crtemu_lite->blur_shader = crtemu_lite_internal_build_shader( crtemu_lite, vs_source, blur_fs_source );
    if( crtemu_lite->blur_shader == 0 ) goto failed;

    crtemu_lite->accumulate_shader = crtemu_lite_internal_build_shader( crtemu_lite, vs_source, accumulate_fs_source );
    if( crtemu_lite->accumulate_shader == 0 ) goto failed;

    crtemu_lite->blend_shader = crtemu_lite_internal_build_shader( crtemu_lite, vs_source, blend_fs_source );
    if( crtemu_lite->blend_shader == 0 ) goto failed;

    crtemu_lite->copy_shader = crtemu_lite_internal_build_shader( crtemu_lite, vs_source, copy_fs_source );
    if( crtemu_lite->copy_shader == 0 ) goto failed;

    crtemu_lite->GenTextures( 1, &crtemu_lite->accumulatetexture_a );
    crtemu_lite->GenFramebuffers( 1, &crtemu_lite->accumulatebuffer_a );

    crtemu_lite->GenTextures( 1, &crtemu_lite->accumulatetexture_b );
    crtemu_lite->GenFramebuffers( 1, &crtemu_lite->accumulatebuffer_b );

    crtemu_lite->GenTextures( 1, &crtemu_lite->blurtexture_a );
    crtemu_lite->GenFramebuffers( 1, &crtemu_lite->blurbuffer_a );

    crtemu_lite->GenTextures( 1, &crtemu_lite->blurtexture_b );
    crtemu_lite->GenFramebuffers( 1, &crtemu_lite->blurbuffer_b );

    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );

    crtemu_lite->GenTextures( 1, &crtemu_lite->frametexture );
    #ifndef CRTEMU_LITE_WEBGL
        // This enable call is not necessary when using fragment shaders, avoid logged warnings in WebGL
        crtemu_lite->Enable( CRTEMU_LITE_GL_TEXTURE_2D );
    #endif
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE2 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->frametexture );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_LINEAR );

    crtemu_lite->GenTextures( 1, &crtemu_lite->backbuffer );
    #ifndef CRTEMU_LITE_WEBGL
        // This enable call is not necessary when using fragment shaders, avoid logged warnings in WebGL
        crtemu_lite->Enable( CRTEMU_LITE_GL_TEXTURE_2D );
    #endif
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->backbuffer );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_NEAREST );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_NEAREST );

    crtemu_lite->GenBuffers( 1, &crtemu_lite->vertexbuffer );
    crtemu_lite->BindBuffer( CRTEMU_LITE_GL_ARRAY_BUFFER, crtemu_lite->vertexbuffer );
    crtemu_lite->EnableVertexAttribArray( 0 );
    crtemu_lite->VertexAttribPointer( 0, 4, CRTEMU_LITE_GL_FLOAT, CRTEMU_LITE_GL_FALSE, 4 * sizeof( CRTEMU_LITE_GLfloat ), 0 );

    #ifdef CRTEMU_LITE_WEBGL
        // Avoid WebGL error "TEXTURE_2D at unit 0 is incomplete: Non-power-of-two textures must have a wrap mode of CLAMP_TO_EDGE."
        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_a );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_b );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->blurtexture_a );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->blurtexture_b );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->frametexture );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->backbuffer );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    #endif

    return crtemu_lite;

failed:
    if( crtemu_lite->accumulatetexture_a ) crtemu_lite->DeleteTextures( 1, &crtemu_lite->accumulatetexture_a );
    if( crtemu_lite->accumulatebuffer_a ) crtemu_lite->DeleteFramebuffers( 1, &crtemu_lite->accumulatebuffer_a );
    if( crtemu_lite->accumulatetexture_b ) crtemu_lite->DeleteTextures( 1, &crtemu_lite->accumulatetexture_b );
    if( crtemu_lite->accumulatebuffer_b ) crtemu_lite->DeleteFramebuffers( 1, &crtemu_lite->accumulatebuffer_b );
    if( crtemu_lite->blurtexture_a ) crtemu_lite->DeleteTextures( 1, &crtemu_lite->blurtexture_a );
    if( crtemu_lite->blurbuffer_a ) crtemu_lite->DeleteFramebuffers( 1, &crtemu_lite->blurbuffer_a );
    if( crtemu_lite->blurtexture_b ) crtemu_lite->DeleteTextures( 1, &crtemu_lite->blurtexture_b );
    if( crtemu_lite->blurbuffer_b ) crtemu_lite->DeleteFramebuffers( 1, &crtemu_lite->blurbuffer_b );
    if( crtemu_lite->frametexture ) crtemu_lite->DeleteTextures( 1, &crtemu_lite->frametexture );
    if( crtemu_lite->backbuffer ) crtemu_lite->DeleteTextures( 1, &crtemu_lite->backbuffer );
    if( crtemu_lite->vertexbuffer ) crtemu_lite->DeleteBuffers( 1, &crtemu_lite->vertexbuffer );

    #ifndef CRTEMU_LITE_SDL
        if( crtemu_lite->gl_dll ) FreeLibrary( crtemu_lite->gl_dll );
    #endif
    CRTEMU_LITE_FREE( crtemu_lite->memctx, crtemu_lite );
    return 0;
    }


void crtemu_lite_destroy( crtemu_lite_t* crtemu_lite )
    {
    crtemu_lite->DeleteTextures( 1, &crtemu_lite->accumulatetexture_a );
    crtemu_lite->DeleteFramebuffers( 1, &crtemu_lite->accumulatebuffer_a );
    crtemu_lite->DeleteTextures( 1, &crtemu_lite->accumulatetexture_b );
    crtemu_lite->DeleteFramebuffers( 1, &crtemu_lite->accumulatebuffer_b );
    crtemu_lite->DeleteTextures( 1, &crtemu_lite->blurtexture_a );
    crtemu_lite->DeleteFramebuffers( 1, &crtemu_lite->blurbuffer_a );
    crtemu_lite->DeleteTextures( 1, &crtemu_lite->blurtexture_b );
    crtemu_lite->DeleteFramebuffers( 1, &crtemu_lite->blurbuffer_b );
    crtemu_lite->DeleteTextures( 1, &crtemu_lite->frametexture );
    crtemu_lite->DeleteTextures( 1, &crtemu_lite->backbuffer );
    crtemu_lite->DeleteBuffers( 1, &crtemu_lite->vertexbuffer );
    #ifndef CRTEMU_LITE_SDL
        FreeLibrary( crtemu_lite->gl_dll );
    #endif
    CRTEMU_LITE_FREE( crtemu_lite->memctx, crtemu_lite );
    }


void crtemu_lite_config(crtemu_lite_t* crtemu_lite, crtemu_lite_config_t const* config)
    {
    crtemu_lite->config = *config;
    }


void crtemu_lite_frame( crtemu_lite_t* crtemu_lite, CRTEMU_LITE_U32* frame_abgr, int frame_width, int frame_height )
    {
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE3 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->frametexture );
    crtemu_lite->TexImage2D( CRTEMU_LITE_GL_TEXTURE_2D, 0, CRTEMU_LITE_GL_RGBA, frame_width, frame_height, 0, CRTEMU_LITE_GL_RGBA, CRTEMU_LITE_GL_UNSIGNED_BYTE, frame_abgr );
    if( frame_abgr )
        crtemu_lite->use_frame = 1.0f;
    else
        crtemu_lite->use_frame = 0.0f;
    }


static void crtemu_lite_internal_blur( crtemu_lite_t* crtemu_lite, CRTEMU_LITE_GLuint source, CRTEMU_LITE_GLuint blurbuffer_a, CRTEMU_LITE_GLuint blurbuffer_b,
    CRTEMU_LITE_GLuint blurtexture_b, float r, int width, int height )
    {
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, blurbuffer_b );
    crtemu_lite->UseProgram( crtemu_lite->blur_shader );
    crtemu_lite->Uniform2f( crtemu_lite->GetUniformLocation( crtemu_lite->blur_shader, "blur" ), r / (float) width, 0 );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->blur_shader, "texture" ), 0 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, source );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->DrawArrays( CRTEMU_LITE_GL_TRIANGLE_FAN, 0, 4 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );

    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, blurbuffer_a );
    crtemu_lite->UseProgram( crtemu_lite->blur_shader );
    crtemu_lite->Uniform2f( crtemu_lite->GetUniformLocation( crtemu_lite->blur_shader, "blur" ), 0, r / (float) height );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->blur_shader, "texture" ), 0 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, blurtexture_b );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->DrawArrays( CRTEMU_LITE_GL_TRIANGLE_FAN, 0, 4 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );
    }


void crtemu_lite_present( crtemu_lite_t* crtemu_lite, CRTEMU_LITE_U64 time_us, CRTEMU_LITE_U32 const* pixels_xbgr, int width, int height,
    CRTEMU_LITE_U32 mod_xbgr, CRTEMU_LITE_U32 border_xbgr )
    {
    int viewport[ 4 ];
    crtemu_lite->GetIntegerv( CRTEMU_LITE_GL_VIEWPORT, viewport );

    if( width != crtemu_lite->last_present_width || height != crtemu_lite->last_present_height )
        {
        crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );

        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_a );
        crtemu_lite->TexImage2D( CRTEMU_LITE_GL_TEXTURE_2D, 0, CRTEMU_LITE_GL_RGB, width, height, 0, CRTEMU_LITE_GL_RGB, CRTEMU_LITE_GL_UNSIGNED_BYTE, 0 );
        crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, crtemu_lite->accumulatebuffer_a );
        crtemu_lite->FramebufferTexture2D( CRTEMU_LITE_GL_FRAMEBUFFER, CRTEMU_LITE_GL_COLOR_ATTACHMENT0, CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_a, 0 );
        crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );

        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_b );
        crtemu_lite->TexImage2D( CRTEMU_LITE_GL_TEXTURE_2D, 0, CRTEMU_LITE_GL_RGB, width, height, 0, CRTEMU_LITE_GL_RGB, CRTEMU_LITE_GL_UNSIGNED_BYTE, 0 );
        crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, crtemu_lite->accumulatebuffer_b );
        crtemu_lite->FramebufferTexture2D( CRTEMU_LITE_GL_FRAMEBUFFER, CRTEMU_LITE_GL_COLOR_ATTACHMENT0, CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_b, 0 );
        crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );

        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->blurtexture_a );
        crtemu_lite->TexImage2D( CRTEMU_LITE_GL_TEXTURE_2D, 0, CRTEMU_LITE_GL_RGB, width, height, 0, CRTEMU_LITE_GL_RGB, CRTEMU_LITE_GL_UNSIGNED_BYTE, 0 );
        crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, crtemu_lite->blurbuffer_a );
        crtemu_lite->FramebufferTexture2D( CRTEMU_LITE_GL_FRAMEBUFFER, CRTEMU_LITE_GL_COLOR_ATTACHMENT0, CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->blurtexture_a, 0 );
        crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );

        crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->blurtexture_b );
        crtemu_lite->TexImage2D( CRTEMU_LITE_GL_TEXTURE_2D, 0, CRTEMU_LITE_GL_RGB, width, height, 0, CRTEMU_LITE_GL_RGB, CRTEMU_LITE_GL_UNSIGNED_BYTE, 0 );
        crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, crtemu_lite->blurbuffer_b );
        crtemu_lite->FramebufferTexture2D( CRTEMU_LITE_GL_FRAMEBUFFER, CRTEMU_LITE_GL_COLOR_ATTACHMENT0, CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->blurtexture_b, 0 );
        crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );
        }


    crtemu_lite->last_present_width = width;
    crtemu_lite->last_present_height = height;

    CRTEMU_LITE_GLfloat vertices[] =
        {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        };
    crtemu_lite->BufferData( CRTEMU_LITE_GL_ARRAY_BUFFER, 4 * 4 * sizeof( CRTEMU_LITE_GLfloat ), vertices, CRTEMU_LITE_GL_STATIC_DRAW );
    crtemu_lite->BindBuffer( CRTEMU_LITE_GL_ARRAY_BUFFER, crtemu_lite->vertexbuffer );

    // Copy to backbuffer
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->backbuffer );
    crtemu_lite->TexImage2D( CRTEMU_LITE_GL_TEXTURE_2D, 0, CRTEMU_LITE_GL_RGBA, width, height, 0, CRTEMU_LITE_GL_RGBA, CRTEMU_LITE_GL_UNSIGNED_BYTE, pixels_xbgr );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );

    crtemu_lite->Viewport( 0, 0, width, height );

    // Blur the previous accumulation buffer
    crtemu_lite_internal_blur( crtemu_lite, crtemu_lite->accumulatetexture_b, crtemu_lite->blurbuffer_a, crtemu_lite->blurbuffer_b, crtemu_lite->blurtexture_b, 1.0f, width, height );

    // Update accumulation buffer
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, crtemu_lite->accumulatebuffer_a );
    crtemu_lite->UseProgram( crtemu_lite->accumulate_shader );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->accumulate_shader, "tex0" ), 0 );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->accumulate_shader, "tex1" ), 1 );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->accumulate_shader, "modulate" ), 1.0f );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->backbuffer );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE1 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->blurtexture_a );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->DrawArrays( CRTEMU_LITE_GL_TRIANGLE_FAN, 0, 4 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE1 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );


    // Store a copy of the accumulation buffer
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, crtemu_lite->accumulatebuffer_b );
    crtemu_lite->UseProgram( crtemu_lite->copy_shader );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->copy_shader, "tex0" ), 0 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_a );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->DrawArrays( CRTEMU_LITE_GL_TRIANGLE_FAN, 0, 4 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );

    // Blend accumulation and backbuffer
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, crtemu_lite->accumulatebuffer_a );
    crtemu_lite->UseProgram( crtemu_lite->blend_shader );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->blend_shader, "tex0" ), 0 );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->blend_shader, "tex1" ), 1 );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->blend_shader, "modulate" ), 1.0f );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->backbuffer );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE1 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_b );
    crtemu_lite->DrawArrays( CRTEMU_LITE_GL_TRIANGLE_FAN, 0, 4 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );
    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE1 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );


    // Add slight blur to backbuffer
    crtemu_lite_internal_blur( crtemu_lite, crtemu_lite->accumulatetexture_a, crtemu_lite->accumulatebuffer_a, crtemu_lite->blurbuffer_b, crtemu_lite->blurtexture_b, /*0.17f*/ 0.0f, width, height );

    // Create fully blurred version of backbuffer
    crtemu_lite_internal_blur( crtemu_lite, crtemu_lite->accumulatetexture_a, crtemu_lite->blurbuffer_a, crtemu_lite->blurbuffer_b, crtemu_lite->blurtexture_b, 1.0f, width, height );


    // Present to screen with CRT shader
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );

    crtemu_lite->Viewport( viewport[ 0 ], viewport[ 1 ], viewport[ 2 ], viewport[ 3 ] );

    int window_width = viewport[ 2 ] - viewport[ 0 ];
    int window_height = viewport[ 3 ] - viewport[ 1 ];

    int aspect_width = (int)( ( window_height * 4 ) / 3 );
    int aspect_height= (int)( ( window_width * 3 ) / 4 );
    int target_width, target_height;
    if( aspect_height <= window_height )
        {
        target_width = window_width;
        target_height = aspect_height;
        }
    else
        {
        target_width = aspect_width;
        target_height = window_height;
        }

    float hscale = target_width / (float) width;
    float vscale = target_height / (float) height;

    float hborder = ( window_width - hscale * width ) / 2.0f;
    float vborder = ( window_height - vscale * height ) / 2.0f;
    float x1 = hborder;
    float y1 = vborder;
    float x2 = x1 + hscale * width;
    float y2 = y1 + vscale * height;

    x1 = ( x1 / window_width ) * 2.0f - 1.0f;
    x2 = ( x2 / window_width ) * 2.0f - 1.0f;
    y1 = ( y1 / window_height ) * 2.0f - 1.0f;
    y2 = ( y2 / window_height ) * 2.0f - 1.0f;

    CRTEMU_LITE_GLfloat screen_vertices[] =
        {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        };
    screen_vertices[  0 ] = x1;
    screen_vertices[  1 ] = y1;
    screen_vertices[  4 ] = x2;
    screen_vertices[  5 ] = y1;
    screen_vertices[  8 ] = x2;
    screen_vertices[  9 ] = y2;
    screen_vertices[ 12 ] = x1;
    screen_vertices[ 13 ] = y2;

    crtemu_lite->BufferData( CRTEMU_LITE_GL_ARRAY_BUFFER, 4 * 4 * sizeof( CRTEMU_LITE_GLfloat ), screen_vertices, CRTEMU_LITE_GL_STATIC_DRAW );
    crtemu_lite->BindBuffer( CRTEMU_LITE_GL_ARRAY_BUFFER, crtemu_lite->vertexbuffer );

    float b = ( ( border_xbgr >> 16 ) & 0xff ) / 255.0f;
    float g = ( ( border_xbgr >> 8  ) & 0xff ) / 255.0f;
    float r = ( ( border_xbgr       ) & 0xff ) / 255.0f;
    crtemu_lite->ClearColor( r, g, b, 1.0f );
    crtemu_lite->Clear( CRTEMU_LITE_GL_COLOR_BUFFER_BIT );

    crtemu_lite->UseProgram( crtemu_lite->crt_shader );

    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "backbuffer" ), 0 );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "blurbuffer" ), 1 );
    crtemu_lite->Uniform1i( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "frametexture" ), 2 );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "use_frame" ), crtemu_lite->use_frame );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "time" ), 1.5f * (CRTEMU_LITE_GLfloat)( ( (double) time_us ) / 1000000.0 ) );
    crtemu_lite->Uniform2f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "resolution" ), (float) window_width, (float) window_height );
    crtemu_lite->Uniform2f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "size" ), (float)( target_width / 2 >= width ? width : target_width / 2 ), (float) ( target_height / 2 >= height ? height : target_height / 2 ) );

    float mod_r = ( ( mod_xbgr >> 16 ) & 0xff ) / 255.0f;
    float mod_g = ( ( mod_xbgr >> 8  ) & 0xff ) / 255.0f;
    float mod_b = ( ( mod_xbgr       ) & 0xff ) / 255.0f;
    crtemu_lite->Uniform3f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "modulate" ), mod_r, mod_g, mod_b );

    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_curvature" ), crtemu_lite->config.curvature );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_scanlines" ), crtemu_lite->config.scanlines );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_shadow_mask" ), crtemu_lite->config.shadow_mask );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_separation" ), crtemu_lite->config.separation );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_ghosting" ), crtemu_lite->config.ghosting );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_noise" ), crtemu_lite->config.noise );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_flicker" ), crtemu_lite->config.flicker );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_vignette" ), crtemu_lite->config.vignette );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_distortion" ), crtemu_lite->config.distortion );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_aspect_lock" ), crtemu_lite->config.aspect_lock );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_hpos" ), crtemu_lite->config.hpos );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_vpos" ), crtemu_lite->config.vpos );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_hsize" ), crtemu_lite->config.hsize );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_vsize" ), crtemu_lite->config.vsize );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_contrast" ), crtemu_lite->config.contrast );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_brightness" ), crtemu_lite->config.brightness );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_saturation" ), crtemu_lite->config.saturation );
    crtemu_lite->Uniform1f( crtemu_lite->GetUniformLocation( crtemu_lite->crt_shader, "cfg_degauss" ), crtemu_lite->config.degauss );

    float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };

    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->accumulatetexture_a );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, CRTEMU_LITE_GL_CLAMP_TO_BORDER );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, CRTEMU_LITE_GL_CLAMP_TO_BORDER );
    #ifndef CRTEMU_LITE_WEBGL
        crtemu_lite->TexParameterfv( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_BORDER_COLOR, color );
    #endif

    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE1 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->blurtexture_a );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_LINEAR );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, CRTEMU_LITE_GL_CLAMP_TO_BORDER );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, CRTEMU_LITE_GL_CLAMP_TO_BORDER );
    #ifndef CRTEMU_LITE_WEBGL
        crtemu_lite->TexParameterfv( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_BORDER_COLOR, color );
    #endif

    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE3 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, crtemu_lite->backbuffer );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MIN_FILTER, CRTEMU_LITE_GL_NEAREST );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_MAG_FILTER, CRTEMU_LITE_GL_NEAREST );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_S, CRTEMU_LITE_GL_CLAMP_TO_BORDER );
    crtemu_lite->TexParameteri( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_WRAP_T, CRTEMU_LITE_GL_CLAMP_TO_BORDER );
    #ifndef CRTEMU_LITE_WEBGL
        crtemu_lite->TexParameterfv( CRTEMU_LITE_GL_TEXTURE_2D, CRTEMU_LITE_GL_TEXTURE_BORDER_COLOR, color );
    #endif

    crtemu_lite->DrawArrays( CRTEMU_LITE_GL_TRIANGLE_FAN, 0, 4 );

    crtemu_lite->ActiveTexture( CRTEMU_LITE_GL_TEXTURE0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 0 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 1 );
    crtemu_lite->BindTexture( CRTEMU_LITE_GL_TEXTURE_2D, 2 );
    crtemu_lite->BindFramebuffer( CRTEMU_LITE_GL_FRAMEBUFFER, 0 );
    }


void crtemu_lite_coordinates_window_to_bitmap( crtemu_lite_t* crtemu_lite, int width, int height, int* x, int* y )
    {
    CRTEMU_LITE_GLint viewport[ 4 ];
    crtemu_lite->GetIntegerv( CRTEMU_LITE_GL_VIEWPORT, viewport );

    int window_width = viewport[ 2 ] - viewport[ 0 ];
    int window_height = viewport[ 3 ] - viewport[ 1 ];

    int aspect_width = (int)( ( window_height * 4 ) / 3 );
    int aspect_height= (int)( ( window_width * 3 ) / 4 );
    int target_width, target_height;
    if( aspect_height <= window_height )
        {
        target_width = window_width;
        target_height = aspect_height;
        }
    else
        {
        target_width = aspect_width;
        target_height = window_height;
        }

    float hscale = target_width / (float) width;
    float vscale = target_height / (float) height;

    float hborder = ( window_width - hscale * width ) / 2.0f;
    float vborder = ( window_height - vscale * height ) / 2.0f;

    float xp = ( ( *x - hborder ) / hscale ) / (float) width;
    float yp = ( ( *y - vborder ) / vscale ) / (float) height;

    xp *= width;
    yp *= height;

    *x = (int) ( xp );
    *y = (int) ( yp );
    }


void crtemu_lite_coordinates_bitmap_to_window( crtemu_lite_t* crtemu_lite, int width, int height, int* x, int* y )
    {
    (void) crtemu_lite, (void) width, (void) height, (void) x, (void) y; // TODO: implement
    }


#endif /* CRTEMU_LITE_IMPLEMENTATION */

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2016 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/
