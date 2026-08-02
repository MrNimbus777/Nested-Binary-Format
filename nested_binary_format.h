#ifndef NESTED_BINARY_FORMAT_H
#define NESTED_BINARY_FORMAT_H


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NBF_FILE_EXTENSION ".nbf"

#ifndef NBF_API
#define NBF_API
#endif

#ifndef NBF_DEFAULT_ALLOC
#define NBF_DEFAULT_ALLOC malloc
#endif

#ifndef NBF_DEFAULT_FREE
#define NBF_DEFAULT_FREE free
#endif


#define NBF_TYPE_FAMILY_EMPTY   \
    X(EMPTY, void*)

#define NBF_TYPE_FAMILY_STRUCT  \
    X(NODE, nbf_node_t)         \
    X(LIST, nbf_list_t)

#define NBF_TYPE_FAMILY_CHAR    \
    X(RAW,    nbf_raw_t)        \
    X(STRING, const char*)

#define NBF_TYPE_FAMILY_INT     \
    X(INT8,    int8_t)          \
    X(INT16,   int16_t)         \
    X(INT32,   int32_t)         \
    X(INT64,   int64_t)         \
    X(UINT8,   uint8_t)         \
    X(UINT16,  uint16_t)        \
    X(UINT32,  uint32_t)        \
    X(UINT64,  uint64_t)

#define NBF_TYPE_FAMILY_FLOAT   \
    X(FLOAT32, float)           \
    X(FLOAT64, double)          \
        
#define NBF_TYPE_FAMILY_NUMERIC \
    NBF_TYPE_FAMILY_INT         \
    NBF_TYPE_FAMILY_FLOAT

#define NBF_TYPE_FAMILY         \
    NBF_TYPE_FAMILY_EMPTY       \
    NBF_TYPE_FAMILY_STRUCT      \
    NBF_TYPE_FAMILY_CHAR        \
    NBF_TYPE_FAMILY_NUMERIC

typedef enum NBF_TYPES {

#define X(x1, x2) NBF_TYPES_##x1,
NBF_TYPE_FAMILY
#undef X
    
    NBF_TYPES_COUNT
} NBF_TYPES;


typedef unsigned char byte;

typedef struct nbf_raw_t {
    byte*    data;
    uint32_t size;
} nbf_raw_t;

NBF_API void printf_bytes(byte* data, size_t size){
    if(size > 0){
        printf("%02X", data[0]);
        for(size_t i = 1; i < size; ++i) {
            printf(" %02X", data[i]);
        }
    }
}

typedef struct nbf_typeless_value_t nbf_typeless_value_t;
typedef struct nbf_value_t          nbf_value_t;
typedef struct nbf_field_t          nbf_field_t;

typedef struct nbf_error_value_t {
    enum ERROR_CODES {
        NULL_PTR,
        DECODE_ERROR,
        ENCODE_ERROR
    } error_code;
} nbf_error_value_t;

typedef struct nbf_node_t {
    nbf_field_t* fields;
    uint16_t     size;
    uint16_t     capacity;
} nbf_node_t; 

typedef struct nbf_list_t {
    nbf_typeless_value_t* values;
    uint16_t              size;
    uint16_t              capacity;
    NBF_TYPES             type;
} nbf_list_t;

typedef enum nbf_ownership_t {
    NBF_OWNERSHIP_UNDEFINED,
    NBF_OWNERSHIP_HEAP
} nbf_ownership_t;

struct nbf_typeless_value_t {
    union {
        #define X(x1, x2) x2 x1;
        NBF_TYPE_FAMILY
        #undef X
    };
    nbf_ownership_t __ownership;
}; 

struct nbf_value_t {
    union 
    {
        nbf_typeless_value_t typeless_value;
        nbf_typeless_value_t tv;
    };
    NBF_TYPES type;
};
struct nbf_field_t {
    const char* name;
    nbf_value_t value;
    nbf_ownership_t __name_ownership;
};

// Custom memory management

typedef void*(*nbf_alloc_f)(size_t);
typedef void(*nbf_value_free_f)(void*);

// You can use it in case your allocator does not need a free feature.
NBF_API void nbf_dummy_free(void*) {}

/*
Takes in the byte sequence and decodes it into a full NBF structure.
*/
NBF_API nbf_value_t nbf_value_decode(byte** cursor, nbf_alloc_f allocator);

/*
Encodes the NBF value (recursively) into a sequence of bytes. The buffer must be pre-allocated (on stack or heap)
*/
NBF_API byte* nbf_value_encode(nbf_value_t* value, byte* buffer);

/*
Returns the full size of the provided value and its children.
*/
NBF_API size_t nbf_value_sizeof(nbf_value_t* value);

/*
Frees correctly any value. Safe to call on stack objects.
*/
NBF_API void nbf_value_free(nbf_value_t* value, nbf_value_free_f free);

/*
Prints the provided value to the standart output.
*/
NBF_API void nbf_value_print(nbf_value_t* value);

/*
Fully clones the value into new allocated memory
*/
NBF_API nbf_value_t nbf_value_clone(nbf_value_t* source, nbf_alloc_f allocator);


/* 
This function return the field with the specified name.
On success: returns the field
On failure: returns NULL (NOT FOUND)
*/
NBF_API nbf_field_t* nbf_node_get(nbf_value_t* node, const char* name);

/*
Creates a new field in the node object. Works similar to adding elements to a dynamic array.
If node contains a field with same name, it replaces it's value, else it adds a new element to fields array.
Note: If node was initially allocated on the stack, and you add a new field, the whole array of fields will be reallocated on the heap.
On success: returns 0
On failure: returns 1 (malloc/realloc failure)
*/
NBF_API int nbf_node_put(nbf_value_t* node, const char* name, nbf_value_t value, nbf_value_free_f free);

/*
This function marks a field as deleted. It sets field's name to NULL and calls nbf_free on the field's value.
On success: returns 0
On failure: returns 1 (NOT FOUND)
*/
NBF_API int nbf_field_release(nbf_field_t* field, nbf_value_free_f free_);

/* 
Firstly calls nbf_node_get and nbf_field_release.
On success: returns 0
On failure: returns 1 (NOT FOUND)
*/
NBF_API int nbf_node_remove(nbf_value_t* node, const char* name, nbf_value_free_f free_);

/*
Works similary to nbf_node_remove, but for all fields.
*/
NBF_API void nbf_node_clear(nbf_value_t* node, nbf_value_free_f free_);


