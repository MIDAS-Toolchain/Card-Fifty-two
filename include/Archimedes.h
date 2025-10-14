#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#ifndef __ARCHIMEDES_H__
#define __ARCHIMEDES_H__

/*
---------------------------------------------------------------
---                         Defines                         ---
---------------------------------------------------------------
*/

#define FPS 60.0
#define LOGIC_RATE ( FPS / 1000 )

#define MAX_KEYBOARD_KEYS 350
#define LOG_LEVEL_COUNT   6

#define SCREEN_WIDTH_16_9_1  1600
#define SCREEN_HEIGHT_16_9_1 900
#define SCREEN_WIDTH_16_9_0  1280
#define SCREEN_HEIGHT_16_9_0 720

#define SCREEN_WIDTH_5_4  1280
#define SCREEN_HEIGHT_5_4 1024

#define SCREEN_WIDTH_4_3_2  1024
#define SCREEN_HEIGHT_4_3_2 768
#define SCREEN_WIDTH_4_3_1  800
#define SCREEN_HEIGHT_4_3_1 600
#define SCREEN_WIDTH_4_3_0  640
#define SCREEN_HEIGHT_4_3_0 480

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

#define FONT_SIZE 32 
#define FONT_TEXTURE_SIZE 512
#define MAX_GLYPHS 400
#define MAX_GLYPH_SIZE 8
#define MAX_WORD_LENGTH 32
#define MAX_LINE_LENGTH 1024

// Text system error codes
#define ARCH_TEXT_SUCCESS 0
#define ARCH_TEXT_ERROR_NULL_POINTER -1
#define ARCH_TEXT_ERROR_INVALID_FONT -2
#define ARCH_TEXT_ERROR_BUFFER_OVERFLOW -3
#define ARCH_TEXT_ERROR_INVALID_UTF8 -4

#ifndef __DAEDALUS_H__

#define MAX_LINE_LENGTH        1024
#define MAX_FILENAME_LENGTH    256
#define MAX_NAME_LENGTH        32
#define MAX_DESCRIPTION_LENGTH 256
#define MAX_INPUT_LENGTH       16
#define PI 3.14159265

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define STRCPY( dest, src ) strncpy( dest, src, sizeof( dest ) ); dest[sizeof(dest) - 1] = '\0'
#define STRNCPY(dest, src, n) strncpy(dest, src, n); dest[n - 1] = '\0'
#define RANDF(lower, upper) (((float)rand() / (float)(RAND_MAX)) * (upper - lower)) + lower
#define MAP( value, start0, start1, end0, end1 ) ( ( value - start0 ) * ( ( end1 - end0 ) / ( start1 - start0 ) ) + end0 )
#define LOG( msg ) printf( "%s | File: %s, Line: %d\n", msg, __FILE__, __LINE__ )

#endif

enum
{
  FONT_CODE_PAGE_437,
  FONT_ENTER_COMMAND,
  FONT_LINUX,
  FONT_GAME,
  FONT_MAX
};

/*
---------------------------------------------------------------
---                         Structs                         ---
---------------------------------------------------------------
*/

typedef struct
{
  char error_msg[MAX_LINE_LENGTH];
  int  error_type;
} aError_t;

typedef struct _widget_t
{
  int type;
  char name[MAX_FILENAME_LENGTH];
  int x, y;
  int w, h;
  char label[MAX_FILENAME_LENGTH];
  int boxed;
  int hidden;
  int padding;
  int flex;
  uint8_t fg[4];
  uint8_t bg[4];
  struct _widget_t* next;
  struct _widget_t* prev;
  void (*action)( void );
  void (*data);
} aWidget_t;

typedef struct
{
  int x, y;
  int w, h;
  int spacing;
  int num_components;
  aWidget_t* components;
} aContainerWidget_t;

typedef struct
{
  int num_options;
  char** options;
  int x, y;
  int w, h;
  int value;
  char* text_name[MAX_NAME_LENGTH];
} aSelectWidget_t;

typedef struct
{
  int x, y;
  int w, h;
  int value;
  int step;
  int wait_on_change;
} aSliderWidget_t;

typedef struct
{
  int x, y;
  int w, h;
  int max_length;
  char* text;
} aInputWidget_t;

typedef struct
{
  int x, y;
  int value;
} aControlWidget_t;

typedef struct
{
  Uint8 r;
  Uint8 g;
  Uint8 b;
  Uint8 a;
} aColor_t;

typedef struct
{
  int x;
  int y;
  uint8_t pressed;
  uint8_t button;
  uint8_t state;
  uint8_t clicks;
  int8_t  wheel;
} aMouse_t;

typedef struct _aImageCacheNode_t
{
  SDL_Surface* surf;
  int ID;
  char filename[MAX_FILENAME_LENGTH];
  struct _aImageCacheNode_t* next;
} aImageCacheNode_t;

typedef struct
{
  aImageCacheNode_t* head;
} aImageCache_t;

typedef struct
{
  unsigned int currentTime; //Delta time
  unsigned int lastTime;

  unsigned int frameStart; //Frame time to calculate fps
  unsigned int frameTime; //Length of each frame
  unsigned int lastFrameCounterClear; //This keeps count of how long it has been since the last time frames was set to 0
  int frames; //actual count of how many frames happen over one second
} aDeltaTime_t;

typedef struct
{
  void (*logic)( float elapsedTime );
  void (*draw)( float elapsedTime );
  void (*onExit)( void );
} aDelegate_t;

