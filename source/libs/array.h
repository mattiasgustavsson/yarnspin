#ifndef array_h
#define array_h

#ifndef ARRAY_BOOL_T
    #define _CRT_NONSTDC_NO_DEPRECATE 
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdbool.h>
    #define ARRAY_BOOL_T bool
#endif

struct array_t;

struct array_t* array_create( int item_size, void* memctx );
void array_destroy( struct array_t* array );
void* array_add( struct array_t* array, void* item );
void array_remove( struct array_t* array, int index );
void array_remove_ordered( struct array_t* array, int index );
ARRAY_BOOL_T array_get( struct array_t* array, int index, void* item );
ARRAY_BOOL_T array_set( struct array_t* array, int index, void const* item );
int array_count( struct array_t* array );
void array_sort( struct array_t* array, int (*compare)( void const*, void const* ) );
int array_bsearch( struct array_t* array, void* key, int (*compare)( void const*, void const* ) );
void* array_item( struct array_t* array, int index );

#endif /* array_h */


#ifdef ARRAY_IMPLEMENTATION
#undef ARRAY_IMPLEMENTATION


#ifndef ARRAY_MALLOC
    #define _CRT_NONSTDC_NO_DEPRECATE 
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #define ARRAY_MALLOC( ctx, size ) ( malloc( size ) )
    #define ARRAY_FREE( ctx, ptr ) ( free( ptr ) )
#endif

#ifndef ARRAY_MEMCPY
    #define _CRT_NONSTDC_NO_DEPRECATE 
    #define _CRT_SECURE_NO_WARNINGS
    #include <string.h>
    #define ARRAY_MEMCPY( dst, src, cnt ) ( memcpy( (dst), (src), (cnt) ) )
#endif 

#ifndef ARRAY_QSORT
    #define _CRT_NONSTDC_NO_DEPRECATE 
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #define ARRAY_QSORT( base, num, size, cmp ) ( qsort( (base), (num), (size), (cmp) ) )
#endif 

#ifndef ARRAY_BSEARCH
    #define _CRT_NONSTDC_NO_DEPRECATE 
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #define ARRAY_BSEARCH( key, base, num, size, cmp ) ( bsearch( (key), (base), (num), (size), (cmp) ) )
#endif 


struct array_t {
    void* memctx;
    int item_size;
    int capacity;
    int count;
    void* items;
};


struct array_t* array_create( int item_size, void* memctx ) {
    struct array_t* array = (struct array_t*) ARRAY_MALLOC( memctx, sizeof( struct array_t ) );
    array->memctx = memctx;
    array->item_size = item_size;
    array->capacity = 256;
    array->count = 0;
    array->items = ARRAY_MALLOC( memctx, array->capacity * item_size );
    return array;
}


void array_destroy( struct array_t* array ) {
    ARRAY_FREE( array->memctx, array->items );
    ARRAY_FREE( array->memctx, array );
}


void* array_add( struct array_t* array, void* item ) {
    if( array->count >= array->capacity ) {
        array->capacity *= 2;
        void* items = array->items;
        array->items = ARRAY_MALLOC( array->memctx, array->capacity * array->item_size );
        ARRAY_MEMCPY( array->items, items, array->count * array->item_size );
        ARRAY_FREE( array->memctx, items );
    }
    ARRAY_MEMCPY( (void*)( ( (uintptr_t) array->items ) + array->count * array->item_size ), item, array->item_size );
    ++array->count;
    return (void*)( ( (uintptr_t) array->items ) + ( array->count - 1 ) * array->item_size );
}


void array_remove( struct array_t* array, int index ) {
    if( index >= 0 && index < array->count ) {
        --array->count;
        memmove( (void*)( ( (uintptr_t) array->items ) + index * array->item_size ), 
            (void*)( ( (uintptr_t) array->items ) + array->count  * array->item_size ), array->item_size );
    }
}

void array_remove_ordered( struct array_t* array, int index ) {
    if( index >= 0 && index < array->count ) {
        --array->count;
        memmove( (void*)( ( (uintptr_t) array->items ) + index * array->item_size ), 
            (void*)( ( (uintptr_t) array->items ) + ( index + 1 ) * array->item_size ), array->item_size * ( array->count - index ) );
    }
}

ARRAY_BOOL_T array_get( struct array_t* array, int index, void* item ) {
    ARRAY_BOOL_T result = index >= 0 && index < array->count;
    if( result ) {
        ARRAY_MEMCPY( item, (void*)( ( (uintptr_t) array->items ) + index * array->item_size ), array->item_size );
    }
    return result;
}

ARRAY_BOOL_T array_set( struct array_t* array, int index, void const* item ) {
    ARRAY_BOOL_T result = index >= 0 && index < array->count;
    if( result ) {
        ARRAY_MEMCPY( (void*)( ( (uintptr_t) array->items ) + index * array->item_size ), item, array->item_size );
    }
    return result;
}

int array_count( struct array_t* array ) {
    int count = array->count;
    return count;
}


void array_sort( struct array_t* array, int (*compare)( void const*, void const* ) ) {
    ARRAY_QSORT( array->items, array->count, array->item_size, compare );
}


int array_bsearch( struct array_t* array, void* key, int (*compare)( void const*, void const* ) ) {
    void* item = ARRAY_BSEARCH( key, array->items, array->count, array->item_size, compare );
    int result = -1;
    if( item ) {
        result = (int)( ( ((uintptr_t)item) - ((uintptr_t)array->items) ) / array->item_size );
    }       
    return result;
}


void* array_item( struct array_t* array, int index ) {
    if(  index >= 0 && index < array->count ) {
        return (void*)( ( (uintptr_t) array->items ) + index * array->item_size );
    } else {
        return NULL;
    }
}

#endif /* ARRAY_IMPLEMENTATION */