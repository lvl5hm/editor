#include "renderer.h"

void init_renderer(Renderer *r, GLuint shader, Font *font, V2 window_size) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_MULTISAMPLE);
  
  GLuint texture;
  gl.GenTextures(1, &texture);
  gl.BindTexture(GL_TEXTURE_2D, texture);
  
  gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  
  // disables texture multisampling
  // GL_LINEAR to enable
  gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font->atlas.bmp.width, font->atlas.bmp.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, font->atlas.bmp.data);
  
  f32 quad_vertices[] = {
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f,  1.0f,
    
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
  };
  u32 quad_vbo;
  gl.GenBuffers(1, &quad_vbo);
  
  u32 vertex_vbo;
  gl.GenBuffers(1, &vertex_vbo);
  
  
  GLuint vao;
  gl.GenVertexArrays(1, &vao);
  gl.BindVertexArray(vao);
  
  gl.UseProgram(shader);
  
  gl.BindBuffer(GL_ARRAY_BUFFER, quad_vbo);
  gl.EnableVertexAttribArray(0);
  gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
  
  gl.BufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
  
  gl.BindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
  gl.EnableVertexAttribArray(1);
  gl.EnableVertexAttribArray(2);
  gl.EnableVertexAttribArray(3);
  gl.EnableVertexAttribArray(4);
  gl.EnableVertexAttribArray(5);
  gl.EnableVertexAttribArray(6);
  
  gl.VertexAttribDivisor(1, 1);
  gl.VertexAttribDivisor(2, 1);
  gl.VertexAttribDivisor(3, 1);
  gl.VertexAttribDivisor(4, 1);
  gl.VertexAttribDivisor(5, 1);
  gl.VertexAttribDivisor(6, 1);
  
  gl.VertexAttribPointer(
    1, 4, GL_SHORT, GL_FALSE, sizeof(Quad_Instance), (void *)offsetof(Quad_Instance, texture_x));
  
  gl.VertexAttribPointer(
    2, 4, GL_FLOAT, GL_FALSE, sizeof(Quad_Instance),
    (void *)offsetof(Quad_Instance, matrix));
  gl.VertexAttribPointer(
    3, 4, GL_FLOAT, GL_FALSE, sizeof(Quad_Instance),
    (void *)(offsetof(Quad_Instance, matrix) + sizeof(f32)*4));
  gl.VertexAttribPointer(
    4, 4, GL_FLOAT, GL_FALSE, sizeof(Quad_Instance),
    (void *)(offsetof(Quad_Instance, matrix) + sizeof(f32)*8));
  gl.VertexAttribPointer(
    5, 4, GL_FLOAT, GL_FALSE, sizeof(Quad_Instance),
    (void *)(offsetof(Quad_Instance, matrix) + sizeof(f32)*12));
  gl.VertexAttribPointer(
    6, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Quad_Instance),
    (void *)offsetof(Quad_Instance, color));
  
  
  
  r->state.font = font;
  r->window_size = window_size;
  r->vertex_vbo = vertex_vbo;
}

V2 draw_string(Renderer *r, String s, V2 pos, V4 color) {
  Font *font = r->state.font;
  V2 offset = pos;
  
  Quad_Instance *instances = scratch_push_array(Quad_Instance, s.count);
  i32 instance_count = 0;
  
  M4 matrix = m4_identity();
  V3 scale = v3(2/r->window_size.x, 2/r->window_size.y, 1);
  
  for (u32 char_index = 0; char_index < s.count; char_index++) {
    if (s.data[char_index] == '#') {
      int brk = 32;
    }
    char first = s.data[char_index] - font->first_codepoint;
    if (s.data[char_index] == '\n') {
      offset.x = pos.x;
      offset.y -= font->line_spacing;
      continue;
    }
    
    char second = s.data[char_index+1] - font->first_codepoint;
    
    Rect2i rect = font->atlas.rects[first];
    V2 origin = font->origins[first];
    
    u16 width = (u16)(rect.max.x - rect.min.x);
    u16 height = (u16)(rect.max.y - rect.min.y);
    
    M4 m = matrix;
    m = m4_scale(m, v3(width, height, 1));
    m = m4_translate(m, v3(offset.x + origin.x,
                           offset.y + origin.y,
                           0));
    m = m4_scale(m, scale);
    
    Quad_Instance inst = {
      .matrix = m,
      .texture_x = (u16)rect.min.x,
      .texture_y = (u16)rect.min.y,
      .texture_w = width,
      .texture_h = height,
      .color = color_v4_to_opengl_u32(color),
    };
    
    instances[instance_count++] = inst;
    
    i8 advance = font->advance[first*font->codepoint_count+second];
    offset.x += advance;
  }
  
  gl.BindBuffer(GL_ARRAY_BUFFER, r->vertex_vbo);
  gl.BufferData(GL_ARRAY_BUFFER, sizeof(Quad_Instance)*instance_count, instances, GL_DYNAMIC_DRAW);
  
  gl.DrawArraysInstanced(GL_TRIANGLES, 0, 6, instance_count);
  
  V2 result = v2_sub(offset, pos);
  return result;
}

void draw_rect(Renderer *r, Rect2 rect, V4 color) {
  Font *font = r->state.font;
  
  V3 scale = v3_mul(v3(2/r->window_size.x, 2/r->window_size.y, 1), 1);
  V2 size = rect2_get_size(rect);
  
  M4 m = m4_identity();
  m = m4_scale(m, v3(size.x, size.y, 1));
  m = m4_translate(m, v3(rect.min.x, rect.min.y, 0));
  m = m4_scale(m, scale);
  
  Rect2i sprite_rect = r->state.font->atlas.rects[r->state.font->atlas.count-1];
  
  Quad_Instance inst = {
    .matrix = m,
    .texture_x = (u16)sprite_rect.min.x,
    .texture_y = (u16)sprite_rect.min.y,
    .texture_w = 1,
    .texture_h = 1,
    .color = color_v4_to_opengl_u32(color),
  };
  Quad_Instance instances[1] = {inst};
  
  
  gl.BindBuffer(GL_ARRAY_BUFFER, r->vertex_vbo);
  gl.BufferData(GL_ARRAY_BUFFER, sizeof(Quad_Instance), instances, GL_DYNAMIC_DRAW);
  
  gl.DrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
}
