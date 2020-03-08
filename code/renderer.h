#ifndef RENDERER_H
#include "font.h"

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
} Renderer_State;

typedef struct {
  Renderer_State state;
  V2 window_size;
  GLuint vertex_vbo;
} Renderer;


#define RENDERER_H
#endif