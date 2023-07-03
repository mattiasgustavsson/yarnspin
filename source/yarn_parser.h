
typedef struct parser_global_t {
    string filename;
    int line_number;
    string keyword;
    array(string)* data;
} parser_global_t;


typedef struct parser_declaration_t {
    string filename;
    int line_number;
    string keyword;
    string identifier;
    array(string)* data;
} parser_declaration_t;


typedef enum section_type_t {
    SECTION_TYPE_UNKNOWN,
    SECTION_TYPE_LOCATION,
    SECTION_TYPE_DIALOG,
    SECTION_TYPE_CHARACTER,
} section_type_t;


typedef struct parser_section_t {
    section_type_t type;
    string filename;
    int line_number;
    string id;
    array(parser_declaration_t)* declarations;
} parser_section_t;


bool is_multi_item_keyword( string keyword ) {
    char const* multi_item_keywords[] = { 
        "display_filters", "logo", "flags", "items",
        "font_description", "font_options", "font_characters", "font_items", "font_name",
        "debug_set_flags", "debug_get_items", "debug_attach_chars", "mus", "amb", "snd", 
    };
    for( int i = 0; i < ARRAY_COUNT( multi_item_keywords ); ++i ) {
        if( cstr_compare_nocase( keyword, multi_item_keywords[ i ] ) == 0 ) {
            return true;
        }
    }
    return false;
}


bool is_global_keyword( string keyword ) {

    char const* global_keywords[] = {
        "title", "author", "start", "items", "flags",
        "palette", "resolution", "colormode", "screenmode", "display_filters", "logo", "logo_music", "alone_text", "nothing_text",
        "font_description", "font_options", "font_characters", "font_items", "font_name",
        "background_location", "background_dialog",
        "color_background", "color_disabled", "color_txt", "color_opt", "color_chr", "color_use", "color_name",
        "debug_start", "debug_set_flags", "debug_get_items", "debug_attach_chars",
    };
    for( int i = 0; i < ARRAY_COUNT( global_keywords ); ++i ) {
        if( cstr_compare_nocase( keyword, global_keywords[ i ] ) == 0 ) {
           return true;
        }
    }
    return false;
}


bool is_section_keyword( string keyword ) {
    char const* section_keywords[] = {
        "mus", "amb", "snd",  "img", "txt", "opt", "act", "use", "chr",
        "say", "name", "short", "face",
    };
    for( int i = 0; i < ARRAY_COUNT( section_keywords ); ++i ) {
        if( cstr_compare_nocase( keyword, section_keywords[ i ] ) == 0 ) {
            return true;
        }
    }
    return false;
}


bool parse_data( lexer_declaration_t const* line, array_param(string)* items ) {
    bool no_error = true;
    char const* start = line->data;
    char const* cur = start;
    if( *cur == ',' ) {
        printf( "%s(%d): invalid data list '%s' at position %d. should not start with ','\n", line->filename,
            line->line_number, line->data, (int)( cur - line->data + 1  ) );
        no_error = false;
    } else {
        while( *cur++ ) {
            if( *cur == ',' || *cur == '\0' ) {
                string data = cstr_n( start, (size_t)( cur - start ) );
                start = cur + ( *cur ? 1 : 0 );
                data = cstr_trim( data );
                if( cstr_len( data ) > 0 ) {
                    array_add( items, &data );
                } else {
                    printf( "%s(%d): invalid data list '%s' at position %d. empty list item\n", line->filename,
                        line->line_number, line->data, (int)( cur - line->data ) );
                    no_error = false;
                    break;
                }
            }
        }
    }
    return no_error;
}


bool parse_conditional( lexer_declaration_t const* line, array_param(string)* items ) {
    bool no_error = true;
    char const* start = line->conditional;
    char const* cur = start;
    if( *cur == ',' ) {
        printf( "%s(%d): invalid conditional '%s' at position %d. should not start with ','\n",
            line->filename, line->line_number, line->data, (int)( cur - line->data + 1 ) );
        no_error = false;
    } else {
        while( *cur++ ) {
            if( *cur == ',' || *cur == '\0' ) {
                string data = cstr_n( start, (size_t)( cur - start ) );
                start = cur + ( *cur ? 1 : 0 );
                data = cstr_trim( data );
                if( cstr_len( data ) > 0 ) {
                    array_add( items, &data );
                } else {
                    printf( "%s(%d): invalid conditionals list '%s' at position %d. empty list item\n",
                        line->filename, line->line_number, line->data, (int)( cur - line->data ) );
                    no_error = false;
                    break;
                }
            }
        }
    }
    return no_error;
}


bool parse_globals( array_param( lexer_declaration_t )* lexer_globals, array_param( parser_global_t)* parser_globals ) {

    bool no_error = true;
    for( int i = 0; i < array_count( lexer_globals ); ++i ) {
        lexer_declaration_t* line = array_item( lexer_globals, i );

        if( !is_global_keyword( line->identifier ) ) {
            printf( "%s(%d): invalid keyword '%s' in global section. \n", line->filename, line->line_number,
                line->identifier );
            no_error = false;
        } else {
            parser_global_t decl;
            decl.filename = line->filename;
            decl.line_number = line->line_number;
            decl.keyword = line->identifier;
            decl.data = managed_array( string );
            if( is_multi_item_keyword( line->identifier ) ) {
                no_error = no_error && parse_data( line, decl.data );
            } else {
                array_add( decl.data, &line->data );
            }
            array_add( parser_globals, &decl );
        }

    }
    return no_error;
}


