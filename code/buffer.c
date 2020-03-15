#include "buffer.h"
#include <lvl5_stretchy_buffer.h>
#include "parser.c"
#include "font.c"
#include "renderer.c"

inline i32 get_gap_count(Text_Buffer *b) {
  i32 result = b->capacity - b->count;
  return result;
}

inline i32 get_gap_start(Text_Buffer *b) {
  i32 gap_count = get_gap_count(b);
  i32 result = b->cursor;
  assert(result <= b->capacity - gap_count);
  return result;
}

inline i32 get_buffer_pos(Text_Buffer *b, i32 pos) {
  i32 gap_start = get_gap_start(b);
  i32 gap_count = get_gap_count(b);
  
  i32 result = pos + (pos >= gap_start)*gap_count;
  
  return result;
}

char get_buffer_char(Text_Buffer *b, i32 pos) {
  char result = b->data[get_buffer_pos(b, pos)];
  return result;
}

V2 get_buffer_xy(Text_Buffer *b, i32 pos) {
  V2 result = v2_zero();
  
  for (i32 char_index = 0; char_index < pos; char_index++) {
    char buffer_char = get_buffer_char(b, char_index);
    if (buffer_char == '\n') {
      result.x = 0;
      result.y++;
    } else {
      result.x++;
    }
  }
  
  return result;
}

V2 get_screen_position_in_buffer(Font *font, Text_Buffer *b, i32 pos) {
  V2 result = v2(0, -(f32)font->line_spacing);
  
  for (i32 char_index = 0; char_index < pos; char_index++) {
    char first = get_buffer_char(b, char_index);
    char second = get_buffer_char(b, char_index+1);
    if (first == '\n') {
      result.x = 0;
      result.y -= font->line_spacing;
      continue;
    }
    
    result.x += font_get_advance(font, first, second);
  }
  return result;
}

i32 seek_line_start(Text_Buffer *b, i32 start) {
  i32 result = max(start, 0);
  while (result > 0 && get_buffer_char(b, result-1) != '\n') {
    result--;
  }
  return result;
}

i32 seek_line_end(Text_Buffer *b, i32 start) {
  i32 result = min(start, b->count - 1);
  while (result < (i32)b->count-1 && get_buffer_char(b, result) != '\n') {
    result++;
  }
  return result;
}

void set_cursor(Text_Buffer *b, i32 pos) {
  assert(pos >= 0 && pos < b->count);
  i32 old_gap_start = get_gap_start(b);
  b->cursor = pos;
  i32 gap_start = get_gap_start(b);
  i32 moved_by = gap_start - old_gap_start;
  
  if (moved_by != 0) {
    // move chars from one gap end to the other
    i32 gap_count = get_gap_count(b);
    
    if (moved_by < 0) {
      for (i32 i = 0; i < -moved_by; i++) {
        b->data[old_gap_start+gap_count-1-i] = b->data[old_gap_start-1-i];
      }
    } else {
      for (i32 i = 0; i < moved_by; i++) {
        b->data[old_gap_start+i] = b->data[old_gap_start+gap_count+i];
      }
    }
    
    for (i32 i = 0; i < gap_count; i++) {
      b->data[gap_start+i] = '|';
    }
  }
}

void buffer_changed(Text_Buffer *b) {
  if (b->tokens) {
    sb_free(b->tokens);
  }
  b->tokens = buffer_parse(b);
}

void buffer_insert_string(Text_Buffer *b, String str) {
  if (b->count + (i32)str.count > b->capacity) {
    char *old_data = b->data;
    i32 old_gap_start = get_gap_start(b);
    i32 old_gap_count = get_gap_count(b);
    
    while (b->count + (i32)str.count > b->capacity) {
      b->capacity = b->capacity*2;
    }
    b->data = alloc_array(char, b->capacity);
    i32 gap_start = get_gap_start(b);
    i32 gap_count = get_gap_count(b);
    
    i32 first_count = min(gap_start, b->count);
    for (i32 i = 0; i < first_count; i++) {
      b->data[i] = old_data[i];
    }
    
    i32 second_count = b->count - first_count;
    for (i32 i = 0; i < second_count; i++) {
      b->data[gap_start+gap_count+i] = old_data[old_gap_start+old_gap_count+i];
    }
  }
  
  
  for (i32 i = 0; i < (i32)str.count; i++) {
    b->data[b->cursor+i] = str.data[i];
  }
  
  if (b->mark > b->cursor) {
    b->mark += (i32)str.count;
  }
  b->cursor += (i32)str.count;
  b->count += (i32)str.count;
  
  buffer_changed(b);
}

