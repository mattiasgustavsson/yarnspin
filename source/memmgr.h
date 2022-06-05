
struct memmgr_item_t {
    void* context;
    void* ptr;
    void (*deleter)( void* context, void* ptr );
};

struct memmgr_t {
    int capacity;
    int count;
    struct memmgr_item_t* items;
};

void memmgr_init( struct memmgr_t* memmgr ) {
    memmgr->capacity = 0;
    memmgr->count = 0;
    memmgr->items = NULL;
}

void* memmgr_add( struct memmgr_t* memmgr, void* ptr, void* context, void (*deleter)( void* context, void* ptr ) ) {
    if( memmgr->capacity == 0  ) {
        memmgr->capacity = 256;
        memmgr->count = 0;
        memmgr->items = (struct memmgr_item_t*) malloc( memmgr->capacity * sizeof( struct memmgr_item_t ) );
    }
    if( memmgr->count >= memmgr->capacity ) {
        memmgr->capacity *= 2;
        memmgr->items = (struct memmgr_item_t*) realloc( memmgr->items, memmgr->capacity * sizeof( struct memmgr_item_t ) );
    }
    struct memmgr_item_t* item = &memmgr->items[ memmgr->count++ ];
    item->context = context;
    item->ptr = ptr;
    item->deleter = deleter;
    return ptr;
}

void memmgr_clear( struct memmgr_t* memmgr ) {
    if( memmgr->items ) {
        for( int i = 0; i < memmgr->count; ++i ) {
            struct memmgr_item_t* item = &memmgr->items[ i ];
            item->deleter( item->context, item->ptr );
        }
        free( memmgr->items );
    }
    memmgr->count = 0;
    memmgr->capacity = 0;
}

int memmgr_restore_point( struct memmgr_t* memmgr ) {
    return memmgr->count;
}

void memmgr_rollback( struct memmgr_t* memmgr, int restore_point ) {
    if( restore_point == 0 ) {
        memmgr_clear( memmgr );
    } else {
        if( memmgr->items && restore_point >= 0 && restore_point <= memmgr->count ) {
            for( int i = restore_point; i < memmgr->count; ++i ) {
                struct memmgr_item_t* item = &memmgr->items[ i ];
                item->deleter( item->context, item->ptr );
            }
            memmgr->count = restore_point;
        }
    }
}
