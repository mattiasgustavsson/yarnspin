
typedef string string_id; // strings of type string_id should be compared with case insensitive comparison


int buffer_write_string( buffer_t* buffer, char const* const* value, int count ) {
    for( int i = 0; i < count; ++i ) {
        char const* str = value[ i ];
        if( !str ) str = "";
        int len = (int) strlen( str ) + 1;
        buffer_write_i32( buffer, &len, 1 );
        if( buffer_write_i8( buffer, str, len ) != len ) {
            return i;
        }
    }
    return count;
}

string read_string( buffer_t* in ) {
    int len = 0;
    buffer_read_i32( in, &len, 1 );
    char* temp = cstr_temp_buffer( (size_t) len );
    buffer_read_i8( in, temp, len );
    return cstr( temp );
}


int read_int( buffer_t* in ) {
    int value = 0;
    buffer_read_i32( in, &value, 1 );
    return value;
}


bool read_bool( buffer_t* in ) {
    bool value = 0;
    buffer_read_bool( in, &value, 1 );
    return value;
}


void read_string_array( buffer_t* in, array_param(string)* array ) {
    int count = read_int( in );
    for( int i = 0; i < count; ++i ) {
        string_id value = read_string( in );
        array_add( array, &value );
    }
}


typedef struct yarn_cond_flag_t {
    bool is_not;
    int flag_index;
} yarn_cond_flag_t;


typedef struct yarn_cond_or_t {
    array(yarn_cond_flag_t)* flags;
} yarn_cond_or_t ;


yarn_cond_or_t* empty_cond_or( void ) {
    static yarn_cond_or_t cond_or;
    cond_or.flags = managed_array( yarn_cond_flag_t );
    return &cond_or;
}


void save_cond_or( buffer_t* out, yarn_cond_or_t* cond_or ) {
    buffer_write_i32( out, &cond_or->flags->count, 1 );
    for( int i = 0; i < cond_or->flags->count; ++i ) {
        buffer_write_bool( out, &cond_or->flags->items[ i ].is_not, 1 );
        buffer_write_i32( out, &cond_or->flags->items[ i ].flag_index, 1 );
    }
}


typedef struct yarn_cond_t {
    array(yarn_cond_or_t)* ands;
} yarn_cond_t;


yarn_cond_t* empty_cond( void ) {
    static yarn_cond_t cond;
    cond.ands = managed_array( yarn_cond_or_t );
    return &cond;
}


void save_cond( buffer_t* out, yarn_cond_t* cond ) {
    buffer_write_i32( out, &cond->ands->count, 1 );
    for( int i = 0; i < cond->ands->count; ++i ) {
        save_cond_or( out, &cond->ands->items[ i ] );
    }
}


void load_cond( buffer_t* in, yarn_cond_t* cond ) {
    cond->ands = managed_array( yarn_cond_or_t );
    int ands_count = read_int( in );
    for( int i = 0; i < ands_count; ++i ) {
        yarn_cond_or_t cond_or;
        cond_or.flags = managed_array( yarn_cond_flag_t );
        int flags_count = read_int( in );
        for( int j = 0; j < flags_count; ++j ) {
            yarn_cond_flag_t flag;
            flag.is_not = read_bool( in );
            flag.flag_index = read_int( in );
            array_add( cond_or.flags, &flag);
        }
        array_add( cond->ands, &cond_or );
    }
}


typedef enum yarn_action_type_t {
    ACTION_TYPE_NONE,
    ACTION_TYPE_GOTO_LOCATION,
    ACTION_TYPE_GOTO_DIALOG,
    ACTION_TYPE_EXIT,
    ACTION_TYPE_FLAG_SET,
    ACTION_TYPE_FLAG_CLEAR,
    ACTION_TYPE_FLAG_TOGGLE,
    ACTION_TYPE_ITEM_GET,
    ACTION_TYPE_ITEM_DROP,
} yarn_action_type_t;


typedef struct yarn_act_t {
    yarn_cond_t cond;
    yarn_action_type_t type;
    int param_location_index;
    int param_dialog_index;
    int param_flag_index;
    int param_item_index;
} yarn_act_t;


yarn_act_t* empty_act( void ) {
    static yarn_act_t act;
    act.cond = *empty_cond();
    act.type = ACTION_TYPE_NONE;
    act.param_location_index = -1;
    act.param_dialog_index = -1;
    act.param_flag_index = -1;
    act.param_item_index = -1;
    return &act;
}


void save_act( buffer_t* out, yarn_act_t* act ) {
    save_cond( out, &act->cond );
    int type = (int)act->type;
    buffer_write_i32( out, &type, 1 );
    buffer_write_i32( out, &act->param_location_index, 1 );
    buffer_write_i32( out, &act->param_dialog_index, 1 );
    buffer_write_i32( out, &act->param_flag_index, 1 );
    buffer_write_i32( out, &act->param_item_index, 1 );
}


void load_act( buffer_t* in, yarn_act_t* act ) {
    load_cond( in, &act->cond );
    act->type = (yarn_action_type_t) read_int( in );
    act->param_location_index = read_int( in );
    act->param_dialog_index = read_int( in );
    act->param_flag_index = read_int( in );
    act->param_item_index = read_int( in );
}


