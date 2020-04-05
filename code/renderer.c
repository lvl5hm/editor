#include "renderer.h"
#include "lvl5_random.h"
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

void render_scale(Renderer *r, V2 scale) {
  r->state.matrix = m4_unscale(r->state.matrix, v2_to_v3(scale, 1.0f));
}

void render_translate(Renderer *r, V2 p) {
  r->state.matrix = m4_untranslate(r->state.matrix, v2_to_v3(p, 0.0f));
}

void render_rotate(Renderer *r, f32 angle) {
  r->state.matrix = m4_unrotate(r->state.matrix, angle);
}

void render_save(Renderer *r) {
  r->stack[r->stack_count++] = r->state;
}

void render_restore(Renderer *r) {
  r->state = r->stack[--r->stack_count];
}

f32 measure_string_width(Renderer *r, String s) {
  Font *font = r->state.font;
  f32 result = 0;
  for (u32 char_index = 0; char_index < s.count - 1; char_index++) {
    result += font_get_advance(font, s.data[char_index], s.data[char_index+1]);
  }
  result += font->advance[s.data[s.count - 1] - font->first_codepoint];
  
  return result;
}


// strings drawn with this MUST have an extra char of padding AFTER count
void draw_string(Renderer *r, String s, V2 p, u32 color) {
  render_translate(r, p);
  
  Render_Item item = {
    .type = Render_Type_STRING,
    .state = r->state,
  };
  item.state.color = color;
  item.string.string = s;
  sb_push(r->items, item);
  
  render_translate(r, v2_negate(p));
}

void draw_rect(Renderer *r, Rect2 rect, u32 color) {
  Render_Item item = {
    .type = Render_Type_RECT,
    .state = r->state,
    .rect = rect,
  };
  item.state.color = color;
  sb_push(r->items, item);
}

void draw_rect_outline(Renderer *r, Rect2 rect, f32 thick, u32 color) {
  V2 size = rect2_get_size(rect);
  
  draw_rect(r, rect2_min_size(rect.min, v2(size.x, thick)), color);
  draw_rect(r, rect2_min_size(rect.min, v2(thick, size.y)), color);
  draw_rect(r, rect2_min_size(v2(rect.min.x, rect.max.y-thick), 
                              v2(size.x, thick)), color);
  draw_rect(r, rect2_min_size(v2(rect.max.x-thick, rect.min.y),
                              v2(thick, size.y)), color);
}

void draw_line(Renderer *r, V2 start, V2 end, f32 thick, u32 color) {
  V2 line = v2_sub(end, start);
  f32 length = v2_length(line);
  f32 angle = atan2_f32(line.y, line.x);
  
  render_save(r);
  render_translate(r, start);
  render_rotate(r, angle);
  
  
  Rect2 rect = rect2_min_size(v2(0, 0 - thick*0.5f), 
                              v2(length, thick));
  draw_rect(r, rect, color);
  
  render_restore(r);
}

void draw_arrow_outline(Renderer *r, V2 p, V2 size, f32 thick, u32 color) {
  V2 bottom_left = v2(p.x - size.x*0.5f, p.y - size.y*0.5f);
  V2 top_left = v2_add(bottom_left, v2(0, size.y));
  V2 middle_right = v2(p.x + size.x*0.5f, p.y);
  
  draw_line(r, bottom_left, middle_right, thick, color);
  draw_line(r, top_left, middle_right, thick, color);
}

void draw_buffer_view(Renderer *r, Rect2 rect, Buffer_View *view, Color_Theme *theme, V2 *scroll) {
  Render_Item item = {
    .type = Render_Type_BUFFER,
    .state = r->state,
  };
  item.buffer.theme = theme;
  item.buffer.rect = rect;
  item.buffer.scroll = scroll;
  item.buffer.view = view;
  
  sb_push(r->items, item);
}


void queue_rect(Quad_Instance *instances, Rect2 rect, Renderer_State state) {
  V2 size = rect2_get_size(rect);
  M4 m = state.matrix;
  m = m4_mul_m4(m, m4_translated(v3(rect.min.x,
                                    rect.min.y, 
                                    0)));
  m = m4_mul_m4(m, m4_scaled(v3(size.x, size.y, 1)));
  Rect2i sprite_rect = state.font->atlas.rects[state.font->atlas.count-1];
  
  Quad_Instance inst = {
    .matrix = m,
    .texture_x = (u16)sprite_rect.min.x,
    .texture_y = (u16)sprite_rect.min.y,
    .texture_w = 1,
    .texture_h = 1,
    .color = state.color,
  };
  sb_push(instances, inst);
}