section_type_t find_section_type( string identifier ) {
    char const* location_types[] = { "txt", "opt", "img", "chr" };
    for( int i = 0; i < ARRAY_COUNT( location_types ); ++i ) {
        if( cstr_compare_nocase( identifier, location_types[ i ] ) == 0 ) {
            return SECTION_TYPE_LOCATION;
        }
    }

    char const* dialog_types[] = { "say" };
    for( int i = 0; i < ARRAY_COUNT( dialog_types ); ++i ) {
        if( cstr_compare_nocase( identifier, dialog_types[ i ] ) == 0 ) {
            return SECTION_TYPE_DIALOG;
        }
    }

    char const* character_types[] = { "name", "short", "face" };
    for( int i = 0; i < ARRAY_COUNT( character_types ); ++i ) {
        if( cstr_compare_nocase( identifier, character_types[ i ] ) == 0 ) {
            return SECTION_TYPE_CHARACTER;
        }
    }

    return SECTION_TYPE_UNKNOWN;
}


char const* find_section_type_name( section_type_t type ) {
    switch( type ) {
        case SECTION_TYPE_UNKNOWN: return "unknown";
        case SECTION_TYPE_LOCATION: return "location";
        case SECTION_TYPE_DIALOG: return "dialog";
        case SECTION_TYPE_CHARACTER: return "character";
    }
    return "unknown";
}


bool parse_sections( array_param(lexer_section_t)* lexer_sections, array_param(parser_section_t)* parser_sections ) {
    bool no_error = true;

    for( int i = 0; i < array_count( lexer_sections ); ++i ) {
        lexer_section_t* lexer_section = array_item( lexer_sections, i );
        parser_section_t parser_section;
        parser_section.type = SECTION_TYPE_UNKNOWN;
        parser_section.filename = lexer_section->filename;
        parser_section.line_number = lexer_section->line_number;
        parser_section.id = lexer_section->id;
        parser_section.declarations = managed_array(parser_declaration_t);
        if( array_count( lexer_section->lines ) == 0 ) {
            printf( "%s(%d): empty section '%s'\n", lexer_section->filename,
                lexer_section->line_number, lexer_section->id );
            no_error = false;
        } else {
            section_type_t type = SECTION_TYPE_UNKNOWN;
            for( int j = 0; j < array_count( lexer_section->lines ); ++j ) {
                lexer_declaration_t* line = array_item( lexer_section->lines, j );
                section_type_t current_type = find_section_type( line->identifier );
                if( type == SECTION_TYPE_UNKNOWN ) {
                    type = current_type;
                    if( current_type == SECTION_TYPE_UNKNOWN && !is_section_keyword( line->identifier )
                        && cstr_len( line->conditional ) <= 0 ) {

                        type = SECTION_TYPE_DIALOG;
                    }
                }
                if( cstr_compare_nocase( line->identifier, "act" ) !=0 && current_type != SECTION_TYPE_UNKNOWN
                    && current_type != type ) {

                    printf( "%s(%d): invalid section declaration for '%s'. Unexpected identifier '%s' makes the "
                        "section of mixed type (started as a '%s' section)\n", lexer_section->filename,
                        lexer_section->line_number, lexer_section->id, line->identifier,
                        find_section_type_name( type ) );
                    no_error = false;
                }

                parser_declaration_t decl;
                decl.filename = line->filename;
                decl.line_number = line->line_number;
                decl.keyword = NULL;
                decl.identifier = NULL;
                decl.data = managed_array( string );

                if( cstr_len( line->conditional ) > 0 ) {
                    decl.keyword = "?";
                    no_error = no_error && parse_conditional( line, decl.data );
                } else if( is_section_keyword( line->identifier ) ) {
                    decl.keyword = line->identifier;
                } else if( is_global_keyword( line->identifier ) ) {
                    printf( "%s(%d): unexpected global keyword '%s' in section '%s'\n", lexer_section->filename,
                        lexer_section->line_number, line->identifier, lexer_section->id );
                    no_error = false;
                } else {
                    if( type == SECTION_TYPE_UNKNOWN ) type = SECTION_TYPE_DIALOG;
                    if( type != SECTION_TYPE_DIALOG ) {
                        printf( "%s(%d): invalid section declaration for '%s'. Unexpected identifier '%s' makes the "
                            "section of mixed type (started as a '%s' section)\n", lexer_section->filename,
                            lexer_section->line_number, lexer_section->id, line->identifier,
                            find_section_type_name( type ) );
                        no_error = false;
                    } else {
                        decl.identifier = line->identifier;
                    }
                }

                if( is_multi_item_keyword( line->identifier ) ) {
                    no_error = no_error && parse_data( line, decl.data );
                } else if( cstr_compare_nocase( decl.keyword, "?" ) != 0 ) {
                    array_add( decl.data, &line->data );
                }
                array_add( parser_section.declarations, &decl );
            }
            parser_section.type = type;
        }
        array_add( parser_sections, &parser_section );
    }

    return no_error;
}


bool yarn_parser( array_param(lexer_declaration_t)* lexer_globals, array_param(lexer_section_t)* lexer_sections,
    array_param(parser_global_t)* parser_globals, array_param(parser_section_t)* parser_sections ) {

    return parse_globals( lexer_globals, parser_globals ) && parse_sections( lexer_sections, parser_sections );
}