void buffer_remove_backward(Text_Buffer *b, i32 count) {
  assert(b->cursor - count >= 0);
  
  if (b->mark >= b->cursor) {
    b->mark -= count;
  }
  b->cursor -= count;
  b->count -= count;
  
  buffer_changed(b);
}

void buffer_remove_forward(Text_Buffer *b, i32 count) {
  assert(b->cursor < b->count - 1);
  
  if (b->mark > b->cursor) {
    b->mark -= count;
  }
  b->count -= count;
  
  buffer_changed(b);
}

f32 get_pixel_position_in_line(Font *font, Text_Buffer *b, i32 pos) {
  i32 line_start = seek_line_start(b, pos);
  f32 result = 0;
  for (i32 i = line_start; i < pos; i++) {
    result += font_get_advance(font, 
                               get_buffer_char(b, i),
                               get_buffer_char(b, i+1));
  }
  return result;
}

b32 move_cursor_direction(Font *font, Text_Buffer *b, os_Keycode direction) {
  b32 result = false;
  i32 cursor = b->cursor;
  switch (direction) {
    case os_Keycode_ARROW_RIGHT: {
      if (cursor < b->count-1) {
        cursor++;
        b->preferred_col_pos = cursor;
        result = true;
      }
    } break;
    
    case os_Keycode_ARROW_LEFT: {
      if (cursor > 0) {
        cursor--;
        b->preferred_col_pos = cursor;
        result = true;
      }
    } break;
    
    case os_Keycode_ARROW_DOWN: {
      i32 line_start = seek_line_start(b, cursor);
      
      f32 cur_pixel = get_screen_position_in_buffer(font, b, b->preferred_col_pos).x;
      i32 line_end = seek_line_end(b, cursor);
      
      i32 want = line_end + 1;
      f32 want_pixel = 0;
      while (get_buffer_char(b, want) != '\n') {
        i8 advance = font_get_advance(font, 
                                      get_buffer_char(b, want), 
                                      get_buffer_char(b, want+1));
        if (want_pixel + advance > cur_pixel) {
          if (cur_pixel - want_pixel < want_pixel + advance - cur_pixel) {
            break;
          } else {
            want++;
            break;
          }
        }
        want_pixel += advance;
        want++;
      }
      
      cursor = want;
    } break;
    
    case os_Keycode_ARROW_UP: {
      i32 line_start = seek_line_start(b, cursor);
      
      f32 cur_pixel = get_screen_position_in_buffer(font, b, b->preferred_col_pos).x;
      i32 line_end = seek_line_end(b, cursor);
      
      i32 want = seek_line_start(b, line_start-1);
      f32 want_pixel = 0;
      while (want + 1 < line_start) {
        i8 advance = font_get_advance(font, 
                                      get_buffer_char(b, want), 
                                      get_buffer_char(b, want+1));
        if (want_pixel + advance > cur_pixel) {
          if (cur_pixel - want_pixel < want_pixel + advance - cur_pixel) {
            break;
          } else {
            want++;
            break;
          }
        }
        want_pixel += advance;
        want++;
      }
      
      cursor = want;
    } break;
    
    default: assert(false);
  }
  set_cursor(b, cursor);
  
  return result;
}

String buffer_part_to_string(Text_Buffer *buffer, i32 start, i32 end) {
  begin_profiler_event("buffer_part_to_string");
  String str = {0};
  str.count = end - start;
  str.data = scratch_push_array(char, str.count);
  for (i32 i = 0; i < (i32)str.count; i++) {
    str.data[i] = get_buffer_char(buffer, start + i);
  }
  
  end_profiler_event("buffer_part_to_string");
  return str;
}

