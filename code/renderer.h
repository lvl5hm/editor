#ifndef RENDERER_H
#include "parser.h"
#include <lvl5_opengl.h>

typedef struct Quad_Instance {
  u16 texture_x;
  u16 texture_y;
  u16 texture_w;
  u16 texture_h;
  M4 matrix;
  u32 color;
} Quad_Instance;

typedef struct {
  Font *font;
  u32 color;
  M4 matrix;
} Renderer_State;

typedef enum {
  Render_Type_NONE,
  Render_Type_STRING,
  Render_Type_RECT,
  Render_Type_BUFFER,
} Render_Type;

typedef struct Buffer_View Buffer_View;
typedef struct Color_Theme Color_Theme;

typedef struct {
  union {
    struct {
      String string;
    } string;
    struct {
      Rect2 rect;
    } rect;
    struct {
      Rect2 rect;
      Buffer_View *view;
      Color_Theme *theme;
      V2 *scroll;
    } buffer;
  };
  Renderer_State state;
  Render_Type type;
} Render_Item;

typedef struct {
  Renderer_State state;
  V2 window_size;
  
  Render_Item *items;
  
  GLuint vertex_vbo;
  GLuint shader;
} Renderer;


#define RENDERER_H
#endif