
// strings of type string_id should be compared with case insensitive comparison
typedef string string_id;

#define CMP(a, b) ( cstr_compare_nocase( (a), (b) ) == 0 )

void add_unique_id( array_param(string_id)* arr_param, string_id val ) {
    array(string_id)* arr = ARRAY_CAST( arr_param );

    bool found = false;
	for( int i = 0; i < arr->count; ++i ) {
		if( CMP( arr->items[ i ], val ) ) { 
            found = true; 
            break; 
        }
	}

	if( !found ) {
        array_add( arr, &val );
    }
}


typedef struct flag_t {
	string_id flag;
	string filename;
	int line_number;
} flag_t;


void add_unique_flag( array_param(flag_t)* arr_param, flag_t* val ) {
    array(flag_t)* arr = ARRAY_CAST( arr_param );

    bool found = false;
	for( int i = 0; i < arr->count; ++i ) {
		if( arr->items[ i ].flag == val->flag ) { 
            found = true; 
            break; 
        }
	}

	if( !found ) {
        array_add( arr, val );
    }
}


typedef struct item_t {
	string_id id;
	string filename;
	int line_number;
} item_t;


void add_unique_item( array_param(item_t)* arr_param, item_t* val ) {
    array(item_t)* arr = ARRAY_CAST( arr_param );

    bool found = false;
	for( int i = 0; i < arr->count; ++i ) {
		if( arr->items[ i ].id == val->id ) { 
            found = true; 
            break; 
        }
	}

	if( !found ) {
        array_add( arr, val );
    }
}


typedef struct char_t {
	string_id id;
	string filename;
	int line_number;
} char_t;


void add_unique_char( array_param(char_t)* arr_param, char_t* val ) {
    array(char_t)* arr = ARRAY_CAST( arr_param );

	bool found = false;
	for( int i = 0; i < arr->count; ++i ) {
		if( arr->items[ i ].id == val->id ) { 
            found = true; 
            break; 
        }
	}

	if( !found ) {
        array_add( arr, val );
    }
}


string concat_data( array_param(string)* data_param ) {
    array(string)* data = ARRAY_CAST( data_param );
	
    string result = "";
	for( int i = 0; i < data->count; ++i ) {
		result = cstr_cat( result, cstr_cat( data->items[ i ], ( i < data->count - 1 ? ", " : "" ) ) ); // TODO: cstr_join
    }

	return result;
}


typedef struct compiled_cond_flag_t {
	bool is_not;
	int flag_index;
} compiled_cond_flag_t;


typedef struct compiled_cond_or_t {
	array(compiled_cond_flag_t)* flags;
} compiled_cond_or_t ;
	
compiled_cond_or_t* empty_cond_or( void ) {
    static compiled_cond_or_t cond_or;
    cond_or.flags = managed_array( compiled_cond_flag_t );
    return &cond_or;
}

typedef struct compiled_cond_t {
	array(compiled_cond_or_t)* ands;
} compiled_cond_t;

compiled_cond_t* empty_cond( void ) {
    static compiled_cond_t cond;
    cond.ands = managed_array( compiled_cond_or_t );
    return &cond;
}

typedef enum action_type_t {
	ACTION_TYPE_NONE,
	ACTION_TYPE_GOTO_LOCATION,
    ACTION_TYPE_GOTO_DIALOG,
	ACTION_TYPE_EXIT,
	ACTION_TYPE_FLAG_SET,
	ACTION_TYPE_FLAG_CLEAR,
	ACTION_TYPE_FLAG_TOGGLE,
	ACTION_TYPE_ITEM_GET,
	ACTION_TYPE_ITEM_DROP,
} action_type_t;

typedef struct compiled_act_t {
	compiled_cond_t cond;
	enum action_type_t type;
	int param_location_index;
	int param_dialog_index;
	int param_flag_index;
	int param_item_index;
} compiled_act_t;

compiled_act_t* empty_act( void ) {
    static compiled_act_t act;
    act.cond = *empty_cond();
    act.type = ACTION_TYPE_NONE;
	act.param_location_index = -1;
	act.param_dialog_index = -1;
	act.param_flag_index = -1;
	act.param_item_index = -1;
    return &act;
}


typedef struct compiled_opt_t {
	compiled_cond_t cond;
	string text;
	array(compiled_act_t)* act;
} compiled_opt_t;
	
compiled_opt_t* empty_opt( void ) {
    static compiled_opt_t opt;
    opt.cond = *empty_cond();
    opt.text = NULL;
    opt.act = managed_array( compiled_act_t );
    return &opt;
}

typedef struct compiled_use_t {
	compiled_cond_t cond;
	array(int)* item_indices;
	array(compiled_act_t)* act;
} compiled_use_t;
	
compiled_use_t* empty_use( void ) {
    static compiled_use_t use;
    use.cond = *empty_cond();
    use.item_indices = managed_array( int );
    use.act = managed_array( compiled_act_t );
    return &use;
}

typedef struct compiled_chr_t {
	compiled_cond_t cond;
	array(int)* chr_indices;
	array(compiled_act_t)* act;
} compiled_chr_t;
	
compiled_chr_t* empty_chr( void ) {
    static compiled_chr_t chr;
    chr.cond = *empty_cond();
    chr.chr_indices = managed_array( int );
    chr.act = managed_array( compiled_act_t );
    return &chr;
}

typedef struct compiled_img_t {
	compiled_cond_t cond;
	int image_index;	
} compiled_img_t;

compiled_img_t* empty_img( void ) {
    static compiled_img_t img;
    img.cond = *empty_cond();
    img.image_index = -1;
    return &img;
}

typedef struct compiled_txt_t {
	compiled_cond_t cond;
	string text;	
} compiled_txt_t;

compiled_txt_t* empty_txt( void ) {
    static compiled_txt_t txt;
    txt.cond = *empty_cond();
    txt.text = NULL;
    return &txt;
}


typedef struct compiled_location_t {
	string_id id;
	array(compiled_img_t)* img;
    array(compiled_txt_t)* txt;
	array(compiled_act_t)* act;
	array(compiled_opt_t)* opt;	
	array(compiled_use_t)* use;
	array(compiled_chr_t)* chr;
} compiled_location_t;

