
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


float read_float( buffer_t* in ) {
    float value = 0;
    buffer_read_float( in, &value, 1 );
    return value;
}


void read_string_array( buffer_t* in, array_param(string)* array ) {
    int count = read_int( in );
    for( int i = 0; i < count; ++i ) {
        string_id value = read_string( in );
        array_add( array, &value );
    }
}


void read_int_array( buffer_t* in, array_param(int)* array ) {
    int count = read_int( in );
    for( int i = 0; i < count; ++i ) {
        int value = read_int( in );
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
    ACTION_TYPE_GOTO_SCREEN,
    ACTION_TYPE_GOTO_LOCATION,
    ACTION_TYPE_GOTO_DIALOG,
    ACTION_TYPE_AUTO_SCREEN,
    ACTION_TYPE_AUTO_LOCATION,
    ACTION_TYPE_AUTO_DIALOG,
    ACTION_TYPE_EXIT,
    ACTION_TYPE_RETURN,
    ACTION_TYPE_RESTART,
    ACTION_TYPE_QUICKSAVE,
    ACTION_TYPE_QUICKLOAD,
    ACTION_TYPE_FLAG_SET,
    ACTION_TYPE_FLAG_CLEAR,
    ACTION_TYPE_FLAG_TOGGLE,
    ACTION_TYPE_ITEM_GET,
    ACTION_TYPE_ITEM_DROP,
    ACTION_TYPE_CHAR_ATTACH,
    ACTION_TYPE_CHAR_DETACH,
} yarn_action_type_t;


typedef struct yarn_act_t {
    yarn_cond_t cond;
    yarn_action_type_t type;
    int param_screen_index;
    int param_location_index;
    int param_dialog_index;
    int param_flag_index;
    int param_item_index;
    int param_char_index;
    float time_min;
    float time_max;
} yarn_act_t;


yarn_act_t* empty_act( void ) {
    static yarn_act_t act;
    act.cond = *empty_cond();
    act.type = ACTION_TYPE_NONE;
    act.param_screen_index = -1;
    act.param_location_index = -1;
    act.param_dialog_index = -1;
    act.param_flag_index = -1;
    act.param_item_index = -1;
    act.param_char_index = -1;
    act.time_min = 0.0f;
    act.time_max = 0.0f;
    return &act;
}


void save_act( buffer_t* out, yarn_act_t* act ) {
    save_cond( out, &act->cond );
    int type = (int)act->type;
    buffer_write_i32( out, &type, 1 );
    buffer_write_i32( out, &act->param_screen_index, 1 );
    buffer_write_i32( out, &act->param_location_index, 1 );
    buffer_write_i32( out, &act->param_dialog_index, 1 );
    buffer_write_i32( out, &act->param_flag_index, 1 );
    buffer_write_i32( out, &act->param_item_index, 1 );
    buffer_write_i32( out, &act->param_char_index, 1 );
    buffer_write_float( out, &act->time_min, 1 );
    buffer_write_float( out, &act->time_max, 1 );
}


void load_act( buffer_t* in, yarn_act_t* act ) {
    load_cond( in, &act->cond );
    act->type = (yarn_action_type_t) read_int( in );
    act->param_screen_index = read_int( in );
    act->param_location_index = read_int( in );
    act->param_dialog_index = read_int( in );
    act->param_flag_index = read_int( in );
    act->param_item_index = read_int( in );
    act->param_char_index = read_int( in );
    act->time_min = read_float( in );
    act->time_max = read_float( in );
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


typedef struct yarn_scr_t {
    yarn_cond_t cond;
    int scr_index;
} yarn_scr_t;


yarn_scr_t* empty_scr( void ) {
    static yarn_scr_t scr;
    scr.cond = *empty_cond();
    scr.scr_index = -1;
    return &scr;
}


typedef enum yarn_audio_type_t {
    YARN_AUDIO_TYPE_MUSIC,
    YARN_AUDIO_TYPE_AMBIENCE,
    YARN_AUDIO_TYPE_SOUND,
} yarn_audio_type_t;


typedef struct yarn_audio_t {
    yarn_cond_t cond;
    yarn_audio_type_t type;
    int audio_index;
    bool loop;
    bool restart;
    bool stop;
    bool resume;
    bool random;
    float crossfade_min;
    float crossfade_max;
    float delay_min;    
    float delay_max;
    float volume_min;
    float volume_max;
} yarn_audio_t;


yarn_audio_t* empty_audio( void ) {
    static yarn_audio_t audio;
    audio.cond = *empty_cond();
    audio.type = (yarn_audio_type_t)-1;
    audio.audio_index = -1;
    audio.loop = false;
    audio.restart = false;
    audio.stop = false;
    audio.resume = false;
    audio.random = false;
    audio.crossfade_min = 0.0f;
    audio.crossfade_max = 0.0f;
    audio.delay_min = 0.0f;
    audio.delay_max = 0.0f;
    audio.volume_min = 1.0f;
    audio.volume_max = 1.0f;
    return &audio;
}


void save_audio( buffer_t* out, yarn_audio_t* audio ) {
    save_cond( out, &audio->cond );
    int type = (int) audio->type;
    buffer_write_i32( out, &type, 1 );
    buffer_write_i32( out, &audio->audio_index, 1 );
    buffer_write_bool( out, &audio->loop, 1 );
    buffer_write_bool( out, &audio->restart, 1 );
    buffer_write_bool( out, &audio->stop, 1 );
    buffer_write_bool( out, &audio->resume, 1 );
    buffer_write_bool( out, &audio->random, 1 );
    buffer_write_float( out, &audio->crossfade_min, 1 );
    buffer_write_float( out, &audio->crossfade_max, 1 );
    buffer_write_float( out, &audio->delay_min, 1 );
    buffer_write_float( out, &audio->delay_max, 1 );
    buffer_write_float( out, &audio->volume_min, 1 );
    buffer_write_float( out, &audio->volume_max, 1 );
}


void load_audio( buffer_t* in, yarn_audio_t* audio ) {
    load_cond( in, &audio->cond );
    int type = read_int( in );
    audio->type = (yarn_audio_type_t) type; 
    audio->audio_index = read_int( in );
    audio->loop = read_bool( in );
    audio->restart = read_bool( in );
    audio->stop = read_bool( in );
    audio->resume = read_bool( in );
    audio->random = read_bool( in );
    audio->crossfade_min = read_float( in );    
    audio->crossfade_max = read_float( in );    
    audio->delay_min = read_float( in );    
    audio->delay_max = read_float( in );    
    audio->volume_min = read_float( in );    
    audio->volume_max = read_float( in );    
}


typedef struct yarn_screen_t {
    string_id id;
    array(yarn_scr_t)* scr;
    array(yarn_audio_t)* audio;
    array(yarn_act_t)* act;
} yarn_screen_t;


yarn_screen_t* empty_screen( void ) {
    static yarn_screen_t screen;
    screen.id = NULL;
    screen.scr = managed_array(yarn_scr_t);
    screen.audio = managed_array(yarn_audio_t);
    screen.act = managed_array(yarn_act_t);
    return &screen;
}


void save_screen( buffer_t* out, yarn_screen_t* screen ) {
    buffer_write_string( out, &screen->id, 1 );

    buffer_write_i32( out, &screen->scr->count, 1 );
    for( int i = 0; i < screen->scr->count; ++i ) {
        save_cond( out, &screen->scr->items[ i ].cond );
        buffer_write_i32( out, &screen->scr->items[ i ].scr_index, 1 );
    }

    buffer_write_i32( out, &screen->audio->count, 1 );
    for( int i = 0; i < screen->audio->count; ++i ) {
        save_audio( out, &screen->audio->items[ i ] );
    }

    buffer_write_i32( out, &screen->act->count, 1 );
    for( int i = 0; i < screen->act->count; ++i ) {
        save_act( out, &screen->act->items[ i ] );
    }
}


void load_screen( buffer_t* in, yarn_screen_t* screen ) {
    screen->id = read_string( in );

    screen->scr = managed_array(yarn_scr_t);
    int scrs_count = read_int( in );
    for( int i = 0; i < scrs_count; ++i ) {
        yarn_scr_t scr;
        load_cond( in, &scr.cond );
        scr.scr_index = read_int( in );
        array_add( screen->scr, &scr );
    }

    screen->audio = managed_array(yarn_audio_t);
    int audio_count = read_int( in );
    for( int i = 0; i < audio_count; ++i ) {
        yarn_audio_t audio;
        load_audio( in, &audio );
        array_add( screen->audio, &audio );
    }

    screen->act = managed_array(yarn_act_t);
    int acts_count = read_int( in );
    for( int i = 0; i < acts_count; ++i ) {
        yarn_act_t act;
        load_act( in, &act );
        array_add( screen->act, &act );
    }
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
    array(yarn_audio_t)* audio;
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
    location.audio = managed_array(yarn_audio_t);
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

    buffer_write_i32( out, &location->audio->count, 1 );
    for( int i = 0; i < location->audio->count; ++i ) {
        save_audio( out, &location->audio->items[ i ] );
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

    location->audio = managed_array(yarn_audio_t);
    int audio_count = read_int( in );
    for( int i = 0; i < audio_count; ++i ) {
        yarn_audio_t audio;
        load_audio( in, &audio );
        array_add( location->audio, &audio );
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
    array(yarn_audio_t)* audio;
    array(yarn_act_t)* act;
    array(yarn_phrase_t)* phrase;
    array(yarn_say_t)* say;
    array(yarn_use_t)* use;
} yarn_dialog_t;


yarn_dialog_t* empty_dialog( void ) {
    static yarn_dialog_t dialog;
    dialog.id = NULL;
    dialog.audio = managed_array(yarn_audio_t);
    dialog.act = managed_array(yarn_act_t);
    dialog.phrase = managed_array(yarn_phrase_t);
    dialog.say = managed_array(yarn_say_t);
    dialog.use = managed_array(yarn_use_t);
    return &dialog;
}


void save_dialog( buffer_t* out, yarn_dialog_t* dialog ) {
    buffer_write_string( out, &dialog->id, 1 );

    buffer_write_i32( out, &dialog->audio->count, 1 );
    for( int i = 0; i < dialog->audio->count; ++i ) {
        save_audio( out, &dialog->audio->items[ i ] );
    }

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

    dialog->audio = managed_array(yarn_audio_t);
    int audio_count = read_int( in );
    for( int i = 0; i < audio_count; ++i ) {
        yarn_audio_t audio;
        load_audio( in, &audio );
        array_add( dialog->audio, &audio );
    }

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
    string id;
    string name;
    string short_name;
    int face_index;
} yarn_character_t;


yarn_character_t* empty_character( void ) {
    static yarn_character_t character;
    character.id = NULL;
    character.name = NULL;
    character.short_name = NULL;
    character.face_index = -1;
    return &character;
}


void save_character( buffer_t* out, yarn_character_t* character ) {
    buffer_write_string( out, &character->id, 1 );
    buffer_write_string( out, &character->name, 1 );
    buffer_write_string( out, &character->short_name, 1 );
    buffer_write_i32( out, &character->face_index, 1 );
}


void load_character( buffer_t* in, yarn_character_t* character ) {
    character->id = read_string( in );
    character->name = read_string( in );
    character->short_name = read_string( in );
    character->face_index = read_int( in );
}


typedef enum yarn_resolution_t {
    YARN_RESOLUTION_RETRO,
    YARN_RESOLUTION_LOW,
    YARN_RESOLUTION_MEDIUM,
    YARN_RESOLUTION_HIGH,
    YARN_RESOLUTION_FULL,
} yarn_resolution_t;


typedef enum yarn_colormode_t {
    YARN_COLORMODE_PALETTE,
    YARN_COLORMODE_RGB,
    YARN_COLORMODE_RGB9,
} yarn_colormode_t;


typedef enum yarn_screenmode_t {
    YARN_SCREENMODE_FULLSCREEN,
    YARN_SCREENMODE_WINDOW,
} yarn_screenmode_t;


typedef enum yarn_display_filter_t {
    YARN_DISPLAY_FILTER_NONE,
    YARN_DISPLAY_FILTER_TV,
    YARN_DISPLAY_FILTER_PC,
    YARN_DISPLAY_FILTER_LITE,
} yarn_display_filter_t;


typedef struct yarn_globals_t {
    string title;
    string author;
    string version;
    string start;
    string debug_start;
    string palette;
    string alone_text;
    string nothing_text;
    string font_txt;
    int font_txt_size;
    string font_opt;
    int font_opt_size;
    string font_dialog;
    int font_dialog_size;
    string font_say;
    int font_say_size;
    string font_response;
    int font_response_size;
    string font_chr;
    int font_chr_size;
    string font_use;
    int font_use_size;
    string font_name;
    int font_name_size;
    yarn_resolution_t resolution;
    yarn_colormode_t colormode;
    yarn_screenmode_t screenmode;
    array(yarn_display_filter_t)* display_filters;
    int background_location;
    int background_dialog;
    int location_print_speed;
    int dialog_print_speed;
    int color_background;
    int color_disabled;
    int color_txt;
    int color_opt;
    int color_dialog;
    int color_say;
    int color_response;
    int color_chr;
    int color_use;
    int color_name;
    int hmargin_txt;
    int vmargin_txt;
    int hmargin_opt;
    int vmargin_opt;
    int hmargin_dialog;
    int vmargin_dialog;
    int hmargin_say;
    int vmargin_say;
    int hmargin_response;
    int vmargin_response;
    int hmargin_chr;
    int vmargin_chr;
    int hmargin_use;
    int vmargin_use;
    int hmargin_name;
    int vmargin_name;

    bool explicit_flags;
    array(string_id)* flags;

    bool explicit_items;
    array(string_id)* items;

    array(string_id)* debug_set_flags;
    array(string_id)* debug_get_items;
    array(string_id)* debug_attach_chars;
} yarn_globals_t;


yarn_globals_t* empty_globals( void ) {
    static yarn_globals_t globals;
    globals.title = NULL;
    globals.author = NULL;
    globals.version = NULL;
    globals.start = NULL;
    globals.debug_start = NULL;
    globals.palette = NULL;
    globals.alone_text = cstr( "You are alone." );
    globals.nothing_text = cstr( "You have nothing." );
    globals.font_txt = NULL;
    globals.font_txt_size = 0;
    globals.font_opt = NULL;
    globals.font_opt_size = 0;
    globals.font_dialog = NULL;
    globals.font_dialog_size = 0;
    globals.font_say = NULL;
    globals.font_say_size = 0;
    globals.font_response = NULL;
    globals.font_response_size = 0;
    globals.font_chr =  NULL;
    globals.font_chr_size = 0;
    globals.font_use = NULL;
    globals.font_use_size = 0;
    globals.font_name = NULL;
    globals.font_name_size = 0;
    globals.resolution = YARN_RESOLUTION_RETRO;
    globals.colormode = YARN_COLORMODE_PALETTE;
    globals.screenmode = YARN_SCREENMODE_FULLSCREEN;
    globals.display_filters = managed_array(int);
    globals.background_location = -1;
    globals.background_dialog = -1;
    globals.location_print_speed = -1;
    globals.dialog_print_speed = -1;
    globals.color_background = -1;
    globals.color_disabled = -1;
    globals.color_txt = -1;
    globals.color_opt = -1;
    globals.color_dialog = -1;
    globals.color_say = -1;
    globals.color_response = -1;
    globals.color_chr = -1;
    globals.color_use = -1;
    globals.color_name = -1;
    globals.hmargin_txt = 0;
    globals.vmargin_txt = 0;
    globals.hmargin_opt = 0;
    globals.vmargin_opt = 0;
    globals.hmargin_dialog = 0;
    globals.vmargin_dialog = 0;
    globals.hmargin_say = 0;
    globals.vmargin_say = 0;
    globals.hmargin_response = 0;
    globals.vmargin_response = 0;
    globals.hmargin_chr = 0;
    globals.vmargin_chr = 0;
    globals.hmargin_use = 0;
    globals.vmargin_use = 0;
    globals.hmargin_name = 0;
    globals.vmargin_name = 0;
    globals.explicit_flags = false;
    globals.flags = managed_array(string_id);
    globals.explicit_items = false;
    globals.items = managed_array(string_id);
    globals.debug_set_flags = managed_array(string_id);
    globals.debug_get_items = managed_array(string_id);
    globals.debug_attach_chars = managed_array(string_id);
    return &globals;
}


void save_globals( buffer_t* out, yarn_globals_t* globals ) {
    buffer_write_string( out, &globals->title, 1 );
    buffer_write_string( out, &globals->author, 1 );
    buffer_write_string( out, &globals->version, 1 );
    buffer_write_string( out, &globals->start, 1 );
    buffer_write_string( out, &globals->debug_start, 1 );
    buffer_write_string( out, &globals->palette, 1 );
    buffer_write_string( out, &globals->alone_text, 1 );
    buffer_write_string( out, &globals->nothing_text, 1 );
    buffer_write_string( out, &globals->font_txt, 1 );
    buffer_write_i32( out, &globals->font_txt_size, 1 );
    buffer_write_string( out, &globals->font_opt, 1 );
    buffer_write_i32( out, &globals->font_opt_size, 1 );
    buffer_write_string( out, &globals->font_dialog, 1 );
    buffer_write_i32( out, &globals->font_dialog_size, 1 );
    buffer_write_string( out, &globals->font_say, 1 );
    buffer_write_i32( out, &globals->font_say_size, 1 );
    buffer_write_string( out, &globals->font_response, 1 );
    buffer_write_i32( out, &globals->font_response_size, 1 );
    buffer_write_string( out, &globals->font_chr, 1 );
    buffer_write_i32( out, &globals->font_chr_size, 1 );
    buffer_write_string( out, &globals->font_use, 1 );
    buffer_write_i32( out, &globals->font_use_size, 1 );
    buffer_write_string( out, &globals->font_name, 1 );
    buffer_write_i32( out, &globals->font_name_size, 1 );
    int resolution = (int) globals->resolution;
    buffer_write_i32( out, &resolution, 1 );
    int colormode = (int) globals->colormode;
    buffer_write_i32( out, &colormode, 1 );
    int screenmode = (int) globals->screenmode;
    buffer_write_i32( out, &screenmode, 1 );

    buffer_write_i32( out, &globals->display_filters->count, 1 );
    for( int i = 0; i < globals->display_filters->count; ++i ) {
        int value = (int) globals->display_filters->items[ i ];
        buffer_write_i32( out, &value, 1 );
    }

    buffer_write_i32( out, &globals->background_location, 1 );
    buffer_write_i32( out, &globals->background_dialog, 1 );
    buffer_write_i32( out, &globals->location_print_speed, 1 );
    buffer_write_i32( out, &globals->dialog_print_speed, 1 );
    buffer_write_i32( out, &globals->color_background, 1 );
    buffer_write_i32( out, &globals->color_disabled, 1 );
    buffer_write_i32( out, &globals->color_txt, 1 );
    buffer_write_i32( out, &globals->color_opt, 1 );
    buffer_write_i32( out, &globals->color_dialog, 1 );
    buffer_write_i32( out, &globals->color_say, 1 );
    buffer_write_i32( out, &globals->color_response, 1 );
    buffer_write_i32( out, &globals->color_chr, 1 );
    buffer_write_i32( out, &globals->color_use, 1 );
    buffer_write_i32( out, &globals->color_name, 1 );
    buffer_write_i32( out, &globals->hmargin_txt, 1 );
    buffer_write_i32( out, &globals->vmargin_txt, 1 );
    buffer_write_i32( out, &globals->hmargin_opt, 1 );
    buffer_write_i32( out, &globals->vmargin_opt, 1 );
    buffer_write_i32( out, &globals->hmargin_dialog, 1 );
    buffer_write_i32( out, &globals->vmargin_dialog, 1 );
    buffer_write_i32( out, &globals->hmargin_say, 1 );
    buffer_write_i32( out, &globals->vmargin_say, 1 );
    buffer_write_i32( out, &globals->hmargin_response, 1 );
    buffer_write_i32( out, &globals->vmargin_response, 1 );
    buffer_write_i32( out, &globals->hmargin_chr, 1 );
    buffer_write_i32( out, &globals->vmargin_chr, 1 );
    buffer_write_i32( out, &globals->hmargin_use, 1 );
    buffer_write_i32( out, &globals->vmargin_use, 1 );
    buffer_write_i32( out, &globals->hmargin_name, 1 );
    buffer_write_i32( out, &globals->vmargin_name, 1 );

    buffer_write_bool( out, &globals->explicit_flags, 1 );
    buffer_write_i32( out, &globals->flags->count, 1 );
    buffer_write_string( out, globals->flags->items, globals->flags->count );

    buffer_write_bool( out, &globals->explicit_items, 1 );
    buffer_write_i32( out, &globals->items->count, 1 );
    buffer_write_string( out, globals->items->items, globals->items->count );

    buffer_write_i32( out, &globals->debug_set_flags->count, 1 );
    buffer_write_string( out, globals->debug_set_flags->items, globals->debug_set_flags->count );

    buffer_write_i32( out, &globals->debug_get_items->count, 1 );
    buffer_write_string( out, globals->debug_get_items->items, globals->debug_get_items->count );

    buffer_write_i32( out, &globals->debug_attach_chars->count, 1 );
    buffer_write_string( out, globals->debug_attach_chars->items, globals->debug_attach_chars->count );
}


void load_globals( buffer_t* in, yarn_globals_t* globals ) {
    globals->title = read_string( in );
    globals->author = read_string( in );
    globals->version = read_string( in );
    globals->start = read_string( in );
    globals->debug_start = read_string( in );
    globals->palette = read_string( in );
    globals->alone_text = read_string( in );
    globals->nothing_text = read_string( in );
    globals->font_txt = read_string( in );
    globals->font_txt_size = read_int( in );
    globals->font_opt = read_string( in );
    globals->font_opt_size = read_int( in );
    globals->font_dialog = read_string( in );
    globals->font_dialog_size = read_int( in );
    globals->font_say = read_string( in );
    globals->font_say_size = read_int( in );
    globals->font_response = read_string( in );
    globals->font_response_size = read_int( in );
    globals->font_chr = read_string( in );
    globals->font_chr_size = read_int( in );
    globals->font_use = read_string( in );
    globals->font_use_size = read_int( in );
    globals->font_name = read_string( in );
    globals->font_name_size = read_int( in );
    globals->resolution = (yarn_resolution_t) read_int( in );
    globals->colormode = (yarn_colormode_t) read_int( in );
    globals->screenmode = (yarn_screenmode_t) read_int( in );

    globals->display_filters = managed_array(int);
    int display_filters_count = read_int( in );
    for( int i = 0; i < display_filters_count; ++i ) {
        yarn_display_filter_t value = (yarn_display_filter_t) read_int( in );
        array_add( globals->display_filters, &value );
    }

    globals->background_location = read_int( in );
    globals->background_dialog = read_int( in );
    globals->location_print_speed = read_int( in );
    globals->dialog_print_speed = read_int( in );
    globals->color_background = read_int( in );
    globals->color_disabled = read_int( in );
    globals->color_txt = read_int( in );
    globals->color_opt = read_int( in );
    globals->color_dialog = read_int( in );
    globals->color_say = read_int( in );
    globals->color_response = read_int( in );
    globals->color_chr = read_int( in );
    globals->color_use = read_int( in );
    globals->color_name = read_int( in );
    globals->hmargin_txt = read_int( in );
    globals->vmargin_txt = read_int( in );
    globals->hmargin_opt = read_int( in );
    globals->vmargin_opt = read_int( in );
    globals->hmargin_dialog = read_int( in );
    globals->vmargin_dialog = read_int( in );
    globals->hmargin_say = read_int( in );
    globals->vmargin_say = read_int( in );
    globals->hmargin_response = read_int( in );
    globals->vmargin_response = read_int( in );
    globals->hmargin_chr = read_int( in );
    globals->vmargin_chr = read_int( in );
    globals->hmargin_use = read_int( in );
    globals->vmargin_use = read_int( in );
    globals->hmargin_name = read_int( in );
    globals->vmargin_name = read_int( in );

    globals->explicit_flags = read_bool( in );
    globals->flags = managed_array(string_id);
    read_string_array( in, globals->flags );

    globals->explicit_items = read_bool( in );
    globals->items = managed_array(string_id);
    read_string_array( in, globals->items );

    globals->debug_set_flags = managed_array(string_id);
    read_string_array( in, globals->debug_set_flags );

    globals->debug_get_items = managed_array(string_id);
    read_string_array( in, globals->debug_get_items );

    globals->debug_attach_chars = managed_array(string_id);
    read_string_array( in, globals->debug_attach_chars );
}

typedef struct yarn_assets_t {
    int palette_count;
    uint32_t palette[ 256 ];

    pixelfont_t* font_txt;
    pixelfont_t* font_opt;
    pixelfont_t* font_dialog;
    pixelfont_t* font_say;
    pixelfont_t* font_response;
    pixelfont_t* font_chr;
    pixelfont_t* font_use;
    pixelfont_t* font_name;

    array(palrle_data_t*)* bitmaps;
    array(qoa_data_t*)* audio;

    uint32_t frame_pc_size;
    void* frame_pc;

    uint32_t frame_tv_size;
    void* frame_tv;
} yarn_assets_t;


yarn_assets_t* empty_assets( void ) {
    static yarn_assets_t assets;

    assets.palette_count = 0;
    memset( assets.palette, 0, sizeof( assets.palette ) );

    assets.font_txt = NULL;
    assets.font_opt = NULL;
    assets.font_dialog = NULL;
    assets.font_say = NULL;
    assets.font_response = NULL;
    assets.font_chr = NULL;
    assets.font_use = NULL;
    assets.font_name = NULL;

    assets.bitmaps = managed_array(palrle_data_t*);
    assets.audio = managed_array(qoa_data_t*);

    assets.frame_pc_size = 0;
    assets.frame_pc = NULL;

    assets.frame_tv_size = 0;
    assets.frame_tv = NULL;

    return &assets;
}


void save_assets( buffer_t* out, yarn_assets_t* assets, yarn_colormode_t colormode ) {
    buffer_write_i32( out, &assets->palette_count, 1 );
    buffer_write_u32( out, assets->palette, assets->palette_count );

    buffer_write_u32( out, &assets->font_txt->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_txt, assets->font_txt->size_in_bytes );

    buffer_write_u32( out, &assets->font_opt->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_opt, assets->font_opt->size_in_bytes );

    buffer_write_u32( out, &assets->font_dialog->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_dialog, assets->font_dialog->size_in_bytes );

    buffer_write_u32( out, &assets->font_say->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_say, assets->font_say->size_in_bytes );

    buffer_write_u32( out, &assets->font_response->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_response, assets->font_response->size_in_bytes );

    buffer_write_u32( out, &assets->font_chr->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_chr, assets->font_chr->size_in_bytes );

    buffer_write_u32( out, &assets->font_use->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_use, assets->font_use->size_in_bytes );

    buffer_write_u32( out, &assets->font_name->size_in_bytes, 1 );
    buffer_write_u8( out, (uint8_t*) assets->font_name, assets->font_name->size_in_bytes );

    buffer_write_i32( out, &assets->bitmaps->count, 1 );
    for( int i = 0; i < assets->bitmaps->count; ++i ) {
        if( colormode == YARN_COLORMODE_PALETTE ) {
            buffer_write_u32( out, &assets->bitmaps->items[ i ]->size, 1 );
            buffer_write_u8( out, (uint8_t*) assets->bitmaps->items[ i ], assets->bitmaps->items[ i ]->size );
        } else {
            qoi_data_t* qoi = (qoi_data_t*) assets->bitmaps->items[ i ];
            buffer_write_u32( out, &qoi->size, 1 );
            buffer_write_u8( out, qoi->data, qoi->size );
        }
    }

    buffer_write_i32( out, &assets->audio->count, 1 );
    for( int i = 0; i < assets->audio->count; ++i ) {
        buffer_write_u32( out, &assets->audio->items[ i ]->size, 1 );
        buffer_write_u8( out, assets->audio->items[ i ]->data, assets->audio->items[ i ]->size );
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


void load_assets( buffer_t* in, yarn_assets_t* assets, yarn_colormode_t colormode ) {
    assets->palette_count = read_int( in );
    buffer_read_u32( in, assets->palette, assets->palette_count );

    uint32_t font_txt;
    buffer_read_u32( in, &font_txt, 1 );
    assets->font_txt = manage_pixelfont( malloc( font_txt ) );
    buffer_read_u8( in, (uint8_t*) assets->font_txt, font_txt );

    uint32_t font_opt_size;
    buffer_read_u32( in, &font_opt_size, 1 );
    assets->font_opt = manage_pixelfont( malloc( font_opt_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_opt, font_opt_size );

    uint32_t font_dialog_size;
    buffer_read_u32( in, &font_dialog_size, 1 );
    assets->font_dialog = manage_pixelfont( malloc( font_dialog_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_dialog, font_dialog_size );

    uint32_t font_say_size;
    buffer_read_u32( in, &font_say_size, 1 );
    assets->font_say = manage_pixelfont( malloc( font_say_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_say, font_say_size );

    uint32_t font_response_size;
    buffer_read_u32( in, &font_response_size, 1 );
    assets->font_response = manage_pixelfont( malloc( font_response_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_response, font_response_size );

    uint32_t font_chr_size;
    buffer_read_u32( in, &font_chr_size, 1 );
    assets->font_chr = manage_pixelfont( malloc( font_chr_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_chr, font_chr_size );

    uint32_t font_use_size;
    buffer_read_u32( in, &font_use_size, 1 );
    assets->font_use = manage_pixelfont( malloc( font_use_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_use, font_use_size );

    uint32_t font_name_size;
    buffer_read_u32( in, &font_name_size, 1 );
    assets->font_name = manage_pixelfont( malloc( font_name_size ) );
    buffer_read_u8( in, (uint8_t*) assets->font_name, font_name_size );

    assets->bitmaps = managed_array(palrle_data_t*);
    int bitmaps_count = read_int( in );
    for( int i = 0; i < bitmaps_count; ++i ) {
        if( colormode == YARN_COLORMODE_PALETTE ) {
            uint32_t size;
            buffer_read_u32( in, &size, 1 );
            palrle_data_t* rle = (palrle_data_t*)manage_alloc( malloc( size ) );
            rle->size = size;
            buffer_read_u8( in, (uint8_t*) rle, size );
            array_add( assets->bitmaps, &rle );
        } else {
            uint32_t size;
            buffer_read_u32( in, &size, 1 );
            qoi_data_t* qoi = (qoi_data_t*)manage_alloc( malloc( sizeof( qoi_data_t ) + ( size - 1 ) ) );
            qoi->size = size;
            buffer_read_u8( in, qoi->data, size );
            array_add( assets->bitmaps, (qoi_data_t*)&qoi );
        }
    }

    assets->audio = managed_array(qoa_data_t*);
    int audio_count = read_int( in );
    for( int i = 0; i < audio_count; ++i ) {
        uint32_t size;
        buffer_read_u32( in, &size, 1 );
        qoa_data_t* audio = (qoa_data_t*)manage_alloc( malloc( sizeof( qoa_data_t ) + ( size - 1 ) ) );
        audio->size = size;
        buffer_read_u8( in, audio->data, size );
        array_add( assets->audio, &audio );
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
    bool is_debug;
    yarn_globals_t globals;
    int start_screen;
    int start_location;
    int start_dialog;
    int debug_start_screen;
    int debug_start_location;
    int debug_start_dialog;

    array(string_id)* flag_ids;
    array(string_id)* item_ids;
    array(string_id)* image_names;
    array(string_id)* audio_names;
    array(string_id)* scr_names;
    array(string_id)* face_names;

    array(yarn_screen_t)* screens;
    array(yarn_location_t)* locations;
    array(yarn_dialog_t)* dialogs;
    array(yarn_character_t)* characters;

    yarn_assets_t assets;
} yarn_t;


yarn_t* empty_yarn( void ) {
    static yarn_t yarn;
    yarn.is_debug = false;
    yarn.globals = *empty_globals();
    yarn.start_screen = -1;
    yarn.start_location = -1;
    yarn.start_dialog = -1;
    yarn.debug_start_screen = -1;
    yarn.debug_start_location = -1;
    yarn.debug_start_dialog = -1;

    yarn.flag_ids = managed_array(string_id);
    yarn.item_ids = managed_array(string_id);
    yarn.image_names = managed_array(string_id);
    yarn.audio_names = managed_array(string_id);
    yarn.scr_names = managed_array(string_id);
    yarn.face_names = managed_array(string_id);

    yarn.screens = managed_array(yarn_screen_t);
    yarn.locations = managed_array(yarn_location_t);
    yarn.dialogs = managed_array(yarn_dialog_t);
    yarn.characters = managed_array(yarn_character_t);

    yarn.assets = *empty_assets();
    return &yarn;
}


void yarn_save( buffer_t* out, yarn_t* yarn ) {
    save_globals( out, &yarn->globals );

    buffer_write_i32( out, &yarn->start_screen, 1 );
    buffer_write_i32( out, &yarn->start_location, 1 );
    buffer_write_i32( out, &yarn->start_dialog, 1 );

    buffer_write_i32( out, &yarn->debug_start_screen, 1 );
    buffer_write_i32( out, &yarn->debug_start_location, 1 );
    buffer_write_i32( out, &yarn->debug_start_dialog, 1 );

    buffer_write_i32( out, &yarn->flag_ids->count, 1 );
    buffer_write_string( out, yarn->flag_ids->items, yarn->flag_ids->count );

    buffer_write_i32( out, &yarn->item_ids->count, 1 );
    buffer_write_string( out, yarn->item_ids->items, yarn->item_ids->count );

    buffer_write_i32( out, &yarn->image_names->count, 1 );
    buffer_write_string( out, yarn->image_names->items, yarn->image_names->count );

    buffer_write_i32( out, &yarn->scr_names->count, 1 );
    buffer_write_string( out, yarn->scr_names->items, yarn->scr_names->count );

    buffer_write_i32( out, &yarn->face_names->count, 1 );
    buffer_write_string( out, yarn->face_names->items, yarn->face_names->count );

    buffer_write_i32( out, &yarn->screens->count, 1 );
    for( int i = 0; i < yarn->screens->count; ++i ) {
        save_screen( out, &yarn->screens->items[ i ] );
    }

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

    save_assets( out, &yarn->assets, yarn->globals.colormode );
}


void yarn_load( buffer_t* in, yarn_t* yarn, bool is_debug ) {
    yarn->is_debug = is_debug;
    load_globals( in, &yarn->globals );

    yarn->start_screen = read_int( in );
    yarn->start_location = read_int( in );
    yarn->start_dialog = read_int( in );

    yarn->debug_start_screen = read_int( in );
    yarn->debug_start_location = read_int( in );
    yarn->debug_start_dialog = read_int( in );

    yarn->flag_ids = managed_array(string_id);
    read_string_array( in, yarn->flag_ids );
    yarn->item_ids = managed_array(string_id);
    read_string_array( in, yarn->item_ids );
    yarn->image_names = managed_array(string_id);
    read_string_array( in, yarn->image_names );
    yarn->scr_names = managed_array(string_id);
    read_string_array( in, yarn->scr_names  );
    yarn->face_names = managed_array(string_id);
    read_string_array( in, yarn->face_names );

    yarn->screens = managed_array(yarn_screen_t);
    int screens_count = read_int( in );
    for( int i = 0; i < screens_count; ++i ) {
        yarn_screen_t screen;
        load_screen( in, &screen );
        array_add( yarn->screens, &screen );
    }

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

    load_assets( in, &yarn->assets, yarn->globals.colormode );
}


#ifndef YARNSPIN_RUNTIME_ONLY

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

    bool palette_mode = yarn.globals.colormode == YARN_COLORMODE_PALETTE;
    bool no_error = true;

    paldither_palette_t* palette = NULL;
    if( palette_mode ) {
        printf( "Processing palette\n" );
        palette = manage_paldither( convert_palette( yarn.globals.palette, NULL ) );
        if( !palette ) {
            printf( "Failed to load palette file '%s'\n", yarn.globals.palette );
            no_error = false;
        }
        yarn.assets.palette_count = palette->color_count;
        memcpy( yarn.assets.palette, palette->colortable, palette->color_count * sizeof( *palette->colortable ) );
    } else {
        yarn.assets.palette_count = 0;
    }

    printf( "Processing fonts\n" );
    yarn.assets.font_txt = manage_pixelfont( convert_font( yarn.globals.font_txt, yarn.globals.font_txt_size, palette_mode ) );
    if( !yarn.assets.font_txt ) {
        printf( "Failed to load font: %s\n", yarn.globals.font_txt );
        no_error = false;
    }
    yarn.assets.font_opt = manage_pixelfont( convert_font( yarn.globals.font_opt, yarn.globals.font_opt_size, palette_mode ) );
    if( !yarn.assets.font_opt ) {
        printf( "Failed to load font: %s\n", yarn.globals.font_opt );
        no_error = false;
    }
        yarn.assets.font_dialog = manage_pixelfont( convert_font( yarn.globals.font_dialog, yarn.globals.font_dialog_size, palette_mode ) );
    if( !yarn.assets.font_dialog ) {
        printf( "Failed to load font: %s\n", yarn.globals.font_dialog );
        no_error = false;
    }
    yarn.assets.font_say = manage_pixelfont( convert_font( yarn.globals.font_say, yarn.globals.font_say_size, palette_mode ) );
    if( !yarn.assets.font_say ) {
        printf( "Failed to load font: %s\n", yarn.globals.font_say );
        no_error = false;
    }
    yarn.assets.font_response = manage_pixelfont( convert_font( yarn.globals.font_response, yarn.globals.font_response_size, palette_mode ) );
    if( !yarn.assets.font_response ) {
        printf( "Failed to load font: %s\n", yarn.globals.font_response );
        no_error = false;
    }
    yarn.assets.font_chr = manage_pixelfont( convert_font( yarn.globals.font_chr, yarn.globals.font_chr_size, palette_mode ) );
    if( !yarn.assets.font_chr ) {
        printf( "Failed to load font: %s\n", yarn.globals.font_chr );
        no_error = false;
    }
    yarn.assets.font_use = manage_pixelfont( convert_font( yarn.globals.font_use, yarn.globals.font_use_size, palette_mode ) );
    if( !yarn.assets.font_use ) {
        printf( "Failed to load font: %s\n", yarn.globals.font_use );
        no_error = false;
    }
    yarn.assets.font_name = manage_pixelfont( convert_font( yarn.globals.font_name, yarn.globals.font_name_size, palette_mode ) );
    if( !yarn.assets.font_name ) {
        printf( "Failed to load font: %s\n", yarn.globals.font_name );
        no_error = false;
    }

    float scale_factors[] = { 1.0f, 1.25f, 1.5f, 2.0f, 4.5f };
    float resolution_scale = scale_factors[ yarn.globals.resolution ];
    bool jpeg = yarn.globals.colormode == YARN_COLORMODE_RGB && yarn.globals.resolution == YARN_RESOLUTION_FULL;

    printf( "Processing images\n" );
    for( int i = 0; i < yarn.scr_names->count; ++i ) {
        string_id scr_name = yarn.scr_names->items[ i ];
        int width = (int)( 320 * resolution_scale );
        int height = (int)( 240 * resolution_scale );
        if( palette_mode ) {
            palrle_data_t* bitmap = manage_palrle( convert_bitmap( scr_name, width, height, yarn.globals.palette, palette, resolution_scale ) );
            array_add( yarn.assets.bitmaps, &bitmap );
            if( !bitmap ) {
                printf( "Failed to load image: %s\n", scr_name );
                no_error = false;
            }
        } else {
            int bpp = yarn.globals.colormode == YARN_COLORMODE_RGB9 ? 9 : 24;
            qoi_data_t* qoi = (qoi_data_t*)manage_alloc( convert_rgb( scr_name, width, height, bpp, resolution_scale, jpeg ) );
            array_add( yarn.assets.bitmaps, (palrle_data_t*)&qoi );
            if( !qoi ) {
                printf( "Failed to load image: %s\n", scr_name );
                no_error = false;
            }
        }
    }
    for( int i = 0; i < yarn.image_names->count; ++i ) {
        int width = (int)( 192 * resolution_scale );
        int height = (int)( 128 * resolution_scale );
        string_id image_name = yarn.image_names->items[ i ];
        if( palette_mode ) {
            palrle_data_t* bitmap = manage_palrle( convert_bitmap( image_name, width, height, yarn.globals.palette, palette, resolution_scale ) );
            array_add( yarn.assets.bitmaps, &bitmap );
            if( !bitmap ) {
                printf( "Failed to load image: %s\n", image_name );
                no_error = false;
            }
        } else {
            int bpp = yarn.globals.colormode == YARN_COLORMODE_RGB9 ? 9 : 24;
            qoi_data_t* qoi = (qoi_data_t*)manage_alloc( convert_rgb( image_name, width, height, bpp, resolution_scale, jpeg ) );
            array_add( yarn.assets.bitmaps, (palrle_data_t*)&qoi );
            if( !qoi ) {
                printf( "Failed to load image: %s\n", image_name );
                no_error = false;
            }
        }
    }

    printf( "Processing faces\n" );
    for( int i = 0; i < yarn.face_names->count; ++i ) {
        string_id face_name = yarn.face_names->items[ i ];
        int width = (int)( 112 * resolution_scale );
        int height = (int)( 112 * resolution_scale );
        if( palette_mode ) {
            palrle_data_t* bitmap = manage_palrle( convert_bitmap( face_name, width, height, yarn.globals.palette, palette, resolution_scale ) );
            array_add( yarn.assets.bitmaps, &bitmap );
            if( !bitmap ) {
                printf( "Failed to load image: %s\n", face_name );
                no_error = false;
            }
        } else {
            int bpp = yarn.globals.colormode == YARN_COLORMODE_RGB9 ? 9 : 24;
            qoi_data_t* qoi = (qoi_data_t*)manage_alloc( convert_rgb( face_name, width, height, bpp, resolution_scale, jpeg ) );
            array_add( yarn.assets.bitmaps, (palrle_data_t*)&qoi );
            if( !qoi ) {
                printf( "Failed to load image: %s\n", face_name );
                no_error = false;
            }
        }
    }

    printf( "Processing audio\n" );
    for( int i = 0; i < yarn.audio_names->count; ++i ) {
        string_id audio_name = yarn.audio_names->items[ i ];
        qoa_data_t* qoa = (qoa_data_t*)manage_alloc( convert_audio( audio_name) );
        array_add( yarn.assets.audio, &qoa );
        if( !qoa ) {
            printf( "Failed to load audio: %s\n", audio_name );
            no_error = false;
        }
    }

    printf( "Processing frames\n" );
    for( int i = 0; i < yarn.globals.display_filters->count; ++i ) {
        if( yarn.globals.display_filters->items[ i ] == YARN_DISPLAY_FILTER_PC ) {
            file_t* file = file_load( "display/crtframe_pc.png", FILE_MODE_BINARY, NULL );
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
            file_t* file = file_load( "display/crtframe_tv.png", FILE_MODE_BINARY, NULL );
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

    buffer_t* buffer = NULL;
    if( no_error ) {
        printf( "Saving compiled yarn\n" );
        buffer = buffer_create();
        yarn_save( buffer, &yarn );
    }
    memmgr_rollback( &g_memmgr, mem_restore );
    cstr_rollback( str_restore );
    return buffer;
}

#endif