/*
Expands into a for loop where the iterator is field_var_name that being a nbf_field_t*.
*/
#define NBF_NODE_FOREACH(node, field_var_name)       \
    for(                                             \
        nbf_field_t* field_var_name = (node).fields; \
        field_var_name < (node).fields+(node).size;  \
        field_var_name++                             \
    )
/*
Expands into a for loop where the iterator is typelessvalue_var_name that being a nbf_typeless_value_t*.
*/
#define NBF_LIST_FOREACH(list, typelessvalue_var_name)                \
    for(                                                              \
        nbf_typeless_value_t* typelessvalue_var_name = (list).values; \
        typelessvalue_var_name < (list).values+(list).size;           \
        typelessvalue_var_name++                                      \
    )



typedef enum NBF_FILESYSTEM_ERRORS {
    NBF_FS_OK = 0,
    NBF_FS_FILE_NOT_FOUND,
    NBF_FS_IO_ERROR,
    NBF_FS_INVALID_FORMAT,
    NBF_FS_OUT_OF_MEMORY
} NBF_FILESYSTEM_ERRORS;

NBF_API int nbf_write_to_file(nbf_value_t* value, const char* path);
NBF_API int nbf_read_from_file(nbf_value_t* unitialized_value, const char* path, nbf_alloc_f);


// CONSTRUCTORS

#define NBF_VEMPTY() ((nbf_value_t){                              \
    .typeless_value = { .__ownership = NBF_OWNERSHIP_UNDEFINED }, \
    .type = NBF_TYPES_EMPTY                                       \
})

#define NBF_VFIELD(name_, value_) ((nbf_field_t){ \
    .name = (name_),                              \
    .value = (nbf_value_t)(value_),               \
    .__name_ownership = NBF_OWNERSHIP_UNDEFINED   \
})

#define NBF_VSTACK_NODE(...) ((nbf_value_t){                                         \
    .typeless_value = {                                                              \
        .NODE = {                                                                    \
            .fields = (nbf_field_t[]){ __VA_ARGS__ },                                \
            .size = sizeof((nbf_field_t[]){ __VA_ARGS__ }) / sizeof(nbf_field_t),    \
            .capacity = sizeof((nbf_field_t[]){ __VA_ARGS__ }) / sizeof(nbf_field_t) \
        },                                                                           \
        .__ownership = NBF_OWNERSHIP_UNDEFINED                                       \
    },                                                                               \
    .type = NBF_TYPES_NODE                                                           \
})

#define NBF_VHEAP_NODE(...) ((nbf_value_t){                                          \
    .typeless_value = {                                                              \
        .NODE = {                                                                    \
            .fields = memcpy(                                                        \
                NBF_DEFAULT_ALLOC(sizeof((nbf_field_t[]){ __VA_ARGS__ })),           \
                (nbf_field_t[]){ __VA_ARGS__ },                                      \
                sizeof((nbf_field_t[]){ __VA_ARGS__ })                               \
            ),                                                                       \
            .size = sizeof((nbf_field_t[]){ __VA_ARGS__ }) / sizeof(nbf_field_t),    \
            .capacity = sizeof((nbf_field_t[]){ __VA_ARGS__ }) / sizeof(nbf_field_t) \
        },                                                                           \
        .__ownership = NBF_OWNERSHIP_HEAP                                            \
    },                                                                               \
    .type = NBF_TYPES_NODE                                                           \
})

static inline nbf_typeless_value_t* nbf_value_to_typeless_value(nbf_value_t* values, size_t n){
    nbf_typeless_value_t* as_typeless = (nbf_typeless_value_t*) values;
    for(size_t i = 1; i < n; ++i) {
        as_typeless[i] = *(nbf_typeless_value_t*)(values+i);
    }
    return as_typeless;
}

#define NBF_VSTACK_LIST(first_element, ...) ((nbf_value_t){                               \
    .typeless_value = {                                                                   \
        .LIST = {                                                                         \
            .values = nbf_value_to_typeless_value(                                        \
                (nbf_value_t[]){ first_element, __VA_ARGS__ },                            \
                1 + sizeof((nbf_value_t[]){ __VA_ARGS__ }) / sizeof(nbf_value_t)          \
            ),                                                                            \
            .size = 1 + sizeof((nbf_value_t[]){ __VA_ARGS__ }) / sizeof(nbf_value_t),     \
            .capacity = 1 + sizeof((nbf_value_t[]){ __VA_ARGS__ }) / sizeof(nbf_value_t), \
            .type = first_element.type                                                    \
        },                                                                                \
        .__ownership = NBF_OWNERSHIP_UNDEFINED                                            \
    },                                                                                    \
    .type = NBF_TYPES_LIST                                                                \
})

#define NBF_VHEAP_LIST(first_element, ...) ((nbf_value_t){                                 \
    .typeless_value = {                                                                    \
        .LIST = {                                                                          \
            .values = memcpy(                                                              \
                NBF_DEFAULT_ALLOC(                                                         \
                    sizeof((nbf_value_t[]){ first_element,  __VA_ARGS__ }) *               \
                    sizeof((nbf_typeless_value_t)/sizeof(nbf_value_t)                      \
                ),                                                                         \
                nbf_value_to_typeless_value(                                               \
                    (nbf_value_t[]){ first_element, __VA_ARGS__ },                         \
                    1 + sizeof((nbf_value_t[]){ __VA_ARGS__ }) / sizeof(nbf_value_t)       \
                ),                                                                         \
                sizeof((nbf_value_t[]){ first_element,  __VA_ARGS__ })                     \
            ),                                                                             \
            .size = 1 + sizeof((nbf_value_t[]){ __VA_ARGS__ }) / sizeof(nbf_value_t),      \
            .capacity = 1 + sizeof((nbf_value_t[]){ __VA_ARGS__ }) / sizeof(nbf_value_t),  \
            .type = first_element.type                                                     \
        },                                                                                 \
        .__ownership = NBF_OWNERSHIP_HEAP                                                  \
    },                                                                                     \
    .type = NBF_TYPES_LIST                                                                 \
})

#define NBF_VSTACK_RAW(...) ((nbf_value_t){                        \
    .typeless_value = {                                            \
        .RAW = {                                                   \
            .data = (byte[]){ __VA_ARGS__ },                       \
            .size = sizeof((byte[]){ __VA_ARGS__ }) / sizeof(byte) \
        },                                                         \
        .__ownership = NBF_OWNERSHIP_UNDEFINED                     \
    },                                                             \
    .type = NBF_TYPES_RAW                                          \
})

