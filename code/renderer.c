#include "renderer.h"
#include <lvl5_random.h>
#include "buffer.c"

void init_renderer(gl_Funcs gl, Renderer *r, GLuint shader, Font *font, V2 window_size) {
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
  
  
  
  r->state.matrix = m4_identity();
  r->state.font = font;
  r->window_size = window_size;
  r->items = sb_new(Render_Item, 1024);
  
  r->vertex_vbo = vertex_vbo;
  r->shader = shader;
}

void renderer_begin_render(Renderer *r, Rect2 rect) {
  V2 ws = r->window_size;
  r->state.matrix = m4_transpose(m4_orthographic(-ws.x*0.5f, ws.x*0.5f, 
                                                 -ws.y*0.5f, ws.y*0.5f,
                                                 1, -1));
  sb_count(r->items) = 0;
  
  V2i min = v2i((i32)(rect.min.x + ws.x*0.5f), (i32)(rect.min.y + ws.y*0.5f));
  V2i size = v2_to_v2i(rect2_get_size(rect));
  glScissor(min.x, min.y, size.x, size.y);
}

void render_scale(Renderer *r, V3 scale) {
  r->state.matrix = m4_mul_m4(r->state.matrix, m4_scaled(scale));
}

void render_translate(Renderer *r, V3 p) {
  r->state.matrix = m4_mul_m4(r->state.matrix, m4_translated(p));
}

void render_rotate(Renderer *r, f32 angle) {
  r->state.matrix = m4_mul_m4(r->state.matrix, m4_rotated(angle));
}


f32 measure_string_width(Renderer *r, String s) {
  Font *font = r->state.font;
  f32 result = 0;
  for (u32 char_index = 0; char_index < s.count; char_index++) {
    result += font_get_advance(font, s.data[char_index], s.data[char_index+1]);
  }
  
  return result;
}


// strings drawn with this MUST have an extra char of padding AFTER count
void draw_string(Renderer *r, String s, V2 p, V4 color) {
  V3 pos = v2_to_v3(p, 0);
  render_translate(r, pos);
  
  Render_Item item = {
    .type = Render_Type_STRING,
    .state = r->state,
  };
  item.state.color = color;
  item.string.string = s;
  sb_push(r->items, item);
  
  render_translate(r, v3_negate(pos));
}

void draw_rect(Renderer *r, Rect2 rect, V4 color) {
  Render_Item item = {
    .type = Render_Type_RECT,
    .state = r->state,
    .rect = rect,
  };
  item.state.color = color;
  sb_push(r->items, item);
}

void draw_rect_outline(Renderer *r, Rect2 rect, f32 thick, V4 color) {
  V2 size = rect2_get_size(rect);
  
  draw_rect(r, rect2_min_size(rect.min, v2(size.x, thick)), color);
  draw_rect(r, rect2_min_size(rect.min, v2(thick, size.y)), color);
  draw_rect(r, rect2_min_size(v2(rect.min.x, rect.max.y-thick), 
                              v2(size.x, thick)), color);
  draw_rect(r, rect2_min_size(v2(rect.max.x-thick, rect.min.y),
                              v2(thick, size.y)), color);
}

void draw_buffer_view(Renderer *r, Rect2 rect, Buffer_View *view, Color_Theme *theme, V2 scroll) {
  Render_Item item = {
    .type = Render_Type_BUFFER,
    .state = r->state,
  };
  item.buffer.theme = theme;
  item.buffer.rect = rect;
  item.buffer.scroll = scroll;
  item.buffer.view = view;
  
  draw_rect(r, rect, color_u32_to_v4(theme->colors[Syntax_BACKGROUND]));
  sb_push(r->items, item);
}

