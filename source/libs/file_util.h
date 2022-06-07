/*
------------------------------------------------------------------------------
		  Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

file_util.h - v0.1 - Utilities for working with files, folders and paths.

Do this:
	#define FILE_UTIL_IMPLEMENTATION
before you include this file in *one* C/C++ file to create the implementation.

Dependencies:
	dir.h
*/

#ifndef file_util_h
#define file_util_h

char const* basename( char const* path, char const* extension );
char const* cdirname( char const* path );
char const* extname( char const* path );

int create_path( char const* path ); 

int file_more_recent( char const* source_path, char const* output_path );

int file_exists( char const* filename );

#endif /* file_util_h */    

/*
----------------------
	IMPLEMENTATION
----------------------
*/

#ifdef FILE_UTIL_IMPLEMENTATION
#undef FILE_UTIL_IMPLEMENTATION

#define _CRT_NONSTDC_NO_DEPRECATE 
#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
	#include <sys/stat.h>
	#if !defined( _WIN32_WINNT ) || _WIN32_WINNT < 0x0501 
	    #undef _WIN32_WINNT
	    #define _WIN32_WINNT 0x0501 // requires Windows XP minimum
	#endif
	// 0x0400=Windows NT 4.0, 0x0500=Windows 2000, 0x0501=Windows XP, 0x0502=Windows Server 2003, 0x0600=Windows Vista, 
	// 0x0601=Windows 7, 0x0602=Windows 8, 0x0603=Windows 8.1, 0x0A00=Windows 10, 
	#define _WINSOCKAPI_
	#pragma warning( push )
    #pragma warning( disable: 4365 )
    #pragma warning( disable: 4619 )
	#pragma warning( disable: 4668 ) // 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives'
    #pragma warning( disable: 4768 ) // __declspec attributes before linkage specification are ignored	
	#pragma warning( disable: 4255 ) // 'function' : no function prototype given: converting '()' to '(void)'
	#include <windows.h>
	#pragma warning( pop )
	#include <direct.h>
#else /* _WIN32 */
	#include <sys/stat.h>
	#include <stdlib.h>
    #include <strings.h>
#endif

#include "dir.h"


char const* basename( char const* path, char const* extension )
	{
	static char result[ 1024 ];
	strcpy( result, "" );

	if( path )
		{
		char const* lastForwardSlash = strrchr( path, '/' );
		char const* lastBackSlash = strrchr( path, '\\' );
		
		char const* name = 0;
		
		if( !lastBackSlash && !lastForwardSlash )
			{
			name = path;
			}
		else if( !lastBackSlash )
			{
			name = lastForwardSlash + 1;
			}
		else if( !lastForwardSlash )
			{
			name = lastBackSlash + 1;
			}
		else if( lastForwardSlash > lastBackSlash )
			{
			name = lastForwardSlash + 1;
			}
		else
			{
			name = lastBackSlash + 1;
			}
			
		strncpy( result, name, sizeof( result ) );
			
		if( extension )
			{
			size_t extlen = strlen( extension );
			size_t reslen = strlen( result );
			
			if( reslen >= extlen )
				{
                #if _WIN32
				if( stricmp( result + (reslen - extlen), extension ) == 0 )
                #else
                if( strcasecmp( result + (reslen - extlen), extension ) == 0 )
                #endif
					{
					result[ reslen - extlen ] = 0;
					}
				}
			
			}
		}

	return result;
	}


char const* cdirname( char const* path )
	{
	static char result[ 1024 ];
	strcpy( result, "" );

	if( path )
		{
		char* lastForwardSlash;
		char* lastBackSlash;
		
		strncpy( result, path, sizeof( result ) );
		lastForwardSlash = strrchr( result, '/' );
		lastBackSlash = strrchr( result, '\\' );
		
		if( !lastBackSlash && !lastForwardSlash )
			{
			result[ 0 ] = 0;
			}
		else if( !lastBackSlash )
			{
			*(lastForwardSlash + 1) = 0;
			}
		else if( !lastForwardSlash )
			{
			*(lastBackSlash +1 ) = 0;
			}
		else if( lastForwardSlash > lastBackSlash )
			{
			*(lastForwardSlash + 1 ) = 0;
			}
		else
			{
			*(lastBackSlash + 1) = 0;
			}
		}

	return result;
	}

	
