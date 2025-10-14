#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#ifndef __DAEDALUS_H__
#define __DAEDALUS_H__

#define MAX_LINE_LENGTH        1024
#define MAX_FILENAME_LENGTH    256
#define MAX_NAME_LENGTH        32
#define MAX_DESCRIPTION_LENGTH 256
#define MAX_INPUT_LENGTH       16

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define STRCPY( dest, src ) strncpy( dest, src, sizeof( dest ) ); dest[sizeof(dest) - 1] = '\0'
#define STRNCPY(dest, src, n) strncpy(dest, src, n); dest[n - 1] = '\0'

#define RANDF(lower, upper) (((float)rand() / (float)(RAND_MAX)) * (upper - lower)) + lower

#define MAP( value, start0, start1, end0, end1 ) ( ( value - start0 ) * ( ( end1 - end0 ) / ( start1 - start0 ) ) + end0 )

#define PI 3.14159265

/**
 * @brief Represents a 2D vector.
 *
 * This structure is used for basic 2-dimensional mathematical operations,
 * such as representing points, directions, or translations in a 2D space.
 */
typedef struct _dVec2_t
{
  float x; /**< The X component of the vector. */
  float y; /**< The Y component of the vector. */
} dVec2_t;

/**
 * @brief Represents a 3D vector.
 *
 * This structure is fundamental for 3-dimensional mathematical operations,
 * commonly used for positions, directions, velocities, or colors in 3D space.
 */
typedef struct _dVec3_t
{
  float x; /**< The X component of the vector. */
  float y; /**< The Y component of the vector. */
  float z; /**< The Z component of the vector. */
} dVec3_t;

/**
 * @brief Represents a 4D vector.
 *
 * This structure is often used for homogeneous coordinates (x, y, z, w)
 * in 3D graphics, or for representing RGBA colors (red, green, blue, alpha).
 */
typedef struct _dVec4_t
{
  float x; /**< The X component of the vector. */
  float y; /**< The Y component of the vector. */
  float z; /**< The Z component of the vector. */
  float w; /**< The W (or Alpha) component of the vector. */
} dVec4_t;

/**
 * @brief Represents a 4x4 matrix, primarily for 3D transformations.
 *
 * This structure is crucial for linear algebra operations in 3D graphics,
 * such as translation, rotation, scaling, and projection transformations.
 * The elements are stored in column-major order (OpenGL convention).
 */
typedef struct _dMat4x4_t
{
  /**
   * @brief A 16-element array storing the matrix in column-major order.
   *
   * The elements are typically indexed as follows for a standard 4x4 matrix:
   * m[0] m[4] m[8]  m[12]
   * m[1] m[5] m[9]  m[13]
   * m[2] m[6] m[10] m[14]
   * m[3] m[7] m[11] m[15]
   */
  float m[16];
} dMat4x4_t;

/**
 * @brief Represents a kinematic body for physics simulations.
 *
 * This structure holds the physical state of an object that moves under
 * the influence of forces, velocity, and acceleration, typically
 * in a simplified physics model (e.g., without rotation or complex collisions).
 */
typedef struct _dKinematicBody
{
  dVec3_t position;     /**< The current position of the body in 3D space. */
  dVec3_t velocity;     /**< The current velocity of the body. */
  dVec3_t acceleration; /**< The current acceleration applied to the body. */
  dVec3_t force;        /**< The accumulated force acting on the body. */
  float mass;           /**< The mass of the body, used in force calculations (F=ma). */
} dKinematicBody_t;

/**
 * @brief Represents a single triangle in 3D space.
 *
 * Triangles are the fundamental building blocks for representing surfaces and
 * meshes in 3D graphics and geometry.
 */
typedef struct
{
  dVec3_t points[3]; /**< An array of three 3D vectors representing the vertices of the triangle. */
} dTriangle_t;

/**
 * @brief Represents a 3D mesh composed of multiple triangles.
 *
 * This structure is used to define the geometry of objects in a 3D scene,
 * allowing for the creation of complex shapes from simple triangular primitives.
 */
typedef struct
{
  dTriangle_t *triangles; /**< A pointer to an array of dTriangle_t structures forming the mesh. */
  int numberOfTriangles;  /**< The total number of triangles in the mesh. */
} dMesh_t;

/**
 * @brief Represents a node in a singly linked list.
 *
 * This structure allows for dynamic, ordered collections of data where
 * elements can be easily added or removed. It includes a small buffer
 * for common string data like filenames, though `data` allows for generic types.
 *
 * @warning The `buffer` member likely depends on `MAX_FILENAME_LENGTH` which
 * must be defined elsewhere. `data` and `buffer` usage may be mutually exclusive
 * or require careful management to avoid conflicts or undefined behavior.
 */
typedef struct _dLinkedList_t
{
  void *data;                      /**< A generic pointer to the actual data stored in this node. */
  char buffer[MAX_FILENAME_LENGTH];/**< A fixed-size buffer, often used for filenames or small strings. */
  struct _dLinkedList_t *next;     /**< Pointer to the next node in the linked list. NULL if this is the last node. */
} dLinkedList_t;

/**
 * @brief Represents a Quadtree data structure for 2D spatial partitioning.
 *
 * A Quadtree is used to efficiently store and query 2D objects (often rectangles)
 * in a hierarchical manner. It partitions space into four quadrants recursively.
 */
typedef struct _dQuadTree_t
{
  float rect[4];             /**< The bounding rectangle of this quadtree node, typically [x, y, width, height]. */
  int capacity;              /**< The maximum number of objects this node can hold before subdividing. */
  dLinkedList_t *objects;    /**< A linked list of objects contained within this quadtree node. */
} dQuadTree_t;


// -- Array Structures ---


/**
 * @brief Represents a dynamic array that can grow or shrink as needed.
 *
 * This structure provides a flexible, resizable array similar to `std::vector` in C++.
 * It handles its own memory allocation and deallocation to accommodate varying numbers of elements.
 *
 * @note This structure is designed to be initialized via functions like `d_InitArray()`,
 * which handle the initial memory allocation for `data` based on a starting capacity.
 * Direct manipulation of its members is generally discouraged for safety.
 */
typedef struct          // dArray_t
{
  size_t capacity;      /**< The current maximum number of elements the array can hold before reallocation. */
  size_t count;         /**< The current number of active elements stored in the array. */
  size_t element_size;  /**< The size in bytes of each individual element stored in the array. */
  void* data;           /**< A pointer to the dynamically allocated contiguous memory block holding the elements. */
} dArray_t;

/**
 * @brief Represents a static (fixed-size) array.
 *
 * This structure provides a fixed-capacity array where memory is allocated once
 * during initialization and does not change. It is ideal for situations requiring
 * predictable memory usage and direct access without reallocation overhead.
 *
 * @warning The `capacity` of a static array is fixed upon creation. Attempting to
 * `append` more elements than its `capacity` will result in failure.
 * @note This structure is designed to be initialized via functions like `d_InitStaticArray()`,
 * which set its `capacity` and allocate the underlying `data` buffer.
 * Direct manipulation of its members is generally discouraged for safety.
 */
typedef struct          // dStaticArray_t
{
  size_t capacity;      /**< The fixed maximum number of elements the array can hold. Set at initialization. */
  size_t count;         /**< The current number of active elements stored in the array. */
  size_t element_size;  /**< The size in bytes of each individual element stored in the array. */
  void* data;           /**< A pointer to the fixed-size contiguous memory block holding the elements. */
} dStaticArray_t;


// -- Table Structures --


/**
 * @brief Function pointer type for hashing a key.
 *
 * A hash function takes a key and its size, and returns a `size_t` hash value.
 * This function must be deterministic (same key always produces same hash)
 * and aim to distribute keys evenly across the hash space.
 *
 * @param key A pointer to the key data to be hashed.
 * @param key_size The size of the key data in bytes.
 * @return The computed hash value.
 */
typedef size_t (*dTableHashFunc)(const void* key, size_t key_size);

/**
 * @brief Function pointer type for comparing two keys for equality.
 *
 * A comparison function takes two keys and their size, and returns 0 if they are equal,
 * and a non-zero value otherwise. This is essential for resolving hash collisions.
 *
 * @param key1 A pointer to the first key.
 * @param key2 A pointer to the second key.
 * @param key_size The size of the keys in bytes.
 * @return 0 if `key1` and `key2` are equal, non-zero otherwise.
 */
typedef int (*dTableCompareFunc)(const void* key1, const void* key2, size_t key_size);

/**
 * @brief Callback function type for iterating over table entries.
 *
 * This function is called for each key-value pair during table iteration.
 * The pointers reference the internal table data and should not be modified.
 *
 * @param key Pointer to the key data
 * @param key_size Size of the key in bytes
 * @param value Pointer to the value data
 * @param value_size Size of the value in bytes
 * @param user_data Generic pointer passed from the caller for context
 */
typedef void (*dTableIteratorFunc)(const void* key, size_t key_size, const void* value, size_t value_size, void* user_data);

/**
 * @brief Callback function type for iterating over static array elements.
 *
 * This function is called for each element during static array iteration.
 * The pointer references the internal array data and should not be modified.
 *
 * @param index The index of the current element in the array
 * @param element Pointer to the element data
 * @param element_size Size of the element in bytes
 * @param user_data Generic pointer passed from the caller for context
 */
typedef void (*dStaticArrayIteratorFunc)(size_t index, const void* element, size_t element_size, void* user_data);

/**
 * @brief Represents a single key-value entry stored within the hash table.
 *
 * This internal structure holds the actual key and value data, making copies
 * of them for internal management.
 */
typedef struct
{
    void* key_data;    /**< A pointer to the internally managed copy of the key data. */
    void* value_data;  /**< A pointer to the internally managed copy of the value data. */
} dTableEntry_t;

/**
 * @brief Represents a hash table (or dictionary) that stores key-value pairs.
 *
 * Inspired by Python's dictionary, this structure provides efficient O(1) average-case
 * lookup, insertion, and deletion of elements. It uses chaining with `dLinkedList_t`
 * to handle collisions and automatically resizes its internal storage to maintain performance.
 *
 * @note This structure is designed to be initialized via `d_InitTable()`, which sets
 * up its internal buckets and function pointers.
 * @note Keys and values are copied internally. The user is responsible for managing
 * the memory of the original data passed to `d_SetDataInTable()`.
 * @warning The performance of the hash table heavily depends on the quality of
 * the hash function and key comparison function provided. Bad functions can
 * degrade performance to O(n) worst-case.
 */
typedef struct          // dTable_t
{
    dArray_t* buckets;      /**< An array of `dLinkedList_t` pointers, where each list serves as a bucket for collision resolution. */
    size_t num_buckets;     /**< The current total number of buckets (linked lists) in the hash table. */
    size_t count;           /**< The current number of key-value pairs stored in the table. */
    size_t key_size;        /**< The size in bytes of each key. */
    size_t value_size;      /**< The size in bytes of each value. */
    dTableHashFunc hash_func;     /**< Pointer to the function used for hashing keys. */
    dTableCompareFunc compare_func; /**< Pointer to the function used for comparing keys. */
    float load_factor_threshold; /**< The ratio of `count` to `num_buckets` at which the table will automatically rehash and grow. */
} dTable_t;

/**
 * @brief Represents a fixed-structure hash table with immutable key set and size.
 *
 * Unlike dTable_t, this hash table variant has a fixed number of keys that are set once
 * during initialization and cannot be changed afterward. Only values can be updated.
 * This design optimizes for scenarios where the structure is known in advance and provides
 * better memory layout predictability and cache performance.
 *
 * @note The key difference from dTable_t is the absence of load factor management,
 * as the table's size (number of keys) is fixed and cannot grow or shrink.
 * @note Keys are immutable after initialization - only values can be updated via d_SetStaticTableValue().
 * @note This structure provides O(1) value lookups and updates for a known, fixed set of keys.
 * @warning Keys cannot be added or removed after initialization. The structure is optimized
 * for use cases where the complete key set is known beforehand.
 */
typedef struct          // dStaticTable_t  
{
    dArray_t* buckets;            /**< An array of `dLinkedList_t` pointers for collision resolution (same as dTable_t). */
    size_t num_buckets;           /**< The fixed total number of buckets in the hash table. */
    size_t num_keys;              /**< The fixed number of keys in the table (immutable after init). */
    size_t key_size;              /**< The size in bytes of each key. */
    size_t value_size;            /**< The size in bytes of each value. */
    dTableHashFunc hash_func;     /**< Pointer to the function used for hashing keys. */
    dTableCompareFunc compare_func; /**< Pointer to the function used for comparing keys. */
    bool is_initialized;          /**< Flag indicating whether the table has been fully initialized with its fixed key set. */
} dStaticTable_t;


// -- String Structures ---


/**
 * @brief Represents a dynamic string (resizable string buffer).
 *
 * This structure provides a safe and flexible way to manage strings in C,
 * handling memory allocation automatically as the string grows or shrinks.
 *
 * @warning Direct modification of the `str` pointer or its content without
 * using provided `dString` functions can lead to memory corruption or undefined behavior.
 * Always use the library's API for string manipulation.
 * @note The `alloced` member includes space for the null terminator. The `len` member
 * represents the actual string length *excluding* the null terminator.
 */
typedef struct          // dString_t
{
    char* str;          /**< A pointer to the dynamically allocated character buffer containing the string. */
    size_t alloced;     /**< The total number of bytes currently allocated for the string buffer, including the null terminator. */
    size_t len;         /**< The current length of the string in characters, excluding the null terminator. */
} dString_t;

// =============================================================================
// LOGGING SYSTEM TYPES AND STRUCTURES
// =============================================================================

// Log levels enumeration
typedef enum {
    D_LOG_LEVEL_DEBUG = 0,
    D_LOG_LEVEL_INFO = 1,
    D_LOG_LEVEL_WARNING = 2,
    D_LOG_LEVEL_ERROR = 3,
    D_LOG_LEVEL_FATAL = 4,
    D_LOG_LEVEL_OFF = 5
} dLogLevel_t;

// Log entry structure - represents a single log message
typedef struct {
    dString_t* message;         // The log message content
    dLogLevel_t level;          // Severity level
    const char* file;           // Source file path
    int line;                   // Line number
    const char* function;       // Function name
    double timestamp;           // Unix timestamp
    const char* context;        // Optional context/module name
    uint32_t thread_id;         // Thread that generated the log
} dLogEntry_t;

// Log handler function pointer - processes log entries
typedef void (*dLogHandler_t)(const dLogEntry_t* entry, void* user_data);

// Handler registration structure
typedef struct {
    dLogHandler_t handler;      // Handler function
    void* user_data;            // User data passed to handler
    dLogLevel_t min_level;      // Minimum level this handler processes
} dLogHandlerReg_t;

// Logger configuration
typedef struct {
    dLogLevel_t default_level;      // Default log level
    bool include_timestamp;         // Include timestamp in output
    bool include_file_info;         // Include file/line info
    bool include_function;          // Include function name
    bool include_thread_id;         // Include thread ID
    bool colorize_output;           // Use ANSI colors for console
    const char* timestamp_format;   // strftime format string
    const char* context_separator;  // Separator for contexts (default: "::")
} dLogConfig_t;

// Filter rule types
typedef enum {
    D_LOG_FILTER_DIRECTORY,     // Match directory path
    D_LOG_FILTER_PREFIX,        // Match filename prefix
    D_LOG_FILTER_SUFFIX,        // Match filename suffix
    D_LOG_FILTER_EXACT,         // Match exact filename
    D_LOG_FILTER_GLOB,          // Match glob pattern
    D_LOG_FILTER_REGEX          // Match regex pattern
} dLogFilterType_t;