typedef struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_AudioDeviceID deviceID;
  aDelegate_t delegate;
  aDeltaTime_t time;
  aColor_t background;
  aImageCache_t* img_cache;
  int keyboard[MAX_KEYBOARD_KEYS];
  aWidget_t* active_widget;
  double font_scale;
  int font_type;
  SDL_Rect glyphs[FONT_MAX][MAX_GLYPHS];
  TTF_Font* fonts[FONT_MAX];
  SDL_Texture* font_textures[FONT_MAX];
  aMouse_t mouse;
  int running;
  char input_text[MAX_INPUT_LENGTH];
  int last_key_pressed;
} aApp_t;

typedef struct
{
  char filename[MAX_FILENAME_LENGTH];
  SDL_AudioSpec spec;
  uint32_t length;
  uint8_t* buffer;
} aAudioClip_t;

typedef struct _textures{
  char name[MAX_FILENAME_LENGTH];
  SDL_Texture* texture;
  struct _textures* next;
} aTexture_t;

/*
---------------------------------------------------------------
---                          Audio                          ---
---------------------------------------------------------------
*/

int a_InitAudio( void );
void a_LoadSounds( const char *filename, aAudioClip_t *clip );
void a_PlaySoundEffect( aAudioClip_t *clip );

/*
---------------------------------------------------------------
---                      Delta Time                         ---
---------------------------------------------------------------
*/

float a_GetDeltaTime( void );

/**
 * @defgroup drawing Drawing System
 * @brief 2D drawing and rendering functions for the Archimedes engine
 * 
 * The drawing system provides comprehensive 2D graphics capabilities including:
 * - Scene management and render cycles
 * - Primitive drawing (points, lines, shapes)
 * - Filled and outlined shapes
 * - Surface and texture blitting
 * - Color management with predefined constants
 * 
 * All drawing functions use SDL2 as the underlying graphics backend and follow
 * a consistent pattern of temporarily setting render colors and restoring them.
 * 
 * @{
 */

/**
 * @brief Prepare the scene for rendering
 * 
 * Initializes the render target and clears the screen with the current background color.
 * This function must be called at the beginning of each frame before any drawing operations.
 * 
 * @note Sets the render draw color to white (255,255,255,255) for subsequent operations
 * @note Must be paired with a_PresentScene() to display the rendered frame
 * @see a_PresentScene()
 * @see app.background
 */
void a_PrepareScene( void );

/**
 * @brief Present the rendered scene to the screen
 * 
 * Displays all drawing operations performed since a_PrepareScene() was called.
 * This completes the render cycle and should be called once per frame after all
 * drawing operations are complete.
 * 
 * @note Uses double-buffered rendering: swaps front and back buffers
 * @note Call this after all drawing operations in a frame
 * @see a_PrepareScene()
 */
void a_PresentScene( void );
/**
 * @brief Draw a single point at specified coordinates
 * 
 * Renders a single pixel at the given screen coordinates using the specified color.
 * The point is clipped automatically if coordinates fall outside screen bounds.
 * 
 * @param x Horizontal position in pixels
 * @param y Vertical position in pixels
 * @param color RGBA color structure for the point
 * 
 * @note Point is drawn as a single pixel
 * @note Coordinates outside screen bounds are clipped by SDL
 * @note Temporarily changes render color, restores to white afterward
 * @see aColor_t
 */
void a_DrawPoint( const int x, const int y, const aColor_t color );
/**
 * @brief Draw a line between two points
 * 
 * Renders a straight line from the starting point to the ending point using
 * the specified color. Uses SDL's line drawing algorithm for optimal performance.
 * 
 * @param x1 Starting X coordinate
 * @param y1 Starting Y coordinate
 * @param x2 Ending X coordinate
 * @param y2 Ending Y coordinate
 * @param color RGBA color structure for the line
 * 
 * @note Handles lines at any angle including vertical and horizontal
 * @note Uses Bresenham's line algorithm internally via SDL
 * @note Temporarily changes render color, restores to white afterward
 * @see a_DrawHorizontalLine()
 * @see a_DrawVerticalLine()
 */
void a_DrawLine( const int x1, const int y1, const int x2, const int y2, const aColor_t color );
/**
 * @brief Draw a horizontal line
 * 
 * Renders a horizontal line between two X coordinates at a fixed Y position.
 * This is optimized for drawing horizontal lines and handles coordinate ordering automatically.
 * 
 * @param x1 Starting X coordinate
 * @param x2 Ending X coordinate
 * @param y Y coordinate (same for both endpoints)
 * @param color RGBA color structure for the line
 * 
 * @note Optimized for horizontal lines
 * @note Order of x1 and x2 doesn't matter (automatically handles swapping)
 * @note Temporarily changes render color, restores to white afterward
 * @see a_DrawLine()
 * @see a_DrawVerticalLine()
 */
void a_DrawHorizontalLine( const int x1, const int x2, const int y, const aColor_t color );
/**
 * @brief Draw a vertical line
 * 
 * Renders a vertical line between two Y coordinates at a fixed X position.
 * This is optimized for drawing vertical lines and handles coordinate ordering automatically.
 * 
 * @param y1 Starting Y coordinate
 * @param y2 Ending Y coordinate
 * @param x X coordinate (same for both endpoints)
 * @param color RGBA color structure for the line
 * 
 * @note Optimized for vertical lines
 * @note Order of y1 and y2 doesn't matter (automatically handles swapping)
 * @note Temporarily changes render color, restores to white afterward
 * @see a_DrawLine()
 * @see a_DrawHorizontalLine()
 */
