char const* skip_whitespace_eol( const char* str ) {
    while( *str ) {
        if( *str == '\n' ) break;
        if( *str > ' ' ) break;
        ++str;
    }
    return str;
}


bool is_eol( char c ) {
    return c == '\0' || c == '\n';
}


bool is_identifier_char( char c ) {
    return ( c >= '0' && c <= '9' ) || c == '.' || c == '_' || c == ' ' ||
        ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
}


bool is_keyword_char( char c ) {
    return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_';
}


typedef enum line_type_t {
    LINE_TYPE_INVALID,
    LINE_TYPE_SECTION,
    LINE_TYPE_DECLARATION,
    LINE_TYPE_CONDITIONAL,
} line_type_t;


typedef struct line_t {
    line_type_t type;
    char const* pos;
    string conditional;
    string identifier;
    string data;
    char const* error_pos;
    string error_msg;
} line_t;


char const* lexer_error( line_t* line, char const* pos, char const* msg, ...  ) {
    line->error_pos = pos;
    va_list args;
    va_start( args, msg);
    line->error_msg = cstr_vformat( msg, args );
    va_end( args );
    return pos;
}


char const* lex_section( const char* line_string, line_t* line ) {
    char const* str = line_string;

    str = skip_whitespace_eol( str );
    if( *str++ != '=' || *str++ != '=' || *str++ != '=' ) {
        return lexer_error( line, str, "a section declaration must start with '==='\n" );
    }
    str = skip_whitespace_eol( str );

    string identifier = NULL;
    while( !is_eol( *str ) && *str != '=' ) {
        if( !is_identifier_char( *str ) ) {
            return lexer_error( line, str, "invalid section identifier: '%s'. section identifier can contain the "
                "following characters: a-z A-Z _ . 0-9\n", cstr_cat( identifier, cstr_left( str, 1 ) ) );
        }

        if( *str == ' ' ) {
            identifier = cstr_cat( identifier, " " );
            str = skip_whitespace_eol( str );
        } else {
            identifier = cstr_cat( identifier, cstr_left( str, 1 ) );
            ++str;
        }
    }

    str = skip_whitespace_eol( str );
    if( *str && *str != '\n' && ( *str++ != '=' || *str++ != '=' || *str++ != '=' ) )
        return lexer_error( line, str, "a section declaration must end with a line break or ===\n" );

    str = skip_whitespace_eol( str );

    if( *str != '\n' ) return lexer_error( line, str, "a section declaration must end with a line break or '==='\n" );

    identifier = cstr_trim( identifier );

    line->type = LINE_TYPE_SECTION;
    line->identifier = identifier;

    return str;
}


char const* lex_declaration( const char* yarn_source, line_t* line ) {
    char const* str = yarn_source;
    str = skip_whitespace_eol( str );
    char const* start = str;
    while( *str && is_keyword_char( *str ) ) {
        ++str;
    }
    if( *str != ':' ) return lexer_error( line, str, "a declaration must be on the form keyword: string\n" );

    string identifier = cstr_n( start, (size_t)( str - start ) );
    ++str;
    identifier = cstr_trim( identifier );

    start = str;
    while( !is_eol( *str ) ) {
        ++str;
    }
    string data = cstr_n( start, (size_t)( str - start ) );
    data = cstr_trim( data );

    line->type = LINE_TYPE_DECLARATION;
    line->identifier = identifier;
    line->data = data;

    return str;
}


char const* lex_conditional( const char* yarn_source, line_t* line ) {
    char const* str = yarn_source;
    while( *str && *str != '?') {
        ++str;
    }
    line->type = LINE_TYPE_CONDITIONAL;
    line->conditional = cstr_trim( cstr_n( yarn_source, (size_t)( str - yarn_source ) ) );
    if( cstr_len( line->conditional ) <= 0 ) return lexer_error( line, str, "invalid conditional declaration\n" );
    return str + 1;
}


char const* lex_comment( const char* yarn_source, line_t* line ) {
    (void) line;
    char const* str = yarn_source;
    while( *str ) {
        if( *str == '\n' ) return ++str;
        ++str;
    }
    return str;
}


char const* lex_line( const char* yarn_source, line_t* line ) {
    char const* str = yarn_source;
    while( *str ) {
        if( *str == '/' && str[ 1 ] == '/' ) return lex_comment( yarn_source, line );
        if( *str == '=' ) return lex_section( yarn_source, line );
        if( *str == '?' ) return lex_conditional( yarn_source, line );
        if( *str == ':' ) return lex_declaration( yarn_source, line );
        if( *str == '\n' ) return ++str;
        ++str;
    }
    return str;
}


int find_line_number( char const* yarn_source, char const* pos ) {
    int line = 1;
    char const* str = yarn_source;
    while( *str && str < pos ) {
        if( *str == '\n' ) ++line;
        ++str;
    }

    return line;
}


typedef struct lexer_declaration_t {
    string filename;
    int line_number;
    string identifier;
    string data;
    string conditional;
} lexer_declaration_t;


typedef struct lexer_section_t {
    string filename;
    int line_number;
    string id;
    array(lexer_declaration_t)* lines;
} lexer_section_t;


bool yarn_lexer( string filename, string yarn_source, array_param(lexer_declaration_t)* globals,
    array_param(lexer_section_t)* sections ) {

    array(line_t)* lexer_lines = array_create( line_t );
    char const* str = yarn_source;
    bool lexer_errors = false;
    while( str && *str ) {
        line_t line = { LINE_TYPE_INVALID };
        line.pos = str;
        str = lex_line( str, &line );
        if( line.error_pos != 0 ) {
            lexer_errors = true;
            printf( "%s(%d): %s", filename, find_line_number( yarn_source, line.error_pos ), line.error_msg );
            while( *str && *str != '\n' ) ++str;
        }
        if( line.type != LINE_TYPE_INVALID )
            array_add( lexer_lines, &line );
    }

    if( !lexer_errors ) {
        lexer_section_t* section = 0;
        for( int i = 0; i < lexer_lines->count; ++i ) {
            line_t* line = &lexer_lines->items[ i ];
            if( line->type == LINE_TYPE_SECTION ) {
                lexer_section_t sect;
                sect.filename = filename;
                sect.line_number = find_line_number( yarn_source, line->pos );
                sect.id = line->identifier;
                sect.lines = managed_array( lexer_declaration_t );
                section = (lexer_section_t*) array_add( sections, &sect );
            } else if( line->type == LINE_TYPE_DECLARATION ) {
                lexer_declaration_t decl;
                decl.filename = filename;
                decl.line_number = find_line_number( yarn_source, line->pos );
                decl.identifier = line->identifier;
                decl.data = line->data;
                decl.conditional = NULL;
                if( section ) {
                    array_add( section->lines, &decl );
                } else {
                    array_add( globals, &decl );
                }
            } else if( line->type == LINE_TYPE_CONDITIONAL ) {
                lexer_declaration_t decl;
                decl.filename = filename;
                decl.line_number = find_line_number( yarn_source, line->pos );
                decl.identifier = NULL;
                decl.data = NULL;
                decl.conditional = line->conditional;
                if( section ) {
                    array_add( section->lines, &decl );
                } else {
                    array_add( globals, &decl );
                }
            } else {
                printf( "%s(%d): invalid line", filename, find_line_number( yarn_source, line->pos ));
            }
        }
    }

    array_destroy( lexer_lines );
    return !lexer_errors;
}