String text_buffer_to_string(Text_Buffer *buffer) {
  String result = buffer_part_to_string(buffer, 0, buffer->count);
  return result;
}

void buffer_copy(Text_Buffer *buffer) {
  i32 start = min(buffer->cursor, buffer->mark);
  i32 end = max(buffer->cursor, buffer->mark);
  buffer->exchange_count = end - start;
  assert(buffer->exchange_count <= MAX_EXCHANGE_COUNT);
  for (i32 i = 0; i < buffer->exchange_count; i++) {
    buffer->exchange[i] = get_buffer_char(buffer, start+i);
  }
}

void buffer_paste(Text_Buffer *buffer) {
  String str = make_string(buffer->exchange, buffer->exchange_count);
  buffer_insert_string(buffer, str);
}

void buffer_cut(Text_Buffer *buffer) {
  buffer_copy(buffer);
  i32 count = buffer->cursor - buffer->mark;
  if (buffer->cursor < buffer->mark) {
    i32 tmp = buffer->cursor;
    buffer->cursor = buffer->mark;
    buffer->mark = tmp;
    count *= -1;
  }
  buffer_remove_backward(buffer, count);
}


void buffer_newline(Text_Buffer *buffer) {
  i32 line_start = seek_line_start(buffer, buffer->cursor);
  String indent_str = { .count = 1 };
  while (get_buffer_char(buffer, 
                         line_start + (i32)indent_str.count-1) == ' ') {
    indent_str.count++;
  }
  
  
  i32 first_non_space_before = buffer->cursor;
  while (get_buffer_char(buffer, first_non_space_before) == ' ' ||
         get_buffer_char(buffer, first_non_space_before) == '\n') {
    first_non_space_before--;
  }
  if (get_buffer_char(buffer, first_non_space_before) == '{') {
    indent_str.count += 2;
  }
  
  indent_str.data = scratch_push_array(char, indent_str.count);
  indent_str.data[0] = '\n';
  for (i32 i = 1; i < (i32)indent_str.count; i++) {
    indent_str.data[i] = ' ';
  }
  
  buffer_insert_string(buffer, indent_str);
}

void buffer_indent(Text_Buffer *buffer) {
  String indent_str = make_string(scratch_push_array(char, 2), 2);
  for (i32 i = 0; i < (i32)indent_str.count; i++) {
    indent_str.data[i] = ' ';
  }
  
  buffer_insert_string(buffer, indent_str);
}

void buffer_input_string(Text_Buffer *buffer, String str) {
  if (str.data[0] == '}') {
    i32 start = seek_line_start(buffer, buffer->cursor);
    bool only_indent = true;
    for (i32 i = start; i < buffer->cursor; i++) {
      if (get_buffer_char(buffer, i) != ' ') {
        only_indent = false;
        break;
      }
    }
    if (only_indent && buffer->cursor >= 2) {
      buffer_remove_backward(buffer, 2);
    }
  }
  if (str.data[0] != '\t' && 
      str.data[0] != '\b' && 
      str.data[0] != '\r') {
    buffer_insert_string(buffer, str);
  }
}

// TODO: if we place the gap before or after visible buffer part,
// we won't have to check where we are reaing token chars from

#define PADDING 4

