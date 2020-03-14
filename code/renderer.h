#ifndef RENDERER_H
#include "font.h"
#include <lvl5_stretchy_buffer.h>
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
  V4 color;
  M4 matrix;
} Renderer_State;

typedef enum {
  Render_Type_STRING,
  Render_Type_RECT,
  Render_Type_TOKEN,
} Render_Type;

typedef struct {
  union {
    struct {
      String string;
      bool scared;
    } string;
    struct {
      Rect2 rect;
    } rect;
    struct {
      Token *token;
    } token;
  };
  Renderer_State state;
  Render_Type type;
} Render_Item;

typedef struct {
  Renderer_State state;
  V2 window_size;
  GLuint vertex_vbo;
  
  Render_Item *items;
} Renderer;


#define RENDERER_H
#endif