void a_DrawVerticalLine( const int y1, const int y2, const int x, const aColor_t color );
/**
 * @brief Draw a circle outline
 * 
 * Renders the outline of a circle at the specified center position with the given radius.
 * Uses Bresenham's circle algorithm for efficient pixel-perfect circle drawing.
 * 
 * @param posX Center X coordinate
 * @param posY Center Y coordinate
 * @param radius Circle radius in pixels
 * @param color RGBA color structure for the circle outline
 * 
 * @note Draws only the perimeter, not filled
 * @note Uses Bresenham's circle algorithm for efficiency
 * @note Radius of 0 draws a single point at center
 * @note Negative radius is treated as 0
 * @see a_DrawFilledCircle()
 */
void a_DrawCircle( const int posX, const int posY, const int radius, const aColor_t color );
/**
 * @brief Draw a filled circle
 * 
 * Renders a completely filled circle at the specified center position with the given radius.
 * Uses an optimized scan-line filling algorithm for smooth, solid circles.
 * 
 * @param posX Center X coordinate
 * @param posY Center Y coordinate
 * @param radius Circle radius in pixels
 * @param color RGBA color structure for the filled circle
 * 
 * @note Fills the entire circle area with color
 * @note Uses optimized scan-line filling algorithm
 * @note Radius of 0 draws a single point at center
 * @note Negative radius is treated as 0
 * @see a_DrawCircle()
 */
void a_DrawFilledCircle( const int posX, const int posY, const int radius, const aColor_t color );
/**
 * @brief Draw a triangle outline
 * 
 * Renders the outline of a triangle by drawing three lines connecting the specified vertices.
 * The triangle can be specified with vertices in any order (clockwise or counter-clockwise).
 * 
 * @param x0 First vertex X coordinate
 * @param y0 First vertex Y coordinate
 * @param x1 Second vertex X coordinate
 * @param y1 Second vertex Y coordinate
 * @param x2 Third vertex X coordinate
 * @param y2 Third vertex Y coordinate
 * @param color RGBA color structure for the triangle outline
 * 
 * @note Draws three lines connecting the vertices
 * @note Vertices can be specified in any order (clockwise or counter-clockwise)
 * @note Degenerate triangles (collinear points) draw as lines
 * @see a_DrawFilledTriangle()
 */
void a_DrawTriangle( const int x0, const int y0, const int x1, const int y1, const int x2,
                     const int y2, const aColor_t color );
/**
 * @brief Draw a filled triangle
 * 
 * Renders a completely filled triangle using the three specified vertices.
 * The implementation uses barycentric coordinate filling for accurate results.
 * 
 * @param x0 First vertex X coordinate
 * @param y0 First vertex Y coordinate
 * @param x1 Second vertex X coordinate
 * @param y1 Second vertex Y coordinate
 * @param x2 Third vertex X coordinate
 * @param y2 Third vertex Y coordinate
 * @param color RGBA color structure for the filled triangle
 * 
 * @note Fills the entire triangle area with color
 * @note Currently implementation is disabled (commented out)
 * @note Would use barycentric coordinate filling when implemented
 * @note Degenerate triangles are handled gracefully
 * @see a_DrawTriangle()
 */
void a_DrawFilledTriangle( const int x0, const int y0, const int x1, const int y1,
                           const int x2, const int y2, const aColor_t color );
/**
 * @brief Draw a rectangle outline
 * 
 * Renders the outline of a rectangle at the specified position and dimensions.
 * Uses individual RGBA color components instead of a color structure.
 * 
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 * @param r Red color component (0-255)
 * @param g Green color component (0-255)
 * @param b Blue color component (0-255)
 * @param a Alpha transparency (0-255, 255=opaque)
 * 
 * @note Draws only the border, not filled
 * @note Zero or negative dimensions are handled gracefully
 * @note Temporarily changes render color, restores to white afterward
 * @see a_DrawFilledRect()
 */
void a_DrawRect( const int x, const int y, const int w, const int h, const int r,
                 const int g, const int b, const int a );
/**
 * @brief Draw a filled rectangle
 * 
 * Renders a completely filled rectangle at the specified position and dimensions.
 * Uses individual RGBA color components and supports alpha blending.
 * 
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 * @param r Red color component (0-255)
 * @param g Green color component (0-255)
 * @param b Blue color component (0-255)
 * @param a Alpha transparency (0-255, 255=opaque)
 * 
 * @note Fills the entire rectangle area with color
 * @note Zero or negative dimensions are handled gracefully
 * @note Supports alpha blending when alpha < 255
 * @note Temporarily changes render color, restores to white afterward
 * @see a_DrawRect()
 */
void a_DrawFilledRect( const int x, const int y, const int w, const int h, const int r,
                       const int g, const int b, const int a );

/**
 * @brief Blit a surface to the screen at specified position
 * 
 * Converts an SDL surface to a texture and renders it at the specified coordinates.
 * The surface dimensions determine the size of the rendered image.
 * 
 * @param surf Source SDL surface to draw
 * @param x Destination X coordinate
 * @param y Destination Y coordinate
 * 
 * @note Converts surface to texture and renders it
 * @note Surface dimensions determine blit size
 * @note Creates and destroys texture each call (not optimized for repeated use)
 * @note NULL surface is handled gracefully without crashing
 * @note Surface remains unchanged after blitting
 * @see a_BlitSurfRect()
 * @see a_BlitTextureRect()
 */
void a_Blit( SDL_Surface* surf, const int x, const int y );