void renderer_end_render(gl_Funcs gl, Renderer *r) {
  begin_profiler_function();
  
  push_scratch_context();
  
  Quad_Instance *instances = sb_new(Quad_Instance, 10000);
  
  u32 item_count = sb_count(r->items);
  {
    u32 i = 1;
    while (i < item_count) {
      Render_Item x = r->items[i];
      u32 j = i - 1;
      while (j >= 0 && r->items[j].state.z > x.state.z) {
        r->items[j+1] = r->items[j];
        j--;
      }
      r->items[j+1] = x;
      i++;
    }
  }
  
  for (u32 item_index = 0; item_index < item_count; item_index++) {
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
            .color = item->state.color,
          };
          
          sb_push(instances, inst);
          
          if (char_index != s.count - 1) {
            i8 advance = font_get_advance(font, s.data[char_index], s.data[char_index+1]);
            offset.x += advance;
          }
        }
      } break;
      
      case Render_Type_RECT: {
        queue_rect(instances,
                   item->rect.rect,
                   item->state);
      } break;
      
      case Render_Type_BUFFER: {
        M4 matrix = item->state.matrix;
        Font *font = item->state.font;
        
        Buffer_View *view = item->buffer.view;
        Buffer *buffer = view->buffer;
        Color_Theme *theme = item->buffer.theme;
        V2 *scroll = item->buffer.scroll;
        Rect2 buffer_rect = item->buffer.rect;
        
        V2 offset = v2(buffer_rect.min.x,
                       buffer_rect.max.y + scroll->y*font->line_spacing);
        i32 gap_start = get_gap_start(buffer);
        i32 gap_count = get_gap_count(buffer);
        
        f32 PADDING = 4.0f;
        
        f32 lines_on_screen = r->window_size.y/font->line_spacing;
        f32 border_top = scroll->y + PADDING;
        
        // TODO(lvl5): something is wrong with border_bottom
        f32 border_bottom = scroll->y + lines_on_screen - PADDING;
        
        i32 line_index = 0;
        
        i32 added = 0;
        
        bool cursor_rendered = false;
        
        Syntax *buffer_colors = buffer->cache.colors;
        
        for (i32 char_index_relative = 0;
             char_index_relative < buffer->count; // last symbol is 0
             char_index_relative++) 
        {
          if (char_index_relative == gap_start) {
            added = gap_count;
          }
          i32 char_index = char_index_relative + added;
          char first = buffer->data[char_index] - font->first_codepoint;
          
          u32 char_color = theme->colors[buffer_colors[char_index_relative]];
          
          i8 advance = font_get_advance(font, buffer->data[char_index], buffer->data[char_index+1]);
          
          if (char_index_relative == buffer->cursor) {
            f32 cursor_y = offset.y-font->line_spacing - font->descent;
            V2 cursor_min = v2(offset.x,
                               cursor_y);
            V2 cursor_size = v2((f32)advance, font->line_height);
            Rect2 cursor_rect = 
              rect2_min_size(cursor_min, cursor_size);
            
            u32 cursor_color = theme->colors[Syntax_CURSOR];
            queue_rect(instances, cursor_rect,
                       (Renderer_State){
                       .matrix = matrix,
                       .font = font,
                       .color = cursor_color,
                       });
            char_color = color_invert(cursor_color);
            
            f32 target = 0;
            if (line_index > border_bottom) {
              target = line_index - border_bottom;
            } else if (line_index < border_top ) {
              target = line_index - border_top;
            }
            scroll->y += target/6;
            
            if (scroll->y < 0) {
              scroll->y = 0;
            }
            cursor_rendered = true;
          } else if (char_index_relative == buffer->mark) {
            V2 cursor_min = v2(offset.x,
                               offset.y-font->line_spacing - font->descent);
            V2 cursor_size = v2((f32)advance, font->line_height);
            Rect2 cursor_rect = 
              rect2_min_size(cursor_min, cursor_size);
            
            u32 cursor_color = theme->colors[Syntax_CURSOR];
            
            Renderer_State state = {
              .matrix = matrix,
              .font = font,
              .color = cursor_color,
            };
            
            f32 thick = 1.0f;
            V2 size = rect2_get_size(cursor_rect);
            
            queue_rect(instances,
                       rect2_min_size(cursor_rect.min, v2(size.x, thick)), state);
            queue_rect(instances,
                       rect2_min_size(cursor_rect.min, v2(thick, size.y)), state);
            queue_rect(instances, rect2_min_size(v2(cursor_rect.min.x, cursor_rect.max.y-thick), 
                                                 v2(size.x, thick)), state);
            queue_rect(instances, rect2_min_size(v2(cursor_rect.max.x-thick, cursor_rect.min.y),
                                                 v2(thick, size.y)), state);
          }
          if (first == '\n') {
            offset.x = buffer_rect.min.x;
            offset.y -= font->line_spacing;
            line_index++;
            if (offset.y < -r->window_size.y*0.5f && cursor_rendered) {
              goto end;
            }
            continue;
          }
          
          if (line_index < scroll->y - 1) {
            continue;
          }
          
          Rect2i rect = font->atlas.rects[first];
          V2 origin = font->origins[first];
          
          u16 width = (u16)(rect.max.x - rect.min.x);
          u16 height = (u16)(rect.max.y - rect.min.y);
          
          M4 m = matrix;
          m = m4_untranslate(m, v2_to_v3(v2_add(offset, origin), 0));
          m = m4_unscale(m, v3(width, height, 1));
          Quad_Instance inst = {
            .matrix = m,
            .texture_x = (u16)rect.min.x,
            .texture_y = (u16)rect.min.y,
            .texture_w = width,
            .texture_h = height,
            .color = char_color,
          };
          
          sb_push(instances, inst);
          offset.x += advance;
        }
        
        
        end:;
      } break;
    }
  }
  
  gl.UseProgram(r->shader);
  
  
  //gl_set_uniform_m4(gl, r->shader, "u_projection", &u_projection, 1);
  
  gl.BindBuffer(GL_ARRAY_BUFFER, r->vertex_vbo);
  gl.BufferData(GL_ARRAY_BUFFER, sizeof(Quad_Instance)*sb_count(instances), instances, GL_DYNAMIC_DRAW);
  
  gl.DrawArraysInstanced(GL_TRIANGLES, 0, 6, sb_count(instances));
  
  pop_context();
  
  end_profiler_function();
}