compiled_location_t* empty_location( void ) {
    static compiled_location_t location;
    location.id = NULL;
	location.img = managed_array(compiled_img_t);
    location.txt = managed_array(compiled_txt_t);
	location.act = managed_array(compiled_act_t);
	location.opt = managed_array(compiled_opt_t);	
	location.use = managed_array(compiled_use_t);
	location.chr = managed_array(compiled_chr_t);
    return &location;
}

typedef struct compiled_phrase_t {
	compiled_cond_t cond;
	int character_index;
	string text;	
} compiled_phrase_t;

compiled_phrase_t* empty_phrase( void ) {
    static compiled_phrase_t phrase;
    phrase.cond = *empty_cond();
    phrase.character_index = -1;
    phrase.text = NULL;
    return &phrase;
}

typedef struct compiled_say_t {
	compiled_cond_t cond;
	string text;
	array(compiled_act_t)* act;
} compiled_say_t;

compiled_say_t* empty_say( void ) {
    static compiled_say_t say;
    say.cond = *empty_cond();
    say.text = NULL;
    say.act = managed_array( compiled_act_t );
    return &say;
}

typedef struct compiled_dialog_t {
	string_id id;
	array(compiled_act_t)* act;
	array(compiled_phrase_t)* phrase;
	array(compiled_say_t)* say;	
	array(compiled_use_t)* use;
} compiled_dialog_t;

compiled_dialog_t* empty_dialog( void ) {
    static compiled_dialog_t dialog;
    dialog.id = NULL;
	dialog.act = managed_array(compiled_act_t);
	dialog.phrase = managed_array(compiled_phrase_t);
	dialog.say = managed_array(compiled_say_t);	
	dialog.use = managed_array(compiled_use_t);
    return &dialog;
}


typedef struct compiled_character_t {
	string name;	
	string short_name;
	int face_index;
} compiled_character_t;
	
compiled_character_t* empty_character( void ) {
    static compiled_character_t character;
    character.name = NULL;
    character.short_name = NULL;
    character.face_index = -1;
	return &character;
}

typedef struct compiled_globals_t {
    string title;
    string author;
    string start;
    string palette;
    string font_description;
    string font_options;
    string font_characters;
    string font_items;
    string font_name;
    array(int)* logo_indices;
    int background_location;
    int background_dialog;
	int color_background;
	int color_disabled;
	int color_txt;
	int color_opt;
	int color_chr;
	int color_use;
	int color_name;
	int color_facebg;

    bool explicit_flags;
    array(string_id)* flags;

    bool explicit_items;
    array(string_id)* items;
} compiled_globals_t;

compiled_globals_t* empty_globals( void ) {
    static compiled_globals_t globals;
    globals.title = NULL;
    globals.author = NULL;
    globals.start = NULL;
    globals.palette = NULL;
    globals.font_description = NULL;
    globals.font_options = NULL;
    globals.font_characters = NULL;
    globals.font_items = NULL;
    globals.font_name = NULL;
    globals.logo_indices = managed_array(int);
    globals.background_location = -1;
    globals.background_dialog = -1;
	globals.color_background = -1;
	globals.color_disabled = -1;
	globals.color_txt = -1;
	globals.color_opt = -1;
	globals.color_chr = -1;
	globals.color_use = -1;
	globals.color_name = -1;
	globals.color_facebg = -1;
    globals.explicit_flags = false;
    globals.flags = managed_array(string_id);
    globals.explicit_items = false;
    globals.items = managed_array(string_id);
	return &globals;
}

typedef struct compiled_yarn_t {
	compiled_globals_t globals;
    int start_location;
    int start_dialog;
	
	array(string_id)* flag_ids;
	array(string_id)* item_ids;
	array(string_id)* image_names;
    array(string_id)* screen_names;
    array(string_id)* face_names;

	array(string_id)* location_ids;	
	array(compiled_location_t)* locations;
	
	array(string_id)* dialog_ids;
	array(compiled_dialog_t)* dialogs;

	array(string_id)* character_ids;
	array(compiled_character_t)* characters;

    array(flag_t)* flags_modified;
    array(flag_t)* flags_tested;
    array(item_t)* items_got;
    array(item_t)* items_dropped;
    array(item_t)* items_used;
	array(char_t)* chars_referenced;
} compiled_yarn_t;

compiled_yarn_t* empty_yarn( void ) {
    static compiled_yarn_t yarn;
	yarn.globals = *empty_globals();
    yarn.start_location = -1;
    yarn.start_dialog = -1;
	yarn.flag_ids = managed_array(string_id);
	yarn.item_ids = managed_array(string_id);
	yarn.image_names = managed_array(string_id);
    yarn.screen_names = managed_array(string_id);
    yarn.face_names = managed_array(string_id);
    yarn.location_ids = managed_array(string_id);
	yarn.locations = managed_array(compiled_location_t);
	yarn.dialog_ids = managed_array(string_id);
	yarn.dialogs = managed_array(compiled_dialog_t);
    yarn.character_ids = managed_array(string_id);
	yarn.characters = managed_array(compiled_character_t);
    yarn.flags_modified = managed_array(flag_t);
    yarn.flags_tested = managed_array(flag_t);
    yarn.items_got = managed_array(item_t);
    yarn.items_dropped = managed_array(item_t);
    yarn.items_used = managed_array(item_t);
	yarn.chars_referenced = managed_array(char_t);
    return &yarn;
}

int find_item_index( string_id id, compiled_yarn_t* compiled_yarn ) {
    for( int i = 0; i < compiled_yarn->item_ids->count; ++i ) {
        if( compiled_yarn->item_ids->items[ i ] == id ) return i;
    }
    return -1;
}


int find_flag_index( string_id id, compiled_yarn_t* compiled_yarn ) {
    for( int i = 0; i < compiled_yarn->flag_ids->count; ++i )
        if( compiled_yarn->flag_ids->items[ i ] == id ) return i;
    return -1;
}

int find_image_index( string_id name, compiled_yarn_t* compiled_yarn ) {
    for( int i = 0; i < compiled_yarn->image_names->count; ++i )
        if( compiled_yarn->image_names->items[ i ] == name ) return i;
    return -1;
}

int find_screen_index( string_id name, compiled_yarn_t* compiled_yarn ) {
    for( int i = 0; i < compiled_yarn->screen_names->count; ++i )
        if( compiled_yarn->screen_names->items[ i ] == name ) return i;
    return -1;
}