// Individual filter rule
typedef struct {
    dLogFilterType_t type;          // Type of filter
    const char* pattern;            // Pattern to match
    size_t pattern_len;             // Pre-computed pattern length
    dLogLevel_t level;              // Log level for matches
    uint32_t priority;              // Higher priority overrides lower
    uint32_t pattern_hash;          // Pre-computed hash for fast matching
    bool recursive;                 // For directory filters
} dLogFilterRule_t;

// Fast filter engine for zero-overhead logging
typedef struct {
    // Bloom filter for fast negative matches
    uint64_t bloom_filter[256];

    // Sorted array of rules by priority
    dArray_t* rules;                // Array of dLogFilterRule_t

    // LRU cache for recent path lookups
    struct {
        uint32_t path_hash;
        dLogLevel_t level;
        uint64_t last_access;
    } cache[1024];

    // Cache statistics
    uint32_t cache_hits;
    uint32_t cache_misses;
} dLogFilterEngine_t;

// Global configuration for compile-time optimization
#define D_MAX_SOURCE_FILES 4096
#define D_LOG_BLOOM_SIZE 256
#define D_LOG_CACHE_SIZE 1024

typedef struct {
    // Pre-computed log levels indexed by file ID (O(1) lookup)
    dLogLevel_t file_log_levels[D_MAX_SOURCE_FILES];

    // Filter engine for dynamic rules
    dLogFilterEngine_t* filter_engine;

    // Default level for unknown files
    dLogLevel_t default_level;

    // Global enable flag
    bool logging_enabled;

    // Statistics
    uint64_t total_logs_processed;
    uint64_t total_logs_suppressed;
} dLogGlobalConfig_t;

// Log statistics for monitoring
typedef struct {
    uint64_t logs_by_level[D_LOG_LEVEL_OFF];   // Count per level
    uint64_t logs_suppressed;                    // Total suppressed
    uint64_t logs_rate_limited;                  // Rate limited count
    double total_log_time;                       // Time spent logging
    uint32_t handler_errors;                     // Handler failures
} dLogStats_t;

// Main logger structure
typedef struct {
    dLogConfig_t config;            // Logger configuration
    dArray_t* handlers;             // Array of dLogHandlerReg_t
    dArray_t* contexts;             // Stack of context strings
    dString_t* format_buffer;       // Thread-local format buffer
    dLogFilterEngine_t* filters;    // Filter rules engine
    void* mutex;                    // Optional mutex for thread safety
    dLogStats_t* stats;             // Per-logger statistics
    bool is_global;                 // Is this the global logger
} dLogger_t;

// Log builder for zero-allocation complex messages
typedef struct {
    dString_t* buffer;              // Message buffer
    dLogLevel_t level;              // Log level
    dLogger_t* logger;              // Target logger
    const char* file;               // Source file
    int line;                       // Line number
    const char* function;           // Function name
    bool committed;                 // Prevent double commits
} dLogBuilder_t;

// Structured log builder for key-value logging
typedef struct {
    dLogBuilder_t base;             // Inherits from log builder
    dArray_t* fields;               // Array of key-value pairs
    bool in_json_mode;              // Output as JSON
} dLogStructured_t;

// Log context for hierarchical logging
typedef struct dLogContext_t {
    const char* name;               // Context name
    dLogger_t* logger;              // Associated logger
    uint64_t start_time;            // For timing contexts
    struct dLogContext_t* parent;   // Parent context for nesting
} dLogContext_t;

// Rate limiter for preventing log spam
typedef struct {
    uint32_t message_hash;          // Hash of the message
    uint32_t count;                 // Times logged
    uint32_t max_count;             // Maximum allowed
    double time_window;             // Time window in seconds
    double first_log_time;          // First log timestamp
    double last_log_time;           // Last log timestamp
} dLogRateLimit_t;

// Log filter configuration builder
typedef struct {
    dArray_t* rules;                // Building list of rules
    uint32_t next_priority;         // Auto-incrementing priority
} dLogFilterBuilder_t;



/* Vector Math Float */
float d_Sqrtf( float number ); //Quake fast inverse square root

int d_Clampi( int value, int min, int max );
float d_Clampf( float value, float min, float max );

float d_LengthOfVec2f( const dVec2_t vec ); //Length of a vector 2f
float d_DistanceVec2f( const dVec2_t a, const dVec2_t b ); //Distance between two vector 2fs
float d_DotProductVec2f( const dVec2_t a, const dVec2_t b ); //Dot product between two vector 2fs
float d_CrossProductVec2f( const dVec2_t a, const dVec2_t b ); //Cross product of two vector 2fs
void  d_AddTwoVec2f( dVec2_t *output, const dVec2_t a, const dVec2_t b ); //Add two vector 2fs
void  d_SubTwoVec2f( dVec2_t *output, const dVec2_t a, const dVec2_t b ); //Subtract two vector 2fs
void  d_GetVec2fFromAnglef( dVec2_t *output, float angle );
void  d_ScaleMultiplyVec2f( dVec2_t *output, const dVec2_t vec, const float value ); //Multiply a vector 2f by a value
void  d_ScaleDivideVec2f( dVec2_t *output, const dVec2_t vec, const float value ); //Divide a vector 2f by a value
void  d_LimitVec2f( dVec2_t *output, const dVec2_t a, const float value ); //Limit a vector 2f within a range
void  d_NormalizeVec2f( dVec2_t *output, const dVec2_t vec ); //Normalize a vector 2f
void  d_CreateNormalVec2f( dVec2_t *output, const dVec2_t a, const dVec2_t b ); //Create a normal vector from two vector 2fs
void  d_FindIntersectionVec2f( dVec2_t *output, const dVec2_t lineA0, const dVec2_t lineA1, const dVec2_t lineB0, const dVec2_t lineB1 ); //Find the intersection between two vector 2fs
dVec2_t d_Perpendicular( dVec2_t vec );
int d_PointOnRightSideOfLine( dVec2_t a, dVec2_t b, dVec2_t p );
int d_PointInsideOfTriangle( dVec2_t a, dVec2_t b, dVec2_t c, dVec2_t p );

float d_LengthOfVec3f( const dVec3_t vec ); //Length of a vector 3f
float d_DistanceVec3f( const dVec3_t a, const dVec3_t b ); //Distance between two vector 3fs
float d_DotProductVec3f( const dVec3_t a, const dVec3_t b ); //Dot product between two vector 3fs
void  d_CrossProductVec3f( dVec3_t *output, const dVec3_t a, const dVec3_t b ); //Cross product of two vector 3fs
void  d_AddTwoVec3f( dVec3_t *output, const dVec3_t a, const dVec3_t b ); //Add two vector 3fs
void  d_SubTwoVec3f( dVec3_t *output, const dVec3_t a, const dVec3_t b ); //Subtract two vector 3fs
void  d_ScaleMultiplyVec3f( dVec3_t *output, const dVec3_t vec, const float value ); //Multiply a vector 3f by a value
void  d_ScaleDivideVec3f( dVec3_t *output, const dVec3_t vec, const float value ); //Divide a vector 3f by a value
void  d_LimitVec3f( dVec3_t *output, const dVec3_t a, const float value ); //Limit a vector 3f within a range
void  d_NormalizeVec3f( dVec3_t *output, const dVec3_t vec ); //Normalize a vector 3f
void  d_CreateNormalVec3f( dVec3_t *output, const dVec3_t a, const dVec3_t b, const dVec3_t c ); //Create a normal vector from two vector 3fs
void  d_NormalizeVec4f( dVec4_t *output, const dVec4_t vec );

/* Matrix Operations */
void d_MatrixClearf( dMat4x4_t *matrix ); //Clear a 4x4 matrix to an identity matrix
void d_MatrixRotateXf( dMat4x4_t *matrix, const float angle_rad ); //Rotate matrix by angle in radians about the x axis
void d_MatrixRotateYf( dMat4x4_t *matrix, const float angle_rad ); //Rotate matrix by angle in radians about the y axis
void d_MatrixRotateZf( dMat4x4_t *matrix, const float angle_rad ); //Rotate matrix by angle in radians about the z axis
void d_MatrixCreateProjectionf( dMat4x4_t *matrix, const float aspect_ratio, const float fov, const float near, const float far ); //Create a projection matrix
void d_MatrixMultiplyf( dMat4x4_t *output, const dMat4x4_t a, const dMat4x4_t b ); //Multiply two 4x4 Matrices together
void d_MatrixTranslateVec3f( dMat4x4_t *matrix, const dVec3_t vec ); //Translate a matrix by a 3D point
void d_MatrixTranslateVec4f( dMat4x4_t *matrix, const dVec4_t vec ); //Translate a matrix by a 4D point
void d_MatrixMultiplyVec3f( dVec3_t *output, dMat4x4_t matrix, const dVec3_t vec ); //Transform a 3D point into a 4x4 matrix

void d_MatrixInverseTransformVec3f( dVec3_t *output, const dMat4x4_t matrix, dVec3_t vec ); //Inverse transform 3D point into a 4x4 matrix
void d_MatrixMultiplyVec4f( dVec4_t *output, const dMat4x4_t matrix, const dVec4_t vec4 ); //Transform a 4D point into a 4x4 matrix

/* Matrix Creation */
void d_MatrixXYf( dMat4x4_t output, const dVec3_t origin, const dVec3_t point0, const dVec3_t point1 );
void d_MatrixXZf( dMat4x4_t output, const dVec3_t origin, const dVec3_t point0, const dVec3_t point1 );
void d_MatrixYXf( dMat4x4_t output, const dVec3_t origin, const dVec3_t point0, const dVec3_t point1 );
void d_MatrixYZf( dMat4x4_t output, const dVec3_t origin, const dVec3_t point0, const dVec3_t point1 );
void d_MatrixZXf( dMat4x4_t output, const dVec3_t origin, const dVec3_t point0, const dVec3_t point1 );
void d_MatrixZYf( dMat4x4_t output, const dVec3_t origin, const dVec3_t point0, const dVec3_t point1 );


// -- Linked Lists Functions --


/**
 * @brief Initializes a new linked list with its first node.
 *
 * This function allocates memory for the head node of a new `dLinkedList_t` and copies
 * the provided data and name into it. This newly created node serves as the starting
 * point of the linked list.
 *
 * @param data A pointer to the data to be stored in the first node. The data
 * will be copied by `size` bytes.
 * @param name A C-string (char array) representing the name to be associated
 * with this initial node. This name will be copied into the node's internal buffer.
 * @param size The size in bytes of the data to be copied from `data`.
 *
 * @return A pointer to the head of the new `dLinkedList_t`, or `NULL` if
 * memory allocation fails.
 *
 * @note The `data` and `name` are copied into the node; the original pointers
 * do not need to remain valid after the function returns.
 * @note To add more elements, use functions like `d_PushBackToLinkedList` or `d_PushFrontToLinkedList`.
 * @note The returned list must be eventually freed using `d_ClearLinkedList` to prevent memory leaks.
 *
 * Example:
 * `int my_int = 42;`
 * `dLinkedList_t* myList = d_InitLinkedList(&my_int, "first_item", sizeof(int));`
 * `if (myList == NULL) { d_LogError("Failed to initialize linked list!"); }`
 */
dLinkedList_t* d_InitLinkedList( void *data, char *name, size_t size );

/**
 * @brief Destroys a linked list and frees all its associated memory.
 *
 * This function iterates through the entire linked list, freeing the `data`
 * pointer within each node and then freeing the node itself.
 * The `head` pointer passed by the caller will be set to `NULL` to prevent
 * dangling pointers after destruction.
 *
 * @param head A pointer to the pointer of the head of the linked list (`dLinkedList_t**`).
 * This allows the function to set the caller's head pointer to `NULL`.
 *
 * @return 0 on success, 1 on failure (e.g., if head pointer is NULL).
 *
 * @note After this function returns, the `dLinkedList_t*` pointer held by the caller
 * that was passed as `*head` will be `NULL`.
 * @note This function handles freeing the `data` stored inside each node,
 * assuming the data was allocated via `malloc` and is owned by the node.
 * If `data` points to external, unowned memory, it should not be freed here.
 *
 * Example:
 * `dLinkedList_t* myList = d_InitLinkedList(...);`
 * `// ... use myList ...`
 * `d_DestroyLinkedList(&myList);`
 * `// myList is now NULL`
 */
int d_DestroyLinkedList( dLinkedList_t **head );

/**
 * @brief Retrieves a pointer to the data stored in a linked list node at a specific index.
 *
 * This function traverses the linked list from the head and returns a pointer
 * to the data of the node found at the specified zero-based index.
 *
 * @param head A pointer to the head node of the linked list.
 * @param index The zero-based index of the node whose data is to be retrieved.
 *
 * @return A pointer to the data of the element at the specified index,
 * or `NULL` if the list is empty or the index is out of bounds.
 *
 * @note The returned pointer points directly to the data owned by the linked list node.
 * It remains valid as long as the node itself is not removed or the list is not destroyed.
 * @note The `index` must be less than the total number of nodes in the list.
 *
 * Example:
 * `int* value_ptr = (int*)d_IndexDataFromLinkedList(myList, 0);`
 * `if (value_ptr) { int first_value = *value_ptr; }`
 */
void* d_IndexDataFromLinkedList( dLinkedList_t *head, int index );

/**
 * @brief Finds and retrieves a pointer to the data stored in a linked list node by its name.
 *
 * This function traverses the linked list from the head and returns a pointer
 * to the data of the first node found with the matching name.
 *
 * @param head A pointer to the head node of the linked list.
 * @param name A C-string (char array) representing the name of the node whose data is to be retrieved.
 *
 * @return A pointer to the data of the element with the matching name,
 * or `NULL` if the list is empty, the `name` is NULL, or no node with the given name is found.
 *
 * @note The comparison is case-sensitive.
 * @note The returned pointer points directly to the data owned by the linked list node.
 * It remains valid as long as the node itself is not removed or the list is not destroyed.
 *
 * Example:
 * `MyStruct* s_ptr = (MyStruct*)d_FindDataFromLinkedList(myList, "player_data");`
 * `if (s_ptr) { // Use s_ptr }`
 */
void* d_FindDataFromLinkedList( dLinkedList_t *head, char *name );

/**
 * @brief Removes the node at a specific index from the linked list.
 *
 * This function locates the node at the given zero-based index, frees its
 * internal data and the node structure itself, and re-links the list.
 *
 * @param head A pointer to the pointer of the head of the linked list (`dLinkedList_t**`).
 * This allows the function to update the caller's head pointer if the
 * first node is removed.
 * @param index The zero-based index of the node to remove.
 *
 * @return 0 on success, 1 on failure (e.g., NULL list, invalid index).
 *
 * @note If the head node is removed (index 0), the `*head` pointer will be updated.
 * @note This function frees the `data` stored within the node. If the `data`
 * points to external memory that should *not* be freed by the list,
 * the caller must handle that memory's lifecycle externally, or a
 * more advanced design with a custom `free_data_func` would be needed.
 *
 * Example:
 * `int result = d_RemoveIndexFromLinkedList(&myList, 0); // Removes the first element`
 * `if (result != 0) { d_LogError("Failed to remove node at index."); }`
 */
int d_RemoveIndexFromLinkedList( dLinkedList_t **head, int index );