#define NBF_VRAW(bytes_ptr, size) ((nbf_value_t){ \
    .typeless_value = {                           \
        .RAW = {                                  \
            .data = bytes_ptr,                    \
            .size = size                          \
        },                                        \
        .__ownership = NBF_OWNERSHIP_UNDEFINED    \
    },                                            \
    .type = NBF_TYPES_RAW                         \
})

#define NBF_VSTRING(v) ((nbf_value_t){         \
    .typeless_value = {                        \
        .STRING = (v),                         \
        .__ownership = NBF_OWNERSHIP_UNDEFINED \
    },                                         \
    .type = NBF_TYPES_STRING                   \
})

#define NBF_VINT8(v) ((nbf_value_t){                                              \
    .typeless_value = { .INT8 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED },    \
    .type = NBF_TYPES_INT8                                                        \
})

#define NBF_VINT16(v) ((nbf_value_t){                                             \
    .typeless_value = { .INT16 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED },   \
    .type = NBF_TYPES_INT16                                                       \
})

#define NBF_VINT32(v) ((nbf_value_t){                                             \
    .typeless_value = { .INT32 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED },   \
    .type = NBF_TYPES_INT32                                                       \
})

#define NBF_VINT64(v) ((nbf_value_t){                                             \
    .typeless_value = { .INT64 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED },   \
    .type = NBF_TYPES_INT64                                                       \
})

#define NBF_VUINT8(v) ((nbf_value_t){                                             \
    .typeless_value = { .UINT8 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED },   \
    .type = NBF_TYPES_UINT8                                                       \
})

#define NBF_VUINT16(v) ((nbf_value_t){                                            \
    .typeless_value = { .UINT16 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED },  \
    .type = NBF_TYPES_UINT16                                                      \
})

#define NBF_VUINT32(v) ((nbf_value_t){                                            \
    .typeless_value = { .UINT32 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED },  \
    .type = NBF_TYPES_UINT32                                                      \
})

#define NBF_VUINT64(v) ((nbf_value_t){                                            \
    .typeless_value = { .UINT64 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED },  \
    .type = NBF_TYPES_UINT64                                                      \
})

#define NBF_VFLOAT32(v) ((nbf_value_t){                                           \
    .typeless_value = { .FLOAT32 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED }, \
    .type = NBF_TYPES_FLOAT32                                                     \
})

#define NBF_VFLOAT64(v) ((nbf_value_t){                                           \
    .typeless_value = { .FLOAT64 = (v), .__ownership = NBF_OWNERSHIP_UNDEFINED }, \
    .type = NBF_TYPES_FLOAT64                                                     \
})


#ifdef NBF_STRIP_PREFIXES
    typedef struct nbf_typeless_value_t typeless_value_t;
    typedef struct nbf_value_t          value_t;
    typedef struct nbf_node_t           node_t;
    typedef struct nbf_field_t          field_t;
    typedef struct nbf_list_t           list_t;
    typedef struct nbf_raw_t            raw_t;
    #define value_decode  nbf_value_decode
    #define value_encode  nbf_value_encode
    #define value_sizeof  nbf_value_sizeof
    #define value_free    nbf_value_free
    #define value_print   nbf_value_print
    #define value_clone   nbf_value_clone
    #define node_get      nbf_node_get
    #define node_put      nbf_node_put
    #define field_release nbf_field_release
    #define node_remove   nbf_node_remove
    #define node_clear    nbf_node_clear
    #define NODE_FOREACH(node, field_var_name)         NBF_NODE_FOREACH(node, field_var_name)
    #define LIST_FOREACH(list, typelessvalue_var_name) NBF_LIST_FOREACH(list, typelessvalue_var_name)
    #define VEMPTY()                        NBF_VEMPTY()
    #define VFIELD(name_, value_)           NBF_VFIELD(name_, value_)
    #define VSTACK_NODE(...)                NBF_VSTACK_NODE(__VA_ARGS__)
    #define VHEAP_NODE(...)                 NBF_VHEAP_NODE(__VA_ARGS__)
    #define VSTACK_LIST(first_element, ...) NBF_VSTACK_LIST(first_element, __VA_ARGS__)
    #define VHEAP_LIST(first_element, ...)  NBF_VHEAP_LIST(first_element, __VA_ARGS__)
    #define VSTACK_RAW(...)                 NBF_VSTACK_RAW(__VA_ARGS__)
    #define VRAW(bytes_ptr, size)           NBF_VRAW(bytes_ptr, size)
    #define VSTRING(v)                      NBF_VSTRING(v)
    #define VINT8(v)                        NBF_VINT8(v)
    #define VINT16(v)                       NBF_VINT16(v)
    #define VINT32(v)                       NBF_VINT32(v)
    #define VINT64(v)                       NBF_VINT64(v)
    #define VUINT8(v)                       NBF_VUINT8(v)
    #define VUINT16(v)                      NBF_VUINT16(v)
    #define VUINT32(v)                      NBF_VUINT32(v)
    #define VUINT64(v)                      NBF_VUINT64(v)
    #define VFLOAT32(v)                     NBF_VFLOAT32(v)
    #define VFLOAT64(v)                     NBF_VFLOAT64(v)
#endif // NBF_STRIP_PREFIXES

#ifdef __cplusplus
}

#endif

 
#ifdef NBF_IMPLEMENTATION
#include <inttypes.h>


// DECLARE DECODE FUNCTIONS AND FUNCTION TABLE

typedef nbf_value_t(*nbf_decode_type_f)(byte**, nbf_alloc_f);
#define X(x1, x2) static nbf_value_t nbf_decode_##x1(byte**, nbf_alloc_f);
NBF_TYPE_FAMILY
#undef X
static const nbf_decode_type_f NBF_DECODE_FUNCTION_TABLE[NBF_TYPES_COUNT] = {
    #define X(x1, x2) nbf_decode_##x1,
    NBF_TYPE_FAMILY
    #undef X
};
NBF_API nbf_value_t nbf_value_decode(byte** cursor, nbf_alloc_f allocator){
    if(allocator == NULL) allocator = NBF_DEFAULT_ALLOC;
    return NBF_DECODE_FUNCTION_TABLE[*((*cursor)++)](cursor, allocator);
}


// DECLARE ENCODE FUNCTIONS AND FUNCTION TABLE