void buffer_draw(Renderer *renderer, Text_Buffer *buffer, Rect2 rect, Color_Theme theme) {
  begin_profiler_event("buffer_draw");
  
  Font *font = renderer->state.font;
  i8 line_spacing = font->line_spacing;
  
  
  V2 rect_size = rect2_get_size(rect);
  i32 first_visible_line = (i32)buffer->scroll_y;
  i32 height_lines = (i32)(rect_size.y / line_spacing);
  
  
  draw_rect(renderer, rect, color_u32_to_v4(theme.colors[Text_Color_Type_BACKGROUND]));
  
  
  {
    // draw cursor and marker
    V2 cursor_p = get_screen_position_in_buffer(font, buffer, buffer->cursor);
    f32 cursor_line = -cursor_p.y / font->line_spacing;
    f32 border_top = buffer->scroll_y + PADDING;
    f32 border_bottom = buffer->scroll_y + height_lines - PADDING;
    
    f32 want_scroll_y = 0;
    if (cursor_line > border_bottom) {
      want_scroll_y = cursor_line - border_bottom;
    } else if (cursor_line < border_top && buffer->scroll_y > 0) {
      want_scroll_y = cursor_line - border_top;
    }
    buffer->scroll_y += want_scroll_y*0.25f;
    
    
    V2 top_left = v2(rect.min.x, rect.max.y + buffer->scroll_y*line_spacing);
    
    
    
    i8 cursor_width = font_get_advance(font, 
                                       get_buffer_char(buffer, buffer->cursor),
                                       get_buffer_char(buffer, buffer->cursor+1));
    draw_rect(renderer, 
              rect2_min_size(v2_add(top_left, cursor_p), v2(cursor_width, line_spacing)), 
              v4(0, 1, 0, 1));
    
    
    i8 mark_width = font_get_advance(font, 
                                     get_buffer_char(buffer, buffer->mark),
                                     get_buffer_char(buffer, buffer->mark+1));
    V2 mark_p = get_screen_position_in_buffer(font, buffer, buffer->mark);
    draw_rect(renderer,
              rect2_min_size(v2_add(top_left, mark_p), 
                             v2(mark_width, line_spacing)),
              v4(0, 1, 0, 0.4f));
  }
  
  i32 first_visible_token = 0;
  i32 skipped_lines = 0;
  
  
  Token *tokens = buffer->tokens;
  
  begin_profiler_event("render");
  while (skipped_lines < first_visible_line) {
    Token *t = tokens + first_visible_token++;
    if (t->kind == T_NEWLINE) {
      skipped_lines++;
    }
  }
  
  
  V2 offset = v2(rect.min.x, 
                 rect.max.y - (skipped_lines - buffer->scroll_y)*line_spacing);
  
  i32 line_index = 0;
  
  i32 token_count = (i32)sb_count(tokens);
  for (i32 token_index = first_visible_token;
       token_index < token_count;
       token_index++) 
  {
    Token *t = tokens + token_index;
    
    if (t->kind == T_NEWLINE) {
      offset.x = rect.min.x;
      offset.y -= line_spacing;
      line_index++;
      if (line_index > height_lines) {
        break;
      }
    } else {
      u32 color = theme.colors[Text_Color_Type_DEFAULT];
      
      if (t->ast_kind == A_ARGUMENT) {
        color = theme.colors[Text_Color_Type_ARG];
      } else if (t->ast_kind == A_FUNCTION) {
        color = theme.colors[Text_Color_Type_FUNCTION];
      } else if (t->ast_kind == A_TYPE) {
        color = theme.colors[Text_Color_Type_TYPE];
      }else if ((t->kind >= T_KEYWORD_FIRST && 
                 t->kind <= T_KEYWORD_LAST) ||
                (t->kind >= T_OPERATOR_FIRST && 
                 t->kind <= T_OPERATOR_LAST)) 
      {
        color = theme.colors[Text_Color_Type_KEYWORD];
      } else if (t->ast_kind == A_MACRO || 
                 t->kind == T_INT || 
                 t->kind == T_FLOAT) 
      {
        color = theme.colors[Text_Color_Type_NUMBER];
      } else if (t->kind == T_STRING ||
                 t->kind == T_CHAR ||
                 t->ast_kind == A_ENUM_MEMBER) {
        color = theme.colors[Text_Color_Type_STRING];
      } else if (t->kind == T_COMMENT) {
        color = theme.colors[Text_Color_Type_COMMENT];
      }
      
      V4 color_float = color_u32_to_v4(color);
      
      bool scared = t->start <= buffer->cursor && t->start+t->count > buffer->cursor;
      
      String token_string = buffer_part_to_string(buffer, 
                                                  t->start, 
                                                  t->start + t->count);
      draw_string(renderer, token_string, offset, color_float, false);
      offset.x += measure_string_width(renderer, token_string);
    }
  }
  
  end_profiler_event("render");
  
  end_profiler_event("buffer_draw");
}