/**
 * @brief Removes the first node from the linked list that matches the specified name.
 *
 * This function traverses the linked list to find the first node with a matching
 * name, frees its internal data and the node structure itself, and re-links the list.
 *
 * @param head A pointer to the pointer of the head of the linked list (`dLinkedList_t**`).
 * This allows the function to update the caller's head pointer if the
 * first node is removed.
 * @param name A C-string (char array) representing the name of the node to remove.
 *
 * @return 0 on success, 1 on failure (e.g., NULL list, NULL name, name not found).
 *
 * @note The comparison for the name is case-sensitive.
 * @note If the head node matches the name, the `*head` pointer will be updated.
 * @note This function frees the `data` stored within the node. If the `data`
 * points to external memory that should *not* be freed by the list,
 * the caller must handle that memory's lifecycle externally.
 *
 * Example:
 * `int result = d_RemoveDataFromLinkedList(&myList, "old_item");`
 * `if (result != 0) { d_LogError("Failed to remove node by name."); }`
 */
int d_RemoveDataFromLinkedList( dLinkedList_t **head, char *name );

/**
 * @brief Adds a new node to the end of the linked list.
 *
 * This function creates a new node with the provided data and name, and
 * appends it to the tail of the linked list. If the list is initially empty,
 * the new node becomes the head of the list.
 *
 * @param head A pointer to the pointer of the head of the linked list (`dLinkedList_t**`).
 * This allows the function to update the caller's head pointer if the
 * list is empty.
 * @param data A pointer to the data to be copied into the new node.
 * @param name A C-string representing the name for the new node.
 * @param size The size in bytes of the data to be copied.
 *
 * @return 0 on success, 1 on failure (e.g., if memory allocation fails).
 *
 * @note The `data` and `name` are copied into the new node; their original
 * pointers do not need to remain valid after the function returns.
 * @note If `*head` is `NULL` (empty list), the new node becomes the head.
 *
 * Example:
 * `dLinkedList_t* myList = NULL; // Start with an empty list`
 * `int value1 = 10;`
 * `d_PushBackToLinkedList(&myList, &value1, "item1", sizeof(int));`
 * `int value2 = 20;`
 * `d_PushBackToLinkedList(&myList, &value2, "item2", sizeof(int));`
 */
int d_PushBackToLinkedList( dLinkedList_t **head, void *data, char *name, size_t size );

/**
 * @brief Adds a new node to the beginning of the linked list.
 *
 * This function creates a new node with the provided data and name, and
 * inserts it at the front of the linked list, making it the new head.
 *
 * @param head A pointer to the pointer of the head of the linked list (`dLinkedList_t**`).
 * This allows the function to update the caller's head pointer to the new node.
 * @param data A pointer to the data to be copied into the new node.
 * @param name A C-string representing the name for the new node.
 * @param size The size in bytes of the data to be copied.
 *
 * @return 0 on success, 1 on failure (e.g., if memory allocation fails).
 *
 * @note The `data` and `name` are copied into the new node; their original
 * pointers do not need to remain valid after the function returns.
 * @note The `*head` pointer will always be updated to point to the newly added node.
 *
 * Example:
 * `dLinkedList_t* myList = d_InitLinkedList(...); // Assume list is already initialized`
 * `int new_value = 5;`
 * `d_PushFrontToLinkedList(&myList, &new_value, "new_item", sizeof(int));`
 * `// new_item is now the first element in myList`
 */
int d_PushFrontToLinkedList( dLinkedList_t **head, void *data, char *name, size_t size );

/**
 * @brief Removes and returns the data from the last node of the linked list.
 *
 * This function removes the tail node from the linked list, frees its memory,
 * and returns a pointer to the data that was stored in that node.
 *
 * @param head A pointer to the pointer of the head of the linked list (`dLinkedList_t**`).
 * This is necessary to correctly handle the case where the last (and only)
 * node is popped, making the list empty.
 *
 * @return A pointer to the data from the removed tail node, or `NULL` if
 * the list is empty or an error occurs.
 *
 * @note The memory for the removed node itself (and its internal `data` buffer)
 * is freed by this function. The caller is responsible for managing the returned
 * `void* data` if they need it after this function call.
 *
 * Example:
 * `int* last_int_ptr = (int*)d_PopBackFromLinkedList(&myList);`
 * `if (last_int_ptr) { printf("Popped: %d\n", *last_int_ptr); free(last_int_ptr); }`
 * `else { d_LogWarning("List was empty or pop failed."); }`
 */
void* d_PopBackFromLinkedList( dLinkedList_t **head );


/**
 * @brief Removes and returns the data from the first node (head) of the linked list.
 *
 * This function removes the head node from the linked list, frees its memory,
 * updates the list's head to the next node, and returns a pointer to the data
 * that was stored in the original head node.
 *
 * @param head A pointer to the pointer of the head of the linked list (`dLinkedList_t**`).
 * This is necessary to correctly update the list's head pointer after removal.
 *
 * @return A pointer to the data from the removed head node, or `NULL` if
 * the list is empty or an error occurs.
 *
 * @note The memory for the removed node itself (and its internal `data` buffer)
 * is freed by this function. The caller is responsible for managing the returned
 * `void* data` if they need it after this function call.
 *
 * Example:
 * `char* first_name_ptr = (char*)d_PopFrontFromLinkedList(&myList);`
 * `if (first_name_ptr) { printf("Popped: %s\n", first_name_ptr); free(first_name_ptr); }`
 * `else { d_LogWarning("List was empty or pop failed."); }`
 */
void* d_PopFrontFromLinkedList( dLinkedList_t **head );



/**
 * @brief Prints the names of all nodes in the linked list to standard output.
 *
 * This function traverses the linked list from the head to the tail and
 * prints the `name` (stored in `current->buffer`) of each node.
 * It's primarily intended for debugging and visualization purposes.
 *
 * @param head A pointer to the head node of the linked list.
 *
 * @note This function uses `printf` to output to `stdout`.
 * @note If the list is empty, it will print a message indicating so.
 * @note Only the `name` (buffer) of each node is printed, not the generic `data` itself.
 *
 * Example:
 * `d_PrintLinkedList(myList);`
 * `// Possible output:`
 * `// Linked List Contents:`
 * `//   - Node: item1`
 * `//   - Node: item2`
 */
void d_PrintLinkedList( dLinkedList_t *head );

/**
 * @brief Calculates and returns the number of nodes (length) in the linked list.
 *
 * This function traverses the entire linked list from head to tail, counting
 * each node encountered.
 *
 * @param head A pointer to the head node of the linked list.
 *
 * @return The total number of nodes in the linked list. Returns 0 if the list is empty.
 *
 * @note The return type is `size_t` for consistency with array counts and to
 * support potentially very large lists.
 *
 * Example:
 * `size_t list_length = d_GetLengthOfLinkedList(myList);`
 * `d_LogInfoF("Linked List Length: %zu", list_length);`
 */
size_t d_GetLengthOfLinkedList( dLinkedList_t *head );

/**
 * @brief Check if a linked list contains a node with the specified name.
 *
 * This function traverses the linked list to determine if any node's name
 * (stored in the buffer field) matches the given target_name. This is useful
 * for hash table implementations where you need to quickly verify existence
 * without retrieving the actual data.
 *
 * @param head A pointer to the head node of the linked list.
 * @param target_name A C-string representing the name to search for.
 *
 * @return 0 if the name is found, 1 if not found or on error.
 *
 * @note The comparison is case-sensitive using strcmp().
 * @note Returns 1 (not found) if head or target_name is NULL.
 * @note This function is optimized for existence checking and returns
 * immediately upon finding the first match.
 *
 * Example:
 * `int exists = d_CheckForNameInLinkedList(myList, "user_config");`
 * `if (exists == 0) { // Name found in list }`
 */
int d_CheckForNameInLinkedList(dLinkedList_t *head, char *target_name);

/**
 * @brief Update existing data by name or insert new node if name not found (upsert operation).
 *
 * This function implements an "upsert" (update or insert) operation for linked lists.
 * It searches for a node with the specified name:
 * - If found: Updates the node's data with new_data, freeing the old data first
 * - If not found: Creates a new node and appends it to the end of the list
 *
 * @param head A pointer to the pointer of the head of the linked list (dLinkedList_t**).
 * This allows the function to update the head pointer if the list is initially empty.
 * @param new_data A pointer to the new data to store or update.
 * @param target_name A C-string representing the name of the node to update or create.
 * @param new_size The size in bytes of the new data.
 *
 * @return 0 on success (either update or insert), 1 on failure.
 *
 * @note The new_data is copied into the node; the original pointer does not
 * need to remain valid after the function returns.
 * @note If updating an existing node, the old data memory is freed before
 * allocating and copying the new data.
 * @note If the list is empty and a new node is created, it becomes the head.
 *
 * Example:
 * `PlayerStats stats = {level: 5, health: 100};`
 * `int result = d_UpdateDataByNameLinkedList(&gameData, &stats, "player1", sizeof(PlayerStats));`
 * `// Updates existing player1 data or creates new player1 node`
 */
int d_UpdateDataByNameLinkedList(dLinkedList_t **head, void *new_data, char *target_name, size_t new_size);

/**
 * @brief Get a direct pointer to the node (not just data) with the specified name.
 *
 * This function traverses the linked list and returns a pointer to the actual
 * dLinkedList_t node structure that matches the given target_name. This is
 * different from d_FindDataFromLinkedList which returns the void* data pointer.
 * Getting the node pointer allows for direct manipulation of the node structure
 * itself, which is useful for advanced operations like hash table chaining.
 *
 * @param head A pointer to the head node of the linked list.
 * @param target_name A C-string representing the name of the node to retrieve.
 *
 * @return A pointer to the dLinkedList_t node that matches the target_name,
 * or NULL if not found or on error.
 *
 * @note The comparison is case-sensitive using strcmp().
 * @note Returns NULL if head or target_name is NULL.
 * @note The returned node pointer remains valid until the node is removed
 * or the list is destroyed.
 * @note Use this function when you need access to the entire node structure,
 * not just the data it contains.
 *
 * Example:
 * `dLinkedList_t* node = d_GetNodeByNameLinkedList(myList, "config_node");`
 * `if (node) { // Direct access to node->data, node->buffer, node->next }`
 */
dLinkedList_t* d_GetNodeByNameLinkedList(dLinkedList_t *head, char *target_name);


// -- Hash Tables --


/**
 * @brief Initialize a new hash table structure.
 *
 * Allocates memory for the dTable_t structure, initializes its internal fields,
 * and allocates the initial array of dLinkedList_t pointers (buckets). Each
 * bucket is initially set to NULL.
 *
 * @param key_size The size in bytes of the keys that will be stored
 * @param value_size The size in bytes of the values that will be stored
 * @param hash_func A pointer to the user-provided hashing function
 * @param compare_func A pointer to the user-provided key comparison function
 * @param initial_num_buckets The initial number of buckets for the table
 *
 * @return A pointer to the newly initialized dTable_t instance, or NULL on failure
 *
 * Example:
 * `dTable_t* table = d_InitTable(sizeof(int), sizeof(char*), my_hash, my_compare, 16);`
 */
dTable_t* d_InitTable(size_t key_size, size_t value_size, dTableHashFunc hash_func,
                      dTableCompareFunc compare_func, size_t initial_num_buckets
                      );

/**
 * @brief Destroy a hash table and free all associated memory.
 *
 * Deallocates all memory associated with a dTable_t instance. This includes
 * iterating through all buckets, destroying each dLinkedList_t (which frees
 * the dTableEntry_ts and their internal key and value data), then freeing
 * the buckets array, and finally the dTable_t structure itself.
 *
 * @param table A pointer to the pointer of the hash table to destroy
 *
 * @return 0 on success, 1 on failure
 *
 * Example:
 * `d_DestroyTable(&my_table); // my_table will be NULL after this`
 */
int d_DestroyTable(dTable_t** table);

/**
 * @brief Insert or update a key-value pair in the hash table (upsert operation).
 *
 * Inserts a new key-value pair into the hash table, or updates the value if the
 * key already exists. Computes the hash, identifies the correct bucket, and either
 * updates an existing dTableEntry_t or creates a new one within that bucket's
 * linked list. Manages the table's count and triggers rehashing if needed.
 *
 * @param table A pointer to the hash table
 * @param key A pointer to the key data to set
 * @param value A pointer to the value data to set
 *
 * @return 0 on success, 1 on failure
 *
 * Example:
 * `int key = 42; char* value = "hello"; d_SetDataInTable(table, &key, &value);`
 */
int d_SetDataInTable(dTable_t* table, const void* key, const void* value);

/**
 * @brief Retrieve a pointer to the value associated with a given key.
 *
 * Retrieves a pointer to the value associated with a given key from the hash table.
 * Computes the hash, finds the relevant bucket, and searches the linked list in that
 * bucket using the compare function to locate the entry with the matching key.
 *
 * @param table A pointer to the hash table
 * @param key A pointer to the key data to search for
 *
 * @return A void* pointer to the internally stored value data if found, or NULL if not found
 *
 * Example:
 * `int key = 42; char** value = (char**)d_GetDataFromTable(table, &key);`
 */
void* d_GetDataFromTable(dTable_t* table, const void* key);

/**
 * @brief Remove a key-value pair from the hash table.
 *
 * Removes a key-value pair from the hash table based on the provided key.
 * Computes the hash, locates the correct bucket, and removes the entry with
 * the matching key from the linked list. Frees the key data, value data,
 * and the entry itself. Manages the table's count.
 *
 * @param table A pointer to the hash table
 * @param key A pointer to the key data to remove
 *
 * @return 0 on success, 1 on failure/key not found
 *
 * Example:
 * `int key = 42; d_RemoveDataFromTable(table, &key);`
 */
int d_RemoveDataFromTable(dTable_t* table, const void* key);

/**
 * @brief Check if a specific key exists in the hash table.
 *
 * Determines if a specific key exists within the hash table. Calculates the
 * bucket and uses the compare function to check for the key's presence in
 * the corresponding linked list.
 *
 * @param table A pointer to the hash table
 * @param key A pointer to the key data to check for
 *
 * @return 0 if the key exists, 1 if not found or error occurred
 *
 * Example:
 * `int key = 42; if (d_CheckForKeyInTable(table, &key) == 0) { // key exists }`
 */
int d_CheckForKeyInTable(const dTable_t* table, const void* key);


/**
 * @brief Get the current number of key-value pairs in the hash table.
 *
 * Returns the current number of key-value pairs (entries) stored in the hash table.
 *
 * @param table A pointer to the hash table
 *
 * @return The total count of entries, or 0 if table is NULL
 *
 * Example:
 * `size_t count = d_GetCountInTable(table);`
 */
size_t d_GetCountInTable(const dTable_t* table);

/**
 * @brief Clear all entries from the hash table but keep the structure intact.
 *
 * Empties the hash table, removing all key-value pairs and freeing their
 * associated memory, but keeps the underlying dTable_t structure (including
 * its buckets array and allocated num_buckets) intact for reuse.
 *
 * @param table A pointer to the hash table to clear
 *
 * @return 0 on success, 1 on failure
 *
 * Example:
 * `d_ClearTable(table); // Table is now empty but ready for reuse`
 */
int d_ClearTable(dTable_t* table);

/**
 * @brief Rehash the table with a new number of buckets to optimize performance.
 *
 * This function creates a new bucket array with the specified size, recalculates
 * the hash for all existing entries, and redistributes them across the new buckets.
 * This is essential for maintaining O(1) performance as the table grows.
 *
 * @param table A pointer to the hash table to rehash
 * @param new_num_buckets The new number of buckets (0 for auto-sizing to double current)
 *
 * @return 0 on success, 1 on failure
 *
 * @note If new_num_buckets is 0, the function will automatically double the current bucket count
 * @note The function will fail if new_num_buckets is not greater than the current bucket count
 * @note All existing entries are preserved and redistributed based on their recalculated hashes
 *
 * Example:
 * `d_RehashTable(table, 32); // Rehash to 32 buckets`
 * `d_RehashTable(table, 0);  // Auto-resize to double current bucket count`
 */
int d_RehashTable(dTable_t* table, size_t new_num_buckets);