typedef struct yarn_opt_t {
    yarn_cond_t cond;
    string text;
    array(yarn_act_t)* act;
} yarn_opt_t;


yarn_opt_t* empty_opt( void ) {
    static yarn_opt_t opt;
    opt.cond = *empty_cond();
    opt.text = NULL;
    opt.act = managed_array( yarn_act_t );
    return &opt;
}


void save_opt( buffer_t* out, yarn_opt_t* opt ) {
    save_cond( out, &opt->cond );
    buffer_write_string( out, &opt->text, 1 );
    buffer_write_i32( out, &opt->act->count, 1 );
    for( int i = 0; i < opt->act->count; ++i ) {
        save_act( out, &opt->act->items[ i ] );
    }
}


typedef struct yarn_use_t {
    yarn_cond_t cond;
    array(int)* item_indices;
    array(yarn_act_t)* act;
} yarn_use_t;


yarn_use_t* empty_use( void ) {
    static yarn_use_t use;
    use.cond = *empty_cond();
    use.item_indices = managed_array( int );
    use.act = managed_array( yarn_act_t );
    return &use;
}


void save_use( buffer_t* out, yarn_use_t* use ) {
    save_cond( out, &use->cond );

    buffer_write_i32( out, &use->item_indices->count, 1 );
    buffer_write_i32( out, use->item_indices->items, use->item_indices->count );

    buffer_write_i32( out, &use->act->count, 1 );
    for( int i = 0; i < use->act->count; ++i ) {
        save_act( out, &use->act->items[ i ] );
    }
}


typedef struct yarn_chr_t {
    yarn_cond_t cond;
    array(int)* chr_indices;
    array(yarn_act_t)* act;
} yarn_chr_t;


yarn_chr_t* empty_chr( void ) {
    static yarn_chr_t chr;
    chr.cond = *empty_cond();
    chr.chr_indices = managed_array( int );
    chr.act = managed_array( yarn_act_t );
    return &chr;
}


void save_chr( buffer_t* out, yarn_chr_t* chr ) {
    save_cond( out, &chr->cond );

    buffer_write_i32( out, &chr->chr_indices->count, 1 );
    buffer_write_i32( out, chr->chr_indices->items, chr->chr_indices->count );

    buffer_write_i32( out, &chr->act->count, 1 );
    for( int i = 0; i < chr->act->count; ++i ) {
        save_act( out, &chr->act->items[ i ] );
    }
}


typedef struct yarn_img_t {
    yarn_cond_t cond;
    int image_index;
} yarn_img_t;


yarn_img_t* empty_img( void ) {
    static yarn_img_t img;
    img.cond = *empty_cond();
    img.image_index = -1;
    return &img;
}


typedef struct yarn_txt_t {
    yarn_cond_t cond;
    string text;
} yarn_txt_t;


yarn_txt_t* empty_txt( void ) {
    static yarn_txt_t txt;
    txt.cond = *empty_cond();
    txt.text = NULL;
    return &txt;
}


typedef struct yarn_location_t {
    string_id id;
    array(yarn_img_t)* img;
    array(yarn_txt_t)* txt;
    array(yarn_act_t)* act;
    array(yarn_opt_t)* opt;
    array(yarn_use_t)* use;
    array(yarn_chr_t)* chr;
} yarn_location_t;


yarn_location_t* empty_location( void ) {
    static yarn_location_t location;
    location.id = NULL;
    location.img = managed_array(yarn_img_t);
    location.txt = managed_array(yarn_txt_t);
    location.act = managed_array(yarn_act_t);
    location.opt = managed_array(yarn_opt_t);
    location.use = managed_array(yarn_use_t);
    location.chr = managed_array(yarn_chr_t);
    return &location;
}


void save_location( buffer_t* out, yarn_location_t* location ) {
    buffer_write_string( out, &location->id, 1 );

    buffer_write_i32( out, &location->img->count, 1 );
    for( int i = 0; i < location->img->count; ++i ) {
        save_cond( out, &location->img->items[ i ].cond );
        buffer_write_i32( out, &location->img->items[ i ].image_index, 1 );
    }

    buffer_write_i32( out, &location->txt->count, 1 );
    for( int i = 0; i < location->txt->count; ++i ) {
        save_cond( out, &location->txt->items[ i ].cond );
        buffer_write_string( out, &location->txt->items[ i ].text, 1 );
    }

    buffer_write_i32( out, &location->act->count, 1 );
    for( int i = 0; i < location->act->count; ++i ) {
        save_act( out, &location->act->items[ i ] );
    }

    buffer_write_i32( out, &location->opt->count, 1 );
    for( int i = 0; i < location->opt->count; ++i ) {
        save_opt( out, &location->opt->items[ i ] );
    }

    buffer_write_i32( out, &location->use->count, 1 );
    for( int i = 0; i < location->use->count; ++i ) {
        save_use( out, &location->use->items[ i ] );
    }

    buffer_write_i32( out, &location->chr->count, 1 );
    for( int i = 0; i < location->chr->count; ++i ) {
        save_chr( out, &location->chr->items[ i ] );
    }
}


