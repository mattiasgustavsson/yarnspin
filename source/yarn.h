
#include "yarn_lexer.h"
#include "yarn_parser.h"
//#include "yarn_compiler.h"

struct yarn_t;

struct yarn_t* yarn_compile( char const* path ) {
    array(struct parser_global_t)* parser_globals = managed_array( struct parser_global_t );
    array(struct parser_section_t)* parser_sections = managed_array( struct parser_section_t );

    bool parser_success = true;

    string scripts_path = cstr_cat( path, "/scripts/" ); // TODO: cstr_join
    dir_t* dir = dir_open( scripts_path );
    if( !dir ) {
        printf( "Could not find 'scripts' folder\n" );
        return 0;
    }

    int files_count = 0;
    for( dir_entry_t* d = dir_read( dir ); d != NULL; d = dir_read( dir ) ) {
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

            array(struct lexer_declaration_t)* lexer_globals = managed_array( struct lexer_declaration_t );
            array(struct lexer_section_t)* lexer_sections = managed_array( struct lexer_section_t );
            bool lexer_success = yarn_lexer( filename, source, lexer_globals, lexer_sections );
            if( lexer_success ) {
                if( !yarn_parser( lexer_globals, lexer_sections, parser_globals, parser_sections ) ) {
                    parser_success = false;
                }
            }

            if( !lexer_success ) {
                printf( "Lexer failed for file '%s'\n", filename );
                return NULL;
            }
        }
    }
    dir_close( dir );

    if( files_count == 0 ) {
        printf( "No files found in 'scripts' folder\n" );
        return NULL;
    }
    if( !parser_success ) {
        printf( "Parser failed\n" );
        return NULL;
    }

    //yarn_t* yarn = yarn_compiler( parser_globals, parser_sections );
    // return yarn;
    return NULL;
}