/**
 * @brief Get an array containing copies of all keys currently stored in the hash table.
 *
 * This function iterates through all buckets and collects all keys, returning them
 * in a newly allocated dArray_t. The keys are copied, so the returned array can
 * be modified independently of the hash table.
 *
 * @param table A pointer to the hash table
 *
 * @return A newly allocated dArray_t containing all keys, or NULL on failure
 *
 * @note The caller is responsible for destroying the returned array with d_DestroyArray
 * @note The keys are copied into the array, not referenced
 * @note If the table is empty, returns an empty array (not NULL)
 *
 * Example:
 * `dArray_t* keys = d_GetAllKeysFromTable(table);`
 * `// Use keys array...`
 * `d_DestroyArray(keys);`
 */
dArray_t* d_GetAllKeysFromTable(const dTable_t* table);

/**
 * @brief Get an array containing copies of all values currently stored in the hash table.
 *
 * This function iterates through all buckets and collects all values, returning them
 * in a newly allocated dArray_t. The values are copied, so the returned array can
 * be modified independently of the hash table.
 *
 * @param table A pointer to the hash table
 *
 * @return A newly allocated dArray_t containing all values, or NULL on failure
 *
 * @note The caller is responsible for destroying the returned array with d_DestroyArray
 * @note The values are copied into the array, not referenced
 * @note If the table is empty, returns an empty array (not NULL)
 *
 * Example:
 * `dArray_t* values = d_GetAllValuesFromTable(table);`
 * `// Use values array...`
 * `d_DestroyArray(values);`
 */
dArray_t* d_GetAllValuesFromTable(const dTable_t* table);

/**
 * @brief Initialize a new static hash table with fixed key structure and initial data.
 *
 * Creates and fully initializes a dStaticTable_t structure with the specified parameters
 * and populates it with the provided keys and initial values. After this function succeeds,
 * the table is ready for use with only value update operations allowed.
 *
 * @param key_size The size in bytes of the keys that will be stored
 * @param value_size The size in bytes of the values that will be stored
 * @param hash_func A pointer to the user-provided hashing function
 * @param compare_func A pointer to the user-provided key comparison function
 * @param num_buckets The number of buckets for the table (should be optimized for expected key count)
 * @param keys Array of key data to initialize with (must contain exactly num_keys elements)
 * @param initial_values Array of initial value data (must contain exactly num_keys elements, parallel to keys)
 * @param num_keys Number of elements in both keys and initial_values arrays
 *
 * @return A pointer to the newly initialized dStaticTable_t instance, or NULL on failure
 *
 * @warning CRITICAL: The keys and initial_values arrays MUST contain exactly num_keys elements.
 *          C cannot validate array sizes at runtime - it is the caller's responsibility to ensure
 *          both arrays have the correct number of elements. Mismatched sizes will cause undefined behavior.
 *
 * @note The num_keys parameter is required because C arrays decay to pointers when passed as parameters,
 *       making automatic size detection impossible.
 *
 * Example:
 * ```c
 * int keys[] = {1, 2, 3}; 
 * char* values[] = {"a", "b", "c"};
 * size_t count = sizeof(keys)/sizeof(keys[0]); // Calculate array size safely
 * dStaticTable_t* table = d_InitStaticTable(sizeof(int), sizeof(char*), my_hash, my_compare, 16, 
 *                                           (const void**)keys, (const void**)values, count);
 * ```
 */
dStaticTable_t* d_InitStaticTable(size_t key_size, size_t value_size, dTableHashFunc hash_func,
                                  dTableCompareFunc compare_func, size_t num_buckets,
                                  const void** keys, const void** initial_values, size_t num_keys);

/**
 * @brief Destroy a static hash table and free all associated memory.
 *
 * Deallocates all memory associated with a dStaticTable_t instance, including
 * all entries, buckets, and the structure itself.
 *
 * @param table A pointer to the pointer of the static hash table to destroy
 *
 * @return 0 on success, 1 on failure
 *
 * Example:
 * `d_DestroyStaticTable(&my_table); // my_table will be NULL after this`
 */
int d_DestroyStaticTable(dStaticTable_t** table);

/**
 * @brief Update the value associated with a key in the static table.
 *
 * Updates the value for an existing key in the static table. Since the key set
 * is immutable, this function only allows value updates, not key insertion.
 *
 * @param table A pointer to the static hash table
 * @param key A pointer to the key data to update
 * @param new_value A pointer to the new value data
 *
 * @return 0 on success, 1 on failure/key not found
 *
 * Example:
 * `int key = 42; char* new_value = "updated"; d_SetStaticTableValue(table, &key, &new_value);`
 */
int d_SetValueInStaticTable(dStaticTable_t* table, const void* key, const void* new_value);


/**
 * @brief Retrieve a pointer to the value associated with a given key.
 *
 * Retrieves a pointer to the value associated with a given key from the static hash table.
 *
 * @param table A pointer to the static hash table
 * @param key A pointer to the key data to search for
 *
 * @return A void* pointer to the internally stored value data if found, or NULL if not found
 *
 * Example:
 * `int key = 42; char** value = (char**)d_GetValueInStaticTable(table, &key);`
 */
void* d_GetValueInStaticTable(const dStaticTable_t* table, const void* key);

/**
 * @brief Check if a specific key exists in the static hash table.
 *
 * @param table A pointer to the static hash table
 * @param key A pointer to the key data to check for
 *
 * @return 0 if the key exists, 1 if not found or error occurred
 *
 * Example:
 * `int key = 42; if (d_CheckForKeyInStaticTable(table, &key) == 0) { // key exists }`
 */
int d_CheckForKeyInStaticTable(const dStaticTable_t* table, const void* key);

/**
 * @brief Get the fixed number of keys in the static hash table.
 *
 * @param table A pointer to the static hash table
 *
 * @return The total count of keys, or 0 if table is NULL or uninitialized
 *
 * Example:
 * `size_t key_count = d_GetKeyCountOfStaticTable(table);`
 */
size_t d_GetKeyCountOfStaticTable(const dStaticTable_t* table);

/**
 * @brief Get an array containing all keys from the static hash table.
 *
 * @param table A pointer to the static hash table
 *
 * @return A newly allocated dArray_t containing all keys, or NULL on failure
 *
 * @note The caller is responsible for destroying the returned array with d_DestroyArray
 * @note The keys are copied into the array, not referenced
 *
 * Example:
 * `dArray_t* keys = d_GetAllKeysFromStaticTable(table);`
 * `d_DestroyArray(keys);`
 */
dArray_t* d_GetAllKeysFromStaticTable(const dStaticTable_t* table);

/**
 * @brief Get an array containing all values from the static hash table.
 *
 * @param table A pointer to the static hash table
 *
 * @return A newly allocated dArray_t containing all values, or NULL on failure
 *
 * @note The caller is responsible for destroying the returned array with d_DestroyArray
 * @note The values are copied into the array, not referenced
 *
 * Example:
 * `dArray_t* values = d_GetAllValuesFromStaticTable(table);`
 * `d_DestroyArray(values);`
 */
dArray_t* d_GetAllValuesFromStaticTable(const dStaticTable_t* table);

/**
 * @brief Clear all entries from the static hash table but keep the structure intact.
 *
 * Empties the static table, removing all key-value pairs and resetting it to
 * an uninitialized state. The table structure is preserved for reuse with
 * d_InitKeysInStaticTable() or by creating a new table with d_InitStaticTable().
 *
 * @param table A pointer to the static hash table to clear
 *
 * @return 0 on success, 1 on failure
 *
 * @note After this operation, the table will be in an uninitialized state
 * @note The bucket structure is preserved, only the entries are removed
 * @note To reuse the table, you must reinitialize it with keys
 *
 * Example:
 * `d_ClearStaticTable(table); // Table is now empty and uninitialized`
 */
int d_ClearStaticTable(dStaticTable_t* table);

/**
 * @brief Get bucket distribution statistics for the static hash table.
 *
 * This function analyzes the distribution of entries across buckets to help
 * evaluate hash function effectiveness and table performance. For static tables,
 * this is particularly useful for choosing optimal bucket count during initialization.
 *
 * @param table A pointer to the static hash table
 * @param min_entries Pointer to store minimum entries in any bucket
 * @param max_entries Pointer to store maximum entries in any bucket
 * @param avg_entries Pointer to store average entries per bucket
 * @param empty_buckets Pointer to store count of empty buckets
 *
 * @return 0 on success, 1 on failure
 *
 * @note All output parameters are optional (can be NULL)
 * @note Average is calculated as a floating-point value
 * @note Useful for performance analysis and optimization
 *
 * Example:
 * `size_t min, max, empty; float avg;`
 * `d_GetStaticTableStats(table, &min, &max, &avg, &empty);`
 */
int d_GetStaticTableStats(const dStaticTable_t* table, size_t* min_entries, 
                          size_t* max_entries, float* avg_entries, size_t* empty_buckets);

/**
 * @brief Create a copy of the static hash table with a different bucket count.
 *
 * Creates a new static table with the same keys and values but redistributed
 * across a different number of buckets. This is useful for optimizing performance
 * when the ideal bucket count is determined after initial creation.
 *
 * @param source_table A pointer to the source static hash table
 * @param new_num_buckets The number of buckets for the new table
 *
 * @return A pointer to the newly created static table, or NULL on failure
 *
 * @note The source table remains unchanged
 * @note All keys and values are copied to the new table
 * @note The caller is responsible for destroying both tables
 * @note Useful for performance optimization without modifying original table
 *
 * Example:
 * `dStaticTable_t* optimized = d_RebucketStaticTable(original, 64);`
 * `// Use optimized table, destroy when done`
 * `d_DestroyStaticTable(&optimized);`
 */
dStaticTable_t* d_RebucketStaticTable(const dStaticTable_t* source_table, size_t new_num_buckets);

/**
 * @brief Save a static hash table to a binary file.
 *
 * Writes the complete static table structure and all key-value pairs to a file
 * in a binary format that can be read back by d_LoadStaticTableFromFile().
 *
 * @param filename Path to the output file
 * @param table Pointer to the static table to save
 * @return 0 on success, 1 on failure
 *
 * @note The hash and compare functions cannot be saved and must be provided when loading
 */
int d_SaveStaticTableToFile(const char* filename, const dStaticTable_t* table);

/**
 * @brief Load a static hash table from a binary file.
 *
 * Reads a file created by d_SaveStaticTableToFile() and reconstructs the static table.
 * The hash and compare functions must be provided as they cannot be stored in the file.
 *
 * @param filename Path to the input file
 * @param hash_func Hash function to use (must match the original table)
 * @param compare_func Compare function to use (must match the original table)
 * @return Pointer to the loaded static table, or NULL on failure
 *
 * @note The caller is responsible for destroying the returned table
 */
dStaticTable_t* d_LoadStaticTableFromFile(const char* filename, dTableHashFunc hash_func, dTableCompareFunc compare_func);

/**
 * @brief Iterate over all key-value pairs in a static hash table.
 *
 * Calls the provided callback function for each key-value pair in the table.
 * This provides a memory-efficient way to process all entries without copying them.
 *
 * @param table Pointer to the static table to iterate over
 * @param callback Function to call for each key-value pair
 * @param user_data Generic pointer passed to the callback for context
 * @return 0 on success, 1 on failure
 */
int d_IterateStaticTable(const dStaticTable_t* table, dTableIteratorFunc callback, void* user_data);

/**
 * @brief Create a complete deep copy of a static hash table.
 *
 * Creates a new static table with the same configuration and copies all key-value pairs.
 * The new table is completely independent of the original.
 *
 * @param source_table Pointer to the source static table to clone
 * @return Pointer to the newly created static table, or NULL on failure
 *
 * @note The caller is responsible for destroying the returned table
 */
dStaticTable_t* d_CloneStaticTable(const dStaticTable_t* source_table);

// =============================================================================
// BUILT-IN HASH FUNCTIONS
// =============================================================================

/**
 * @brief Hash function for 32-bit integers using Knuth's multiplicative method.
 *
 * Uses Knuth's magic constant for excellent distribution with integer keys.
 * Suitable for most general-purpose integer hashing needs.
 *
 * @param key Pointer to integer key
 * @param key_size Size of the key (should be sizeof(int))
 * @return Hash value
 *
 * Example:
 * ```c
 * dTable_t* table = d_InitTable(sizeof(int), sizeof(char*), 
 *                               d_HashInt, d_CompareInt, 16, 0.75f);
 * ```
 */
size_t d_HashInt(const void* key, size_t key_size);

/**
 * @brief Hash function for null-terminated strings using FNV-1a algorithm.
 *
 * FNV-1a provides excellent distribution for string data with good performance.
 * Works with any null-terminated string regardless of the key_size parameter.
 *
 * @param key Pointer to null-terminated string
 * @param key_size Size parameter (can be 0 for null-terminated strings)
 * @return Hash value
 *
 * Example:
 * ```c
 * dTable_t* table = d_InitTable(sizeof(char*), sizeof(int), 
 *                               d_HashString, d_CompareString, 16, 0.75f);
 * ```
 */
size_t d_HashString(const void* key, size_t key_size);

/**
 * @brief Hash function for string literals (const char*).
 *
 * Similar to d_HashString but expects the key to be a char* directly,
 * not a pointer to a char*. Useful for string literal keys.
 *
 * @param key Pointer to string (char*)
 * @param key_size Length of the string (can be 0 for null-terminated)
 * @return Hash value
 */
size_t d_HashStringLiteral(const void* key, size_t key_size);

/**
 * @brief Hash function for floating-point numbers.
 *
 * Converts float to integer representation for hashing.
 * Note: Be careful with floating-point precision when using as keys.
 *
 * @param key Pointer to float value
 * @param key_size Size of the key (should be sizeof(float))
 * @return Hash value
 *
 * Example:
 * ```c
 * dTable_t* table = d_InitTable(sizeof(float), sizeof(int), 
 *                               d_HashFloat, d_CompareFloat, 16, 0.75f);
 * ```
 */
size_t d_HashFloat(const void* key, size_t key_size);

/**
 * @brief Hash function for double-precision floating-point numbers.
 *
 * Converts double to integer representation for hashing.
 * Note: Be careful with floating-point precision when using as keys.
 *
 * @param key Pointer to double value
 * @param key_size Size of the key (should be sizeof(double))
 * @return Hash value
 */
size_t d_HashDouble(const void* key, size_t key_size);

/**
 * @brief General-purpose hash function for binary data using FNV-1a.
 *
 * Can hash any binary data of specified length. Useful for structs,
 * arrays, or any fixed-size binary data.
 *
 * @param key Pointer to binary data
 * @param key_size Size of the data in bytes
 * @return Hash value
 *
 * Example:
 * ```c
 * typedef struct { int x, y; } Point;
 * dTable_t* table = d_InitTable(sizeof(Point), sizeof(int), 
 *                               d_HashBinary, d_CompareBinary, 16, 0.75f);
 * ```
 */
size_t d_HashBinary(const void* key, size_t key_size);

/**
 * @brief Hash function for pointers (memory addresses).
 *
 * Hashes the pointer value itself, not the data it points to.
 * Useful when you want to use memory addresses as keys.
 *
 * @param key Pointer to pointer value
 * @param key_size Size of pointer (should be sizeof(void*))
 * @return Hash value
 */
size_t d_HashPointer(const void* key, size_t key_size);

// =============================================================================
// BUILT-IN COMPARISON FUNCTIONS
// =============================================================================

/**
 * @brief Comparison function for 32-bit integers.
 *
 * @param key1 Pointer to first integer
 * @param key2 Pointer to second integer
 * @param key_size Size of keys (should be sizeof(int))
 * @return 0 if equal, non-zero if different
 */