void load_location( buffer_t* in, yarn_location_t* location ) {
    location->id = read_string( in );

    location->img = managed_array(yarn_img_t);
    int imgs_count = read_int( in );
    for( int i = 0; i < imgs_count; ++i ) {
        yarn_img_t img;
        load_cond( in, &img.cond );
        img.image_index = read_int( in );
        array_add( location->img, &img );
    }

    location->txt = managed_array(yarn_txt_t);
    int txts_count = read_int( in );
    for( int i = 0; i < txts_count; ++i ) {
        yarn_txt_t txt;
        load_cond( in, &txt.cond );
        txt.text = read_string( in );
        array_add( location->txt, &txt );
    }

    location->act = managed_array(yarn_act_t);
    int acts_count = read_int( in );
    for( int i = 0; i < acts_count; ++i ) {
        yarn_act_t act;
        load_act( in, &act );
        array_add( location->act, &act );
    }

    location->opt = managed_array(yarn_opt_t);
    int opts_count = read_int( in );
    for( int i = 0; i < opts_count; ++i ) {
        yarn_opt_t opt;
        load_cond( in, &opt.cond );
        opt.text = read_string( in );
        opt.act = managed_array(yarn_act_t);
        int count = read_int( in );
        for( int j = 0; j < count; ++j ) {
            yarn_act_t act;
            load_act( in, &act );
            array_add( opt.act, &act);
        }
        array_add( location->opt, &opt );
    }

    location->use = managed_array(yarn_use_t);
    int uses_count = read_int( in );
    for( int i = 0; i < uses_count; ++i ) {
        yarn_use_t use;
        load_cond( in, &use.cond );
        use.item_indices = managed_array(int);
        int index_count = read_int( in );
        for( int j = 0; j < index_count; ++j ) {
            int value = read_int( in );
            array_add( use.item_indices, &value );
        }
        use.act = managed_array(yarn_act_t);
        int count = read_int( in );
        for( int j = 0; j < count; ++j ) {
            yarn_act_t act;
            load_act( in, &act );
            array_add( use.act, &act);
        }
        array_add( location->use, &use );
    }

    location->chr = managed_array(yarn_chr_t);
    int chrs_count = read_int( in );
    for( int i = 0; i < chrs_count; ++i ) {
        yarn_chr_t chr;
        load_cond( in, &chr.cond );
        chr.chr_indices = managed_array(int);
        int index_count = read_int( in );
        for( int j = 0; j < index_count; ++j ) {
            int value = read_int( in );
            array_add( chr.chr_indices, &value );
        }
        chr.act = managed_array(yarn_act_t);
        int count = read_int( in );
        for( int j = 0; j < count; ++j ) {
            yarn_act_t act;
            load_act( in, &act );
            array_add( chr.act, &act);
        }
        array_add( location->chr, &chr );
    }

}


typedef struct yarn_phrase_t {
    yarn_cond_t cond;
    int character_index;
    string text;
} yarn_phrase_t;


yarn_phrase_t* empty_phrase( void ) {
    static yarn_phrase_t phrase;
    phrase.cond = *empty_cond();
    phrase.character_index = -1;
    phrase.text = NULL;
    return &phrase;
}


typedef struct yarn_say_t {
    yarn_cond_t cond;
    string text;
    array(yarn_act_t)* act;
} yarn_say_t;


yarn_say_t* empty_say( void ) {
    static yarn_say_t say;
    say.cond = *empty_cond();
    say.text = NULL;
    say.act = managed_array( yarn_act_t );
    return &say;
}


void save_say( buffer_t* out, yarn_say_t* say ) {
    save_cond( out, &say->cond );
    buffer_write_string( out, &say->text, 1 );
    buffer_write_i32( out, &say->act->count, 1 );
    for( int i = 0; i < say->act->count; ++i ) {
        save_act( out, &say->act->items[ i ] );
    }
}


typedef struct yarn_dialog_t {
    string_id id;
    array(yarn_act_t)* act;
    array(yarn_phrase_t)* phrase;
    array(yarn_say_t)* say;
    array(yarn_use_t)* use;
} yarn_dialog_t;


yarn_dialog_t* empty_dialog( void ) {
    static yarn_dialog_t dialog;
    dialog.id = NULL;
    dialog.act = managed_array(yarn_act_t);
    dialog.phrase = managed_array(yarn_phrase_t);
    dialog.say = managed_array(yarn_say_t);
    dialog.use = managed_array(yarn_use_t);
    return &dialog;
}


