
#include "yarn_lexer.h"
//#include "yarn_parser.h"
//#include "yarn_compiler.h"


struct parser_global_t {
    string filename;
    int line_number;
    string keyword;
    array(string)* data;
};


struct parser_declaration_t {
    string filename;
    int line_number;
    string keyword;
    string identifier;
    array(string)* data;
};


enum section_type_t {
    SECTION_TYPE_UNKNOWN,
    SECTION_TYPE_LOCATION,
    SECTION_TYPE_DIALOG,
    SECTION_TYPE_CHARACTER,
};


struct parser_section_t {
    enum section_type_t type;
    string filename;
    int line_number;
    string id;
    array(struct parser_declaration_t)* declarations;
};


struct yarn_t;

struct yarn_t* yarn_compile( char const* path ) {
    array(struct parser_global_t)* parser_globals = array_create( struct parser_global_t );
    array(struct parser_section_t)* parser_sections = array_create( struct parser_section_t );

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

            struct yarn_lexer_output_t lexer;
            lexer.globals = array_create( struct lexer_declaration_t );
            lexer.sections = array_create( struct lexer_section_t );
            bool lexer_success = yarn_lexer( filename, source, &lexer );
            if( lexer_success ) {
                //parser_success = parser_success && yarn_parser( &lexer, parser_globals, parser_sections );
            }
            for( int i = 0; i < array_count( lexer.sections ); ++i ) {
                struct lexer_section_t* section = (struct lexer_section_t*) array_item( lexer.sections, i );
                array_destroy( section->lines );
            }
            array_destroy( lexer.sections );
            array_destroy( lexer.globals );
            if( !lexer_success ) {
                printf( "Lexer failed for file '%s'\n", filename );
                array_destroy( parser_sections );
                array_destroy( parser_globals);
                return NULL;
            }
        }
    }
    dir_close( dir );

    if( files_count == 0 ) {
        printf( "No files found in 'scripts' folder\n" );
        array_destroy( parser_sections );
        array_destroy( parser_globals);
        return NULL;
    }
    if( !parser_success ) {
        printf( "Parser failed\n" );
        array_destroy( parser_sections );
        array_destroy( parser_globals);
        return NULL;
    }

    //yarn_t* yarn = yarn_compiler( parser_globals, parser_sections );

    array_destroy( parser_sections );
    array_destroy( parser_globals);

    // return compiled_yarn;
    return NULL;
}