int d_CompareInt(const void* key1, const void* key2, size_t key_size);

/**
 * @brief Comparison function for null-terminated strings.
 *
 * Compares strings pointed to by char** keys using strcmp.
 *
 * @param key1 Pointer to first string pointer (char**)
 * @param key2 Pointer to second string pointer (char**)
 * @param key_size Size parameter (unused for null-terminated strings)
 * @return 0 if equal, non-zero if different
 */
int d_CompareString(const void* key1, const void* key2, size_t key_size);

/**
 * @brief Comparison function for string literals (const char*).
 *
 * Compares strings directly, expecting keys to be char* not char**.
 *
 * @param key1 Pointer to first string (char*)
 * @param key2 Pointer to second string (char*)
 * @param key_size Length to compare (0 for full null-terminated comparison)
 * @return 0 if equal, non-zero if different
 */
int d_CompareStringLiteral(const void* key1, const void* key2, size_t key_size);

/**
 * @brief Comparison function for floating-point numbers.
 *
 * Compares floats with exact equality. Be aware that floating-point
 * precision issues may cause problems with hash table lookups.
 *
 * @param key1 Pointer to first float
 * @param key2 Pointer to second float
 * @param key_size Size of keys (should be sizeof(float))
 * @return 0 if equal, non-zero if different
 */
int d_CompareFloat(const void* key1, const void* key2, size_t key_size);

/**
 * @brief Comparison function for double-precision floating-point numbers.
 *
 * Compares doubles with exact equality. Be aware that floating-point
 * precision issues may cause problems with hash table lookups.
 *
 * @param key1 Pointer to first double
 * @param key2 Pointer to second double
 * @param key_size Size of keys (should be sizeof(double))
 * @return 0 if equal, non-zero if different
 */
int d_CompareDouble(const void* key1, const void* key2, size_t key_size);

/**
 * @brief Comparison function for binary data using memcmp.
 *
 * Performs byte-by-byte comparison of binary data.
 *
 * @param key1 Pointer to first data block
 * @param key2 Pointer to second data block
 * @param key_size Size of data blocks in bytes
 * @return 0 if equal, non-zero if different
 */
int d_CompareBinary(const void* key1, const void* key2, size_t key_size);

/**
 * @brief Comparison function for pointers (memory addresses).
 *
 * Compares pointer values, not the data they point to.
 *
 * @param key1 Pointer to first pointer value
 * @param key2 Pointer to second pointer value
 * @param key_size Size of pointer (should be sizeof(void*))
 * @return 0 if equal, non-zero if different
 */
int d_ComparePointer(const void* key1, const void* key2, size_t key_size);

// =============================================================================
// SPECIALIZED HASH FUNCTIONS FOR COMMON PATTERNS
// =============================================================================

/**
 * @brief Hash function optimized for small positive integers (0-999).
 *
 * Uses simple multiplication with good distribution for small integer ranges.
 * More efficient than general-purpose hashing for known small ranges.
 *
 * @param key Pointer to integer key
 * @param key_size Size of the key (should be sizeof(int))
 * @return Hash value
 */
size_t d_HashSmallInt(const void* key, size_t key_size);

/**
 * @brief Hash function for case-insensitive strings.
 *
 * Converts characters to lowercase before hashing for case-insensitive keys.
 *
 * @param key Pointer to string pointer (char**)
 * @param key_size Size parameter (unused for null-terminated strings)
 * @return Hash value
 */
size_t d_HashStringCaseInsensitive(const void* key, size_t key_size);

/**
 * @brief Comparison function for case-insensitive strings.
 *
 * Performs case-insensitive string comparison.
 *
 * @param key1 Pointer to first string pointer (char**)
 * @param key2 Pointer to second string pointer (char**)
 * @param key_size Size parameter (unused for null-terminated strings)
 * @return 0 if equal, non-zero if different
 */
int d_CompareStringCaseInsensitive(const void* key1, const void* key2, size_t key_size);

/**
 * @brief Comparison function for dStaticArray_t objects.
 *
 * Compares two static arrays element by element using binary comparison.
 * Arrays must have the same element_size to be comparable.
 *
 * @param array1 Pointer to first dStaticArray_t* (pointer to dStaticArray_t pointer)
 * @param array2 Pointer to second dStaticArray_t* (pointer to dStaticArray_t pointer)
 * @param unused Size parameter (unused for arrays)
 * @return 0 if equal, non-zero if different
 *
 * Example:
 * ```c
 * dStaticArray_t *arr1, *arr2;
 * if (d_CompareStaticArray(&arr1, &arr2, 0) == 0) {
 *     // Arrays are identical
 * }
 * ```
 */
int d_CompareStaticArray(const void* array1, const void* array2, size_t unused);

/**
 * @brief Comparison function for dArray_t objects.
 *
 * Compares two dynamic arrays element by element using binary comparison.
 * Arrays must have the same element_size to be comparable.
 *
 * @param array1 Pointer to first dArray_t* (pointer to dArray_t pointer)
 * @param array2 Pointer to second dArray_t* (pointer to dArray_t pointer)
 * @param unused Size parameter (unused for arrays)
 * @return 0 if equal, non-zero if different
 *
 * Example:
 * ```c
 * dArray_t *arr1, *arr2;
 * if (d_CompareDArray(&arr1, &arr2, 0) == 0) {
 *     // Arrays are identical
 * }
 * ```
 */
int d_CompareDArray(const void* array1, const void* array2, size_t unused);

/**
 * @brief Comparison function for dTable_t objects.
 *
 * Compares two dynamic hash tables by comparing their structure and all key-value pairs.
 * Tables must have matching key_size, value_size, and identical contents to be equal.
 *
 * @param table1 Pointer to first dTable_t* (pointer to dTable_t pointer)
 * @param table2 Pointer to second dTable_t* (pointer to dTable_t pointer)
 * @param unused Size parameter (unused for tables)
 * @return 0 if equal, non-zero if different
 *
 * Example:
 * ```c
 * dTable_t *table1, *table2;
 * if (d_CompareTable(&table1, &table2, 0) == 0) {
 *     // Tables have identical structure and contents
 * }
 * ```
 */
int d_CompareTable(const void* table1, const void* table2, size_t unused);

/**
 * @brief Comparison function for dStaticTable_t objects.
 *
 * Compares two static hash tables by comparing their structure and all key-value pairs.
 * Tables must have matching key_size, value_size, num_keys, and identical contents to be equal.
 *
 * @param table1 Pointer to first dStaticTable_t* (pointer to dStaticTable_t pointer)
 * @param table2 Pointer to second dStaticTable_t* (pointer to dStaticTable_t pointer)
 * @param unused Size parameter (unused for tables)
 * @return 0 if equal, non-zero if different
 *
 * Example:
 * ```c
 * dStaticTable_t *table1, *table2;
 * if (d_CompareStaticTable(&table1, &table2, 0) == 0) {
 *     // Tables have identical structure and contents
 * }
 * ```
 */
int d_CompareStaticTable(const void* table1, const void* table2, size_t unused);

/**
 * @brief Comparison function for dString_t objects.
 *
 * @param key1 Pointer to first dString_t* (pointer to dString_t pointer)
 * @param key2 Pointer to second dString_t* (pointer to dString_t pointer)
 * @param key_size Size parameter (unused for dString_t)
 * @return 0 if equal, non-zero if different
 */
int d_CompareDString(const void* key1, const void* key2, size_t key_size);

/* Quad Tree */
dQuadTree_t *d_CreateQuadtree( float *rect, int capacity );
void d_InsertObjectInQuadtree( dQuadTree_t *tree, void *object );
void d_SubdivideQuadtree( dQuadTree_t *tree );

/* Kinematic Body 2D  */
void d_CreateKinmaticBody( dKinematicBody_t *output, const dVec2_t position, const dVec2_t velocity, const dVec2_t acceleration, const float mass );

/* Strings */

/**
 * @brief Checks if a dString_t is NULL, contains a NULL buffer, or is empty.
 *
 * @param s The dString_t pointer to validate.
 * @return true if the string is considered invalid or empty, otherwise false.
 */
static inline bool d_IsStringInvalid(const dString_t* s)
{
    return (s == NULL || s->str == NULL || s->len == 0);
}

char* d_CreateStringFromFile(const char *filename);

/**
 * @brief Create a new string builder.
 *
 * @return A new string builder, or NULL on allocation failure.
 */
dString_t *d_InitString(void);

/**
 * @brief Destroy a string builder and free its memory.
 * 
 * @param sb The string builder to destroy.
 * 
 * @return: Success/failure indicator.
 */
void d_DestroyString(dString_t* sb);

/**
 * @brief Add a C string to the string builder. 
 * 
 * @param sb The string builder to append to.
 * @param str The string to append (must be null-terminated if len is 0).
 * @param len The length of string to append, or 0 to use strlen().
 * 
 * @return: Success/failure indicator.
 */
void d_AppendToString(dString_t* sb, const char* str, size_t len);

/**
 * @brief Set the content of an existing dString_t to a new value.
 *
 * @param string: Pointer to existing dString_t structure.
 * @param content: C-string content to copy into the dString_t.
 * @param flags: Optional flags for string handling behavior.
 * 
 * @return: Success/failure indicator.
 */
int d_SetString(dString_t* string, const char* content, int flags);

/**
 * @brief Create a new string builder with the same content as another string.
 *
 * @param source The string to copy content from.
 *
 * @return A new string builder with the same content as the source, or NULL on error.
 */
dString_t* d_CloneString(const dString_t* source);

/**
 * @brief Negatively Append, removing content from the end of the string.
 * 
 * @param sb The string builder to remove from.
 * @param count The number of characters to remove.
 */
void d_AppendToStringN(dString_t* sb, const char* str, size_t max_len);

/**
 * @brief Add a single character to the string builder.
 * 
 * @param sb The string builder to append to.
 * @param c The character to append.
 */
void d_AppendCharToString(dString_t* sb, char c);
/**
 * @brief Add an integer to the string builder as a decimal string.
 * 
 * @param sb The string builder to append to.
 * @param val The integer value to append.
 
 */
void d_AppendIntToString(dString_t* sb, int val);
/**
 * @brief Add a floating-point number to the string builder.
 * 
 * @param sb The string builder to append to.
 * @param val The floating-point value to append.
 * @param decimals The number of decimal places to show (0-10).
 * 
 * @return: Success/failure indicator.
 */
void d_AppendFloatToString(dString_t* sb, float val, int decimals);
/**
 * @brief Clear the string builder content.
 * 
 * @param sb The string builder to clear.
 *
 * @return: Success/failure indicator.
 */
void d_ClearString(dString_t* sb);
/*
 * Truncate the string builder to a specific length
 *
 * `sb` - Pointer to string builder
 * `len` - New length (must be <= current length)
 *
 * -- Does nothing if sb is NULL or len >= current length
 * -- Memory is not freed, only the length is reduced
 * -- The string remains null-terminated at the new length
 */
void d_TruncateString(dString_t* sb, size_t len);
/*
 * Remove characters from the beginning of the string builder
 *
 * `sb` - Pointer to string builder
 * `len` - Number of characters to remove from the beginning
 *
 * -- Does nothing if sb is NULL or len is 0
 * -- If len >= current length, the string builder is cleared completely
 * -- Uses memmove for safe overlapping memory operation
 */
void d_DropString(dString_t* sb, size_t len);
/*
 * Get the current length of the string builder content
 *
 * `sb` - Pointer to string builder
 *
 * `size_t` - Current length in characters, or 0 if sb is NULL
 *
 * -- Return value does not include the null terminator
 * -- Safe to call with NULL pointer (returns 0)
 */
size_t d_GetLengthOfString(const dString_t* sb);
/*
 * Get a read-only pointer to the string builder's content
 *
 * `sb` - Pointer to string builder
 *
 * `const char*` - Pointer to internal string, or NULL if sb is NULL
 *
 * -- Do not modify the returned string or free the pointer
 * -- The pointer becomes invalid after any modification to the string builder
 * -- The returned string is always null-terminated
 * -- Safe to call with NULL pointer (returns NULL)
 */
const char* d_PeekString(const dString_t* sb);
/*
 * Create a copy of the string builder's content
 *
 * `sb` - Pointer to string builder
 * `len` - Optional pointer to receive the length of the returned string
 *
 * `char*` - Newly allocated copy of the string, or NULL on error
 *
 * -- The caller is responsible for freeing the returned pointer
 * -- If len is not NULL, it receives the length of the string (excluding null terminator)
 * -- Returns NULL if sb is NULL or memory allocation fails
 * -- The returned string is always null-terminated
 */
char *d_DumpString(const dString_t* sb, size_t* len);
/*
 * Add formatted text to the string builder using printf-style formatting
 *
 * `sb` - Pointer to string builder
 * `format` - Printf-style format string
 * `...` - Variable arguments corresponding to format specifiers
 *
 * -- Does nothing if sb or format is NULL
 * -- Uses vsnprintf internally for safe formatting
 * -- Supports all standard printf format specifiers (%d, %s, %f, etc.)
 * -- Automatically calculates required space and grows buffer as needed
 * -- Appends formatted text to existing content (does not replace)
 */
void d_FormatString(dString_t* sb, const char* format, ...);
/*
 * Add text with template substitution to the string builder
 *
 * `sb` - Pointer to string builder
 * `tmplt` - Template string with placeholders in {key} format
 * `keys` - Array of key strings to match against placeholders
 * `values` - Array of replacement values corresponding to keys
 * `count` - Number of key-value pairs
 *
 * -- Does nothing if sb or tmplt is NULL
 * -- Placeholders must be in format {keyname} with no spaces
 * -- Keys are matched exactly (case-sensitive)
 * -- If a placeholder has no matching key, it is left unchanged
 * -- Keys longer than 255 characters are treated as literal text
 * -- Supports nested braces by treating unmatched { as literal characters
 */
void d_ApplyTemplateToString(dString_t* sb, const char* tmplt, const char** keys, const char** values, int count);
/*
 * Add an ASCII progress bar to the string builder
 *
 * `sb` - Pointer to string builder
 * `current` - Current progress value
 * `max` - Maximum progress value
 * `width` - Width of the progress bar (excluding brackets)
 * `fill_char` - Character to use for filled portions
 * `empty_char` - Character to use for empty portions
 *
 * -- Does nothing if sb is NULL, width <= 0, or max <= 0
 * -- Progress bar format: [████████----] where █ is fill_char and - is empty_char
 * -- If current > max, the bar is filled completely
 * -- If current < 0, the bar is empty
 * -- Total visual width is width + 2 (for brackets)
 */
void d_AppendProgressBarToString(dString_t* sb, int current, int max, int width, char fill_char, char empty_char);
/*
  * Add text padded to the center with specified character to reach target width
  *
  * `sb` - Pointer to string builder
  * `text` - Text to center (must be null-terminated)
  * `width` - Target total width including padding
  * `pad_char` - Character to use for padding
  *
  * -- Does nothing if sb or text is NULL
  * -- If text length >= width, adds text without padding
  * -- Text is centered with padding distributed evenly on both sides
  * -- If padding cannot be evenly distributed, left side gets one less character
  * -- Example: d_PadCenterString(sb, "Hi", 6, '.') produces "..Hi.."
  * -- Example: d_PadCenterString(sb, "Hi", 7, '.') produces "..Hi..."
  * -- Commonly used for centered headers and titles in ASCII interfaces
  */
