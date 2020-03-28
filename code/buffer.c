#include "buffer.h"
#include "lvl5_stretchy_buffer.h"
#include "parser.c"

inline i32 get_gap_count(Buffer *b) {
  i32 result = b->capacity - b->count;
  return result;
}

inline i32 get_gap_start(Buffer *b) {
  i32 gap_count = get_gap_count(b);
  i32 result = b->cursor;
  assert(result <= b->capacity - gap_count);
  return result;
}

inline i32 get_buffer_pos(Buffer *b, i32 pos) {
  i32 gap_start = get_gap_start(b);
  i32 gap_count = get_gap_count(b);
  
  i32 result = pos + (pos >= gap_start)*gap_count;
  
  return result;
}

char get_buffer_char(Buffer *b, i32 pos) {
  char result = b->data[get_buffer_pos(b, pos)];
  return result;
}

V2 get_buffer_xy(Buffer *b, i32 pos) {
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

V2 get_screen_position_in_buffer(Font *font, Buffer *b, i32 pos) {
  begin_profiler_function();
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
  
  end_profiler_function();
  return result;
}

i32 seek_line_start(Buffer *b, i32 start) {
  begin_profiler_function();
  i32 result = max(start, 0);
  while (result > 0 && get_buffer_char(b, result-1) != '\n') {
    result--;
  }
  end_profiler_function();
  return result;
}

i32 seek_line_end(Buffer *b, i32 start) {
  begin_profiler_function();
  i32 result = min(start, b->count - 1);
  while (result < (i32)b->count-1 && get_buffer_char(b, result) != '\n') {
    result++;
  }
  end_profiler_function();
  return result;
}

void set_cursor(Buffer *b, i32 pos) {
  begin_profiler_function();
  
  assert(pos >= 0 && pos < b->count);
  i32 old_gap_start = get_gap_start(b);
  b->cursor = pos;
  i32 gap_start = get_gap_start(b);
  i32 moved_by = gap_start - old_gap_start;
  
  if (moved_by != 0) {
    // move chars from one gap end to the other
    i32 gap_count = get_gap_count(b);
    
    if (moved_by < 0) {
#if 0      
      memmove(b->data + gap_start + gap_count,
              b->data + gap_start,
              -moved_by);
#else
      for (i32 i = 0; i < -moved_by; i++) {
        b->data[old_gap_start+gap_count-1-i] = b->data[old_gap_start-1-i];
      }
#endif
    } else {
#if 0      
      memmove(b->data + old_gap_start,
              b->data + old_gap_start + gap_count,
              moved_by);
#else
      for (i32 i = 0; i < moved_by; i++) {
        b->data[old_gap_start+i] = b->data[old_gap_start+gap_count+i];
      }
#endif
    }
  }
  
  end_profiler_function();
}

void buffer_parse(Buffer *buffer) {
  begin_profiler_function();
  
  arena_set_mark(&buffer->cache.arena, 0);
  push_arena_context(&buffer->cache.arena); {
    buffer->cache.scope = add_scope(null, 1024);
    buffer->cache.dependencies = sb_new(String, 16);
    buffer->cache.colors = sb_new(Syntax, buffer->count);
    
    u32 average_token_size = 3;
    u32 token_count_guess = buffer->count / average_token_size;
    buffer->cache.tokens = sb_new(Token, token_count_guess);
    
    Parser _parser = {
      .token_index = 0,
      .buffer = buffer,
      .scope = buffer->cache.scope,
    };
    
    
    Parser *parser = &_parser;
    
    buffer_tokenize(parser);
    parse_program(parser);
    
    buffer->cache.locked = false;
  }
  pop_context();
  
  end_profiler_function();
}

String resolve_include_path(String include) {
  String result = include;
  return result;
}

Buffer **buffer_get_dependent_buffers(Buffer *buffer) {
  begin_profiler_function();
  push_scratch_context();
  Buffer **dependents = sb_new(Buffer **, 16);
  
  for (u32 buffer_index = 0;
       buffer_index < sb_count(buffer->editor->buffers);
       buffer_index++) 
  {
    Buffer *other = buffer->editor->buffers + buffer_index;
    for (u32 dep_index = 0; 
         dep_index < sb_count(other->cache.dependencies);
         dep_index++) 
    {
      // TODO(lvl5): need to search in the file system like the preprocessor does
      String include = other->cache.dependencies[dep_index];
      if (string_compare(include, buffer->path)) {
        sb_push(dependents, other);
        break;
      }
    }
  }
  
  pop_context();
  
  end_profiler_function();
  return dependents;
}

void buffer_update_cache(Buffer *buffer) {
  buffer_parse(buffer);
  buffer->cache.generation = buffer->editor->generation;
  
  Buffer **dependents = buffer_get_dependent_buffers(buffer);
  for (u32 i = 0; i < sb_count(dependents); i++) {
    Buffer *dep = dependents[i];
    
    if (dep->cache.generation != dep->editor->generation) {
      if (_InterlockedCompareExchange((volatile long *)&dep->cache.locked,
                                      true,
                                      false) == false) 
      {
        global_os.queue_add(global_os.thread_queue, buffer_update_cache, dep);
      }
    }
  }
  
}

// NOTE(lvl5): only call this from the main thread
void buffer_changed(Buffer *buffer) {
  begin_profiler_function();
  
  buffer->editor->generation++;
  buffer->cache.locked = true;
  buffer_update_cache(buffer);
  buffer->cache.locked = false;
  
  end_profiler_function();
}


#define BUFFER_INCREMENT_SIZE 1024

void buffer_insert_string(Buffer *b, String str) {
  begin_profiler_function();
  
  bool not_scratch = get_context()->allocator == system_allocator;
  assert(not_scratch);
  if (b->count + (i32)str.count > b->capacity) {
    char *old_data = b->data;
    i32 old_gap_start = get_gap_start(b);
    i32 old_gap_count = get_gap_count(b);
    
    i32 required_count = b->count + (i32)str.count;
    if (b->count + (i32)str.count > b->capacity) {
      b->capacity = ceil_f32_i32((f32)required_count / (f32)BUFFER_INCREMENT_SIZE)*BUFFER_INCREMENT_SIZE;
    }
    while (b->count + (i32)str.count > b->capacity) {
      b->capacity = b->capacity*2;
    }
    // one extra \0 after the buffer for kerning
    b->data = alloc_array(char, b->capacity + 1);
    b->data[b->capacity] = '\0';
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
  
  end_profiler_function();
}


Buffer *make_empty_buffer(Editor *editor, String path) {
  begin_profiler_function();
  
  Buffer b = {0};
  b.capacity = 0;
  b.path = alloc_string(path.data, path.count);
  b.editor = editor;
  sb_push(editor->buffers, b);
  Buffer *buffer = editor->buffers + sb_count(editor->buffers) - 1;
  
  
  bool not_scratch = get_context()->allocator == system_allocator;
  assert(not_scratch);
  
  Mem_Size arena_size = megabytes(16);
  arena_init(&buffer->cache.arena, alloc_array(byte, arena_size), arena_size);
  
  buffer_insert_string(buffer, const_string("\0"));
  set_cursor(buffer, 0);
  
  end_profiler_function();
  return buffer;
}

void buffer_remove_backward(Buffer *b, i32 count) {
  begin_profiler_function();
  
  if (b->cursor - count >= 0) {
    if (b->mark >= b->cursor) {
      b->mark -= count;
    }
    b->cursor -= count;
    b->count -= count;
    
    buffer_changed(b);
  }
  
  end_profiler_function();
}

void buffer_remove_forward(Buffer *b, i32 count) {
  begin_profiler_function();
  
  if (b->cursor < b->count - 1) {
    if (b->mark > b->cursor) {
      b->mark -= count;
    }
    b->count -= count;
    
    buffer_changed(b);
  }
  
  end_profiler_function();
}

f32 get_pixel_position_in_line(Font *font, Buffer *b, i32 pos) {
  begin_profiler_function();
  
  i32 line_start = seek_line_start(b, pos);
  f32 result = 0;
  for (i32 i = line_start; i < pos; i++) {
    result += font_get_advance(font, 
                               get_buffer_char(b, i),
                               get_buffer_char(b, i + 1));
  }
  
  end_profiler_function();
  return result;
}

b32 move_cursor_direction(Font *font, Buffer *b, Command direction) {
  begin_profiler_function();
  
  b32 result = false;
  i32 cursor = b->cursor;
  switch (direction) {
    case Command_MOVE_CURSOR_RIGHT: {
      if (cursor < b->count-1) {
        cursor++;
        b->preferred_col_pos = cursor;
        result = true;
      }
    } break;
    
    case Command_MOVE_CURSOR_LEFT: {
      if (cursor > 0) {
        cursor--;
        b->preferred_col_pos = cursor;
        result = true;
      }
    } break;
    
    case Command_MOVE_CURSOR_DOWN: {
      i32 line_start = seek_line_start(b, cursor);
      
      f32 cur_pixel = get_screen_position_in_buffer(font, b, b->preferred_col_pos).x;
      i32 line_end = seek_line_end(b, cursor);
      
      i32 want = min(line_end + 1, b->count - 1);
      f32 want_pixel = 0;
      while (want < b->count-1 && get_buffer_char(b, want) != '\n') {
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
    
    case Command_MOVE_CURSOR_UP: {
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
    
    case Command_MOVE_CURSOR_LINE_END: {
      i32 line_end = seek_line_end(b, cursor);
      cursor = line_end;
      b->preferred_col_pos = cursor;
    } break;
    
    case Command_MOVE_CURSOR_LINE_START: {
      i32 line_start = seek_line_start(b, cursor);
      while (get_buffer_char(b, line_start) == ' ') {
        line_start++;
      }
      cursor = line_start;
      b->preferred_col_pos = cursor;
    } break;
    
    default: assert(false);
  }
  set_cursor(b, cursor);
  
  end_profiler_function();
  return result;
}

String buffer_part_to_string(Buffer *buffer, i32 start, i32 end) {
  begin_profiler_function();
  
  String str = {0};
  str.count = end - start;
  
  if ((start < buffer->cursor) & (end >= buffer->cursor)) {
    str.data = scratch_push_array(char, str.count);
    for (i32 i = 0; i < (i32)str.count; i++) {
      str.data[i] = get_buffer_char(buffer, start + i);
    }
  } else if (start < buffer->cursor) {
    str.data = buffer->data + start;
  } else {
    str.data = buffer->data + start + (buffer->capacity - buffer->count);
  }
  
  end_profiler_function();
  return str;
}

String text_buffer_to_string(Buffer *buffer) {
  String result = buffer_part_to_string(buffer, 0, buffer->count);
  return result;
}

void buffer_copy(Buffer *buffer, Exchange *exchange) {
  i32 start = min(buffer->cursor, buffer->mark);
  i32 end = max(buffer->cursor, buffer->mark);
  exchange->count = end - start;
  assert(exchange->count <= MAX_EXCHANGE_COUNT);
  for (i32 i = 0; i < exchange->count; i++) {
    exchange->data[i] = get_buffer_char(buffer, start+i);
  }
}

void buffer_paste(Buffer *buffer, Exchange *exchange) {
  String str = make_string(exchange->data, exchange->count);
  buffer_insert_string(buffer, str);
}

void buffer_cut(Buffer *buffer, Exchange *exchange) {
  buffer_copy(buffer, exchange);
  i32 count = buffer->cursor - buffer->mark;
  if (buffer->cursor < buffer->mark) {
    i32 tmp = buffer->cursor;
    buffer->cursor = buffer->mark;
    buffer->mark = tmp;
    count *= -1;
  }
  buffer_remove_backward(buffer, count);
}


void buffer_newline(Buffer *buffer) {
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

void buffer_indent(Buffer *buffer) {
  String indent_str = make_string(scratch_push_array(char, 2), 2);
  for (i32 i = 0; i < (i32)indent_str.count; i++) {
    indent_str.data[i] = ' ';
  }
  
  buffer_insert_string(buffer, indent_str);
}

void buffer_input_string(Buffer *buffer, String str) {
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