typedef byte*(*nbf_value_encode_type_f)(nbf_typeless_value_t*, byte*);
#define X(x1, x2) static byte* nbf_value_encode_##x1(nbf_typeless_value_t*, byte*);
NBF_TYPE_FAMILY
#undef X
static const nbf_value_encode_type_f NBF_ENCODE_FUNCTION_TABLE[NBF_TYPES_COUNT] = {
    #define X(x1, x2) nbf_value_encode_##x1,
    NBF_TYPE_FAMILY
    #undef X
};
NBF_API byte* nbf_value_encode(nbf_value_t* value, byte* buffer){
    return NBF_ENCODE_FUNCTION_TABLE[value->type](&value->typeless_value, buffer);
}


// DECLARE SIZEOF FUNCTIONS AND FUNCTION TABLE

typedef size_t(*nbf_value_sizeof_type_f)(nbf_typeless_value_t*);
#define X(x1, x2) static size_t nbf_value_sizeof_##x1(nbf_typeless_value_t*);
NBF_TYPE_FAMILY
#undef X
static const nbf_value_sizeof_type_f NBF_SIZEOF_FUNCTION_TABLE[NBF_TYPES_COUNT] = {
    #define X(x1, x2) nbf_value_sizeof_##x1,
    NBF_TYPE_FAMILY
    #undef X
};
NBF_API size_t nbf_value_sizeof(nbf_value_t* value){
    return NBF_SIZEOF_FUNCTION_TABLE[value->type](&value->typeless_value);
}


// DECLARE FREE FUNCTIONS AND FUNCTION TABLE

typedef void(*nbf_value_free_type_f)(nbf_typeless_value_t*, nbf_value_free_f);
#define X(x1, x2) static void nbf_value_free_##x1(nbf_typeless_value_t*, nbf_value_free_f);
NBF_TYPE_FAMILY_STRUCT
NBF_TYPE_FAMILY_CHAR
#undef X
static void nbf_value_free_ignored_(nbf_typeless_value_t*, nbf_value_free_f){}
static const nbf_value_free_type_f NBF_FREE_FUNCTION_TABLE[NBF_TYPES_COUNT] = {
    #define X(x1, x2) nbf_value_free_ignored_,
    NBF_TYPE_FAMILY_EMPTY
    #undef X
    #define X(x1, x2) nbf_value_free_##x1,
    NBF_TYPE_FAMILY_STRUCT
    NBF_TYPE_FAMILY_CHAR
    #undef X
    #define X(x1, x2) nbf_value_free_ignored_,
    NBF_TYPE_FAMILY_NUMERIC
    #undef X
};
NBF_API void nbf_value_free(nbf_value_t* value, nbf_value_free_f free_){
    if(free_ == NULL) free_ = NBF_DEFAULT_FREE;
    return NBF_FREE_FUNCTION_TABLE[value->type](&value->typeless_value, free_);
}


// DECLARE PRINT FUNCTIONS AND FUNCTION TABLE

typedef void(*nbf_value_print_type_f)(nbf_typeless_value_t*);
#define X(x1, x2) static void nbf_value_print_##x1(nbf_typeless_value_t*);
NBF_TYPE_FAMILY
#undef X
static const nbf_value_print_type_f NBF_PRINT_FUNCTION_TABLE[NBF_TYPES_COUNT] = {
    #define X(x1, x2) nbf_value_print_##x1,
    NBF_TYPE_FAMILY
    #undef X
};
NBF_API void nbf_value_print(nbf_value_t* value){
    return NBF_PRINT_FUNCTION_TABLE[value->type](&value->typeless_value);
}


// DECLARE CLONE FUNCTIONS AND FUNCTION TABLE

typedef nbf_value_t(*nbf_value_clone_type_f)(nbf_typeless_value_t*, nbf_alloc_f);
#define X(x1, x2) static nbf_value_t nbf_value_clone_##x1(nbf_typeless_value_t*, nbf_alloc_f);
NBF_TYPE_FAMILY
#undef X
static const nbf_value_clone_type_f NBF_CLONE_FUNCTION_TABLE[NBF_TYPES_COUNT] = {
    #define X(x1, x2) nbf_value_clone_##x1,
    NBF_TYPE_FAMILY
    #undef X
};
NBF_API nbf_value_t nbf_value_clone(nbf_value_t* source, nbf_alloc_f allocator){
    if(allocator == NULL) allocator = NBF_DEFAULT_ALLOC;
    return NBF_CLONE_FUNCTION_TABLE[source->type](&source->typeless_value, allocator);
}



static inline void nbf_write_64(byte* buf, uint64_t v) {
    buf[0] = (v >> 56) & 0xFF;
    buf[1] = (v >> 48) & 0xFF;
    buf[2] = (v >> 40) & 0xFF;
    buf[3] = (v >> 32) & 0xFF;
    buf[4] = (v >> 24) & 0xFF;
    buf[5] = (v >> 16) & 0xFF;
    buf[6] = (v >>  8) & 0xFF;
    buf[7] = (v >>  0) & 0xFF;
}
static inline void nbf_write_32(byte* buf, uint32_t v) { 
    buf[0] = (v >> 24) & 0xFF;
    buf[1] = (v >> 16) & 0xFF;
    buf[2] = (v >>  8) & 0xFF;
    buf[3] = (v >>  0) & 0xFF;
}
static inline void nbf_write_16(byte* buf, uint16_t v) {
    buf[0] = (v >> 8) & 0xFF;
    buf[1] = (v >> 0) & 0xFF;
}
static inline void nbf_write_8(byte* buf, uint8_t v) {
    buf[0] = v;
}

static inline uint64_t nbf_read_64(const byte* buf) {
    return ((uint64_t)buf[0] << 56) |
           ((uint64_t)buf[1] << 48) |
           ((uint64_t)buf[2] << 40) |
           ((uint64_t)buf[3] << 32) |
           ((uint64_t)buf[4] << 24) |
           ((uint64_t)buf[5] << 16) |
           ((uint64_t)buf[6] <<  8) |
           ((uint64_t)buf[7] <<  0);
}
static inline uint32_t nbf_read_32(const byte* buf) {
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] <<  8) |
           ((uint32_t)buf[3] <<  0);
}
static inline uint16_t nbf_read_16(const byte* buf) {
    return ((uint16_t)buf[0] << 8) |
           ((uint16_t)buf[1] << 0);
}
static inline uint8_t nbf_read_8(const byte* buf) {
    return buf[0];
}