int find_face_index( string_id name, compiled_yarn_t* compiled_yarn ) {
    for( int i = 0; i < compiled_yarn->face_names->count; ++i )
        if( compiled_yarn->face_names->items[ i ] == name ) return i;
    return -1;
}


int find_location_index( string_id id, compiled_yarn_t* compiled_yarn ) {
    for( int i = 0; i < compiled_yarn->location_ids->count; ++i )
        if( compiled_yarn->location_ids->items[ i ] == id ) return i;
    return -1;
}
    
int find_dialog_index( string_id id, compiled_yarn_t* compiled_yarn ) {
    for( int i = 0; i < compiled_yarn->dialog_ids->count; ++i )
        if( compiled_yarn->dialog_ids->items[ i ] == id ) return i;
    return -1;
}

int find_character_index( string_id id, compiled_yarn_t* compiled_yarn ) {
    for( int i = 0; i < compiled_yarn->character_ids->count; ++i )
        if( compiled_yarn->character_ids->items[ i ] == id ) return i;
    return -1;
}

bool skip_word_if_match( string_id* str_param, string word ) {
    string_id str = *str_param;
	if( CMP( cstr_mid( str, 0, cstr_len( word ) ), word ) && isspace( *cstr_mid( str, cstr_len( word ), 1 ) ) ) {
		str = cstr_trim( cstr_mid( str, cstr_len( word ), 0 ) );
        *str_param = str;
		return true;
	}
    *str_param = str;
	return false;
}


bool extract_declaration_fields( parser_section_t* section, compiled_yarn_t* compiled_yarn ) {
    bool no_error = true;
	for( int j = 0; j < section->declarations->count; ++j ) {
		parser_declaration_t* decl = &section->declarations->items[ j ];
		if( CMP( decl->keyword, "?" ) ) {
			for( int i = 0; i < decl->data->count; ++i ) {
				string_id str = cstr_trim( decl->data->items[ i ] );
				skip_word_if_match( &str, "not" );
				if( cstr_len( str ) > 0 ) {
					flag_t flag;
					flag.flag = str;
					flag.filename = decl->filename;
					flag.line_number = decl->line_number;
					add_unique_flag( compiled_yarn->flags_tested, &flag );
					add_unique_id( compiled_yarn->flag_ids, str );
				} else {
					printf( "%s(%d): invalid conditional declaration, flag '%s' is not valid\n", decl->filename, decl->line_number, decl->data->items[ i ] );
					no_error = false;
				}
			}
		}
		if( CMP( decl->keyword,  "img" ) ) {
			if( decl->data->count != 1 || ( decl->data->count == 1 && cstr_len( cstr_trim( decl->data->items[ 0 ] ) ) <= 0 ) ) {
				printf( "%s(%d): invalid image name '%s'\n", decl->filename, decl->line_number, concat_data( decl->data ) );
				no_error = false;
			} else {
				string_id image_name = cstr_cat( "images/", section->declarations->items[ j ].data->items[ 0 ] );
                add_unique_id( compiled_yarn->image_names, image_name );
			}
		} else if( CMP( decl->keyword, "face" ) ) {
			if( decl->data->count != 1 || ( decl->data->count == 1 && cstr_len( cstr_trim( decl->data->items[ 0 ] ) ) <= 0 ) ) {
				printf( "%s(%d): invalid image name '%s'\n", decl->filename, decl->line_number, concat_data( decl->data ) );
				no_error = false;
			} else {
				string_id image_name = cstr_cat( "faces/", section->declarations->items[ j ].data->items[ 0 ] );
                add_unique_id( compiled_yarn->face_names, image_name );
			}
		} else if( CMP( decl->keyword, "act" ) ) {
			if( decl->data->count != 1 || ( decl->data->count == 1 && cstr_len( cstr_trim( decl->data->items[ 0 ] ) ) <= 0 ) ) {
				printf( "%s(%d): invalid 'act:' declaration '%s'\n", decl->filename, decl->line_number, concat_data( decl->data ) );
				no_error = false;
			} else {
				string_id str = decl->data->items[ 0 ];
				if( skip_word_if_match( &str, "set" ) || skip_word_if_match( &str, "clear" ) || skip_word_if_match( &str, "toggle" ) ) {
					flag_t flag;
					flag.flag = str;
					flag.filename = decl->filename;
					flag.line_number = decl->line_number;
					add_unique_flag( compiled_yarn->flags_modified, &flag );
					add_unique_id( compiled_yarn->flag_ids, str );
				} else if( skip_word_if_match( &str, "get" ) ) {
					item_t item;
					item.id = str;
					item.filename = decl->filename;
					item.line_number = decl->line_number;
					add_unique_item( compiled_yarn->items_got, &item );
					add_unique_id( compiled_yarn->item_ids, str );
				} else if( skip_word_if_match( &str, "drop" ) ) {
					item_t item;
					item.id = str;
					item.filename = decl->filename;
					item.line_number = decl->line_number;
					add_unique_item( compiled_yarn->items_dropped, &item );
					add_unique_id( compiled_yarn->item_ids, str );
				}			
			}
		} else if( CMP( decl->keyword, "use" ) ) {
            for( int i = 0; i < decl->data->count; ++i ) {
                string_id item_id = cstr_trim( decl->data->items[ i ] );
                if( cstr_len( item_id ) > 0 ) {
                    if( compiled_yarn->globals.explicit_items && array_find( compiled_yarn->globals.items, item_id ) < 0 ) {
        				printf( "%s(%d): item '%s' used without being declared\n", decl->filename, decl->line_number, item_id );
                        no_error = false;
                    } else {
						item_t item;
						item.id = item_id;
						item.filename = decl->filename;
						item.line_number = decl->line_number;
						add_unique_item( compiled_yarn->items_used, &item );
                        add_unique_id( compiled_yarn->item_ids, item_id );
                    }
                }
            }
        } else if( CMP( decl->keyword, "chr" ) ) {
            for( int i = 0; i < decl->data->count; ++i ) {
                string_id char_id = cstr_trim( decl->data->items[ i ] );
                if( cstr_len( char_id ) > 0 ) {
					char_t chr;
					chr.id = char_id;
					chr.filename = decl->filename;
					chr.line_number = decl->line_number;
					add_unique_char( compiled_yarn->chars_referenced, &chr );
                }
            }
        }
	}
    return no_error;
}


