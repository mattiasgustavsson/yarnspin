/*
------------------------------------------------------------------------------
          Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

file.h - v1.0 - C/C++ functions to load/save an entire file to/from memory.

Do this:
    #define FILE_IMPLEMENTATION
before you include this file in *one* C/C++ file to create the implementation.
*/

#ifndef file_h
#define file_h

#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#include <stddef.h>

struct file_t
    {
    void* memctx;
    size_t size;
    char data[ 1 ]; /* "open" array - it is [size] elements long, not [1]. */
    };

typedef struct file_t file_t;

file_t* file_create( size_t size, void* memctx );

typedef enum file_mode_t
    {
    FILE_MODE_BINARY,
    FILE_MODE_TEXT,
    } file_mode_t;

file_t* file_load( char const* filename, file_mode_t mode, void* memctx );

void file_destroy( file_t* file );

int file_save( file_t const* file, char const* filename, file_mode_t mode );
int file_save_data( void const* data, size_t size, char const* filename, file_mode_t mode );

#endif /* file_h */


/*

Examples:

------------------------------------------------------------------------------

    #define FILE_IMPLEMENTATION
    #include "file.h"

    #include <stdio.h>
    #include <string.h>

    int main()
        {
        char const* test = "This is just a test file.\nTest test test.";
        file_save_data( test, strlen( test ), "test.txt", FILE_TEXT );

        file_t* file = file_load( "test.txt", FILE_TEXT );
        if( file && file->size > 0 )
            printf( "%s", file->data );

        file_save( file, "test_copy.txt", FILE_TEXT );
        file_destroy( file );
        return 0;
        }
*/


/*
----------------------
    IMPLEMENTATION
----------------------
*/

#ifdef FILE_IMPLEMENTATION
#undef FILE_IMPLEMENTATION

#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#ifndef FILE_MALLOC
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #if defined(__cplusplus)
        #define FILE_MALLOC( ctx, size ) ( ::malloc( size ) )
        #define FILE_FREE( ctx, ptr ) ( ::free( ptr ) )
    #else
        #define FILE_MALLOC( ctx, size ) ( malloc( size ) )
        #define FILE_FREE( ctx, ptr ) ( free( ptr ) )
    #endif
#endif


file_t* file_create( size_t size, void* memctx )
    {
    file_t* file = 0;
    size_t const size_to_alloc = size + sizeof( file_t );
    file = (file_t*) FILE_MALLOC( memctx, size_to_alloc );
    if( file )
        {
        file->memctx = memctx;
        file->size = size;
        }
    return file;
    }


file_t* file_load( char const* filename, file_mode_t mode, void* memctx )
    {
    file_t* file = 0;
    FILE* const fp = fopen( filename, mode == FILE_MODE_BINARY ? "rb" : "r" );
    if( fp )
        {
        fseek( fp, 0, SEEK_END );
        size_t const file_size = (size_t) ftell( fp );
        fseek( fp, 0, SEEK_SET );
        file = file_create( file_size + ( mode == FILE_MODE_BINARY ? 0 : 1 ), memctx );
        if( file )
            {
            size_t const count = fread( file->data, 1, file_size, fp );
            if( mode == FILE_MODE_TEXT ) file->data[ count ] = 0;
            file->memctx = memctx;
            file->size = count;
            }
        fclose( fp );
        }

    return file;
    }


void file_destroy( file_t* file )
    {
    if( file )
        FILE_FREE( file->memctx, file );
    }


int file_save_data( void const* data, size_t size, char const* filename, file_mode_t mode )
    {
    if( data )
        {
        FILE* const fp = fopen( filename, mode == FILE_MODE_BINARY ? "wb" : "w" );
        if( fp )
            {
            int const skip_last = ( mode == FILE_MODE_BINARY || size <= 0 ) ? 0 : ( (char const*) data )[ size - 1 ] == '\0' ? 1 : 0;
            if( ( skip_last == 0 && size > 0 ) || ( skip_last != 0 && size > 1 ) )
                fwrite( data, 1, size - skip_last, fp );
            fclose( fp );
            return 1;
            }
        }
    return 0;
    }


int file_save( file_t const* file, char const* filename, file_mode_t mode )
    {
    if( file) return file_save_data( file->data, file->size, filename, mode );
    return 0;
    }


#endif /* FILE_IMPLEMENTATION */

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2015 Mattias Gustavsson

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