void save_dialog( buffer_t* out, yarn_dialog_t* dialog ) {
    buffer_write_string( out, &dialog->id, 1 );

    buffer_write_i32( out, &dialog->act->count, 1 );
    for( int i = 0; i < dialog->act->count; ++i ) {
        save_act( out, &dialog->act->items[ i ] );
    }

    buffer_write_i32( out, &dialog->phrase->count, 1 );
    for( int i = 0; i < dialog->phrase->count; ++i ) {
        save_cond( out, &dialog->phrase->items[ i ].cond );
        buffer_write_i32( out, &dialog->phrase->items[ i ].character_index, 1 );
        buffer_write_string( out, &dialog->phrase->items[ i ].text, 1 );
    }

    buffer_write_i32( out, &dialog->say->count, 1 );
    for( int i = 0; i < dialog->say->count; ++i ) {
        save_say( out, &dialog->say->items[ i ] );
    }

    buffer_write_i32( out, &dialog->use->count, 1 );
    for( int i = 0; i < dialog->use->count; ++i ) {
        save_use( out, &dialog->use->items[ i ] );
    }
}


void load_dialog( buffer_t* in, yarn_dialog_t* dialog ) {
    dialog->id = read_string( in );

    dialog->act = managed_array(yarn_act_t);
    int acts_count = read_int( in );
    for( int i = 0; i < acts_count; ++i ) {
        yarn_act_t act;
        load_act( in, &act );
        array_add( dialog->act, &act );
    }

    dialog->phrase = managed_array(yarn_phrase_t);
    int phrases_count = read_int( in );
    for( int i = 0; i < phrases_count; ++i ) {
        yarn_phrase_t phrase;
        load_cond( in, &phrase.cond );
        phrase.character_index = read_int( in );
        phrase.text = read_string( in );
        array_add( dialog->phrase, &phrase );
    }

    dialog->say = managed_array(yarn_say_t);
    int says_count = read_int( in );
    for( int i = 0; i < says_count; ++i ) {
        yarn_say_t say;
        load_cond( in, &say.cond );
        say.text = read_string( in );
        say.act = managed_array(yarn_act_t);
        int count = read_int( in );
        for( int j = 0; j < count; ++j ) {
            yarn_act_t act;
            load_act( in, &act );
            array_add( say.act, &act);
        }
        array_add( dialog->say, &say );
    }


    dialog->use = managed_array(yarn_use_t);
    int uses_count = read_int( in );
    for( int i = 0; i < uses_count; ++i ) {
        yarn_use_t use;
        load_cond( in, &use.cond );
        use.item_indices = managed_array(int);
        int index_count = read_int( in );
        for( int j = 0; j < index_count; ++j ) {
            int value = read_int( in );
            array_add( use.item_indices, &value );
        }
        use.act = managed_array(yarn_act_t);
        int count = read_int( in );
        for( int j = 0; j < count; ++j ) {
            yarn_act_t act;
            load_act( in, &act );
            array_add( use.act, &act);
        }
        array_add( dialog->use, &use );
    }
}


typedef struct yarn_character_t {
    string name;
    string short_name;
    int face_index;
} yarn_character_t;


yarn_character_t* empty_character( void ) {
    static yarn_character_t character;
    character.name = NULL;
    character.short_name = NULL;
    character.face_index = -1;
    return &character;
}


void save_character( buffer_t* out, yarn_character_t* character ) {
    buffer_write_string( out, &character->name, 1 );
    buffer_write_string( out, &character->short_name, 1 );
    buffer_write_i32( out, &character->face_index, 1 );
}


void load_character( buffer_t* in, yarn_character_t* character ) {
    character->name = read_string( in );
    character->short_name = read_string( in );
    character->face_index = read_int( in );
}


typedef enum yarn_resolution_t {
    YARN_RESOLUTION_LOW,
    YARN_RESOLUTION_MEDIUM,
    YARN_RESOLUTION_HIGH,
    YARN_RESOLUTION_FULL,
} yarn_resolution_t;


typedef enum yarn_display_filter_t {
    YARN_DISPLAY_FILTER_NONE,
    YARN_DISPLAY_FILTER_TV,
    YARN_DISPLAY_FILTER_PC,
} yarn_display_filter_t;


typedef struct yarn_globals_t {
    string title;
    string author;
    string start;
    string palette;
    string alone_text;
    string font_description;
    string font_options;
    string font_characters;
    string font_items;
    string font_name;
    yarn_resolution_t resolution;
    array(yarn_display_filter_t)* display_filters;
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
} yarn_globals_t;