/**
 * @brief Blit a rectangular region of a surface to the screen
 * 
 * Converts a portion of an SDL surface to a texture and renders it with scaling.
 * Only the specified rectangular region from the source is drawn.
 * 
 * @param surf Source SDL surface to draw from
 * @param src Source rectangle within the surface
 * @param x Destination X coordinate
 * @param y Destination Y coordinate
 * @param scale Scaling factor for the destination size
 * 
 * @note Blits only the specified rectangular region from source
 * @note Destination size is source rectangle dimensions multiplied by scale
 * @note Frees the source surface after creating texture
 * @note NULL surface is handled gracefully without crashing
 * @note Source rectangle is clipped to surface bounds
 * @see a_Blit()
 * @see a_BlitTextureRect()
 */
void a_BlitSurfRect( SDL_Surface* surf, SDL_Rect src, const int x, const int y,
                     const int scale );
void a_BlitTextureRect( SDL_Texture* texture, SDL_Rect src, const int x,
                        const int y, const int scale, aColor_t color );

/**
 * @brief Blit a texture scaled to specific dimensions
 *
 * Renders the entire texture stretched or shrunk to fit the specified width and height.
 * This provides independent control over horizontal and vertical scaling without
 * requiring SDL_Rect construction or scale factor calculations.
 *
 * @param texture SDL texture to render (must not be NULL)
 * @param x Destination X coordinate (top-left corner)
 * @param y Destination Y coordinate (top-left corner)
 * @param w Scaled width in pixels
 * @param h Scaled height in pixels
 *
 * @note Renders entire texture (no source rectangle clipping)
 * @note Width and height are independent - aspect ratio not preserved automatically
 * @note Texture remains unchanged after rendering
 * @note NULL texture will be handled silently (no crash)
 * @note Zero or negative dimensions will result in no rendering
 * @see a_BlitTextureRect()
 * @see a_LoadTexture()
 */
void a_BlitTextureScaled( SDL_Texture* texture, const int x, const int y,
                           const int w, const int h );

/*
 * Update the window title text
 *
 * `title` - New title string for the window
 *
 * -- Changes the text displayed in the window's title bar
 * -- String is copied internally, original can be freed after call
 * -- Empty string results in blank title
 * -- NULL title is handled gracefully (implementation dependent)
 * -- Works on all supported platforms (Windows, Linux, macOS)
 */
void a_UpdateTitle( const char *title );

/*
---------------------------------------------------------------
---                          Image                          ---
---------------------------------------------------------------
*/

int a_InitImage( void );
SDL_Surface* a_Image( const char *filename );
int a_CleanUpImageCache( aImageCache_t* head );
int a_Screenshot( SDL_Renderer *renderer, const char *filename );

/*
---------------------------------------------------------------
---                       Initialize                        ---
---------------------------------------------------------------
*/

extern aApp_t app;

/**
 * @brief Initializes the Archimedes framework and SDL subsystems.
 *
 * This function sets up the complete Archimedes environment including SDL video,
 * audio, and timer subsystems, creates a window and renderer, initializes all
 * app state variables, and sets up the audio, image, texture, and font systems.
 * The random number generator is also seeded with the current time.
 *
 * The initialization sequence includes:
 * - SDL video, audio, and timer subsystem initialization
 * - PNG image format support initialization
 * - TTF font support initialization
 * - Window and renderer creation with specified dimensions
 * - Mouse and timing state initialization
 * - Image cache setup
 * - Audio, image, texture, and font system initialization
 * - Random number generator seeding
 *
 * This function must be called before any other Archimedes functions.
 * The global `app` structure will be populated with initialized values.
 *
 * @param width Window width in pixels (must be > 0)
 * @param height Window height in pixels (must be > 0)
 * @param title Window title string (cannot be NULL)
 * @return 0 on success, negative value on failure (-1: SDL init, -2: IMG init, -3: TTF init, -4: window creation)
 */
int a_Init( const int width, const int height, const char *title );
/**
 * @brief Cleans up all Archimedes resources and shuts down SDL.
 *
 * This function performs complete cleanup of the Archimedes framework by
 * calling the optional onExit delegate callback, destroying the renderer
 * and window, cleaning up the image cache, and shutting down SDL subsystems.
 * Should be called before program termination to ensure proper resource cleanup.
 *
 * The cleanup sequence includes:
 * - Calling the optional onExit delegate callback if set
 * - Destroying the SDL renderer
 * - Destroying the SDL window
 * - Cleaning up the image cache and freeing memory
 * - Shutting down TTF, IMG, and SDL subsystems
 * - Resetting the app running state
 *
 * This function is safe to call multiple times and will handle NULL pointers gracefully.
 * After calling this function, a_Init() must be called again before using any other
 * Archimedes functions.
 */
void a_Quit( void );


/*
---------------------------------------------------------------
---                          Input                          ---
---------------------------------------------------------------
*/

/**
 * @brief Processes all pending SDL events and dispatches them to appropriate handlers.
 *
 * This function continuously polls for SDL events (keyboard, mouse, quit, text input)
 * using `SDL_PollEvent` and dispatches them to internal static functions (`a_DoKeyDown`,
 * `a_DoKeyUp`, `a_DoMouseDown`, `a_DoMouseUp`, `a_DoMouseWheel`, `a_DoMouseMotion`).
 * It updates the global `app` state based on these events, managing the game's input.
 * Unknown event types are silently ignored.
 *
 * @note The `app.running` flag is set to `0` if an `SDL_QUIT` event is received.
 * @note For `SDL_TEXTINPUT` events, the input text is copied to `app.input_text`
 * if its length is valid (between 1 and `MAX_INPUT_LENGTH - 1`).
 *
 * @see a_DoKeyDown
 * @see a_DoKeyUp
 * @see a_DoMouseDown
 * @see a_DoMouseUp
 * @see a_DoMouseWheel
 * @see a_DoMouseMotion
 */
