#include "editor.h"

i32 get_gap_count(Text_Buffer *b) {
  i32 result = b->capacity - b->count;
  return result;
}

i32 get_gap_start(Text_Buffer *b) {
  i32 gap_count = get_gap_count(b);
  i32 result = b->cursor;
  assert(result <= b->capacity - gap_count);
  return result;
}

i32 get_buffer_pos(Text_Buffer *b, i32 pos) {
  i32 result = 0;
  i32 gap_start = get_gap_start(b);
  i32 gap_count = get_gap_count(b);
  
  if (pos < gap_start) {
    result = pos;
  } else {
    result = pos + gap_count;
  }
  return result;
}

char get_buffer_char(Text_Buffer *b, i32 pos) {
  char result = b->data[get_buffer_pos(b, pos)];
  return result;
}

V2 get_screen_position_in_buffer(Font *font, Text_Buffer *b, i32 pos) {
  V2 result = v2(0, -(f32)font->line_spacing);
  
  for (i32 char_index = 0; char_index < pos; char_index++) {
    char buffer_char = get_buffer_char(b, char_index);
    char first = buffer_char - font->first_codepoint;
    if (buffer_char == '\n') {
      result.x = 0;
      result.y -= font->line_spacing;
      continue;
    }
    
    char second = get_buffer_char(b, char_index+1) - font->first_codepoint;
    result.x += font->advance[first*font->codepoint_count+second];
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
    b->mark += str.count;
  }
  b->cursor += str.count;
  b->count += str.count;
  
}

void buffer_remove_backward(Text_Buffer *b, i32 count) {
  assert(b->cursor - count >= 0);
  
  if (b->mark >= b->cursor) {
    b->mark -= count;
  }
  b->cursor -= count;
  b->count -= count;
}

void buffer_remove_forward(Text_Buffer *b, i32 count) {
  assert(b->cursor < b->count - 1);
  
  if (b->mark > b->cursor) {
    b->mark -= count;
  }
  b->count -= count;
}

b32 move_cursor_direction(Text_Buffer *b, os_Keycode direction) {
  b32 result = false;
  i32 cursor = b->cursor;
  switch (direction) {
    case os_Keycode_ARROW_RIGHT: {
      if (cursor < b->count-1) {
        cursor++;
        result = true;
      }
    } break;
    
    case os_Keycode_ARROW_LEFT: {
      if (cursor > 0) {
        cursor--;
        result = true;
      }
    } break;
    
    case os_Keycode_ARROW_DOWN: {
      i32 line_start = seek_line_start(b, cursor);
      i32 col = cursor - line_start;
      i32 line_end = seek_line_end(b, cursor);
      i32 want = line_end + 1 + col;
      i32 max_possible = seek_line_end(b, line_end+1);
      
      if (want > max_possible) {
        want = max_possible;
      }
      cursor = want;
    } break;
    
    case os_Keycode_ARROW_UP: {
      i32 line_start = seek_line_start(b, cursor);
      i32 col = cursor - line_start;
      i32 prev_line_start = seek_line_start(b, line_start-1);
      i32 want = prev_line_start + col;
      
      i32 max_possible = max(line_start - 1, 0);
      
      if (want > max_possible) {
        want = max_possible;
      }
      
      cursor = want;
    } break;
    
    default: assert(false);
  }
  set_cursor(b, cursor);
  
  return result;
}

String text_buffer_to_string(Text_Buffer *buffer) {
  String str = {0};
  str.data = scratch_push_array(char, buffer->count);
  str.count = buffer->count;
  for (i32 i = 0; i < buffer->count; i++) {
    str.data[i] = get_buffer_char(buffer, i);
  }
  
  return str;
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