yarn_globals_t* empty_globals( void ) {
    static yarn_globals_t globals;
    globals.title = NULL;
    globals.author = NULL;
    globals.start = NULL;
    globals.palette = NULL;
    globals.alone_text = cstr( "You are alone." );
    globals.font_description = cstr( "fonts/Berkelium64.ttf" );
    globals.font_options = cstr( "fonts/Sierra-SCI-Menu-Font.ttf" );
    globals.font_characters =  cstr( "fonts/Berkelium64.ttf" );
    globals.font_items = cstr( "fonts/Berkelium64.ttf" );
    globals.font_name = cstr( "fonts/Sierra-SCI-Menu-Font.ttf" );
    globals.resolution = YARN_RESOLUTION_FULL;
    globals.display_filters = managed_array(int);
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


void save_globals( buffer_t* out, yarn_globals_t* globals ) {
    buffer_write_string( out, &globals->title, 1 );
    buffer_write_string( out, &globals->author, 1 );
    buffer_write_string( out, &globals->start, 1 );
    buffer_write_string( out, &globals->palette, 1 );
    buffer_write_string( out, &globals->alone_text, 1 );
    buffer_write_string( out, &globals->font_description, 1 );
    buffer_write_string( out, &globals->font_options, 1 );
    buffer_write_string( out, &globals->font_characters, 1 );
    buffer_write_string( out, &globals->font_items, 1 );
    buffer_write_string( out, &globals->font_name, 1 );
    int resolution = (int) globals->resolution;
    buffer_write_i32( out, &resolution, 1 );

    buffer_write_i32( out, &globals->display_filters->count, 1 );
    for( int i = 0; i < globals->display_filters->count; ++i ) {
        int value = (int) globals->display_filters->items[ i ];
        buffer_write_i32( out, &value, 1 );
    }

    buffer_write_i32( out, &globals->logo_indices->count, 1 );
    buffer_write_i32( out, globals->logo_indices->items, globals->logo_indices->count );

    buffer_write_i32( out, &globals->background_location, 1 );
    buffer_write_i32( out, &globals->background_dialog, 1 );
    buffer_write_i32( out, &globals->color_background, 1 );
    buffer_write_i32( out, &globals->color_disabled, 1 );
    buffer_write_i32( out, &globals->color_txt, 1 );
    buffer_write_i32( out, &globals->color_opt, 1 );
    buffer_write_i32( out, &globals->color_chr, 1 );
    buffer_write_i32( out, &globals->color_use, 1 );
    buffer_write_i32( out, &globals->color_name, 1 );
    buffer_write_i32( out, &globals->color_facebg, 1 );

    buffer_write_bool( out, &globals->explicit_flags, 1 );
    buffer_write_i32( out, &globals->flags->count, 1 );
    buffer_write_string( out, globals->flags->items, globals->flags->count );

    buffer_write_bool( out, &globals->explicit_items, 1 );
    buffer_write_i32( out, &globals->items->count, 1 );
    buffer_write_string( out, globals->items->items, globals->items->count );
}


void load_globals( buffer_t* in, yarn_globals_t* globals ) {
    globals->title = read_string( in );
    globals->author = read_string( in );
    globals->start = read_string( in );
    globals->palette = read_string( in );
    globals->alone_text = read_string( in );
    globals->font_description = read_string( in );
    globals->font_options = read_string( in );
    globals->font_characters = read_string( in );
    globals->font_items = read_string( in );
    globals->font_name = read_string( in );
    globals->resolution = (yarn_resolution_t) read_int( in );

    globals->display_filters = managed_array(int);
    int display_filters_count = read_int( in );
    for( int i = 0; i < display_filters_count; ++i ) {
        yarn_display_filter_t value = (yarn_display_filter_t) read_int( in );
        array_add( globals->display_filters, &value );
    }

    globals->logo_indices = managed_array(int);
    int logo_indices_count = read_int( in );
    for( int i = 0; i < logo_indices_count; ++i ) {
        int value = read_int( in );
        array_add( globals->logo_indices, &value );
    }

    globals->background_location = read_int( in );
    globals->background_dialog = read_int( in );
    globals->color_background = read_int( in );
    globals->color_disabled = read_int( in );
    globals->color_txt = read_int( in );
    globals->color_opt = read_int( in );
    globals->color_chr = read_int( in );
    globals->color_use = read_int( in );
    globals->color_name = read_int( in );
    globals->color_facebg = read_int( in );

    globals->explicit_flags = read_bool( in );
    globals->flags = managed_array(string_id);
    read_string_array( in, globals->flags );

    globals->explicit_items = read_bool( in );
    globals->items = managed_array(string_id);
    read_string_array( in, globals->items );

}

typedef struct yarn_assets_t {
    int palette_count;
    uint32_t palette[ 256 ];

    pixelfont_t* font_description;
    pixelfont_t* font_options;
    pixelfont_t* font_characters;
    pixelfont_t* font_items;
    pixelfont_t* font_name;

    array(palrle_data_t*)* bitmaps;

    uint32_t frame_pc_size;
    void* frame_pc;

    uint32_t frame_tv_size;
    void* frame_tv;
} yarn_assets_t;


yarn_assets_t* empty_assets( void ) {
    static yarn_assets_t assets;

    assets.palette_count = 0;
    memset( assets.palette, 0, sizeof( assets.palette ) );

    assets.font_description = NULL;
    assets.font_options = NULL;
    assets.font_characters = NULL;
    assets.font_items = NULL;
    assets.font_name = NULL;

    assets.bitmaps = managed_array(palrle_data_t*);

    assets.frame_pc_size = 0;
    assets.frame_pc = NULL;

    assets.frame_tv_size = 0;
    assets.frame_tv = NULL;

    return &assets;
}


void save_assets( buffer_t* out, yarn_assets_t* assets ) {
    buffer_write_i32( out, &assets->palette_count, 1 );
    buffer_write_u32( out, assets->palette, assets->palette_count );

    buffer_write_u32( out, &assets->font_description->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_description, assets->font_description->size_in_bytes );

    buffer_write_u32( out, &assets->font_options->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_options, assets->font_options->size_in_bytes );

    buffer_write_u32( out, &assets->font_characters->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_characters, assets->font_characters->size_in_bytes );

    buffer_write_u32( out, &assets->font_items->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_items, assets->font_items->size_in_bytes );

    buffer_write_u32( out, &assets->font_name->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_name, assets->font_name->size_in_bytes );

    buffer_write_i32( out, &assets->bitmaps->count, 1 );
    for( int i = 0; i < assets->bitmaps->count; ++i ) {
        buffer_write_u32( out, &assets->bitmaps->items[ i ]->size, 1 );
        buffer_write_u8( out, (uint8_t*) assets->bitmaps->items[ i ], assets->bitmaps->items[ i ]->size );
    }

    buffer_write_u32( out, &assets->frame_pc_size, 1 );
    if( assets->frame_pc ) {
        buffer_write_u8( out, (uint8_t*) assets->frame_pc, assets->frame_pc_size );
    }

    buffer_write_u32( out, &assets->frame_tv_size, 1 );
    if( assets->frame_tv ) {
        buffer_write_u8( out, (uint8_t*) assets->frame_tv, assets->frame_tv_size );
    }
}


void load_assets( buffer_t* in, yarn_assets_t* assets ) {
    assets->palette_count = read_int( in );
    buffer_read_u32( in, assets->palette, assets->palette_count );

    uint32_t font_description_size;
    buffer_read_u32( in, &font_description_size, 1 );
    assets->font_description = manage_pixelfont( malloc( font_description_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_description, font_description_size );

    uint32_t font_options_size;
    buffer_read_u32( in, &font_options_size, 1 );
    assets->font_options = manage_pixelfont( malloc( font_options_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_options, font_options_size );

    uint32_t font_characters_size;
    buffer_read_u32( in, &font_characters_size, 1 );
    assets->font_characters = manage_pixelfont( malloc( font_characters_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_characters, font_characters_size );

    uint32_t font_items_size;
    buffer_read_u32( in, &font_items_size, 1 );
    assets->font_items = manage_pixelfont( malloc( font_items_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_items, font_items_size );

    uint32_t font_name_size;
    buffer_read_u32( in, &font_name_size, 1 );
    assets->font_name = manage_pixelfont( malloc( font_name_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_name, font_name_size );

    assets->bitmaps = managed_array(palrle_data_t*);
    int bitmaps_count = read_int( in );
    for( int i = 0; i < bitmaps_count; ++i ) {
        uint32_t size;
        buffer_read_u32( in, &size, 1 );
        palrle_data_t* rle = manage_palrle( malloc( size ) );
        buffer_read_u8( in, (uint8_t*) rle, size );
        array_add( assets->bitmaps, &rle );
    }


    buffer_read_u32( in, &assets->frame_pc_size, 1 );
    if( assets->frame_pc_size ) {
        assets->frame_pc = manage_alloc( malloc( assets->frame_pc_size ) );
        buffer_read_u8( in, (uint8_t*) assets->frame_pc, assets->frame_pc_size );
    }

    buffer_read_u32( in, &assets->frame_tv_size, 1 );
    if( assets->frame_tv_size ) {
        assets->frame_tv = manage_alloc( malloc( assets->frame_tv_size ) );
        buffer_read_u8( in, (uint8_t*) assets->frame_tv, assets->frame_tv_size );
    }
}


typedef struct yarn_t {
    yarn_globals_t globals;
    int start_location;
    int start_dialog;

    array(string_id)* flag_ids;
    array(string_id)* item_ids;
    array(string_id)* image_names;
    array(string_id)* screen_names;
    array(string_id)* face_names;

    array(yarn_location_t)* locations;
    array(yarn_dialog_t)* dialogs;
    array(yarn_character_t)* characters;

    yarn_assets_t assets;
} yarn_t;


yarn_t* empty_yarn( void ) {
    static yarn_t yarn;
    yarn.globals = *empty_globals();
    yarn.start_location = -1;
    yarn.start_dialog = -1;

    yarn.flag_ids = managed_array(string_id);
    yarn.item_ids = managed_array(string_id);
    yarn.image_names = managed_array(string_id);
    yarn.screen_names = managed_array(string_id);
    yarn.face_names = managed_array(string_id);

    yarn.locations = managed_array(yarn_location_t);
    yarn.dialogs = managed_array(yarn_dialog_t);
    yarn.characters = managed_array(yarn_character_t);

    yarn.assets = *empty_assets();
    return &yarn;
}


void yarn_save( buffer_t* out, yarn_t* yarn ) {
    save_globals( out, &yarn->globals );

    buffer_write_i32( out, &yarn->start_location, 1 );
    buffer_write_i32( out, &yarn->start_dialog, 1 );

    buffer_write_i32( out, &yarn->flag_ids->count, 1 );
    buffer_write_string( out, yarn->flag_ids->items, yarn->flag_ids->count );

    buffer_write_i32( out, &yarn->item_ids->count, 1 );
    buffer_write_string( out, yarn->item_ids->items, yarn->item_ids->count );

    buffer_write_i32( out, &yarn->image_names->count, 1 );
    buffer_write_string( out, yarn->image_names->items, yarn->image_names->count );

    buffer_write_i32( out, &yarn->screen_names->count, 1 );
    buffer_write_string( out, yarn->screen_names->items, yarn->screen_names->count );

    buffer_write_i32( out, &yarn->face_names->count, 1 );
    buffer_write_string( out, yarn->face_names->items, yarn->face_names->count );

    buffer_write_i32( out, &yarn->locations->count, 1 );
    for( int i = 0; i < yarn->locations->count; ++i ) {
        save_location( out, &yarn->locations->items[ i ] );
    }

    buffer_write_i32( out, &yarn->dialogs->count, 1 );
    for( int i = 0; i < yarn->dialogs->count; ++i ) {
        save_dialog( out, &yarn->dialogs->items[ i ] );
    }

    buffer_write_i32( out, &yarn->characters->count, 1 );
    for( int i = 0; i < yarn->characters->count; ++i ) {
        save_character( out, &yarn->characters->items[ i ] );
    }

    save_assets( out, &yarn->assets );
}


void yarn_load( buffer_t* in, yarn_t* yarn ) {
    load_globals( in, &yarn->globals );

    yarn->start_location = read_int( in );
    yarn->start_dialog = read_int( in );

    yarn->flag_ids = managed_array(string_id);
    read_string_array( in, yarn->flag_ids );
    yarn->item_ids = managed_array(string_id);
    read_string_array( in, yarn->item_ids );
    yarn->image_names = managed_array(string_id);
    read_string_array( in, yarn->image_names );
    yarn->screen_names = managed_array(string_id);
    read_string_array( in, yarn->screen_names  );
    yarn->face_names = managed_array(string_id);
    read_string_array( in, yarn->face_names );

    yarn->locations = managed_array(yarn_location_t);
    int locations_count = read_int( in );
    for( int i = 0; i < locations_count; ++i ) {
        yarn_location_t location;
        load_location( in, &location );
        array_add( yarn->locations, &location );
    }

    yarn->dialogs = managed_array(yarn_dialog_t);
    int dialogs_count = read_int( in );
    for( int i = 0; i < dialogs_count; ++i ) {
        yarn_dialog_t dialog;
        load_dialog( in, &dialog );
        array_add( yarn->dialogs, &dialog );
    }

    yarn->characters = managed_array(yarn_character_t);
    int characters_count = read_int( in );
    for( int i = 0; i < characters_count; ++i ) {
        yarn_character_t character;
        load_character( in, &character );
        array_add( yarn->characters, &character );
    }

    load_assets( in, &yarn->assets );
}


#include "yarn_lexer.h"
#include "yarn_parser.h"
#include "yarn_compiler.h"


buffer_t* yarn_compile( char const* path ) {
    struct cstr_restore_point_t* str_restore = cstr_restore_point();
    int mem_restore = memmgr_restore_point( &g_memmgr );

    array(parser_global_t)* parser_globals = managed_array( parser_global_t );
    array(parser_section_t)* parser_sections = managed_array( parser_section_t );

    bool parser_success = true;

    string scripts_path = cstr_cat( path, "/scripts/" ); // TODO: cstr_join
    dir_t* dir = dir_open( scripts_path );
    if( !dir ) {
        printf( "Could not find 'scripts' folder\n" );
        memmgr_rollback( &g_memmgr, mem_restore );
        cstr_rollback( str_restore );
        return NULL;
    }

    int files_count = 0;
    for( dir_entry_t* d = dir_read( dir ); d != NULL; d = dir_read( dir ) ) {
        if( dir_is_file( d ) ) {
            string filename = cstr_cat( scripts_path, dir_name( d ) ); // TODO: cstr_join
            file_t* file = file_load( filename, FILE_MODE_TEXT, 0 );
            if( !file )  {
                printf( "Could not open file '%s'\n", filename );
                memmgr_rollback( &g_memmgr, mem_restore );
                cstr_rollback( str_restore );
                dir_close( dir );
                return NULL;
            }
            ++files_count;
            string source = cstr( (char const*) file->data );
            file_destroy( file );

            array(lexer_declaration_t)* lexer_globals = managed_array( lexer_declaration_t );
            array(lexer_section_t)* lexer_sections = managed_array( lexer_section_t );
            bool lexer_success = yarn_lexer( filename, source, lexer_globals, lexer_sections );
            if( lexer_success ) {
                if( !yarn_parser( lexer_globals, lexer_sections, parser_globals, parser_sections ) ) {
                    parser_success = false;
                }
            }

            if( !lexer_success ) {
                printf( "Lexer failed for file '%s'\n", filename );
                memmgr_rollback( &g_memmgr, mem_restore );
                cstr_rollback( str_restore );
                dir_close( dir );
                return NULL;
            }
        }
    }
    dir_close( dir );

    if( files_count == 0 ) {
        printf( "No files found in 'scripts' folder\n" );
        memmgr_rollback( &g_memmgr, mem_restore );
        cstr_rollback( str_restore );
        return NULL;
    }
    if( !parser_success ) {
        printf( "Parser failed\n" );
        memmgr_rollback( &g_memmgr, mem_restore );
        cstr_rollback( str_restore );
        return NULL;
    }

    yarn_t yarn;
    if( !yarn_compiler( parser_globals, parser_sections, &yarn ) ) {
        printf( "Compiler failed\n" );
        memmgr_rollback( &g_memmgr, mem_restore );
        cstr_rollback( str_restore );
        return NULL;
    }

    bool no_error = true;
    printf( "Processing palette\n" );
    paldither_palette_t* palette = manage_paldither( convert_palette( yarn.globals.palette, NULL ) );
    if( !palette ) {
        printf( "Failed to load palette file '%s'\n", yarn.globals.palette );
        no_error = false;
    }
    yarn.assets.palette_count = palette->color_count;
    memcpy( yarn.assets.palette, palette->colortable, palette->color_count * sizeof( *palette->colortable ) );

    printf( "Processing fonts\n" );
    yarn.assets.font_description = manage_pixelfont( convert_font( yarn.globals.font_description ) );
    yarn.assets.font_options = manage_pixelfont( convert_font( yarn.globals.font_options ) );
    yarn.assets.font_characters = manage_pixelfont( convert_font( yarn.globals.font_characters ) );
    yarn.assets.font_items = manage_pixelfont( convert_font( yarn.globals.font_items ) );
    yarn.assets.font_name = manage_pixelfont( convert_font( yarn.globals.font_name ) );

    printf( "Processing images\n" );
    for( int i = 0; i < yarn.screen_names->count; ++i ) {
        string_id screen_name = yarn.screen_names->items[ i ];
        int widths[] = { 320, 480, 640, 1440 };
        int heights[] = { 240, 360, 480, 1080 };
        palrle_data_t* bitmap = manage_palrle( convert_bitmap( screen_name, widths[ yarn.globals.resolution], heights[ yarn.globals.resolution], yarn.globals.palette, palette ) );
        array_add( yarn.assets.bitmaps, &bitmap );
        if( !bitmap ) {
            printf( "Failed to load image: %s\n", screen_name );
            no_error = false;
        }

    }
    for( int i = 0; i < yarn.image_names->count; ++i ) {
        string_id image_name = yarn.image_names->items[ i ];
        int widths[] = { 192, 288, 384, 864 };
        int heights[] = { 128, 192, 256, 576 };
        palrle_data_t* bitmap = manage_palrle( convert_bitmap( image_name, widths[ yarn.globals.resolution], heights[ yarn.globals.resolution], yarn.globals.palette, palette ) );
        array_add( yarn.assets.bitmaps, &bitmap );
        if( !bitmap ) {
            printf( "Failed to load image: %s\n", image_name );
            no_error = false;
        }

    }

    printf( "Processing faces\n" );
    for( int i = 0; i < yarn.face_names->count; ++i ) {
        string_id face_name = yarn.face_names->items[ i ];
        int widths[] = { 90, 135, 180, 405 };
        int heights[] = { 90, 135, 180, 405 };
        palrle_data_t* bitmap = manage_palrle( convert_bitmap( face_name, widths[ yarn.globals.resolution], heights[ yarn.globals.resolution], yarn.globals.palette, palette ) );
        array_add( yarn.assets.bitmaps, &bitmap );
        if( !bitmap ) {
            printf( "Failed to load image: %s\n", face_name );
            no_error = false;
        }

    }

    printf( "Processing frames\n" );
    for( int i = 0; i < yarn.globals.display_filters->count; ++i ) {
        if( yarn.globals.display_filters->items[ i ] == YARN_DISPLAY_FILTER_PC ) {
            file_t* file = file_load( "images/crtframe_pc.png", FILE_MODE_BINARY, NULL );
            yarn.assets.frame_pc_size = file ? (uint32_t) file->size : 0;
            if( yarn.assets.frame_pc_size ) {
                yarn.assets.frame_pc = manage_alloc( malloc( yarn.assets.frame_pc_size ) );
                memcpy( yarn.assets.frame_pc, file->data, yarn.assets.frame_pc_size );
            } else {
                yarn.assets.frame_pc = NULL;
            }
            file_destroy( file );
        }
        if( yarn.globals.display_filters->items[ i ] == YARN_DISPLAY_FILTER_TV ) {
            file_t* file = file_load( "images/crtframe_tv.png", FILE_MODE_BINARY, NULL );
            yarn.assets.frame_tv_size = file ? (uint32_t) file->size : 0;
            if( yarn.assets.frame_tv_size ) {
                yarn.assets.frame_tv = manage_alloc( malloc( yarn.assets.frame_tv_size ) );
                memcpy( yarn.assets.frame_tv, file->data, yarn.assets.frame_tv_size );
            } else {
                yarn.assets.frame_tv = NULL;
            }
            file_destroy( file );
        }
    }

    printf( "Saving compiled yarn\n" );
    buffer_t* buffer = buffer_create();
    yarn_save( buffer, &yarn );
    memmgr_rollback( &g_memmgr, mem_restore );
    cstr_rollback( str_restore );
    return buffer;
}