void a_DoInput( void );

/*
---------------------------------------------------------------
---                          Text                           ---
---------------------------------------------------------------
*/

/**
 * @defgroup text Text Rendering System
 * @brief TTF font rendering with alignment and wrapping support
 *
 * The text system provides bitmap font rendering using SDL_ttf.
 * Text is rendered using preloaded font atlases for optimal performance.
 *
 * @{
 */

/**
 * @brief Text alignment modes
 */
enum
{
  TEXT_ALIGN_LEFT,    /**< Align text to the left of X coordinate */
  TEXT_ALIGN_CENTER,  /**< Center text horizontally at X coordinate */
  TEXT_ALIGN_RIGHT    /**< Align text to the right of X coordinate */
};

/**
 * @brief Calculate the height of wrapped text
 *
 * Computes how tall the rendered text will be when word-wrapped to fit
 * within the specified maximum width.
 *
 * @param text String to measure (UTF-8 supported)
 * @param font_type Font to use (FONT_GAME, FONT_ENTER_COMMAND, etc.)
 * @param max_width Maximum line width in pixels before wrapping
 * @return Total height in pixels needed to render the wrapped text
 *
 * @note Useful for dynamic UI layouts that adapt to text size
 * @note Returns 0 if text is NULL or empty
 * @see a_CalcTextDimensions()
 */
int a_GetWrappedTextHeight( char* text, int font_type, int max_width );

/**
 * @brief Calculate the dimensions of rendered text
 *
 * Measures the width and height of text when rendered with the specified font.
 * Does not account for wrapping (use a_GetWrappedTextHeight for that).
 *
 * @param text String to measure (UTF-8 supported)
 * @param font_type Font to use (FONT_GAME, FONT_ENTER_COMMAND, etc.)
 * @param w Output pointer for width in pixels
 * @param h Output pointer for height in pixels
 *
 * @note Width/height are set to 0 if text is NULL or empty
 * @note Height is the font's line height, not the actual glyph height
 * @note Essential for proper text centering in buttons/UI elements
 *
 * @code
 * int text_w, text_h;
 * a_CalcTextDimensions("Hello", FONT_GAME, &text_w, &text_h);
 * int centered_y = button_y + (button_h - text_h) / 2;
 * @endcode
 */
void a_CalcTextDimensions( char* text, int font_type, int* w, int* h );

/**
 * @brief Draw text to the screen
 *
 * Renders text at the specified position with color and alignment.
 * The (x, y) coordinate represents the TOP-LEFT corner of the text bounding box
 * (after alignment is applied).
 *
 * @param text String to render (UTF-8 supported)
 * @param x X coordinate - meaning depends on alignment:
 *          - TEXT_ALIGN_LEFT: left edge of text
 *          - TEXT_ALIGN_CENTER: horizontal center of text
 *          - TEXT_ALIGN_RIGHT: right edge of text
 * @param y Y coordinate - TOP edge of text (NOT center!)
 * @param r Red color component (0-255)
 * @param g Green color component (0-255)
 * @param b Blue color component (0-255)
 * @param font_type Font to use (FONT_GAME, FONT_ENTER_COMMAND, FONT_LINUX, etc.)
 * @param align Alignment mode (TEXT_ALIGN_LEFT/CENTER/RIGHT)
 * @param max_width Maximum width before wrapping (0 = no wrapping)
 *
 * @note Y coordinate is the TOP of the text, not the baseline or center!
 * @note To vertically center text: y = center_y - (text_height / 2)
 * @note max_width of 0 disables wrapping
 * @note Clipping is handled automatically by SDL
 *
 * @warning Common mistake: Forgetting that Y is the TOP of the text
 *
 * @code
 * // Draw centered text in a button
 * int text_w, text_h;
 * a_CalcTextDimensions("Click Me", FONT_GAME, &text_w, &text_h);
 * int text_x = button_x + button_w / 2;
 * int text_y = button_y + (button_h - text_h) / 2;  // Vertically centered!
 * a_DrawText("Click Me", text_x, text_y, 255, 255, 255, FONT_GAME, TEXT_ALIGN_CENTER, 0);
 * @endcode
 *
 * @see a_CalcTextDimensions()
 */
void a_DrawText( char* text, int x, int y, int r, int g, int b, int font_type, int align, int max_width );

/**
 * @brief Get an SDL_Texture containing rendered text
 *
 * Creates a texture with the specified text rendered using the given font.
 * Texture must be destroyed by caller using SDL_DestroyTexture().
 *
 * @param text String to render (UTF-8 supported)
 * @param font_type Font to use (FONT_GAME, FONT_ENTER_COMMAND, etc.)
 * @return SDL_Texture pointer, or NULL on error
 *
 * @note Caller is responsible for calling SDL_DestroyTexture()
 * @note More efficient to call a_DrawText() for one-time rendering
 * @note Useful when text needs to be rendered multiple times per frame
 *
 * @warning Memory leak if texture is not destroyed!
 */
SDL_Texture* a_GetTextTexture( char* text, int font_type );

/**
 * @brief Initialize the font rendering system
 *
 * Loads TTF fonts and creates glyph atlases for text rendering.
 * Called automatically by a_Init() - do not call manually.
 *
 * @note Loads fonts from resources/fonts/ directory
 * @note Creates texture atlases for fast glyph rendering
 * @note Program will exit if font loading fails
 *
 * @see a_Init()
 */
void a_InitFonts( void );

/** @} */ // end of text group