void d_PadCenterString(dString_t* sb, const char* text, int width, char pad_char);
/*
  * Add text padded to the left with specified character to reach target width
  *
  * `sb` - Pointer to string builder
  * `text` - Text to pad (must be null-terminated)
  * `width` - Target total width including padding
  * `pad_char` - Character to use for padding
  *
  * -- Does nothing if sb or text is NULL
  * -- If text length >= width, adds text without padding
  * -- Padding is added to the left side of the text
  * -- Example: d_PadLeftString(sb, "Hi", 5, '.') produces "...Hi"
  * -- Commonly used for right-aligned text in tables and menus
  */
void d_PadLeftString(dString_t* sb, const char* text, int width, char pad_char);
/*
  * Add text padded to the right with specified character to reach target width

  * `sb` - Pointer to string builder
  * `text` - Text to pad (must be null-terminated)
  * `width` - Target total width including padding
  * `pad_char` - Character to use for padding
  *
  * -- Does nothing if sb or text is NULL
  * -- If text length >= width, adds text without padding
  * -- Padding is added to the right side of the text
  * -- Example: d_PadRightString(sb, "Hi", 5, '.') produces "Hi..."
  * -- Commonly used for left-aligned text in tables and menus
  */
void d_PadRightString(dString_t* sb, const char* text, int width, char pad_char);
/*
 * Add repeated characters to the string builder
 *
 * `sb` - Pointer to string builder
 * `character` - Character to repeat
 * `count` - Number of times to repeat the character
 *
 * -- Does nothing if sb is NULL or count <= 0
 * -- Efficiently adds multiple copies of the same character
 * -- Used internally by progress bar and padding functions
 */
void d_RepeatString(dString_t* sb, char character, int count);

// Pythonic String Utils
/*
  * Join an array of strings with a separator (like Python's str.join())
  *
  * `sb` - Pointer to string builder
  * `strings` - Array of string pointers to join
  * `count` - Number of strings in the array
  * `separator` - String to insert between each element
  *
  * -- Does nothing if sb is NULL or count <= 0
  * -- If strings array is NULL, does nothing
  * -- NULL strings in the array are treated as empty strings
  * -- If separator is NULL, strings are joined without separation
  * -- Example: d_JoinStrings(sb, {"a", "b", "c"}, 3, ", ") produces "a, b, c"
  * -- Commonly used for creating comma-separated lists, file paths, etc.
  */
void d_JoinStrings(dString_t* sb, const char** strings, int count, const char* separator);
/*
  * Extract a substring using Python-style slice notation (like str[start:end])
  *
  * `sb` - Pointer to string builder
  * `text` - Source string to slice (must be null-terminated)
  * `start` - Starting index (inclusive, negative values count from end)
  * `end` - Ending index (exclusive, negative values count from end, -1 means end of string)
  *
  * -- Does nothing if sb or text is NULL
  * -- Negative indices count from the end: -1 is last character, -2 is second-to-last, etc.
  * -- If start >= end (after resolving negative indices), no text is added
  * -- Indices are clamped to valid range [0, string_length]
  * -- Example: d_SliceString(sb, "Hello", 1, 4) produces "ell"
  * -- Example: d_SliceString(sb, "Hello", -3, -1) produces "llo"
  * -- Example: d_SliceString(sb, "Hello", 0, -1) produces "Hello"
  */
void d_SliceString(dString_t* sb, const char* text, int start, int end);

/*
 * Compare two dString_t objects lexicographically
 *
 * `str1` - The first Daedalus string object
 * `str2` - The second Daedalus string object
 *
 * `int` - An integer less than, equal to, or greater than zero if str1 is found,
 * respectively, to be less than, to match, or be greater than str2
 *
 * -- If both are the same kind of invalid, consider them equal
 * -- If one is invalid and the other is not, they are not equal
 * -- Invalid str1 is considered "less than" valid str2
 * -- Valid str1 is considered "greater than" invalid str2
 * -- Uses strcmp() for lexicographical comparison of valid strings
 */
int d_CompareStrings(const dString_t* str1, const dString_t* str2);

/*
 * Compare a dString_t with a standard C-string
 *
 * `d_str` - The Daedalus string object to compare
 * `c_str` - The standard null-terminated C-string to compare against
 *
 * `int` - An integer less than, equal to, or greater than zero if d_str is found,
 * respectively, to be less than, to match, or be greater than c_str
 *
 * -- An invalid dString and a NULL/empty C-string are considered "equal"
 * -- Invalid d_str is considered "less than" a valid c_str
 * -- Valid d_str is considered "greater than" a NULL c_str
 * -- Uses strcmp() for lexicographical comparison when both are valid
 * -- Avoids need to create temporary dString_t for comparison
 * -- Critical for performance when interfacing with external C-strings
 */
int d_CompareStringToCString(const dString_t* d_str, const char* c_str);