NBF_API int nbf_write_to_file(nbf_value_t* value, const char* path){
    size_t size = nbf_value_sizeof(value);
    byte* buffer = (byte*)malloc(size);
    if (!buffer) return NBF_FS_OUT_OF_MEMORY;
    
    nbf_value_encode(value, buffer);
    
    FILE* f = fopen(path, "wb");
    
    if(f == NULL) {
        free(buffer);
        return 1;
    }

    if(fwrite(buffer, 1, size, f) != size){
        fclose(f);
        free(buffer);
        return NBF_FS_IO_ERROR;
    }
    fclose(f);
    free(buffer);
    return 0;
}

NBF_API int nbf_read_from_file(nbf_value_t* out, const char* path, nbf_alloc_f allocator){
    FILE* f = fopen(path, "rb");
    if (!f) {
        return NBF_FS_FILE_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size <= 0) {
        fclose(f);
        return NBF_FS_IO_ERROR;
    }

    byte* buffer = (byte*)malloc((size_t)size);
    if (!buffer) {
        fclose(f);
        return NBF_FS_OUT_OF_MEMORY;
    }

    if (fread(buffer, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        free(buffer);
        return NBF_FS_IO_ERROR;
    }

    fclose(f);

    byte* cursor = buffer;

    *out = nbf_value_decode(&cursor, allocator);
    free(buffer);
    return 0;
}

// API LAYER FOR NODE

NBF_API nbf_field_t* nbf_node_get(nbf_value_t* node, const char* name){
    NBF_NODE_FOREACH(node->tv.NODE, field) if(strcmp(field->name, name) == 0) return field;
    return NULL;
}

NBF_API int nbf_node_put(nbf_value_t* node, const char* name, nbf_value_t value, nbf_value_free_f free_){
    nbf_node_t* n = &node->tv.NODE;

    NBF_NODE_FOREACH(*n, f) if(*f->name == 0 || strcmp(f->name, name) == 0) {
        if(node->tv.__ownership == NBF_OWNERSHIP_HEAP) nbf_value_free(&f->value, free_);
        if(f->__name_ownership == NBF_OWNERSHIP_HEAP)  free_((void*)f->name);
        f->name  = name;
        f->value = value; 
        return 0;
    }
    if(n->size+1 > n->capacity){
        n->capacity = n->capacity ? n->capacity*2 : 2;
        if(node->tv.__ownership == NBF_OWNERSHIP_HEAP) {
            nbf_field_t* tmp = (nbf_field_t*) realloc(n->fields, sizeof(nbf_field_t)*n->capacity);
            if(!tmp) return 1;
            n->fields = tmp;
        } else {
            nbf_field_t* tmp = (nbf_field_t*) memcpy((byte*)NBF_DEFAULT_ALLOC(sizeof(nbf_field_t)*n->capacity), (byte*) n->fields, sizeof(nbf_field_t)*n->size);
            if(!tmp) return 1;
            n->fields = tmp;
            node->tv.__ownership = NBF_OWNERSHIP_HEAP;
        }
    }
    n->fields[n->size].name  = name;
    n->fields[n->size].value = value;
    ++n->size;
    return 0;
}


NBF_API int nbf_field_release(nbf_field_t* field, nbf_value_free_f free_){
    if(field) {
        if(field->__name_ownership == NBF_OWNERSHIP_HEAP) {
            free_((void*)field->name);
            field->__name_ownership = NBF_OWNERSHIP_UNDEFINED;
        }
        field->name = NULL;
        nbf_value_free(&field->value, free_);
        return 0;
    }
    return 1;
}

NBF_API int nbf_node_remove(nbf_value_t* node, const char* name, nbf_value_free_f free_){
    return nbf_field_release(nbf_node_get(node, name), free_);
}

NBF_API void nbf_node_clear(nbf_value_t* node, nbf_value_free_f free_){
    NBF_NODE_FOREACH(node->tv.NODE, field) {
        if(field->__name_ownership == NBF_OWNERSHIP_HEAP) {
            free_((void*)field->name);
            field->__name_ownership = NBF_OWNERSHIP_UNDEFINED;
        }
        field->name = NULL;
    }
}


// PER TYPE FUNCTIONS SLOPE(A.K. C meta-programming :)) ).

static nbf_value_t nbf_decode_EMPTY(byte** cursor, nbf_alloc_f allocator){
    *cursor += 0;
    return NBF_VEMPTY();
}

static nbf_value_t nbf_decode_NODE(byte** cursor, nbf_alloc_f allocator){
    uint16_t size = nbf_read_16(*cursor);
    *cursor += sizeof(uint16_t);

    nbf_field_t* fields = (nbf_field_t*) allocator(size*sizeof(nbf_field_t));

    for(uint16_t i = 0; i < size; ++i){
        uint16_t name_len = nbf_read_16(*cursor);
        *cursor += sizeof(uint16_t);
        
        char* name = (char*) memcpy((byte*)allocator((name_len+1)*sizeof(char)), *cursor, name_len);
        name[name_len] = 0;
        *cursor += name_len;

        fields[i] = (nbf_field_t) {
            .name = name,
            .value = nbf_value_decode(cursor, allocator),
            .__name_ownership = NBF_OWNERSHIP_HEAP
        };
    }

    return (nbf_value_t){
        .typeless_value = {
            .NODE = {
                .fields = fields,
                .size = size
            },
            .__ownership = NBF_OWNERSHIP_HEAP
        },
        .type = NBF_TYPES_NODE
    };
}

static nbf_value_t nbf_decode_LIST(byte** cursor, nbf_alloc_f allocator){
    uint16_t size = nbf_read_16(*cursor);
    *cursor += sizeof(uint16_t);

    uint8_t type = nbf_read_8(*cursor);
    *cursor += sizeof(uint8_t);

    nbf_typeless_value_t* values = (nbf_typeless_value_t*) allocator(size*sizeof(nbf_typeless_value_t));

    for(uint16_t i = 0; i < size; ++i){
        values[i] = NBF_DECODE_FUNCTION_TABLE[type](cursor, allocator).typeless_value;
    }

    return (nbf_value_t){
        .typeless_value = {
            .LIST = {
                .values = values,
                .size = size,
                .capacity = size,
                .type = (NBF_TYPES)type
            },
            .__ownership = NBF_OWNERSHIP_HEAP
        },
        .type = NBF_TYPES_LIST
    };
}

static nbf_value_t nbf_decode_RAW(byte** cursor, nbf_alloc_f allocator){
    uint32_t size = nbf_read_32(*cursor);
    *cursor += sizeof(uint32_t);
    byte* bytes = (byte*) memcpy((byte*)allocator(size), *cursor, size);
    *cursor += size;
    return (nbf_value_t){
        .typeless_value = {
            .RAW = {
                .data = bytes,
                .size = size
            },
            .__ownership = NBF_OWNERSHIP_HEAP
        },
        .type = NBF_TYPES_RAW
    };
}

static nbf_value_t nbf_decode_STRING(byte** cursor, nbf_alloc_f allocator){
    uint32_t size = nbf_read_32(*cursor);
    *cursor += sizeof(uint32_t);
    char* str = (char*) memcpy((byte*)allocator(size+1), *cursor, size);
    str[size] = 0;
    *cursor += size;
    return (nbf_value_t){
        .typeless_value = {
            .STRING = str,
            .__ownership = NBF_OWNERSHIP_HEAP
        },
        .type = NBF_TYPES_STRING
    };
}

static nbf_value_t nbf_decode_INT8(byte** cursor, nbf_alloc_f allocator){
    int8_t val = nbf_read_8(*cursor);
    (*cursor) += sizeof(int8_t);
    return NBF_VINT8(val);
}
static nbf_value_t nbf_decode_INT16(byte** cursor, nbf_alloc_f allocator){
    int16_t val = nbf_read_16(*cursor);
    (*cursor) += sizeof(int16_t);
    return NBF_VINT16(val);
}
static nbf_value_t nbf_decode_INT32(byte** cursor, nbf_alloc_f allocator){
    int32_t val = nbf_read_32(*cursor);
    (*cursor) += sizeof(int32_t);
    return NBF_VINT32(val);
}
static nbf_value_t nbf_decode_INT64(byte** cursor, nbf_alloc_f allocator){
    int64_t val = nbf_read_64(*cursor);
    (*cursor) += sizeof(int64_t);
    return NBF_VINT64(val);
}
static nbf_value_t nbf_decode_UINT8(byte** cursor, nbf_alloc_f allocator){
    uint8_t val = nbf_read_8(*cursor);
    (*cursor) += sizeof(uint8_t);
    return NBF_VUINT8(val);
}
static nbf_value_t nbf_decode_UINT16(byte** cursor, nbf_alloc_f allocator){
    uint16_t val = nbf_read_16(*cursor);
    (*cursor) += sizeof(uint16_t);
    return NBF_VUINT16(val);
}
static nbf_value_t nbf_decode_UINT32(byte** cursor, nbf_alloc_f allocator){
    uint32_t val = nbf_read_32(*cursor);
    (*cursor) += sizeof(uint32_t);
    return NBF_VUINT32(val);
}
static nbf_value_t nbf_decode_UINT64(byte** cursor, nbf_alloc_f allocator){
    uint64_t val = nbf_read_64(*cursor);
    (*cursor) += sizeof(uint64_t);
    return NBF_VUINT64(val);
}

static nbf_value_t nbf_decode_FLOAT32(byte** cursor, nbf_alloc_f allocator){
    uint32_t tmp = nbf_read_32(*cursor);
    (*cursor) += sizeof(uint32_t);
    return NBF_VFLOAT32(*(float*)&tmp);
}

static nbf_value_t nbf_decode_FLOAT64(byte** cursor, nbf_alloc_f allocator){
    uint64_t tmp = nbf_read_64(*cursor);
    (*cursor) += sizeof(uint64_t);
    return NBF_VFLOAT64(*(double*)&tmp);
}



static void nbf_value_free_NODE(nbf_typeless_value_t* value, nbf_value_free_f free_){
    nbf_node_t node = value->NODE;
    for(uint16_t i = 0; i < node.size; ++i) {
        if(node.fields[i].__name_ownership == NBF_OWNERSHIP_HEAP) free_((void*)node.fields[i].name);
        nbf_value_free(&node.fields[i].value, free_);
    }
    if(value->__ownership == NBF_OWNERSHIP_UNDEFINED) return;
    value->__ownership = NBF_OWNERSHIP_UNDEFINED;
    free_(node.fields);
}

static void nbf_value_free_LIST(nbf_typeless_value_t* value, nbf_value_free_f free_){
    nbf_list_t list = value->LIST;
    for(uint16_t i = 0; i < list.size; ++i) {
        NBF_FREE_FUNCTION_TABLE[list.type](list.values+i, free_);
    }
    if(value->__ownership == NBF_OWNERSHIP_UNDEFINED) return;
    value->__ownership = NBF_OWNERSHIP_UNDEFINED;
    free_(list.values);
}

static void nbf_value_free_RAW(nbf_typeless_value_t* value, nbf_value_free_f free_){
    if(value->__ownership == NBF_OWNERSHIP_UNDEFINED) return;
    value->__ownership = NBF_OWNERSHIP_UNDEFINED;
    free_(value->RAW.data);
}

static void nbf_value_free_STRING(nbf_typeless_value_t* value, nbf_value_free_f free_){
    if(value->__ownership == NBF_OWNERSHIP_UNDEFINED) return;
    value->__ownership = NBF_OWNERSHIP_UNDEFINED;
    free_((void*)value->STRING);
}




static byte* nbf_value_encode_EMPTY(nbf_typeless_value_t*, byte* buffer){
    *buffer = 0;
    return buffer;
}

static byte* nbf_value_encode_NODE(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_NODE;
    nbf_node_t node = value->NODE;
    size_t padding = sizeof(uint16_t)+1; // first 3 bytes occupied 
    size_t deleted = 0;
    for(uint16_t i = 0; i < node.size; ++i) {
        nbf_field_t field = node.fields[i];

        const char* str = field.name;
        
        if(str == NULL) {
            ++deleted;
            continue;
        }

        uint16_t n = 0;
        n = 0;
        padding += sizeof(uint16_t);
        for(const char* c = str; *c != '\0'; ++c) {            
            buffer[padding+n] = *(byte*)c;
            ++n;
        }
        nbf_write_16(buffer+padding-sizeof(uint16_t), n);
        padding += n;

        nbf_value_encode(&field.value, buffer+padding);

        padding += nbf_value_sizeof(&field.value);
    }
    nbf_write_16(buffer+1, node.size-deleted);   // fields count 
    return buffer;
}

static byte* nbf_value_encode_LIST(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_LIST;
    nbf_list_t list = value->LIST;
    size_t padding = sizeof(uint16_t)+1; 
    nbf_write_16(buffer+1, list.size);   // elements count
    nbf_write_8(buffer+3, list.type);    // elements type
    for(uint16_t i = 0; i < list.size; ++i) {
        byte tmp = buffer[padding];
        NBF_ENCODE_FUNCTION_TABLE[list.type](list.values+i, buffer+padding);
        buffer[padding] = tmp;
        padding += NBF_SIZEOF_FUNCTION_TABLE[list.type](list.values+i)-1;
    }
    return buffer;
}

static byte* nbf_value_encode_RAW(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_RAW;
    nbf_raw_t raw = value->RAW;
    nbf_write_32(buffer+1, raw.size);   // bytes count
    for(size_t i = 0; i < raw.size; ++i) {
        buffer[sizeof(uint32_t)+i+1] = raw.data[i];
    }
    return buffer;
}

static byte* nbf_value_encode_STRING(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_STRING;
    const char* str = value->STRING;
    uint32_t n = 0;
    for(const char* c = str; *c != '\0'; ++c) {
        buffer[sizeof(uint32_t)+(++n)] = *(byte*)c;
    }
    nbf_write_32(buffer+1, n);   // string len
    return buffer;
}

static byte* nbf_value_encode_INT8(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_INT8;
    nbf_write_8(buffer+1, value->INT8);
    return buffer;
}
static byte* nbf_value_encode_INT16(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_INT16;
    nbf_write_16(buffer+1, value->INT16);
    return buffer;
}
static byte* nbf_value_encode_INT32(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_INT32;
    nbf_write_32(buffer+1, value->INT32);
    return buffer;
}
static byte* nbf_value_encode_INT64(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_INT64;
    nbf_write_64(buffer+1, value->INT64);
    return buffer;
}
static byte* nbf_value_encode_UINT8(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_UINT8;
    nbf_write_8(buffer+1, value->UINT8);
    return buffer;
}
static byte* nbf_value_encode_UINT16(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_UINT16;
    nbf_write_16(buffer+1, value->UINT16);
    return buffer;
}
static byte* nbf_value_encode_UINT32(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_UINT32;
    nbf_write_32(buffer+1, value->UINT32);
    return buffer;
}
static byte* nbf_value_encode_UINT64(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_UINT64;
    nbf_write_64(buffer+1, value->UINT64);
    return buffer;
}

static byte* nbf_value_encode_FLOAT32(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_FLOAT32;
    nbf_write_32(buffer + 1, *(uint32_t*)&value->FLOAT32);
    return buffer;
}

static byte* nbf_value_encode_FLOAT64(nbf_typeless_value_t* value, byte* buffer){
    *buffer = NBF_TYPES_FLOAT64;
    nbf_write_64(buffer + 1, *(uint64_t*)&value->FLOAT64);
    return buffer;
}



static size_t nbf_value_sizeof_EMPTY(nbf_typeless_value_t*){
    return 1; // header only which is 0x00
}

static size_t nbf_value_sizeof_NODE(nbf_typeless_value_t* value){
    nbf_node_t node = value->NODE;
    size_t size = 1 +               // header
                  sizeof(uint16_t); // fields count
    for(size_t i = 0; i < node.size; ++i){
        nbf_field_t field = node.fields[i];
        
        const char* str = field.name;

        if(*str == 0) continue;

        uint16_t n = 0;
        for(const char* c = str; *c != '\0'; ++c) ++n;

        size += NBF_SIZEOF_FUNCTION_TABLE[field.value.type](&field.value.tv) +
                2 + // 2 bytes for name len
                n;  // name content
    }
    return size;
}

static size_t nbf_value_sizeof_LIST(nbf_typeless_value_t* value){
    nbf_list_t list = value->LIST;
    size_t size = 1 +               // header
                  1 +               // type
                  sizeof(uint16_t); // fields count
    for(size_t i = 0; i < list.size; ++i){
        size += NBF_SIZEOF_FUNCTION_TABLE[list.type](list.values+i) - 
                1; // substracting the header as it is already hold in the list.type.
    }
    return size;
}

static size_t nbf_value_sizeof_RAW(nbf_typeless_value_t* value){
    return value->RAW.size +        // raw content
           sizeof(uint32_t) +       // raw len
           1;                       // header
}

static size_t nbf_value_sizeof_STRING(nbf_typeless_value_t* value){
    const char* str = value->STRING;
    size_t n = 0;
    for(const char* c = str; *c != '\0'; ++c) ++n;
    return n +                     // string content
           sizeof(uint32_t) +      // string len
           1;                      // header
}
static size_t nbf_value_sizeof_INT8(nbf_typeless_value_t*){
    static size_t size = sizeof(int8_t) +     // value
                         1;                   // header
    return size;
}
static size_t nbf_value_sizeof_INT16(nbf_typeless_value_t*){
    static size_t size = sizeof(int16_t) +    // value
                         1;                   // header
    return size;
}
static size_t nbf_value_sizeof_INT32(nbf_typeless_value_t*){
    static size_t size = sizeof(int32_t) +    // value
                         1;                   // header
    return size;
}
static size_t nbf_value_sizeof_INT64(nbf_typeless_value_t*){
    static size_t size = sizeof(int64_t) +    // value
                         1;                   // header
    return size;
}
static size_t nbf_value_sizeof_UINT8(nbf_typeless_value_t*){
    static size_t size = sizeof(uint8_t) +    // value
                         1;                   // header
    return size;
}
static size_t nbf_value_sizeof_UINT16(nbf_typeless_value_t*){
    static size_t size = sizeof(uint16_t) +   // value
                         1;                   // header
    return size;
}
static size_t nbf_value_sizeof_UINT32(nbf_typeless_value_t*){
    static size_t size = sizeof(uint32_t) +   // value
                         1;                   // header
    return size;
}
static size_t nbf_value_sizeof_UINT64(nbf_typeless_value_t*){
    static size_t size = sizeof(uint64_t) +   // value
                         1;                   // header
    return size;
}

static size_t nbf_value_sizeof_FLOAT32(nbf_typeless_value_t*){
    static size_t size = sizeof(float) +      // value
                         1;                   // header
    return size;
}

static size_t nbf_value_sizeof_FLOAT64(nbf_typeless_value_t*){
    static size_t size = sizeof(double) +     // value
                         1;                   // header
    return size;
}



static void nbf_value_print_EMPTY(nbf_typeless_value_t*){
    printf("null");
}

static void nbf_value_print_NODE(nbf_typeless_value_t* value){
    nbf_node_t node = value->NODE;
    printf("{");
    if(node.size > 0){
        nbf_field_t field = node.fields[0];
        if(*field.name != 0){
            printf("\"%s\":", field.name); nbf_value_print(&field.value);
        }
        for(size_t i = 1; i < node.size; ++i) {
            nbf_field_t field = node.fields[i];
            if(*field.name == 0) continue;
            printf(", \"%s\":", field.name); nbf_value_print(&field.value);
        }
    }
    printf("}");
}

static void nbf_value_print_LIST(nbf_typeless_value_t* value){
    nbf_list_t list = value->LIST;
    printf("[");
    if(list.size > 0){
        NBF_PRINT_FUNCTION_TABLE[list.type](list.values);
        for(size_t i = 1; i < list.size; ++i) {
            printf(", "); NBF_PRINT_FUNCTION_TABLE[list.type](list.values+i);
        }
    }
    printf("]");
}

static void nbf_value_print_RAW(nbf_typeless_value_t* value){
    printf_bytes(value->RAW.data, value->RAW.size);
}

static void nbf_value_print_STRING(nbf_typeless_value_t* value){
    printf("\"%s\"", value->STRING);
}

static void nbf_value_print_INT8(nbf_typeless_value_t* value){
    printf("%" PRIi8, value->INT8);
}
static void nbf_value_print_INT16(nbf_typeless_value_t* value){
    printf("%" PRIi16, value->INT16);
}
static void nbf_value_print_INT32(nbf_typeless_value_t* value){
    printf("%" PRIi32, value->INT32);
}
static void nbf_value_print_INT64(nbf_typeless_value_t* value){
    printf("%" PRIi64, value->INT64);
}
static void nbf_value_print_UINT8(nbf_typeless_value_t* value){
    printf("%" PRIu8, value->UINT8);
}
static void nbf_value_print_UINT16(nbf_typeless_value_t* value){
    printf("%" PRIu16, value->UINT16);
}
static void nbf_value_print_UINT32(nbf_typeless_value_t* value){
    printf("%" PRIu32, value->UINT32);
}
static void nbf_value_print_UINT64(nbf_typeless_value_t* value){
    printf("%" PRIu64, value->UINT64);
}

static void nbf_value_print_FLOAT32(nbf_typeless_value_t* value){
    printf("%f", value->FLOAT32);
}
static void nbf_value_print_FLOAT64(nbf_typeless_value_t* value){
    printf("%lf", value->FLOAT64);
}


static nbf_value_t nbf_value_clone_EMPTY(nbf_typeless_value_t* source, nbf_alloc_f allocator){
    return NBF_VEMPTY();
}

static nbf_value_t nbf_value_clone_NODE(nbf_typeless_value_t* source, nbf_alloc_f allocator){
    nbf_node_t node = (nbf_node_t){
        .fields = (nbf_field_t*) allocator(sizeof(nbf_field_t)*source->NODE.capacity),
        .size = source->NODE.size,
        .capacity = source->NODE.capacity
    };

    for(size_t i = 0; i < source->NODE.size; ++i) {
        size_t name_len = strlen(source->NODE.fields[i].name); 
        node.fields[i].name = (char*) memcpy(allocator(sizeof(char)*(name_len+1)), source->NODE.fields[i].name, name_len+1);
        node.fields[i].__name_ownership = NBF_OWNERSHIP_HEAP;
        node.fields[i].value = nbf_value_clone(&source->NODE.fields[i].value, allocator);
    }

    return (nbf_value_t){
        .tv = (nbf_typeless_value_t) {
            .NODE = node,
            .__ownership = NBF_OWNERSHIP_HEAP
        },
        .type = NBF_TYPES_NODE
    };
}

static nbf_value_t nbf_value_clone_LIST(nbf_typeless_value_t* source, nbf_alloc_f allocator){
    nbf_list_t list = (nbf_list_t) {
        .values = (nbf_typeless_value_t*) allocator(sizeof(nbf_typeless_value_t)*source->LIST.capacity),
        .size = source->LIST.size,
        .capacity = source->LIST.capacity,
        .type = source->LIST.type
    };

    for(size_t i = 0; i < source->LIST.size; ++i) {
        list.values[i] = NBF_CLONE_FUNCTION_TABLE[list.type](source->LIST.values+i, allocator).tv;
    }

    return (nbf_value_t) {
        .tv = (nbf_typeless_value_t) {
            .LIST = list,
            .__ownership = NBF_OWNERSHIP_HEAP
        },
        .type = NBF_TYPES_LIST
    };
}

static nbf_value_t nbf_value_clone_RAW(nbf_typeless_value_t* source, nbf_alloc_f allocator){
    return (nbf_value_t){
        .typeless_value = {
            .RAW = {
                .data = (byte*) memcpy(allocator(sizeof(byte)*source->RAW.size), source->RAW.data, source->RAW.size),
                .size = source->RAW.size
            },
            .__ownership = NBF_OWNERSHIP_HEAP
        },
        .type = NBF_TYPES_RAW
    };
}

static nbf_value_t nbf_value_clone_STRING(nbf_typeless_value_t* source, nbf_alloc_f allocator){
    size_t src_len = strlen(source->STRING)+1;
    return (nbf_value_t){
        .typeless_value = {
            .STRING = (char*) memcpy(allocator(sizeof(char)*src_len), source->STRING, src_len),
            .__ownership = NBF_OWNERSHIP_HEAP
        },
        .type = NBF_TYPES_STRING
    };
}

static nbf_value_t nbf_value_clone_INT8(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VINT8(source->INT8);
}
static nbf_value_t nbf_value_clone_INT16(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VINT16(source->INT16);
}
static nbf_value_t nbf_value_clone_INT32(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VINT32(source->INT32);
}
static nbf_value_t nbf_value_clone_INT64(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VINT64(source->INT64);
}
static nbf_value_t nbf_value_clone_UINT8(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VUINT8(source->UINT8);
}
static nbf_value_t nbf_value_clone_UINT16(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VUINT16(source->UINT16);
}
static nbf_value_t nbf_value_clone_UINT32(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VUINT32(source->UINT32);
}
static nbf_value_t nbf_value_clone_UINT64(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VUINT64(source->UINT64);
}

static nbf_value_t nbf_value_clone_FLOAT32(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VFLOAT32(source->FLOAT32);
}

static nbf_value_t nbf_value_clone_FLOAT64(nbf_typeless_value_t* source, nbf_alloc_f){
    return NBF_VFLOAT64(source->FLOAT64);
}

#endif

#endif // NESTED_BINARY_FORMAT_H