void renderer_end_render(gl_Funcs gl, Renderer *r) {
  begin_profiler_event("renderer_output");
  
  push_scratch_context();
  
  Quad_Instance *instances = sb_new(Quad_Instance, 10000);
  
  for (u32 item_index = 0; item_index < sb_count(r->items); item_index++) {
    Render_Item *item = r->items + item_index;
    Font *font = item->state.font;
    
    switch (item->type) {
      case Render_Type_STRING: {
        M4 matrix = item->state.matrix;
        String s = item->string.string;
        V2 offset = v2_zero();
        
        for (u32 char_index = 0; char_index < s.count; char_index++) {
          char first = s.data[char_index] - font->first_codepoint;
          
          Rect2i rect = font->atlas.rects[first];
          V2 origin = font->origins[first];
          
          u16 width = (u16)(rect.max.x - rect.min.x);
          u16 height = (u16)(rect.max.y - rect.min.y);
          
          
          M4 m = matrix;
          m = m4_mul_m4(m, m4_translated(v3(offset.x + origin.x,
                                            offset.y + origin.y, 
                                            0)));
          m = m4_mul_m4(m, m4_scaled(v3(width, height, 1)));
          
          
          Quad_Instance inst = {
            .matrix = m,
            .texture_x = (u16)rect.min.x,
            .texture_y = (u16)rect.min.y,
            .texture_w = width,
            .texture_h = height,
            .color = color_v4_to_opengl_u32(item->state.color),
          };
          
          sb_push(instances, inst);
          
          // token strings are guaranteed to have one additional char
          // in the end for kerning
          i8 advance = font_get_advance(font, s.data[char_index], s.data[char_index+1]);
          offset.x += advance;
        }
      } break;
      
      case Render_Type_RECT: {
        Rect2 rect = item->rect.rect;
        
        V2 size = rect2_get_size(rect);
        
        M4 m = item->state.matrix;
        m = m4_mul_m4(m, m4_translated(v3(rect.min.x,
                                          rect.min.y, 
                                          0)));
        m = m4_mul_m4(m, m4_scaled(v3(size.x, size.y, 1)));
        
        
        
        Rect2i sprite_rect = r->state.font->atlas.rects[r->state.font->atlas.count-1];
        
        Quad_Instance inst = {
          .matrix = m,
          .texture_x = (u16)sprite_rect.min.x,
          .texture_y = (u16)sprite_rect.min.y,
          .texture_w = 1,
          .texture_h = 1,
          .color = color_v4_to_opengl_u32(item->state.color),
        };
        sb_push(instances, inst);
      } break;
      
      case Render_Type_BUFFER: {
        begin_profiler_event("render_buffer");
        
        M4 matrix = item->state.matrix;
        Font *font = item->state.font;
        
        Buffer_View *view = item->buffer.view;
        Buffer *buffer = view->buffer;
        Color_Theme *theme = item->buffer.theme;
        V2 scroll = item->buffer.scroll;
        Rect2 rect = item->buffer.rect;
        
        V2 offset = v2(rect.min.x, rect.max.y);
        i32 gap_start = get_gap_start(buffer);
        i32 gap_count = get_gap_count(buffer);
        
        i32 added = 0;
        for (i32 char_index_relative = 0;
             char_index_relative < buffer->count; // last symbol is 0
             char_index_relative++) 
        {
          if (char_index_relative == gap_start) {
            added = gap_count;
          }
          i32 char_index = char_index_relative + added;
          
          char first = buffer->data[char_index] - font->first_codepoint;
          if (first == '\n') {
            offset.x = rect.min.x;
            offset.y -= font->line_spacing;
            if (offset.y < -500) {
              goto end;
            }
            continue;
          }
          
          Rect2i rect = font->atlas.rects[first];
          V2 origin = font->origins[first];
          
          u16 width = (u16)(rect.max.x - rect.min.x);
          u16 height = (u16)(rect.max.y - rect.min.y);
          
          
          M4 m = matrix;
          m.e30 += (offset.x + origin.x)*m.e00;
          m.e31 += (offset.y + origin.y)*m.e11;
          m.e00 *= width;
          m.e11 *= height;
          
          
          i8 advance = font_get_advance(font, buffer->data[char_index], buffer->data[char_index+1]);
          offset.x += advance;
          
          Syntax syntax = buffer->colors[char_index_relative];
          Quad_Instance inst = {
            .matrix = m,
            .texture_x = (u16)rect.min.x,
            .texture_y = (u16)rect.min.y,
            .texture_w = width,
            .texture_h = height,
            .color = theme->colors[syntax],
          };
          
          sb_push(instances, inst);
        }
        
        
        end:
        end_profiler_event("render_buffer");
      } break;
    }
  }
  
  gl.UseProgram(r->shader);
  
  
  //gl_set_uniform_m4(gl, r->shader, "u_projection", &u_projection, 1);
  
  gl.BindBuffer(GL_ARRAY_BUFFER, r->vertex_vbo);
  gl.BufferData(GL_ARRAY_BUFFER, sizeof(Quad_Instance)*sb_count(instances), instances, GL_DYNAMIC_DRAW);
  
  gl.DrawArraysInstanced(GL_TRIANGLES, 0, 6, sb_count(instances));
  
  pop_context();
  
  end_profiler_event("renderer_output");
}