/*
---------------------------------------------------------------
---                       Textures                          ---
---------------------------------------------------------------
*/

SDL_Texture* a_LoadTexture( char* filename );
SDL_Texture* a_ToTexture( SDL_Surface* surf, int destroy );
void a_InitTextures( void );

/*
---------------------------------------------------------------
---                        Widgets                          ---
---------------------------------------------------------------
*/

enum
{
  WT_BUTTON,
  WT_SELECT,
  WT_SLIDER,
  WT_INPUT,
  WT_CONTROL,
  WT_CONTAINER,
};

/**
 * @brief Draws all visible widgets.
 *
 * This function iterates through the linked list of widgets, starting from `widget_head.next`,
 * and calls the appropriate drawing function for each widget based on its type.
 * Hidden widgets are skipped as they are not drawn.
 */
void a_DrawWidgets( void );

/**
 * @brief Handles the logic and interactions for all active widgets.
 *
 * This function is the main update loop for the widget system. It processes mouse clicks,
 * keyboard inputs (left, right, space, return), and manages the active widget state.
 * It also handles transitions to specific input or control modes if an appropriate
 * widget is activated.
 *
 * It checks for mouse clicks to activate widgets based on their coordinates.
 * For container widgets, it iterates through their components to find the clicked one.
 * Keyboard inputs are handled for navigating widgets (though up/down are commented out)
 * and triggering actions or entering specific widget interaction modes (input/control).
 */
void a_DoWidget( void );

/**
 * @brief Retrieves a widget by its name.
 *
 * This function searches through the linked list of widgets (starting from `widget_head.next`)
 * to find a widget with a matching name. If a widget with the given name is found,
 * a pointer to that widget is returned. If no widget is found, an SDL warning
 * message is logged, and `NULL` is returned.
 *
 * @param name The string name of the widget to retrieve.
 * @return A pointer to the `aWidget_t` if found, otherwise `NULL`.
 */
aWidget_t* a_GetWidget( char* name );

/**
 * @brief Retrieves a container widget by its name and casts it to `aContainerWidget_t`.
 *
 * This function first calls `a_GetWidget` to locate a widget by the given name.
 * If the widget is found, it attempts to cast its `data` member to an `aContainerWidget_t*`.
 * Error messages are printed to stdout if the widget is not found or if the cast results in NULL.
 *
 * @param name The string name of the container widget to retrieve.
 * @return A pointer to the `aContainerWidget_t` if successful, otherwise `NULL`.
 */
aContainerWidget_t* a_GetContainerFromWidget( char* name );

/**
 * @brief Initializes the widget system from a configuration file.
 *
 * This function sets up the initial state of the widget system.
 * It clears the widget head, sets the tail, loads widgets from the specified
 * filename, and initializes various global widget-related variables such as
 * `slider_delay`, `cursor_blink`, `handle_input_widget`, and `handle_control_widget`.
 *
 * @param filename The path to the file containing widget configuration data.
 */
void a_InitWidgets( const char* filename );
int a_ClearWidgetCache( aWidget_t* widget );

/*
---------------------------------------------------------------
---                         Colors                          ---
---------------------------------------------------------------
*/

extern aColor_t black;
extern aColor_t blue;
extern aColor_t green;
extern aColor_t cyan;
extern aColor_t red;
extern aColor_t magenta;
extern aColor_t yellow;
extern aColor_t white;
extern aColor_t shit0;
extern aColor_t shit1;
extern aColor_t shit2;
extern aColor_t shit3;

extern aColor_t gray9;
extern aColor_t gray8;
extern aColor_t gray7;
extern aColor_t gray6;
extern aColor_t gray5;
extern aColor_t gray4;
extern aColor_t gray3;
extern aColor_t gray2;
extern aColor_t gray1;
extern aColor_t gray0;

/*
---------------------------------------------------------------
---                         Errors                          ---
---------------------------------------------------------------
*/

typedef enum
{
  NORMAL,
  WARNING,
  FATAL,
  INFO,
  DEBUG,
  TRACE,
} aLogLevel_t;

extern char* log_level_strings[LOG_LEVEL_COUNT];

/*
---------------------------------------------------------------
---                         Layout                          ---
---------------------------------------------------------------
*/

/**
 * @defgroup flexbox FlexBox Layout System
 * @brief CSS-inspired automatic layout engine for UI positioning
 *
 * The FlexBox system provides automatic positioning of UI elements using
 * CSS flexbox-like rules. Items can be arranged horizontally or vertically
 * with various alignment and spacing options.
 *
 * @{
 */

/**
 * @brief Layout direction for FlexBox containers
 */
typedef enum {
    FLEX_DIR_ROW,      /**< Horizontal layout (left to right) */
    FLEX_DIR_COLUMN    /**< Vertical layout (top to bottom) */
} FlexDirection_t;

/**
 * @brief Main-axis justification modes
 */
typedef enum {
    FLEX_JUSTIFY_START,          /**< Align to start (left/top) */
    FLEX_JUSTIFY_CENTER,         /**< Center items */
    FLEX_JUSTIFY_END,            /**< Align to end (right/bottom) */
    FLEX_JUSTIFY_SPACE_BETWEEN,  /**< Equal spacing, edges aligned */
    FLEX_JUSTIFY_SPACE_AROUND    /**< Equal spacing including edges */
} FlexJustify_t;

/**
 * @brief Cross-axis alignment modes
 */
typedef enum {
    FLEX_ALIGN_START,   /**< Cross-axis start */
    FLEX_ALIGN_CENTER,  /**< Cross-axis center */
    FLEX_ALIGN_END      /**< Cross-axis end */
} FlexAlign_t;

