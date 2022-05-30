
#include "yarn_lexer.h"


struct yarn_t;

struct yarn_t* yarn_compile( char const* path ) {
    string scripts_path = cstr_cat( path, "/scripts/" ); // TODO: cstr_join
	dir_t* dir = dir_open( scripts_path );
    if( !dir ) {
		printf( "Could not find 'scripts' folder\n" );
        return 0;
    }
    int files_count = 0;
    
    for( dir_entry_t* d = dir_read( dir ); d != NULL; d = dir_read( dir ) )
        {
        if( dir_is_file( d ) ) {
            string filename = cstr_cat( scripts_path, dir_name( d ) ); // TODO: cstr_join
		    file_t* file = file_load( filename, FILE_MODE_TEXT, 0 );
		    if( !file )  {
			    printf( "Could not open file '%s'\n", filename );
			    return false;
			}
            ++files_count;
		    string source = cstr( (char const*) file->data );
		    file_destroy( file );

		    array(lexer_declaration_t)* lexer_globals = array_create( sizeof( struct lexer_declaration_t ), NULL );	
		    array(lexer_section_t)* lexer_sections = array_create( sizeof( struct lexer_section_t ), NULL );	
		    bool lexer_success = yarn_lexer( filename, source, lexer_globals, lexer_sections );
		    if( !lexer_success ) return 0;
		
            printf( "\n\n\n\"%s\"\n\n", filename );

            for( int i = 0; i < array_count( lexer_globals ); ++i ) {
                struct lexer_declaration_t* decl = (struct lexer_declaration_t*) array_item( lexer_globals, i );                
                if( decl->conditional ) {
                    printf( "%s ? ", decl->conditional );
                } else {
                    printf( "%s: %s\n", decl->identifier, decl->data );
                }
            }
            for( int i = 0; i < array_count( lexer_sections ); ++i ) {
                struct lexer_section_t* section = (struct lexer_section_t*) array_item( lexer_sections, i );
                printf( "=== %s ===\n", section->id );
                for( int j = 0; j < array_count( section->lines ); ++j ) {
                    struct lexer_declaration_t* decl = (struct lexer_declaration_t*) array_item( section->lines, j );                
                    if( decl->conditional ) {
                        printf( "%s ? ", decl->conditional );
                    } else {
                        printf( "%s: %s\n", decl->identifier, decl->data );
                    }
                }
                printf( "\n" );
            }

            for( int i = 0; i < array_count( lexer_sections ); ++i ) {
                struct lexer_section_t* section = (struct lexer_section_t*) array_item( lexer_sections, i );
                array_destroy( section->lines );
            }
            array_destroy( lexer_sections );
            array_destroy( lexer_globals );
		}
    }
    dir_close( dir );	

    if( files_count == 0 ) { 
        printf( "No files found in 'scripts' folder\n" ); 
        return NULL; 
    }

    return NULL; // TODO: yarn compilation
}
