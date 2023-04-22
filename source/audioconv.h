
typedef struct qoa_data_t {
    uint32_t size;
    uint8_t data[ 1 ];
} qoa_data_t;


qoa_data_t* convert_audio( string audio_filename ) {
    string processed_filename_no_ext = cstr_format( ".cache/audio/%s", cstr( cbasename( audio_filename ) ), cstr_mid( cextname( audio_filename ), 1, 0 ) ) ;
    string processed_filename = cstr_cat( processed_filename_no_ext, ".qoa" );

    if( true || !file_exists( processed_filename ) || g_cache_version != YARNSPIN_VERSION ||
        file_more_recent( audio_filename, processed_filename ) ) {

        // try load as ogg
        int channels = 0;
        int sample_rate = 0;
        short* short_samples = NULL;
        int sample_count = stb_vorbis_decode_filename( audio_filename, &channels, &sample_rate, &short_samples );
        if( sample_count <= 0 || !( channels == 1 || channels == 2 ) ) {
            if( short_samples ) {
                free( short_samples );
                short_samples = NULL;
            }
        }
                
        // try load as wav
        if( !short_samples ) {
            uint32_t wav_channels = 0;
            uint32_t wav_sample_rate = 0;
            uint64_t wav_sample_count = 0;
            short_samples = drwav_open_file_and_read_pcm_frames_s16( audio_filename, &wav_channels, &wav_sample_rate, &wav_sample_count, NULL );
            channels = (int) wav_channels;
            sample_rate = (int) wav_sample_rate;
            sample_count = (int) wav_sample_count;
            if( sample_count <= 0 || !( channels == 1 || channels == 2 ) ) {
                if( short_samples ) {
                    free( short_samples );
                    short_samples = NULL;
                }
            }
        }

        // try load as flac
        if( !short_samples ) {
            uint32_t flac_channels = 0;
            uint32_t flac_sample_rate = 0;
            uint64_t flac_sample_count = 0;
            short_samples = drflac_open_file_and_read_pcm_frames_s16( audio_filename, &flac_channels, &flac_sample_rate, &flac_sample_count, NULL );
            channels = (int) flac_channels;
            sample_rate = (int) flac_sample_rate;
            sample_count = (int) flac_sample_count;
            if( sample_count <= 0 || !( channels == 1 || channels == 2 ) ) {
                if( short_samples ) {
                    free( short_samples );
                    short_samples = NULL;
                }
            }
        }

        // try load as mp3
        if( !short_samples ) {
            uint32_t mp3_channels = 0;
            uint32_t mp3_sample_rate = 0;
            uint64_t mp3_sample_count = 0;
            drmp3_config mp3_config = { 0, 0 };
            short_samples = drmp3_open_file_and_read_pcm_frames_s16( audio_filename, &mp3_config, &mp3_sample_count, NULL );
            channels = (int) mp3_config.channels;
            sample_rate = (int) mp3_config.sampleRate;
            sample_count = (int) mp3_sample_count;
            if( sample_count <= 0 || !( channels == 1 || channels == 2 ) ) {
                if( short_samples ) {
                    free( short_samples );
                    short_samples = NULL;
                }
            }
        }

        if( !short_samples ) {
            return NULL;
        }

        float* samples = (float*) malloc( sizeof( float ) * sample_count );
        if( channels == 1 ) {
            for( int i = 0; i < sample_count; ++i ) {
                samples[ i ] = ( (float) short_samples[ i ] ) / 32768.0f;
            }
        } else if( channels == 2 ) {
            for( int i = 0; i < sample_count; ++i ) {
                samples[ i ] = ( ( (float) short_samples[ i * 2 + 0 ] ) / 32768.0f + ( (float) short_samples[ i * 2 + 1 ] ) / 32768.0f ) / 2.0f;
            }
        }

        int converted_sample_count = (int)(sample_count * ( 22050.0f / (float) sample_rate ) );
        float* converted_samples = malloc( sizeof( float ) * converted_sample_count );
    
        SRC_DATA src;
        src.data_in = samples;
        src.data_out = converted_samples;
        src.input_frames = sample_count;
        src.output_frames = converted_sample_count;
        src.input_frames_used = 0;
        src.output_frames_gen = 0;
        src.end_of_input = 0;
        src.src_ratio = 22050.0 / (double) sample_rate;

        if( src_simple( &src, SRC_SINC_BEST_QUALITY, 1 ) ) {
            free( samples );
            free( converted_samples );
            free( short_samples );
            return NULL;
        }

        short* converted_short_samples = malloc( sizeof( short ) * converted_sample_count );
        for( int i = 0; i < converted_sample_count; ++i ) {
            float sample = converted_samples[ i ] * 32767.0f;
            converted_short_samples[ i ] = (short)( sample > 32767.0f ? 32767.0f : sample < -32767.0f ? -32767.0f : sample );
        }

        create_path( processed_filename, 0 );

        qoa_desc desc;
        desc.channels = 1;
        desc.samplerate = 22050;
        desc.samples = converted_sample_count;
        int res = qoa_write( processed_filename, converted_short_samples, &desc );

        free( samples );
        free( converted_short_samples );
        free( converted_samples );
        free( short_samples );
    }

    file_t* file = file_load( processed_filename, FILE_MODE_BINARY, 0 );
    qoa_data_t* qoa = (qoa_data_t*) malloc( sizeof( qoa_data_t ) + ( file->size - 1 ) );
    qoa->size = (uint32_t) file->size;
    memcpy( qoa->data, file->data, file->size );
    file_destroy( file );

    return qoa;
}