/**
 * @brief Individual item in a FlexBox container
 */
typedef struct {
    int w, h;                /**< Item dimensions */
    int calc_x, calc_y;      /**< Calculated position (set by a_FlexLayout) */
    void* user_data;         /**< Custom data pointer (not managed by FlexBox) */
} FlexItem_t;

/**
 * @brief FlexBox container for automatic layout
 */
typedef struct {
    int x, y;                    /**< Container position */
    int w, h;                    /**< Container dimensions */
    FlexDirection_t direction;   /**< Layout direction */
    FlexJustify_t justify;       /**< Main-axis alignment */
    FlexAlign_t align;           /**< Cross-axis alignment */
    int gap;                     /**< Spacing between items */
    int padding;                 /**< Internal padding */
    void* children;              /**< Internal array of FlexItem_t */
    int dirty;                   /**< Layout needs recalculation */
} FlexBox_t;

/**
 * @brief Create a new FlexBox layout container
 *
 * Creates a FlexBox container at the specified position and dimensions.
 * The container will automatically position child items based on flexbox
 * layout rules (direction, justify, align).
 *
 * @param x Top-left X coordinate of the container
 * @param y Top-left Y coordinate of the container
 * @param w Width of the container in pixels
 * @param h Height of the container in pixels
 * @return Pointer to newly created FlexBox_t, or NULL on allocation failure
 *
 * @note Default direction: FLEX_DIR_ROW (horizontal)
 * @note Default justify: FLEX_JUSTIFY_START (left/top aligned)
 * @note Default align: FLEX_ALIGN_START (cross-axis start)
 * @note Must call a_DestroyFlexBox() when done to free memory
 * @see a_DestroyFlexBox()
 */
FlexBox_t* a_CreateFlexBox(int x, int y, int w, int h);

/**
 * @brief Destroy a FlexBox and free all memory
 *
 * Destroys the FlexBox container and frees all internal allocations.
 * The pointer is set to NULL after destruction.
 *
 * @param box Pointer to FlexBox pointer (will be set to NULL)
 *
 * @note Safe to call with NULL pointer
 * @note Does NOT free user_data pointers stored in items
 * @see a_CreateFlexBox()
 */
void a_DestroyFlexBox(FlexBox_t** box);

/**
 * @brief Set the layout direction (row or column)
 *
 * Controls whether items are arranged horizontally (ROW) or vertically (COLUMN).
 * Marks the layout as dirty, requiring recalculation on next a_FlexLayout() call.
 *
 * @param box FlexBox container to configure
 * @param direction FLEX_DIR_ROW (horizontal) or FLEX_DIR_COLUMN (vertical)
 *
 * @note ROW: items flow left-to-right
 * @note COLUMN: items flow top-to-bottom
 * @see FlexDirection_t
 */
void a_FlexSetDirection(FlexBox_t* box, FlexDirection_t direction);

/**
 * @brief Set main-axis justification (how items are spaced)
 *
 * Controls how items are distributed along the main axis (horizontal for ROW,
 * vertical for COLUMN).
 *
 * @param box FlexBox container to configure
 * @param justify Justification mode (START/CENTER/END/SPACE_BETWEEN/SPACE_AROUND)
 *
 * @note START: items align to left/top
 * @note CENTER: items centered
 * @note END: items align to right/bottom
 * @note SPACE_BETWEEN: equal spacing, edges aligned to container
 * @note SPACE_AROUND: equal spacing including edges
 * @see FlexJustify_t
 */
void a_FlexSetJustify(FlexBox_t* box, FlexJustify_t justify);

/**
 * @brief Set cross-axis alignment
 *
 * Controls how items are aligned perpendicular to the main axis.
 * For ROW: controls vertical alignment. For COLUMN: controls horizontal alignment.
 *
 * @param box FlexBox container to configure
 * @param align Alignment mode (START/CENTER/END)
 *
 * @note START: align to top (ROW) or left (COLUMN)
 * @note CENTER: center items on cross-axis
 * @note END: align to bottom (ROW) or right (COLUMN)
 * @see FlexAlign_t
 */
void a_FlexSetAlign(FlexBox_t* box, FlexAlign_t align);

/**
 * @brief Set spacing between items
 *
 * Sets the gap (in pixels) between adjacent items in the layout.
 *
 * @param box FlexBox container to configure
 * @param gap Spacing between items in pixels (0 for no gap)
 *
 * @note Gap is applied between items, not at edges
 * @note Negative values are allowed but not recommended
 */
void a_FlexSetGap(FlexBox_t* box, int gap);

/**
 * @brief Configure FlexBox direction, justification, and gap in one call
 *
 * Convenience function to set the three most common FlexBox properties.
 * Equivalent to calling a_FlexSetDirection(), a_FlexSetJustify(), and
 * a_FlexSetGap() in sequence.
 *
 * @param box FlexBox container to configure
 * @param direction Layout direction (FLEX_DIR_ROW or FLEX_DIR_COLUMN)
 * @param justify Main-axis justification (START/CENTER/END/SPACE_BETWEEN/SPACE_AROUND)
 * @param gap Spacing between items in pixels
 *
 * @note This is a convenience wrapper - individual set functions still work
 * @note Reduces FlexBox configuration from 3 calls to 1
 * @see a_FlexSetDirection()
 * @see a_FlexSetJustify()
 * @see a_FlexSetGap()
 */
void a_FlexConfigure(FlexBox_t* box, FlexDirection_t direction, FlexJustify_t justify, int gap);