/* Dynamic Arrays */
/*
 * Create a new dynamic array with specified capacity and element size
 *
 * `capacity` - Initial number of elements the array can hold
 * `element_size` - Size in bytes of each element (e.g., sizeof(int))
 *
 * `dArray_t*` - Pointer to new dynamic array, or NULL on allocation failure
 *
 * -- Must be destroyed with d_DestroyArray() to free memory
 * -- Initial count is 0 even though capacity may be larger
 * -- Elements can be any type as long as element_size is correct
 * -- Capacity of 0 is allowed but array cannot store elements until resized
 */

 /* Logging System */

 // =============================================================================
 // CORE LOGGING FUNCTIONS
 // =============================================================================

 /*
  * Log a message at the specified level
  *
  * `level` - Log level (DEBUG, INFO, WARNING, ERROR, FATAL)
  * `message` - Message string to log
  *
  * -- Does nothing if global logger is not initialized
  * -- Message is filtered based on current log level and file filters
  * -- Automatically adds timestamp and other metadata based on configuration
  */
 void d_Log(dLogLevel_t level, const char* message);

 /*
  * Log a formatted message at the specified level (printf-style)
  *
  * `level` - Log level (DEBUG, INFO, WARNING, ERROR, FATAL)
  * `format` - Printf-style format string
  * `...` - Variable arguments corresponding to format specifiers
  *
  * -- Does nothing if global logger is not initialized or format is NULL
  * -- Uses thread-local buffer to avoid allocations
  * -- Supports all standard printf format specifiers
  */
 void d_LogF(dLogLevel_t level, const char* format, ...);

 /*
  * Log with explicit file/line/function information
  *
  * `level` - Log level
  * `file` - Source file path
  * `line` - Line number
  * `func` - Function name
  * `message` - Message to log
  *
  * -- Internal function typically called through macros
  * -- Provides full source location context
  */
 void d_LogEx(dLogLevel_t level, const char* file, int line, const char* func, const char* message);

 /*
  * Log with explicit file/line/function and formatting
  *
  * `level` - Log level
  * `file` - Source file path
  * `line` - Line number
  * `func` - Function name
  * `format` - Printf-style format string
  * `...` - Variable arguments
  *
  * -- Internal function typically called through macros
  * -- Combines source location with formatted output
  */
 void d_LogExF(dLogLevel_t level, const char* file, int line, const char* func, const char* format, ...);

 // =============================================================================
 // CONVENIENCE LOGGING FUNCTIONS
 // =============================================================================

 /*
  * Log a debug message
  *
  * `message` - Message to log
  *
  * -- Convenience wrapper for d_Log(D_LOG_LEVEL_DEBUG, message)
  * -- Typically compiled out in release builds
  */
 void d_LogDebug(const char* message);

 /*
  * Log an info message
  *
  * `message` - Message to log
  *
  * -- Convenience wrapper for d_Log(D_LOG_LEVEL_INFO, message)
  * -- Standard informational logging
  */
 void d_LogInfo(const char* message);

 /*
  * Log a warning message
  *
  * `message` - Message to log
  *
  * -- Convenience wrapper for d_Log(D_LOG_LEVEL_WARNING, message)
  * -- For potentially problematic situations
  */
 void d_LogWarning(const char* message);

 /*
  * Log an error message
  *
  * `message` - Message to log
  *
  * -- Convenience wrapper for d_Log(D_LOG_LEVEL_ERROR, message)
  * -- For recoverable error conditions
  */
 void d_LogError(const char* message);

 /*
  * Log a fatal message
  *
  * `message` - Message to log
  *
  * -- Convenience wrapper for d_Log(D_LOG_LEVEL_FATAL, message)
  * -- For unrecoverable errors
  * -- May trigger application shutdown depending on configuration
  */
 void d_LogFatal(const char* message);

 // Formatted versions
 void d_LogDebugF(const char* format, ...);
 void d_LogInfoF(const char* format, ...);
 void d_LogWarningF(const char* format, ...);
 void d_LogErrorF(const char* format, ...);
 void d_LogFatalF(const char* format, ...);

 // =============================================================================
 // LOGGER MANAGEMENT
 // =============================================================================

 /*
  * Create a new logger with the specified configuration
  *
  * `config` - Logger configuration settings
  *
  * `dLogger_t*` - Pointer to new logger, or NULL on allocation failure
  *
  * -- Must be destroyed with d_DestroyLogger() to free memory
  * -- Can be set as global logger with d_SetGlobalLogger()
  * -- Multiple loggers can exist simultaneously for different subsystems
  */
 dLogger_t* d_CreateLogger(dLogConfig_t config);

 /*
  * Destroy a logger and free all associated resources
  *
  * `logger` - Pointer to logger to destroy
  *
  * -- Safe to call with NULL pointer
  * -- If this is the global logger, global logging is disabled
  * -- All handlers are removed and their resources freed
  */
 void d_DestroyLogger(dLogger_t* logger);

 /*
  * Set a logger as the global default logger
  *
  * `logger` - Logger to set as global (can be NULL to disable)
  *
  * -- Previous global logger is not destroyed (caller's responsibility)
  * -- NULL disables global logging
  * -- All d_Log* functions use the global logger
  */
 void d_SetGlobalLogger(dLogger_t* logger);

 /*
  * Get the current global logger
  *
  * `dLogger_t*` - Pointer to global logger, or NULL if none set
  *
  * -- Do not destroy the returned logger without unsetting it first
  * -- Can be used to temporarily modify logger configuration
  */
 dLogger_t* d_GetGlobalLogger(void);

 // =============================================================================
 // CONFIGURATION AND FILTERING
 // =============================================================================

 /*
  * Create a new filter configuration builder
  *
  * `dLogFilterBuilder_t*` - New filter builder, or NULL on error
  *
  * -- Must be destroyed with d_DestroyFilterBuilder()
  * -- Use to build complex filtering rules before applying
  */
 dLogFilterBuilder_t* d_CreateFilterBuilder(void);

 /*
  * Add a directory-based filter rule
  *
  * `builder` - Filter builder
  * `path` - Directory path to match (e.g., "src/items/")
  * `level` - Minimum log level for this directory
  *
  * -- Path should use forward slashes on all platforms
  * -- Trailing slash determines if rule is recursive
  * -- Higher priority than previously added rules
  */
 void d_FilterBuilder_AddDirectory(dLogFilterBuilder_t* builder, const char* path, dLogLevel_t level);

 /*
  * Add a file prefix filter rule
  *
  * `builder` - Filter builder
  * `prefix` - Filename prefix to match (e.g., "test_")
  * `level` - Minimum log level for matching files
  *
  * -- Matches beginning of filename only, not full path
  * -- Case-sensitive matching
  */
 void d_FilterBuilder_AddPrefix(dLogFilterBuilder_t* builder, const char* prefix, dLogLevel_t level);

 /*
  * Add a file suffix filter rule
  *
  * `builder` - Filter builder
  * `suffix` - Filename suffix to match (e.g., "Math.c")
  * `level` - Minimum log level for matching files
  *
  * -- Matches end of filename including extension
  * -- Period in suffix is matched literally
  */
 void d_FilterBuilder_AddSuffix(dLogFilterBuilder_t* builder, const char* suffix, dLogLevel_t level);

 /*
  * Apply filter configuration to a logger
  *
  * `logger` - Logger to configure (NULL for global logger)
  * `builder` - Filter builder with rules
  *
  * -- Replaces any existing filter configuration
  * -- Builder can be reused after applying
  * -- Pre-computes hashes and optimizes for fast runtime checks
  */
 void d_FilterBuilder_Apply(dLogger_t* logger, dLogFilterBuilder_t* builder);

 /*
  * Load filter configuration from a string
  *
  * `logger` - Logger to configure (NULL for global logger)
  * `config_str` - Configuration string in DSL format
  *
  * `int` - 0 on success, non-zero on parse error
  *
  * -- Format: "path/pattern: LEVEL" separated by semicolons or newlines
  * -- Example: "src/all: INFO; test_*: OFF; *Math.c: WARNING"
  * -- Comments start with # and continue to end of line
  */
 int d_FilterBuilder_FromString(dLogger_t* logger, const char* config_str);

 /*
  * Destroy a filter builder and free resources
  *
  * `builder` - Filter builder to destroy
  *
  * -- Safe to call with NULL pointer
  * -- Does not affect already-applied configurations
  */
 void d_DestroyFilterBuilder(dLogFilterBuilder_t* builder);

 /*
  * Set the minimum log level for a logger
  *
  * `logger` - Logger to configure (NULL for global logger)
  * `level` - Minimum level to log
  *
  * -- Messages below this level are discarded
  * -- File-specific filters can override to allow lower levels
  */
 void d_SetLogLevel(dLogger_t* logger, dLogLevel_t level);

 /*
  * Get the current log level for a logger
  *
  * `logger` - Logger to get level from (NULL for global logger)
  *
  * Returns: Current minimum log level for the logger
  *
  * -- Returns the effective log level that determines message filtering
  * -- File-specific filters may override this for specific sources
  */
 dLogLevel_t d_GetLogLevel(dLogger_t* logger);

 /*
  * Enable or disable logging globally
  *
  * `enabled` - true to enable, false to disable
  *
  * -- When disabled, all logging compiles to near-zero overhead
  * -- Affects all loggers, not just global logger
  * -- Useful for performance-critical sections
  */
 void d_SetLoggingEnabled(bool enabled);

 /*
  * Check if logging is globally enabled
  *
  * Returns: true if logging is enabled, false if disabled
  *
  * -- Returns the current global logging state
  * -- When false, all logging operations are no-ops
  */
 bool d_IsLoggingEnabled(void);

 // =============================================================================
 // LOG HANDLERS
 // =============================================================================

 /*
  * Add a log handler to a logger
  *
  * `logger` - Logger to add handler to (NULL for global logger)
  * `handler` - Handler function to call for each log entry
  * `user_data` - Arbitrary data passed to handler
  *
  * -- Multiple handlers can be added to a single logger
  * -- Handlers are called in order of registration
  * -- user_data can be NULL if handler doesn't need it
  */
 void d_AddLogHandler(dLogger_t* logger, dLogHandler_t handler, void* user_data);

 /*
  * Remove a log handler from a logger
  *
  * `logger` - Logger to remove handler from (NULL for global logger)
  * `handler` - Handler function to remove
  *
  * -- Removes first matching handler
  * -- Does nothing if handler not found
  */
 void d_RemoveLogHandler(dLogger_t* logger, dLogHandler_t handler);

 /*
  * Built-in console log handler
  *
  * `entry` - Log entry to output
  * `user_data` - Unused (pass NULL)
  *
  * -- Outputs to stdout/stderr based on level
  * -- Supports ANSI color codes if configured
  * -- Thread-safe output
  */
 void d_ConsoleLogHandler(const dLogEntry_t* entry, void* user_data);

 /*
  * Built-in file log handler
  *
  * `entry` - Log entry to output
  * `user_data` - FILE* to write to
  *
  * -- Writes formatted log entries to file
  * -- user_data must be valid FILE* opened for writing
  * -- Caller responsible for opening/closing file
  */
 void d_FileLogHandler(const dLogEntry_t* entry, void* user_data);

 /*
  * Built-in string builder log handler
  *
  * `entry` - Log entry to output
  * `user_data` - dString_t* to append to
  *
  * -- Appends formatted entries to string builder
  * -- Useful for capturing logs in tests
  * -- user_data must be valid dString_t*
  */
 void d_StringLogHandler(const dLogEntry_t* entry, void* user_data);

 // =============================================================================
 // LOG BUILDER PATTERN
 // =============================================================================

 /*
  * Begin building a log message
  *
  * `level` - Log level for the message
  *
  * `dLogBuilder_t*` - Log builder instance, or NULL on error
  *
  * -- Must call d_LogBuilder_End() or d_LogBuilder_Commit() to send
  * -- Reuses thread-local buffers for zero allocation
  * -- Can chain multiple append operations
  */
 dLogBuilder_t* d_LogBegin(dLogLevel_t level);

 /*
  * Append a string to the log builder
  *
  * `builder` - Log builder instance
  * `text` - Text to append
  *
  * `dLogBuilder_t*` - Same builder for chaining
  *
  * -- Returns NULL if builder is NULL
  * -- Can chain: d_LogBegin(INFO)->Append("Hello")->Append(" World")->End()
  */
 dLogBuilder_t* d_LogBuilder_Append(dLogBuilder_t* builder, const char* text);

 /*
  * Append an integer to the log builder
  *
  * `builder` - Log builder instance
  * `value` - Integer value to append
  *
  * `dLogBuilder_t*` - Same builder for chaining
  */
 dLogBuilder_t* d_LogBuilder_AppendInt(dLogBuilder_t* builder, int value);

 /*
  * Append a float to the log builder
  *
  * `builder` - Log builder instance
  * `value` - Float value to append
  * `decimals` - Number of decimal places (0-10)
  *
  * `dLogBuilder_t*` - Same builder for chaining
  */
 dLogBuilder_t* d_LogBuilder_AppendFloat(dLogBuilder_t* builder, float value, int decimals);

 /*
  * Append formatted text to the log builder
  *
  * `builder` - Log builder instance
  * `format` - Printf-style format string
  * `...` - Variable arguments
  *
  * `dLogBuilder_t*` - Same builder for chaining
  */
 dLogBuilder_t* d_LogBuilder_AppendF(dLogBuilder_t* builder, const char* format, ...);

 /*
  * Finish building and send the log message
  *
  * `builder` - Log builder instance
  *
  * -- Sends the built message through normal logging pipeline
  * -- Builder is automatically cleaned up after this call
  * -- Do not use builder after calling End()
  */
 void d_LogBuilder_End(dLogBuilder_t* builder);

 /*
  * Alias for d_LogBuilder_End() for clearer intent
  */
 void d_LogBuilder_Commit(dLogBuilder_t* builder);

 // =============================================================================
 // STRUCTURED LOGGING
 // =============================================================================

 /*
  * Begin building a structured log entry
  *
  * `level` - Log level for the message
  *
  * `dLogStructured_t*` - Structured log builder, or NULL on error
  *
  * -- Must call d_LogStructured_Commit() to send
  * -- Outputs in key=value or JSON format
  * -- Ideal for machine-parseable logs
  */
 dLogStructured_t* d_LogStructured(dLogLevel_t level);

 /*
  * Add a string field to structured log
  *
  * `builder` - Structured log builder
  * `key` - Field name
  * `value` - Field value
  *
  * `dLogStructured_t*` - Same builder for chaining
  */
 dLogStructured_t* d_LogStructured_Field(dLogStructured_t* builder, const char* key, const char* value);

 /*
  * Add an integer field to structured log
  *
  * `builder` - Structured log builder
  * `key` - Field name
  * `value` - Integer value
  *
  * `dLogStructured_t*` - Same builder for chaining
  */
 dLogStructured_t* d_LogStructured_FieldInt(dLogStructured_t* builder, const char* key, int value);

 /*
  * Add a float field to structured log
  *
  * `builder` - Structured log builder
  * `key` - Field name
  * `value` - Float value
  * `decimals` - Decimal places (0-10)
  *
  * `dLogStructured_t*` - Same builder for chaining
  */
 dLogStructured_t* d_LogStructured_FieldFloat(dLogStructured_t* builder, const char* key, float value, int decimals);

 /*
  * Set structured log output format
  *
  * `builder` - Structured log builder
  * `json` - true for JSON format, false for key=value format
  *
  * `dLogStructured_t*` - Same builder for chaining
  */
 dLogStructured_t* d_LogStructured_SetFormat(dLogStructured_t* builder, bool json);

 /*
  * Finish and send the structured log
  *
  * `builder` - Structured log builder
  *
  * -- Formats and sends the structured data
  * -- Builder is cleaned up after this call
  */
 void d_LogStructured_Commit(dLogStructured_t* builder);

 /*
  * Add a boolean field to structured log
  *
  * `builder` - Structured log builder
  * `key` - Field name
  * `value` - Boolean value
  *
  * `dLogStructured_t*` - Same builder for chaining
  */
 dLogStructured_t* d_LogStructured_FieldBool(dLogStructured_t* builder, const char* key, bool value);

 /*
  * Add a timestamp field to structured log
  *
  * `builder` - Structured log builder
  * `key` - Field name
  *
  * `dLogStructured_t*` - Same builder for chaining
  *
  * -- Adds current timestamp in ISO 8601 format
  * -- Timestamp is captured when this function is called
  */
 dLogStructured_t* d_LogStructured_FieldTimestamp(dLogStructured_t* builder, const char* key);

 /*
  * Clone an existing structured log for reuse
  *
  * `source` - Source structured log to clone
  *
  * `dLogStructured_t*` - New structured log with copied fields, or NULL on error
  *
  * -- Creates a new structured log with all fields from source
  * -- Useful for creating logs with shared context
  * -- Source log remains unchanged and usable
  */
 dLogStructured_t* d_LogStructured_Clone(dLogStructured_t* source);

 // =============================================================================
 // LOG CONTEXTS
 // =============================================================================

 /*
  * Push a new logging context onto the stack
  *
  * `name` - Context name (e.g., "Physics", "Renderer")
  *
  * `dLogContext_t*` - Context handle, or NULL on error
  *
  * -- All subsequent logs include this context
  * -- Contexts can be nested
  * -- Must call d_PopLogContext() to remove
  */
 dLogContext_t* d_PushLogContext(const char* name);

 /*
  * Pop a logging context from the stack
  *
  * `context` - Context handle from d_PushLogContext()
  *
  * -- Removes the context from subsequent logs
  * -- Safe to call with NULL
  * -- Contexts must be popped in LIFO order
  */
 void d_PopLogContext(dLogContext_t* context);

 /*
  * Log the execution time of a context when popped
  *
  * `context` - Context to enable timing for
  *
  * -- When context is popped, logs execution time
  * -- Useful for performance profiling
  */
 void d_LogContext_EnableTiming(dLogContext_t* context);

 // =============================================================================
 // CONDITIONAL AND RATE-LIMITED LOGGING
 // =============================================================================
 // Rate limiting behavior flags
 typedef enum {
     // Hashes the final, fully-rendered message. Prone to issues with formatted strings.
     D_LOG_RATE_LIMIT_FLAG_HASH_FINAL_MESSAGE = 0,
     // Hashes the format string itself. Correctly limits repeated formatted calls.
     D_LOG_RATE_LIMIT_FLAG_HASH_FORMAT_STRING = 1
 } dLogRateLimitFlag_t;
 /*
  * Log only if condition is true
  *
  * `condition` - Boolean condition to check
  * `level` - Log level if condition is true
  * `message` - Message to log
  *
  * -- Zero overhead when condition is false
  * -- Useful for debug assertions and conditional warnings
  */
 void d_LogIf(bool condition, dLogLevel_t level, const char* message);

 /*
  * Log with printf formatting only if condition is true
  *
  * `condition` - Boolean condition to check
  * `level` - Log level if condition is true
  * `format` - Printf-style format string
  * `...` - Variable arguments
  */
 void d_LogIfF(bool condition, dLogLevel_t level, const char* format, ...);

 /*
  * Log with rate limiting to prevent spam
  *
  * `level` - Log level
  * `max_count` - Maximum logs allowed in time window
  * `time_window` - Time window in seconds
  * `message` - Message to log
  *
  * -- Prevents identical messages from flooding logs
  * -- Uses message hash for comparison
  * -- Resets counter after time window expires
  */
 void d_LogRateLimited(dLogLevel_t level, uint32_t max_count, double time_window, const char* message);

 /*
  * Rate-limited logging with formatting
  *
  * `flag` - Determines what to hash for rate limiting (the format string or the final message)
  * `level` - Log level
  * `max_count` - Maximum logs allowed in time window
  * `time_window` - Time window in seconds
  * `format` - Printf-style format string
  * `...` - Variable arguments
  */
 void d_LogRateLimitedF(dLogRateLimitFlag_t flag, dLogLevel_t level, uint32_t max_count, double time_window, const char* format, ...);


 // =============================================================================
 // UTILITY FUNCTIONS
 // =============================================================================

 /*
  * Get string representation of log level
  *
  * `level` - Log level enum value
  *
  * `const char*` - String like "DEBUG", "INFO", etc.
  *
  * -- Returns "UNKNOWN" for invalid levels
  * -- Useful for custom handlers
  */
 const char* d_LogLevel_ToString(dLogLevel_t level);

 /*
  * Parse log level from string
  *
  * `str` - String like "DEBUG", "INFO", etc.
  *
  * `dLogLevel_t` - Parsed level, or D_LOG_LEVEL_INFO on error
  *
  * -- Case-insensitive parsing
  * -- Accepts full names or single letters (D, I, W, E, F)
  */
 dLogLevel_t d_LogLevel_FromString(const char* str);

 /*
  * Get ANSI color code for log level
  *
  * `level` - Log level
  *
  * `const char*` - ANSI escape sequence, or empty string if colors disabled
  *
  * -- Returns escape codes like "\033[31m" for red
  * -- Used by console handler for colored output
  */
 const char* d_LogLevel_GetColor(dLogLevel_t level);

 /*
  * Get current timestamp as Unix time
  *
  * `double` - Current time in seconds since epoch
  *
  * -- High precision timestamp for log entries
  * -- Platform-independent implementation
  */
 double d_GetTimestamp(void);

 /*
  * Format timestamp into string
  *
  * `buffer` - Output buffer
  * `buffer_size` - Size of buffer
  * `timestamp` - Unix timestamp
  * `format` - strftime format string (NULL for ISO-8601)
  *
  * -- Safe string formatting with bounds checking
  * -- Default format: "YYYY-MM-DD HH:MM:SS.mmm"
  */
 void d_FormatTimestamp(char* buffer, size_t buffer_size, double timestamp, const char* format);

 /*
  * Get logging statistics
  *
  * `logger` - Logger to get stats from (NULL for global)
  * `stats` - Output structure to fill
  *
  * -- Provides counts of logs by level, suppressed logs, etc.
  * -- Useful for monitoring and debugging
  */
 void d_GetLogStats(dLogger_t* logger, dLogStats_t* stats);

 /*
  * Reset logging statistics
  *
  * `logger` - Logger to reset stats for (NULL for global)
  *
  * -- Clears all counters back to zero
  * -- Does not affect configuration
  */
 void d_ResetLogStats(dLogger_t* logger);

 /*
  * Resets the global rate limiter cache, clearing all tracked messages.
  *
  * -- FOR TESTING AND DEBUGGING USE ONLY.
  * -- This function is not thread-safe and should be called when no other
  * threads are performing logging.
  * -- It clears all history, allowing rate-limited messages to be logged again.
  */
 void d_ResetRateLimiterCache(void);

 /*
  * Gets the current number of unique messages being tracked by the rate limiter.
  *
  * `size_t` - The number of entries in the rate limiter's cache.
  *
  * -- FOR TESTING AND DEBUGGING USE ONLY.
  * -- Can be used to verify that the cache is growing as expected.
  */
 size_t d_GetRateLimiterCacheEntryCount(void);
 // =============================================================================
 // ZERO-OVERHEAD MACROS
 // =============================================================================

 // These macros provide zero overhead when logging is disabled at compile time
 // They automatically capture file, line, and function information

 #ifndef D_LOG_CURRENT_FILE_ID
     #define D_LOG_CURRENT_FILE_ID (__COUNTER__)
 #endif

 #ifdef D_LOGGING_ENABLED
     #define D_LOG(level, msg) \
         do { \
             extern dLogGlobalConfig_t g_log_config; \
             if ((level) >= g_log_config.file_log_levels[D_LOG_CURRENT_FILE_ID]) { \
                 d_LogEx(level, __FILE__, __LINE__, __func__, msg); \
             } \
         } while(0)

     #define D_LOGF(level, fmt, ...) \
         do { \
             extern dLogGlobalConfig_t g_log_config; \
             if ((level) >= g_log_config.file_log_levels[D_LOG_CURRENT_FILE_ID]) { \
                 d_LogExF(level, __FILE__, __LINE__, __func__, fmt, __VA_ARGS__); \
             } \
         } while(0)
 #else
     #define D_LOG(level, msg) ((void)0)
     #define D_LOGF(level, fmt, ...) ((void)0)
 #endif

 // Convenience macros for common log levels
 #define D_LOG_DEBUG(msg) D_LOG(D_LOG_LEVEL_DEBUG, msg)
 #define D_LOG_INFO(msg) D_LOG(D_LOG_LEVEL_INFO, msg)
 #define D_LOG_WARNING(msg) D_LOG(D_LOG_LEVEL_WARNING, msg)
 #define D_LOG_ERROR(msg) D_LOG(D_LOG_LEVEL_ERROR, msg)
 #define D_LOG_FATAL(msg) D_LOG(D_LOG_LEVEL_FATAL, msg)

 #define D_LOG_DEBUGF(fmt, ...) D_LOGF(D_LOG_LEVEL_DEBUG, fmt, __VA_ARGS__)
 #define D_LOG_INFOF(fmt, ...) D_LOGF(D_LOG_LEVEL_INFO, fmt, __VA_ARGS__)
 #define D_LOG_WARNINGF(fmt, ...) D_LOGF(D_LOG_LEVEL_WARNING, fmt, __VA_ARGS__)
 #define D_LOG_ERRORF(fmt, ...) D_LOGF(D_LOG_LEVEL_ERROR, fmt, __VA_ARGS__)
 #define D_LOG_FATALF(fmt, ...) D_LOGF(D_LOG_LEVEL_FATAL, fmt, __VA_ARGS__)

 // Builder pattern macro
 #define D_LOG_BUILD(level) d_LogBegin(level)

 // Timed block macro
 #define D_LOG_TIMED_BLOCK(name) \
     for (dLogContext_t* _ctx = d_PushLogContext(name); _ctx; d_PopLogContext(_ctx), _ctx = NULL) \
         for (d_LogContext_EnableTiming(_ctx); _ctx; _ctx = NULL)

/**
 * @brief Initialize a Dynamic Array.
 * 
 * @param capacity The initial capacity of the array in elements.
 * @param element_size The size of each element in bytes.
 * 
 * @return A pointer to the new array, or NULL on error.
 * 
 * -- Must be destroyed with d_DestroyArray() to free memory
 * -- Initial count is 0 even though capacity may be larger
 * -- Elements can be any type as long as element_size is correct
 * -- Capacity of 0 is allowed but array cannot store elements until resized
 * 
 * Example: `dArray_t* array = d_InitArray(10, sizeof(int));`
 * This creates a new array with a capacity of 10 elements, each of size 4 bytes.
 */
dArray_t* d_InitArray( size_t capacity, size_t element_size );