bool verify_opt( compiled_opt_t* opt, parser_declaration_t* decl ) {
    if( !opt ) {
        return true;
    }
    if( opt->act->count == 0 ) {
        printf( "%s(%d): declaration 'opt: %s' does not have an 'act' declaration'\n", decl->filename, decl->line_number, opt->text );
        return false;
    }
    return true;
}

bool verify_say( compiled_say_t* say, parser_declaration_t* decl ) {
    if( !say ) { 
        return true;
    }
    if( say->act->count == 0 ) {
        printf( "%s(%d): declaration 'say: %s' does not have an 'act' declaration'\n", decl->filename, decl->line_number, say->text );
        return false;
    }
    return true;
}

bool verify_use( compiled_use_t* use, parser_declaration_t* decl ) {
    if( !use ) {
        return true;
    }
    if( use->act->count == 0 ) {
        printf( "%s(%d): 'use' declaration does not have an 'act' declaration'\n", decl->filename, decl->line_number );
        return false;
    }
    return true;
}


bool verify_chr( compiled_chr_t* chr, parser_declaration_t* decl ) {
    if( !chr ) {
        return true;
    }
    if( chr->act->count == 0 ) {
        printf( "%s(%d): 'chr' declaration does not have an 'act' declaration'\n", decl->filename, decl->line_number );
        return false;
    }
    return true;
}


bool compile_action( array_param(string)* data_param, compiled_act_t* compiled_action, string filename, int line_number, compiled_yarn_t* compiled_yarn ) {
    array(string)* data = ARRAY_CAST( data_param );
    if( data->count != 1 ) {
        printf( "%s(%d): invalid declaration 'act: %s'\n", filename, line_number, concat_data( data ) );
        return false;
    }
    string_id command = data->items[ 0 ];
    if( CMP( command, "exit" ) ) {
        compiled_action->type = ACTION_TYPE_EXIT;
    } else if( skip_word_if_match( &command, "set" ) ) {
        compiled_action->type = ACTION_TYPE_FLAG_SET;
        int flag_index = find_flag_index( command, compiled_yarn );
        compiled_action->param_flag_index = flag_index;
    } else if( skip_word_if_match( &command, "clear" ) ) {
        compiled_action->type = ACTION_TYPE_FLAG_CLEAR;
        int flag_index = find_flag_index( command, compiled_yarn );
        compiled_action->param_flag_index = flag_index;
    } else if( skip_word_if_match( &command, "toggle" ) ) {
        compiled_action->type = ACTION_TYPE_FLAG_TOGGLE;
        int flag_index = find_flag_index( command, compiled_yarn );
        compiled_action->param_flag_index = flag_index;
    } else if( skip_word_if_match( &command, "get" ) ) {
        compiled_action->type = ACTION_TYPE_ITEM_GET;
        int item_index = find_item_index( command, compiled_yarn );
        compiled_action->param_item_index = item_index;
    } else if( skip_word_if_match( &command, "drop" ) ) {
        compiled_action->type = ACTION_TYPE_ITEM_DROP;
        int item_index = find_item_index( command, compiled_yarn );
        compiled_action->param_item_index = item_index;
    } else if( find_location_index( command, compiled_yarn ) >= 0 ) {
        compiled_action->type = ACTION_TYPE_GOTO_LOCATION;
        int location_index = find_location_index( command, compiled_yarn );
        if( location_index >= 0 ) {
            compiled_action->param_location_index = location_index;
        } else {
            printf( "%s(%d): location '%s' was not declared\n", filename, line_number, command );
            return false;
        }
    } else if( find_dialog_index( command, compiled_yarn ) >= 0 ) {
        compiled_action->type = ACTION_TYPE_GOTO_DIALOG;
        int dialog_index = find_dialog_index( command, compiled_yarn );
        if( dialog_index >= 0 ) {
            compiled_action->param_dialog_index = dialog_index;
        } else {
            printf( "%s(%d): dialog '%s' was not declared\n", filename, line_number, command );
            return false;
        }
    }

    return true;
}
	

bool compile_cond( array_param(string)* data_param, compiled_cond_or_t* compiled_cond, string filename, int line_number, compiled_yarn_t* compiled_yarn ) {
    array(string)* data = ARRAY_CAST( data_param );
	for( int i = 0; i < data->count; ++i ) {
		compiled_cond_flag_t flag;
		flag.is_not = false;
		flag.flag_index = -1;

		string_id str = cstr_trim( data->items[ i ] );
		if( skip_word_if_match( &str, "not" ) ) {
            flag.is_not = true;
        }

		int flag_index = find_flag_index( str, compiled_yarn );
		if( flag_index >= 0 ) {
			flag.flag_index = flag_index;
        } else {
			printf( "%s(%d): invalid conditional declaration, flag '%s' is not recognized\n", filename, line_number, data->items[ i ] );
            return false;
		}
        array_add( compiled_cond->flags, &flag );
	}
    return true;
}
	