/**
 * @brief Set internal padding of the container
 *
 * Sets uniform padding on all sides of the container's inner area.
 * Items are positioned within the padded area.
 *
 * @param box FlexBox container to configure
 * @param padding Padding in pixels (applied to all sides)
 *
 * @note Reduces available space for items by 2*padding on each axis
 * @note Padding is included in container bounds visualization
 */
void a_FlexSetPadding(FlexBox_t* box, int padding);

/**
 * @brief Update the container's position and dimensions
 *
 * Changes the FlexBox container's absolute position and size.
 * Marks layout as dirty, requiring recalculation.
 *
 * @param box FlexBox container to update
 * @param x New top-left X coordinate
 * @param y New top-left Y coordinate
 * @param w New width in pixels
 * @param h New height in pixels
 *
 * @note Use this to reposition containers dynamically (e.g., from parent FlexBox)
 * @note Common use: `a_FlexSetBounds(row, 0, parent_y + 10, SCREEN_WIDTH, 50)`
 */
void a_FlexSetBounds(FlexBox_t* box, int x, int y, int w, int h);

/**
 * @brief Add an item to the FlexBox
 *
 * Adds a new item with specified dimensions to the container.
 * Position is calculated automatically by a_FlexLayout().
 *
 * @param box FlexBox container
 * @param w Item width in pixels
 * @param h Item height in pixels
 * @param user_data Optional custom data pointer (can be NULL)
 * @return Index of added item, or -1 on failure
 *
 * @note user_data is NOT freed by FlexBox - manage externally
 * @note Item position (calc_x, calc_y) is set by a_FlexLayout()
 * @see a_FlexLayout()
 * @see a_FlexGetItemX()
 * @see a_FlexGetItemY()
 */
int a_FlexAddItem(FlexBox_t* box, int w, int h, void* user_data);

/**
 * @brief Remove an item from the FlexBox
 *
 * Marks an item as removed by setting its dimensions to zero.
 * Does not actually free memory (simple array implementation limitation).
 *
 * @param box FlexBox container
 * @param index Index of item to remove
 *
 * @note Item is marked as 0×0 size, not truly removed
 * @note For full cleanup, use a_FlexClearItems()
 */
void a_FlexRemoveItem(FlexBox_t* box, int index);

/**
 * @brief Clear all items from the FlexBox
 *
 * Removes all items and frees the internal item array.
 * A new empty array is allocated.
 *
 * @param box FlexBox container
 *
 * @note Does NOT free user_data pointers - manage externally
 */
void a_FlexClearItems(FlexBox_t* box);

/**
 * @brief Get the number of items in the FlexBox
 *
 * @param box FlexBox container
 * @return Number of items, or 0 if box is NULL
 */
int a_FlexGetItemCount(const FlexBox_t* box);

/**
 * @brief Get a specific item from the FlexBox
 *
 * Returns a const pointer to the item's data.
 * Access calc_x and calc_y for the item's calculated position.
 *
 * @param box FlexBox container
 * @param index Index of item to retrieve
 * @return Const pointer to FlexItem_t, or NULL if invalid index
 *
 * @note Use a_FlexGetItemX() / a_FlexGetItemY() for convenience
 * @see FlexItem_t
 */
const FlexItem_t* a_FlexGetItem(const FlexBox_t* box, int index);

/**
 * @brief Update an item's dimensions
 *
 * Changes the width and height of an existing item.
 * Marks layout as dirty for recalculation.
 *
 * @param box FlexBox container
 * @param index Index of item to update
 * @param w New width in pixels
 * @param h New height in pixels
 */
void a_FlexUpdateItem(FlexBox_t* box, int index, int w, int h);

/**
 * @brief Calculate and apply FlexBox layout
 *
 * Recalculates item positions based on container settings.
 * Updates calc_x and calc_y for all items.
 *
 * @param box FlexBox container
 * @return 0 on success, -1 on error
 *
 * @note Only recalculates if layout is dirty
 * @note Sets dirty flag to 0 after calculation
 * @note Call this before reading item positions with a_FlexGetItemX/Y()
 */
int a_FlexLayout(FlexBox_t* box);

/**
 * @brief Check if layout needs recalculation
 *
 * @param box FlexBox container
 * @return 1 if layout is dirty (needs recalc), 0 otherwise
 */
int a_FlexIsDirty(const FlexBox_t* box);

/**
 * @brief Get the calculated X position of an item
 *
 * Returns the item's final X coordinate after layout calculation.
 *
 * @param box FlexBox container
 * @param index Index of item
 * @return Calculated X position in pixels, or 0 if invalid
 *
 * @note Call a_FlexLayout() first to ensure position is current
 * @see a_FlexLayout()
 */
int a_FlexGetItemX(const FlexBox_t* box, int index);

/**
 * @brief Get the calculated Y position of an item
 *
 * Returns the item's final Y coordinate after layout calculation.
 *
 * @param box FlexBox container
 * @param index Index of item
 * @return Calculated Y position in pixels, or 0 if invalid
 *
 * @note Call a_FlexLayout() first to ensure position is current
 * @see a_FlexLayout()
 */
int a_FlexGetItemY(const FlexBox_t* box, int index);

/**
 * @brief Render debug visualization of the FlexBox
 *
 * Draws container bounds (white), padding area (yellow), and
 * item rectangles (cyan) for debugging layout issues.
 *
 * @param box FlexBox container
 *
 * @note For debugging only - renders directly to screen
 * @note Container: white outline
 * @note Padding area: yellow outline
 * @note Items: cyan outlines
 */
void a_FlexDebugRender(const FlexBox_t* box);

/** @} */ // end of flexbox group

#endif