/**
 * @brief Destroy a dynamic array.
 * 
 * @param array The array to destroy.
 * 
 * @return: 0 on success, 1 on failure.
 * 
 * -- Frees both the data buffer and the array structure itself
 * -- After calling this function, the pointer is invalid and should not be used
 * 
 * Example: `d_DestroyArray(array);`
 * This destroys the dynamic array and frees its memory.
 */
int d_DestroyArray( dArray_t* array );

/**
 * @brief Resize the internal data buffer of a dynamic array.
 *
 * This function adjusts the allocated memory for the array's elements.
 *
 * @param array A pointer to the dynamic array whose internal buffer is to be resized.
 * @param new_size_in_bytes The desired new total size of the internal data buffer in bytes.
 * This will determine the new capacity in elements.
 *
 * @return 0 on success, 1 on failure.
 *
 * @note If `new_size_in_bytes` is 0, the internal data buffer will be freed, and the array's
 * capacity and count will be reset to 0.
 * @note If `new_size_in_bytes` is larger than the current allocated size, the data buffer
 * is reallocated, potentially moving to a new memory location. Existing data (up to
 * the old capacity) is preserved.
 * @note If `new_size_in_bytes` is smaller than the current allocated size, the data buffer
 * is truncated. If the current `count` of elements exceeds the new capacity, `count`
 * will be adjusted down to match the new capacity, effectively truncating the array's
 * contents.
 * @warning This function only affects the internal data buffer (`array->data`). The `dArray_t* array`
 * pointer itself remains valid and points to the same `dArray_t` structure.
 * Pointers obtained previously via `d_IndexDataFromArray` (or similar direct access)
 * will become invalid if the underlying `array->data` buffer is reallocated and moved.
 *
 * Example: `d_ResizeArray(myArray, 10 * sizeof(int));`
 * This resizes the internal buffer of `myArray` to accommodate 10 integer elements.
 * If `myArray` previously held more than 10 elements, its `count` will be truncated.
 */
int d_ResizeArray( dArray_t* array, size_t new_capacity );

/**
 * @brief Grow the array by a number of additional bytes.
 * 
 * @param array The array to grow.
 * @param additional_bytes The number of bytes to add.
 * 
 * @return 0 on success, 1 on failure.
 * 
 * -- Convenience function that calls d_ResizeArray() internally
 * 
 * Example: `d_GrowArray(array, 10 * sizeof(int));`
 * This grows the dynamic array by 10 elements, each of size 4 bytes.
 */
int d_GrowArray( dArray_t* array, size_t additional_capacity );

/**
 * @brief Append an element to the end of the dynamic array.
 *
 * This function adds a new element to the array. If the array's current capacity
 * is insufficient, it will automatically grow to accommodate the new element,
 * typically by doubling its capacity.
 *
 * @param array A pointer to the dynamic array to append to.
 * @param data A pointer to the element data to copy into the array.
 * The data pointed to will be copied by `element_size` bytes.
 *
 * @return 0 on success, 1 on failure.
 *
 * @note Fails if `array` or `data` is NULL.
 * @note If the array's capacity is reached, it will attempt to grow its
 * internal buffer. If reallocation fails, the append operation will fail.
 * @note Copies `element_size` bytes from the `data` pointer into the array.
 * @note Increments the array's `count` on successful append.
 *
 * Example: `int my_value = 123; int result = d_AppendDataToArray(myArray, &my_value);`
 * This appends an integer value to the end of `myArray`.
 */
int d_AppendDataToArray( dArray_t* array, void* data );

/**
 * @brief Get a pointer to the data at a specific index.
 * 
 * @param array The array to get data from.
 * @param index The index of the element to get.
 * 
 * @return A pointer to the element data, or NULL if index is out of bounds.
 * 
 * -- Returns NULL if array is NULL or index >= count
 * -- Returned pointer is valid until the array is modified or destroyed
 * -- Caller can read/write through the returned pointer safely
 * -- Use appropriate casting: int* ptr = (int*)d_IndexDataFromArray(array, 0)
 * -- Index must be less than count, not capacity (only counts appended elements)
 * 
 * Example: `void* data = d_IndexDataFromArray(array, 0);`
 * This retrieves the first element from the dynamic array.
 */
void* d_IndexDataFromArray(dArray_t* array, size_t index);

/**
 * @brief Remove and return the last element from the array.
 * 
 * @param array The array to pop from.
 * 
 * @return A pointer to the last element's data, or NULL if array is empty.
 * 
 * -- Decrements count but does not free memory (element data remains in buffer)
 * -- Returned pointer becomes invalid after next append or array modification
 * -- Implements stack-like behavior for dynamic arrays
 * -- Memory is not reallocated, only the count is decremented for efficiency
 * 
 * Example: `void* data = d_PopDataFromArray(array);`
 * This removes and returns the last element from the dynamic array.
 */
void* d_PopDataFromArray(dArray_t* array);

/**
 * @brief Insert data at a specific index in the array
 * 
 * @param array The array to insert into
 * @param data Pointer to the data to insert
 * @param index The index where to insert the data
 * 
 * @return 0 on success, 1 on failure
 * 
 * -- Shifts existing elements to the right to make space
 * -- Grows the array capacity if needed
 * -- index must be <= array->count (can insert at end)
 * -- Uses memmove for safe overlapping memory operations
 * 
 * Example: `d_InsertDataIntoArray(array, &value, 2);`
 * This inserts a value at index 2, shifting existing elements to the right.
 */
int d_InsertDataIntoArray(dArray_t* array, void* data, size_t index);

/**
 * @brief Remove data at a specific index from the array
 *
 * @param array The array to remove from
 * @param index The index of the element to remove
 *
 * @return 0 on success, 1 on failure
 *
 * -- Shifts existing elements to the left to fill the gap
 * -- Does not resize the array capacity (use d_TrimCapacityOfArray for that)
 * -- index must be < array->count
 * -- Uses memmove for safe overlapping memory operations
 *
 * Example: `d_RemoveDataFromArray(array, 2);`
 * This removes the element at index 2, shifting remaining elements to the left.
 */
int d_RemoveDataFromArray(dArray_t* array, size_t index);

/**
 * @brief Clear all elements from the array without deallocating memory
 *
 * @param array The array to clear
 *
 * @return 0 on success, 1 on failure
 *
 * -- Sets count to 0, preserving capacity for efficient reuse
 * -- O(1) operation - no memory operations performed
 * -- Does not zero memory or shrink capacity
 * -- Ideal for clearing collections that will be reused (hands, queues, temp buffers)
 *
 * Example: `d_ClearArray(array);`
 * This clears all elements from the array, resetting count to 0 while keeping capacity.
 */
int d_ClearArray(dArray_t* array);

/**
 * @brief Optimize memory usage by shrinking the array's capacity to match its count
 *
 * @param array The array to trim
 *
 * @return 0 on success, 1 on failure
 *
 * -- Reduces allocated memory to exactly match the number of elements
 * -- If array is empty, frees the data buffer
 * -- Does nothing if array is already optimally sized
 * -- Useful after bulk removal operations to reclaim memory
 *
 * Example: `d_TrimCapacityOfArray(array);`
 * This trims the array's capacity to match its count, freeing memory if necessary.
 */
int d_TrimCapacityOfArray(dArray_t* array);

/**
 * @brief Ensure the array has enough capacity for at least min_capacity elements
 *
 * @param array The array to ensure capacity for
 * @param min_capacity Minimum number of elements the array should accommodate
 *
 * @return 0 on success, 1 on failure
 *
 * -- Grows the array if current capacity is less than min_capacity
 * -- Uses exponential growth strategy to minimize reallocations
 * -- Never shrinks the array - use d_TrimCapacityOfArray for that
 * -- Useful for pre-allocating space before bulk operations
 *
 * Example: `d_EnsureCapacityOfArray(array, 100);`
 * This ensures the array has at least 100 elements allocated, growing the array if needed.
 */
int d_EnsureCapacityOfArray(dArray_t* array, size_t min_capacity);


/* --- Static Arrays --- */


/**
 * @brief Initialize a new static array with fixed capacity
 *
 * @param capacity: Maximum number of elements the array can hold
 * @param element_size: Size of each element in bytes
 * 
 * @return: Pointer to new static array, or NULL on allocation failure
 *
 * -- Allocates memory for both the structure and the data buffer
 * -- Data buffer size = capacity * element_size
 * -- count is initialized to 0
 * -- capacity and element_size are fixed for the lifetime of the array
 * 
 * Example: `dStaticArray_t* array = d_InitStaticArray(10, sizeof(int));`
 * This creates a new static array with a capacity of 10 elements, each of size 4 bytes.
 */
dStaticArray_t* d_InitStaticArray(size_t capacity, size_t element_size);

/**
 * @brief: Destroy a static array and free all associated memory
 *
 * @param array: Pointer to static array to destroy
 * @return: 0 on success, 1 on failure
 *
 * -- Safe to call with NULL pointer
 * -- Frees data buffer first, then structure
 * -- After calling, the pointer becomes invalid
 * 
 * Example: `d_DestroyStaticArray(array);`
 * This destroys the static array and frees its memory.
 */
int d_DestroyStaticArray(dStaticArray_t* array);

/** 
 * @brief Append an element to the end of the static array.
 * *
 * @param array: Pointer to static array
 * @param data: Pointer to element data to copy
 * @return: 0 on success, 1 on failure
 *
 * -- Fails if array is at maximum capacity
 * -- Fails if array or data is NULL
 * -- Copies element_size bytes from data
 * -- Increments count on successful append
 * 
 * Example: `d_AppendDataToStaticArray(array, &data);`
 * This appends a pointer to a data element to the end of the static array.
 */
int d_AppendDataToStaticArray(dStaticArray_t* array, void* data);

/**
 * @brief: Get a pointer to an element at the specified index
 *
 * @param array: Pointer to static array
 * @param index: Zero-based index of element to retrieve
 * @return: Pointer to element data, or NULL if out of bounds
 *
 * -- Returns NULL if array is NULL or index >= count
 * -- Performs bounds checking for safety
 * -- Returned pointer is valid until array is modified or destroyed
 * 
 * Example: `void* data = d_IndexDataFromStaticArray(array, 0);`
 * This retrieves the first element from the static array.
 */
void* d_IndexDataFromStaticArray(dStaticArray_t* array, size_t index);

/**
 * @brief: Remove and return a pointer to the last element (LIFO behavior)
 *
 * @param array: Pointer to static array
 * @return: Pointer to last element data, or NULL if empty
 *
 * -- Returns NULL if array is NULL or empty
 * -- Decrements count but doesn't free memory
 * -- Element data remains in buffer but is no longer active
 * -- Returned pointer becomes invalid after next append
 * 
 * Example: `void* data = d_PopDataFromStaticArray(array);`
 * This removes and returns the last element from the static array.
 */
void* d_PopDataFromStaticArray(dStaticArray_t* array);

/**
 * @brief Get the number of free slots remaining in the static array
 *
 * @param array: Pointer to static array
 * @return: Number of elements that can still be added, or 0 if array is NULL/full
 *
 * -- Returns 0 if array is NULL (no space available)
 * -- Returns capacity - count for valid arrays
 * -- Useful for checking if array can accept more elements before attempting append
 * 
 * Example: `size_t free_slots = d_GetFreeSpaceInStaticArray(array);`
 * This returns how many more elements can be added to the array.
 */
size_t d_GetFreeSpaceInStaticArray(dStaticArray_t* array);

/**
 * @brief Fill the static array with multiple copies of a given value
 *
 * @param array: Pointer to static array
 * @param value: Pointer to the value to copy into each element
 * @param num_elements: Number of elements to fill
 * @return: 0 on success, 1 on failure
 *
 * -- Fails if array or value is NULL
 * -- Fails if num_elements exceeds array capacity
 * -- Copies element_size bytes from value to each position
 * -- Sets array count to num_elements after successful fill
 * -- Overwrites existing data in the array
 * 
 * Example: `int value = 42; d_FillDataInStaticArray(array, &value, 5);`
 * This fills the first 5 positions of the array with the value 42.
 */
int d_FillDataInStaticArray(dStaticArray_t* array, const void* value, size_t num_elements);

/**
 * @brief Get direct access to the raw memory buffer of the static array
 *
 * @param array: Pointer to static array
 * @return: Pointer to the start of the raw data buffer, or NULL on error
 *
 * -- Returns NULL if array is NULL or data buffer is NULL
 * -- Provides access to entire allocated buffer regardless of current count
 * -- Returned pointer is valid until array is destroyed
 * -- Use with caution - direct memory access bypasses safety checks
 * -- Primarily intended for advanced operations or serialization
 * 
 * Example: `void* raw_buffer = d_PeekRawMemoryOfStaticArray(array);`
 * This returns a pointer to the entire data buffer of the array.
 */
void* d_PeekRawMemoryOfStaticArray(dStaticArray_t* array);

/**
 * @brief Save a static array to a binary file
 *
 * @param filename Path to the file where the array should be saved
 * @param array Pointer to the static array to save
 * @return 0 on success, 1 on failure
 *
 * -- Saves array metadata (capacity, count, element_size) and data
 * -- Uses binary format with magic number for validation
 * -- File can be loaded later using d_LoadStaticArrayFromFile
 * 
 * Example: `d_SaveStaticArrayToFile("myarray.bin", array);`
 */
int d_SaveStaticArrayToFile(const char* filename, const dStaticArray_t* array);

/**
 * @brief Load a static array from a binary file
 *
 * @param filename Path to the file containing the saved array
 * @return Pointer to new static array, or NULL on failure
 *
 * -- Validates file format and magic number
 * -- Allocates new array with original capacity and data
 * -- Returns NULL if file format is invalid or allocation fails
 * 
 * Example: `dStaticArray_t* array = d_LoadStaticArrayFromFile("myarray.bin");`
 */
dStaticArray_t* d_LoadStaticArrayFromFile(const char* filename);

/**
 * @brief Iterate over all elements in a static array
 *
 * @param array Pointer to the static array to iterate
 * @param callback Function to call for each element
 * @param user_data Optional data to pass to the callback
 * @return 0 on success, 1 on failure
 *
 * -- Calls callback function for each element in the array (up to count)
 * -- Provides element index, data pointer, and size to callback
 * -- Safe iteration - callback receives read-only access to elements
 * 
 * Example: `d_IterateStaticArray(array, my_callback, &context);`
 */
int d_IterateStaticArray(const dStaticArray_t* array, dStaticArrayIteratorFunc callback, void* user_data);

// Turning Strings Into Dynamic Arrays
// src/dStrings-dArrays.c
/*
 * Split a string by delimiter into a dynamic array of string builders (like Python's str.split())
 *
 * `text` - The null-terminated string to be split.
 * `delimiter` - The null-terminated string to split by.
 *
 * `dArray_t*` - A new dynamic array containing dString_t* pointers, or NULL on error.
 *
 * -- Returns NULL if text or delimiter is NULL, or if memory allocation fails.
 * -- The caller must free the result using d_FreeSplitStringArray() to prevent memory leaks.
 * -- Each element in the array is a `dString_t*` which is also allocated and must be destroyed.
 * -- Empty strings between delimiters are included in the result as empty dString_t objects.
 * -- If the delimiter is not found, returns an array with a single element containing a copy of the original string.
 */
dArray_t* d_SplitString(const char* text, const char* delimiter);

/*
 * Free memory allocated by d_SplitString() for the dynamic array of strings.
 *
 * `string_array` - The dynamic array of dString_t* pointers returned by d_SplitString().
 *
 * -- Safe to call with a NULL pointer.
 * -- Frees each individual dString_t object, then the array data, and finally the array structure itself.
 * -- Must be called for every successful d_SplitString() call to prevent memory leaks.
 */
void d_FreeSplitStringArray(dArray_t* string_array);

#endif