bool compile_location( parser_section_t* section, compiled_yarn_t* compiled_yarn ) {
    bool no_error = true;
    compiled_location_t* location = array_add( compiled_yarn->locations, empty_location() );
    location->id = section->id;
    
    compiled_opt_t* opt = 0;
    compiled_use_t* use = 0;
    compiled_chr_t* chr = 0;
    compiled_cond_t* cond = 0;
    compiled_cond_t cond_inst;

    for( int i = 0; i < section->declarations->count; ++i ) {
        parser_declaration_t* decl = &section->declarations->items[ i ];
        if( CMP( decl->keyword, "txt" ) ) {
            if( !opt && !use && !chr ) {
                if( decl->data->count == 1 ) {
					compiled_txt_t* txt = array_add( location->txt, empty_txt() );
					if( cond ) { 
                        txt->cond = *cond; 
                        cond = 0; 
                    }
                    txt->text = cstr_trim( decl->data->items[ 0 ] );
                } else {
					printf( "%s(%d): invalid declaration 'txt: %s'\n", decl->filename, decl->line_number, concat_data( decl->data ) );
					no_error = false;
				}
            } else {
				printf( "%s(%d): 'txt:' declaration not valid inside an 'opt:', 'chr' or 'use:' block\n", decl->filename, decl->line_number );
                no_error = false;
            }
        } else if( CMP( decl->keyword, "img" ) ) {
            if( !opt && !use && !chr ) {
				compiled_img_t* img = array_add( location->img, empty_img() );
				if( cond ) { 
                    img->cond = *cond; 
                    cond = 0; 
                }
                img->image_index = find_image_index( cstr_cat( "images/", decl->data->items[ 0 ] ), compiled_yarn );
                img->image_index += compiled_yarn->screen_names->count;
                // TODO: error checking
            } else {
				printf( "%s(%d): 'img:' declaration not valid inside an 'opt:', 'chr' or 'use:' block\n", decl->filename, decl->line_number );
                no_error = false;
            }
        } else if( CMP( decl->keyword, "act"  ) ) {
            compiled_act_t* action = 0;
            if( opt ) action = array_add( opt->act, empty_act() );
            else if( use ) action = array_add( use->act, empty_act() );
            else if( chr ) action = array_add( chr->act, empty_act() );
            else action = array_add( location->act, empty_act() );
			if( cond ) { action->cond = *cond; cond = 0; }
            no_error = no_error && compile_action( decl->data, action, decl->filename, decl->line_number, compiled_yarn );
        } else if( CMP( decl->keyword, "opt" ) ) {
            if( opt ) no_error = no_error && verify_opt( opt, decl );
            if( use ) no_error = no_error && verify_use( use, decl );
            if( chr ) no_error = no_error && verify_chr( chr, decl );
            use = 0;
            chr = 0;
            opt = array_add( location->opt, empty_opt() );
			if( cond ) { opt->cond = *cond; cond = 0; }
            if( decl->data->count == 1 && cstr_len( cstr_trim( decl->data->items[ 0 ] ) ) > 0 ) {
                opt->text = cstr_trim( decl->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid declaration '%s: %s'\n", decl->filename, decl->line_number, decl->keyword, concat_data( decl->data ) );
                no_error = false;
            }
        } else if( CMP( decl->keyword, "use" ) ) {
            if( opt ) no_error = no_error && verify_opt( opt, decl );
            if( use ) no_error = no_error && verify_use( use, decl );
            if( chr ) no_error = no_error && verify_chr( chr, decl );
            opt = 0;
            chr = 0;
            use = array_add( location->use, empty_use() );
			if( cond ) { use->cond = *cond; cond = 0; }
            
            bool invalid_index = false;
            for( int j = 0; j < decl->data->count; ++j ) {
                if( cstr_len( cstr_trim( decl->data->items[ j ] ) ) > 0 ) {
                    int item_index = find_item_index( cstr_trim( decl->data->items[ j ] ), compiled_yarn );
                    if( item_index >= 0 ) {
                        array_add( use->item_indices, &item_index );
                    } else {
        				printf( "%s(%d): invalid item id '%s'\n", decl->filename, decl->line_number, decl->data->items[ j ]);
                        no_error = false;
                        invalid_index = true;
                    }
                } else {
    				printf( "%s(%d): empty item id in declaration '%s: %s'\n", decl->filename, decl->line_number, decl->keyword, concat_data( decl->data ) );
                    no_error = false;
                }
            }

            if( use->item_indices->count == 0 && !invalid_index ) {
    			printf( "%s(%d): invalid declaration '%s: %s'. Must specify at least one item\n", decl->filename, decl->line_number, decl->keyword, concat_data( decl->data ) );
                no_error = false;
            }
        } else if( CMP( decl->keyword, "chr" ) ) {
            if( opt ) no_error = no_error && verify_opt( opt, decl );
            if( use ) no_error = no_error && verify_use( use, decl );
            if( chr ) no_error = no_error && verify_chr( chr, decl );
            opt = 0;
            use = 0;
            chr = array_add( location->chr, empty_chr() );
			if( cond ) { chr->cond = *cond; cond = 0; }
            
            bool invalid_index = false;
            for( int j = 0; j < decl->data->count; ++j ) {
                if( cstr_len( cstr_trim( decl->data->items[ j ] ) ) > 0 ) {
                    int chr_index = find_character_index( cstr_trim( decl->data->items[ j ] ), compiled_yarn );
                    if( chr_index >= 0 ) {
                        array_add( chr->chr_indices, &chr_index );
                    } else {
        				printf( "%s(%d): invalid character id '%s'\n", decl->filename, decl->line_number, decl->data->items[ j ]);
                        no_error = false;
                        invalid_index = true;
                    }
                } else {
    				printf( "%s(%d): empty character id in declaration '%s: %s'\n", decl->filename, decl->line_number, decl->keyword, concat_data( decl->data ) );
                    no_error = false;
                }
            }

            if( chr->chr_indices->count == 0 && !invalid_index ) {
    			printf( "%s(%d): invalid declaration '%s: %s'. Must specify at least one character\n", decl->filename, decl->line_number, decl->keyword, concat_data( decl->data ) );
                no_error = false;
            }
        } else if( CMP( decl->keyword, "?" ) ) {
            if( !cond ) {
				cond_inst = *empty_cond();
				cond = &cond_inst;
			}
            no_error = no_error && compile_cond( decl->data, array_add( cond->ands, empty_cond_or() ), decl->filename, decl->line_number, compiled_yarn );
        } else {
        	printf( "%s(%d): unknown keyword '%s'\n", decl->filename, decl->line_number, decl->keyword );
            no_error = false;
		}		
    }
    
    if( cond ) {
        parser_declaration_t* last_decl = &section->declarations->items[ section->declarations->count - 1 ];
        printf( "%s(%d): unexpected conditional\n", last_decl->filename, last_decl->line_number );
        no_error = false;
	} 
	       
    if( location->txt->count <= 0 ) {
		printf( "%s(%d): location '%s' does not have a valid 'txt:' declaration\n", section->filename, section->line_number, section->id );
        no_error = false;
    }
    if( section->declarations->count > 0 ) {
        parser_declaration_t* last_decl = &section->declarations->items[ section->declarations->count - 1 ];
        if( opt ) no_error = no_error && verify_opt( opt, last_decl );
        if( use ) no_error = no_error && verify_use( use, last_decl );
    }

	return no_error;
}

	
bool compile_dialog( parser_section_t* section, compiled_yarn_t* compiled_yarn ) {
    bool no_error = true;
    compiled_dialog_t* dialog = array_add( compiled_yarn->dialogs, empty_dialog() );
    dialog->id = section->id;
    
    compiled_say_t* say = 0;
    compiled_use_t* use = 0;
    compiled_cond_t* cond = 0;
    compiled_cond_t cond_inst;

    for( int i = 0; i < section->declarations->count; ++i ) {
        parser_declaration_t* decl = &section->declarations->items[ i ];
        if( cstr_len( decl->keyword ) <= 0 && cstr_len( decl->identifier ) > 0 ) {
            if( !say && !use ) {
                if( decl->data->count == 1 ) {
					compiled_phrase_t* phrase = array_add( dialog->phrase, empty_phrase() );
					if( cond ) { phrase->cond = *cond; cond = 0; }
					if( CMP( decl->identifier, "player" ) ) {
						phrase->character_index = -1;
                    } else {
						phrase->character_index = find_character_index( decl->identifier, compiled_yarn ); // TODO: error check
                    }
					phrase->text = cstr_trim( decl->data->items[ 0 ] );
                } else {
					printf( "%s(%d): invalid declaration '%s: %s'\n", decl->filename, decl->line_number, decl->identifier, concat_data( decl->data ) );
					no_error = false;
				}
            } else {
				printf( "%s(%d): phrase declaration not valid inside a 'say:' or 'use:' block\n", decl->filename, decl->line_number );
                no_error = false;
            }
		} else if( CMP( decl->keyword, "act" ) ) {
            compiled_act_t* action = 0;
            if( say ) action = array_add( say->act, empty_act() );
            else if( use ) action = array_add( use->act, empty_act() );
            else action =array_add( dialog->act, empty_act() );
			if( cond ) { action->cond = *cond; cond = 0; }
            no_error = no_error && compile_action( decl->data, action, decl->filename, decl->line_number, compiled_yarn );
        } else if( CMP( decl->keyword, "say" ) ) {
            if( say ) no_error = no_error && verify_say( say, decl );
            if( use ) no_error = no_error && verify_use( use, decl );
            use = 0;
            say = array_add( dialog->say, empty_say() );
			if( cond ) { say->cond = *cond; cond = 0; }
            if( decl->data->count == 1 && cstr_len( cstr_trim( decl->data->items[ 0 ] ) ) > 0 ) {
                say->text = cstr_trim( decl->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid declaration '%s: %s'\n", decl->filename, decl->line_number, decl->keyword, concat_data( decl->data ) );
                no_error = false;
            }
        }
         else if( CMP( decl->keyword, "use" ) ) {
            if( say ) no_error = no_error && verify_say( say, decl );
            if( use ) no_error = no_error && verify_use( use, decl );
            say = 0;
            use = array_add( dialog->use, empty_use() );
			if( cond ) { use->cond = *cond; cond = 0; }
            
            bool invalid_index = false;
            for( int j = 0; j < decl->data->count; ++j ) {
                if( cstr_len( cstr_trim( decl->data->items[ j ] ) ) > 0 ) {
                    int item_index = find_item_index( cstr_trim( decl->data->items[ j ] ), compiled_yarn );
                    if( item_index >= 0 ) {
                        array_add( use->item_indices, &item_index );
                    } else {
        				printf( "%s(%d): invalid item id '%s'\n", decl->filename, decl->line_number, decl->data->items[ j ]);
                        no_error = false;
                        invalid_index = true;
                    }
                } else {
    				printf( "%s(%d): empty item id in declaration '%s: %s'\n", decl->filename, decl->line_number, decl->keyword, concat_data( decl->data ) );
                    no_error = false;
                }
            }

            if( use->item_indices->count == 0 && !invalid_index ) {
    			printf( "%s(%d): invalid declaration '%s: %s'. Must specify at least one item\n", decl->filename, decl->line_number, decl->keyword, concat_data( decl->data ) );
                no_error = false;
            }
        } else if( CMP( decl->keyword, "?" ) ) {
            if( !cond ) {
				cond_inst = *empty_cond();
				cond = &cond_inst;
			}
            no_error = no_error && compile_cond( decl->data, array_add( cond->ands, empty_cond_or() ), decl->filename, decl->line_number, compiled_yarn );
        } else {
        	printf( "%s(%d): unknown keyword '%s'\n", decl->filename, decl->line_number, decl->keyword );
            no_error = false;
		}		
    }
    
    if( cond ) {
        parser_declaration_t* last_decl = &section->declarations->items[ section->declarations->count - 1 ];
        printf( "%s(%d): unexpected conditional\n", last_decl->filename, last_decl->line_number );
        no_error = false;
		} 
	       
    if( dialog->phrase->count <= 0 && dialog->say->count <= 0 ) {
		printf( "%s(%d): dialog '%s' does not contain any phrase or say declaration\n", section->filename, section->line_number, section->id );
        no_error = false;
    }
    if( section->declarations->count > 0 ) {
        parser_declaration_t* last_decl = &section->declarations->items[ section->declarations->count - 1 ];
        if( say ) no_error = no_error && verify_say( say, last_decl );
        if( use ) no_error = no_error && verify_use( use, last_decl );
    }

	return no_error;
}
	

bool compile_character( parser_section_t* section, compiled_yarn_t* compiled_yarn ) {
	bool no_error = true;
	
	compiled_character_t* character = array_add( compiled_yarn->characters, empty_character() ) ;
	for( int i = 0; i < section->declarations->count; ++i ) {
        parser_declaration_t* decl = &section->declarations->items[ i ];
        if( CMP( decl->keyword, "name" ) ) {
			character->name = cstr_trim( decl->data->items[ 0 ] );
		} else if( CMP( decl->keyword, "short" ) ) {
			character->short_name = cstr_trim( decl->data->items[ 0 ] );
		} else if( CMP( decl->keyword, "face" ) ) {
			character->face_index = find_face_index( cstr_cat( "faces/", decl->data->items[ 0 ] ), compiled_yarn );
			character->face_index += compiled_yarn->screen_names->count;
			character->face_index += compiled_yarn->image_names->count;
		} else {
			printf( "%s(%d): unexpected keyword '%s'. character sections may only contain 'name', 'short' and 'face' keywords\n", decl->filename, decl->line_number, decl->keyword );
			no_error = false;
		}
	}
		
	return no_error;
}
	
	
bool compile_globals( array_param(parser_global_t)* globals_param, compiled_yarn_t* compiled_yarn )	 {
    array(parser_global_t)* globals = ARRAY_CAST( globals_param);
    bool no_error = true;
    
    bool found_logo = false;
    compiled_yarn->globals.explicit_flags = false;
    compiled_yarn->globals.explicit_items = false;
    compiled_yarn->globals.background_location = -1;
    compiled_yarn->globals.background_dialog = -1;
	compiled_yarn->globals.color_background = -1;
	compiled_yarn->globals.color_disabled = -1;
	compiled_yarn->globals.color_txt = -1;
	compiled_yarn->globals.color_opt = -1;
	compiled_yarn->globals.color_chr = -1;
	compiled_yarn->globals.color_use = -1;
	compiled_yarn->globals.color_name = -1;
	compiled_yarn->globals.color_facebg = -1;
    compiled_yarn->start_location = -1;
    compiled_yarn->start_dialog = -1;

    
    for( int i = 0; i < globals->count; ++i ) {
        parser_global_t* global = &globals->items[ i ];
        if( CMP( global->keyword, "title" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.title = cstr_trim( global->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid title declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "author" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.author = cstr_trim( global->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid author declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "start" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.start = cstr_trim( global->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid start declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "palette" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.palette = cstr_cat( "palettes/", cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid palette declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "font_description" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.font_description = cstr_trim( global->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid font_description declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "font_options" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.font_options = cstr_trim( global->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid font_options declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "font_characters" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.font_characters = cstr_trim( global->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid font_characters declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "font_items" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.font_items = cstr_trim( global->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid font_items declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "font_name" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.font_name = cstr_trim( global->data->items[ 0 ] );
            } else {
				printf( "%s(%d): invalid font_name declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "flags" ) ) {
            compiled_yarn->globals.explicit_flags = true;
            for( int j = 0; j < global->data->count; ++j ) {
                if( cstr_len( cstr_trim( global->data->items[ j ] ) ) > 0 ) {
                    string_id flag = cstr_trim( global->data->items[ j ] );
                    array_add( compiled_yarn->globals.flags, &flag );
                } else {
				    printf( "%s(%d): invalid flags declaration '%s: %s'. Flag index %d is empty\n", global->filename, global->line_number, global->keyword, concat_data( global->data ), j + 1 );
                    no_error = false;
                }
            }
        } else if( CMP( global->keyword, "items" ) ) {
            compiled_yarn->globals.explicit_items = true;
            for( int j = 0; j < global->data->count; ++j ) {
                if( cstr_len( cstr_trim( global->data->items[ j ] ) ) > 0 ) {
                    string_id item = cstr_trim( global->data->items[ j ] );
                    array_add( compiled_yarn->globals.items, &item );
                } else {
				    printf( "%s(%d): invalid items declaration '%s: %s'. Item index %d is empty\n", global->filename, global->line_number, global->keyword, concat_data( global->data ), j + 1 );
                    no_error = false;
                }
            }
        } else if( CMP( global->keyword, "logo" ) ) {
            found_logo = true;
            for( int j = 0; j < global->data->count; ++j ) {
                if( cstr_len( cstr_trim( global->data->items[ j ] ) ) > 0 ) {
                    int image_index = find_screen_index( cstr_cat( "images/", cstr_trim( global->data->items[ j ] ) ), compiled_yarn );
                    // TODO: error checking
                    array_add( compiled_yarn->globals.logo_indices, &image_index );
                }
            }
        } else if( CMP( global->keyword, "background_location" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
				int image_index = find_screen_index( cstr_cat( "images/", cstr_trim( global->data->items[ 0 ] ) ), compiled_yarn );
                compiled_yarn->globals.background_location = image_index;
            } else {
				printf( "%s(%d): invalid background_location declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "background_dialog" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
				int image_index = find_screen_index( cstr_cat( "images/", cstr_trim( global->data->items[ 0 ] ) ), compiled_yarn );
                compiled_yarn->globals.background_dialog = image_index;
            } else {
				printf( "%s(%d): invalid background_dialog declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "color_background" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.color_background = atoi( cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid color declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "color_disabled" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.color_disabled = atoi( cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid color declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        }
         else if( CMP( global->keyword, "color_txt" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.color_txt = atoi( cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid color declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "color_opt" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.color_opt = atoi( cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid color declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "color_chr" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.color_chr = atoi( cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid color declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "color_use" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.color_use = atoi( cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid color declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "color_name" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.color_name = atoi( cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid color declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "color_facebg" ) ) {
            if( global->data->count == 1 && cstr_len( cstr_trim( global->data->items[ 0 ] ) ) > 0 ) {
                compiled_yarn->globals.color_facebg = atoi( cstr_trim( global->data->items[ 0 ] ) );
            } else {
				printf( "%s(%d): invalid color declaration '%s: %s'\n", global->filename, global->line_number, global->keyword, concat_data( global->data ) );
                no_error = false;
            }
        } else if( CMP( global->keyword, "?" ) ) {
        	printf( "%s(%d): conditionals not allowed for global declarations\n", global->filename, global->line_number );
            no_error = false;
		} else {
			printf( "%s(%d): invalid global declaration '%s'\n", global->filename, global->line_number, global->keyword );
            no_error = false;
        }
    }
      
    if( !found_logo ) {
        int image_index = find_screen_index( "images/yarnspin_logo.png", compiled_yarn );
        // TODO: error checking
        array_add( compiled_yarn->globals.logo_indices, &image_index );
    }


    if( cstr_len( compiled_yarn->globals.title ) <= 0 ) {
        printf( "No title defined. Add a global declaration of the form 'title: My Game' to one of your script files\n" );
        no_error = false;
    }

    if( cstr_len( compiled_yarn->globals.author ) <= 0 ) {
        printf( "No author defined. Add a global declaration of the form 'author: Jane Doe' to one of your script files\n" );
        no_error = false;
    }
  
    if( cstr_len( compiled_yarn->globals.palette ) <= 0 ) {
        compiled_yarn->globals.palette = "palettes/yarnspin_default.png";
    }
    return no_error;
}
	

bool yarn_compiler( array_param(parser_global_t)* parser_globals_param, array_param(parser_section_t)* parser_sections_param, compiled_yarn_t* compiled_yarn ) {
    array(parser_global_t)* parser_globals = ARRAY_CAST( parser_globals_param );
    array(parser_section_t)* parser_sections = ARRAY_CAST( parser_sections_param );

	*compiled_yarn = *empty_yarn();

	bool no_error = true;
	for( int i = 0; i < parser_sections->count; ++i ) {
		parser_section_t* a = &parser_sections->items[ i ];
		for( int j = i + 1; j < parser_sections->count; ++j ) {
			parser_section_t* b = &parser_sections->items[ j ];
			if( CMP( a->id, b->id ) ) {
				printf( "%s(%d): duplicate section id '%s'\n", a->filename, a->line_number, a->id );
				printf( "%s(%d): duplicate section id '%s'\n", b->filename, b->line_number, b->id );
				no_error = false;
			}
		}
	}
		
	for( int i = 0; i < parser_globals->count; ++i ) {
		parser_global_t* a = &parser_globals->items[ i ];
        if( CMP( a->keyword, "flags" ) || CMP( a->keyword, "items" ) ) continue;

		for( int j = i + 1; j < parser_globals->count; ++j ) {
			parser_global_t* b = &parser_globals->items[ j ];
			if( CMP( a->keyword, b->keyword ) ) {
				printf( "%s(%d): duplicate global declaration '%s: %s'\n", a->filename, a->line_number, a->keyword, concat_data( a->data ) );
				printf( "%s(%d): duplicate global declaration '%s: %s'\n", b->filename, b->line_number, b->keyword, concat_data( b->data ) );
				no_error = false;
			}
		}
	}

	bool found_logo = false;
	for( int i = 0; i < parser_globals->count; ++i ) {
		parser_global_t* global = &parser_globals->items[ i ];
        if( CMP( global->keyword, "logo" ) ) {
            found_logo = true;
            for( int j = 0; j < global->data->count; ++j ) {
                string_id logo_name = cstr_cat( "images/", cstr_trim( global->data->items[ j ] ) );
                if( cstr_len( logo_name ) > 0 ) add_unique_id( compiled_yarn->screen_names, logo_name );
            }
        } else if( CMP( global->keyword, "background_location" ) || CMP( global->keyword, "background_dialog" ) ) {
            for( int j = 0; j < global->data->count; ++j ) {
                string_id screen_name = cstr_cat( "images/", cstr_trim( global->data->items[ j ] ) );
                if( cstr_len( screen_name ) > 0 ) add_unique_id( compiled_yarn->screen_names, screen_name );
            }
        }
    }
        
    if( !found_logo ) {
        add_unique_id( compiled_yarn->screen_names, "images/yarnspin_logo.png" );
    }
        
    if( !no_error ) return false;

    no_error = no_error && compile_globals( parser_globals, compiled_yarn );
				
	for( int i = 0; i < parser_sections->count; ++i ) {
		parser_section_t* section = &parser_sections->items[ i ];
		
		if( section->type == SECTION_TYPE_LOCATION ) {
			array_add( compiled_yarn->location_ids, &section->id );
            no_error = no_error && extract_declaration_fields( section, compiled_yarn );
		} else if( section->type == SECTION_TYPE_DIALOG ) {
			array_add( compiled_yarn->dialog_ids, &section->id );
            no_error = no_error && extract_declaration_fields( section, compiled_yarn );
		} else if( section->type == SECTION_TYPE_CHARACTER ) {
			array_add( compiled_yarn->character_ids, &section->id );
            no_error = no_error && extract_declaration_fields( section, compiled_yarn );
		} else {
			printf( "%s(%d): invalid section '%s'\n", section->filename, section->line_number, section->id );
			no_error = false;
		}
	}
		
	for( int i = 0; i < compiled_yarn->flags_tested->count; ++i ) {
		flag_t a = compiled_yarn->flags_tested->items[ i ];
		bool found = false;
		for( int j = 0; j < compiled_yarn->flags_modified->count; ++j ) {
			flag_t b = compiled_yarn->flags_modified->items[ j ];
			if( CMP( a.flag, b.flag ) ) found = true;
		}
		if( !found ) {
			printf( "%s(%d): flag '%s' is tested, but never modified\n", a.filename, a.line_number, a.flag );
			no_error = false;
		}
	}
		
	for( int i = 0; i < compiled_yarn->flags_modified->count; ++i ) {
		flag_t a = compiled_yarn->flags_modified->items[ i ];
		bool found = false;
		for( int j = 0; j < compiled_yarn->flags_tested->count; ++j ) {
			flag_t b = compiled_yarn->flags_tested->items[ j ];
			if( CMP( a.flag, b.flag ) ) found = true;
		}
		if( !found ) {
			printf( "%s(%d): flag '%s' is modified, but never tested\n", a.filename, a.line_number, a.flag );
			no_error = false;
		}
	}
		
	for( int i = 0; i < compiled_yarn->chars_referenced->count; ++i ) {
		char_t chr = compiled_yarn->chars_referenced->items[ i ];
		bool found = false;
		for( int j = 0; j < compiled_yarn->character_ids->count; ++j ) {
			string_id id = compiled_yarn->character_ids->items[ j ];
			if( chr.id == id) found = true;
		}
		if( !found ) {
			printf( "%s(%d): character'%s' is referenced, but not declared\n", chr.filename, chr.line_number, chr.id );
			no_error = false;
		}
	}

	for( int i = 0; i < parser_sections->count; ++i ) {
		parser_section_t* section = &parser_sections->items[ i ];
		
        if( section->type == SECTION_TYPE_LOCATION ) {
			no_error = no_error && compile_location( section, compiled_yarn );
        } else if( section->type == SECTION_TYPE_DIALOG ) {
			no_error = no_error && compile_dialog( section, compiled_yarn );
        } else if( section->type == SECTION_TYPE_CHARACTER ) {
			no_error = no_error && compile_character( section, compiled_yarn );
        } else {
			printf( "%s(%d): invalid section '%s'\n", section->filename, section->line_number, section->id );
			no_error = false;
		}
	}
		
	compiled_yarn->start_location = find_location_index( compiled_yarn->globals.start, compiled_yarn );
	compiled_yarn->start_dialog = find_dialog_index( compiled_yarn->globals.start, compiled_yarn );
	if( compiled_yarn->start_location < 0 && compiled_yarn->start_dialog < 0 ) {
		printf( "No start section defined. Add a global declaration of the form 'start: first_section_id' to one of your script files\n" );
		no_error = false;
	}

	return no_error;
}