char const* extname( char const* path )
	{
	static char result[ 1024 ];
	strcpy( result, "" );

	if( path )
		{
		char const* lastForwardSlash = strrchr( path, '/' );
		char const* lastBackSlash = strrchr( path, '\\' );
		
		char const* name = 0;
		char const* ext = 0;
		
		if( !lastBackSlash && !lastForwardSlash )
			{
			name = path;
			}
		else if( !lastBackSlash )
			{
			name = lastForwardSlash + 1;
			}
		else if( !lastForwardSlash )
			{
			name = lastBackSlash + 1;
			}
		else if( lastForwardSlash > lastBackSlash )
			{
			name = lastForwardSlash + 1;
			}
		else
			{
			name = lastBackSlash + 1;
			}
			
		ext = strrchr( name, '.' );
		
		if( ext && !( ext[ 0 ] == '.' && ext[ 1 ] == 0 ) )
			{
			strncpy( result, ext, sizeof( result ) );            
			}
		}
 
	return result;
	}     

void makedir( char const* path )
	{
	#ifdef _WIN32
		CreateDirectoryA( path, NULL );
	#else /* _WIN32 */
		mkdir( path, S_IRWXU );
	#endif
	}


static int recursive_create_path( char* fullpath,  char* dirname )
	{
	if( dirname )
		{
		char* next_dir = strtok( 0, "/\\" ); 
	    dir_t* dir = dir_open( fullpath );

	    if( dir )
		    {
		    int dir_found = 0;
		    size_t len;
		    
		    dir_entry_t* ent = dir_read( dir );
		    while( ent )
			    {
                #ifdef _WIN32
			    if( stricmp( dir_name( ent ), dirname ) == 0 )
                #else
                if( strcasecmp( dir_name( ent ), dirname ) == 0 )
                #endif
			        {
			        if( dir_is_folder( ent ) )
				        {
				        dir_found = 1;
				        break;
			            }
			        else if( dir_is_file( ent ) )
			            {
			            dir_close( dir );
			            return 0;
			            }
			        }
				ent = dir_read( dir );
			    }

			dir_close( dir );
		    
			len = strlen( fullpath );
			if( len > 0 && fullpath[ len - 1 ] != '\\' && fullpath[ len - 1 ] != '/')
				{
				strncat( fullpath, "/", 1024 - strlen( fullpath ) );
				}
			strncat( fullpath, dirname, 1024 - strlen( fullpath ) );

		    if( !dir_found )
		        {
		        makedir( fullpath );
		        }

			if( next_dir )
				{
				return recursive_create_path( fullpath, next_dir );
				}
			else
				{
				return 1;
				}

		    }    
		}

	return 0;
	}


int create_path( char const* path )
	{
	int retval = 0;
	
	if( path )
		{
		char fullpath[ 1024 ] = "";
		char str[ 1024 ];
		strncpy( str, path, 1024 );
		if( str[1] == ':' )
			{
			strncpy( fullpath, str, 3);
			retval = recursive_create_path( fullpath, strtok( str + 2, "/\\" ) );        
			}
		else
			{
			strcpy( fullpath, "./" );
			retval = recursive_create_path( fullpath, strtok( str, "/\\" ) );        
			}
		}
	
	return retval;
	}


time_t file_last_changed( char const* filename )
	{
	if( filename )
		{
		struct stat result;
		int ret;   
		ret = stat( filename, &result );
		if( ret  == 0 )
	        {
			return result.st_mtime;
	        }
		}
	    
	return 0;	    
	}


int file_more_recent( char const* source_path,  char const* output_path  )
	{
	return file_last_changed( source_path ) > file_last_changed( output_path );
	}



int file_exists( char const* filename )
	{
	struct stat result;
	int ret;   
	ret = stat( filename, &result );
	if( ret == 0 )
		{
		return result.st_mode & S_IFREG;
		}

	return 0;
	}

#endif /* FILE_UTIL_IMPLEMENTATION */

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
