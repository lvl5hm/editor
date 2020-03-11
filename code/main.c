#include <lvl5_os.h>
#include <lvl5_files.h>
#include "font.c"
#include "renderer.c"
#include "editor.c"



os_entry_point() {
  context_init(megabytes(20));
  os_Window window = os_create_window();
  
  
  // NOTE(lvl5): windows font stuff
  Font font = load_font(const_string("c:/windows/fonts/consola.ttf"));
  GLuint shader = gl_create_shader_from_file(const_string("shader.glsl"));
  Renderer _renderer = {0};
  Renderer *renderer = &_renderer;
  
  
  V2 window_size = os_get_window_size(window);
  init_renderer(renderer, shader, &font, window_size);
  
  
  Text_Buffer buffer = {
    .data = alloc_array(char, 128),
    .capacity = 128,
    .count = 0,
    .cursor = 0,
  };
  
  
  String str;
  // TODO: all files should end with \0
  {
    FILE *file;
    errno_t err = fopen_s(&file, "test.c", "rb");
    fseek(file, 0, SEEK_END);
    i32 file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *file_memory = alloc_array(char, file_size+1);
    size_t read = fread(file_memory, 1, file_size, file);
    assert(read == file_size);
    fclose(file);
    str = make_string(file_memory, file_size+1);
    str.data[file_size] = 0;
  }
#if 0
  String str = const_string(
    "typedef int i32;\n"
    "typedef struct {\n"
    "  int foo;\n"
    "  char *bar;\n"
    "} Foo;\n"
    "\n"
    "void buffer_copy(Text_Buffer *buffer) {\n"
    "  Foo foo = {0};\n"
    "  i32 start = min(buffer->cursor, buffer->mark);\n"
    "  i32 end = max(buffer->cursor, buffer->mark);\n"
    "  buffer->exchange_count = end - start;\n"
    "  assert(buffer->exchange_count <= MAX_EXCHANGE_COUNT);\n"
    "  for (i32 i = 0; i < buffer->exchange_count; i++) {\n"
    "    buffer->exchange[i] = get_buffer_char(buffer, start+i);\n"
    "  }\n"
    "}\n"
    "\n"
    "void buffer_paste(Text_Buffer *buffer) {\n"
    "  String str = make_string(buffer->exchange, buffer->exchange_count);\n"
    "  buffer_insert_string(buffer, str);\n"
    "}\n"
    "\n"
    "void buffer_cut(Text_Buffer *buffer) {\n"
    "  buffer_copy(buffer);\n"
    "  i32 count = buffer->cursor - buffer->mark;\n"
    "  if (buffer->cursor < buffer->mark) {\n"
    "    i32 tmp = buffer->cursor;\n"
    "    buffer->cursor = buffer->mark;\n"
    "    buffer->mark = tmp;\n"
    "    count *= -1;\n"
    "  }\n"
    "  buffer_remove_backward(buffer, count);\n"
    "}\0");
#endif
  
  
  buffer_insert_string(&buffer, str);
  set_cursor(&buffer, 0);
  
  i32 thing = get_buffer_pos(&buffer, 20);
  if (thing > 40) {
    OutputDebugStringA("fdsff");
  }
  
  b32 running = true;
  while (running) {
    scratch_reset();
    os_collect_messages(window);
    
    os_Event event;
    while (os_pop_event(&event)) {
      switch (event.type) {
        case os_Event_Type_CLOSE: {
          running = false;
        } break;
        
        case os_Event_Type_RESIZE: {
          window_size = os_get_window_size(window);
          V2i size = v2_to_v2i(window_size);
          gl.Viewport(0, 0, size.x, size.y);
          renderer->window_size = window_size;
        } break;
        
        default: {} break;
      }
    }
    
    os_Keycode arrows_key_codes[] = {
      os_Keycode_ARROW_RIGHT,
      os_Keycode_ARROW_LEFT,
      os_Keycode_ARROW_DOWN,
      os_Keycode_ARROW_UP,
    };
    for (i32 i = 0; i < array_count(arrows_key_codes); i++) {
      os_Keycode code = arrows_key_codes[i];
      if (input.keys[code].pressed) {
        move_cursor_direction(&buffer, code);
      }
    }
    
    
    if (input.ctrl) {
      if (input.keys[os_Keycode_SPACE].went_down) {
        buffer.mark = buffer.cursor;
      } else if (input.keys['C'].pressed) {
        buffer_copy(&buffer);
      } else if (input.keys['X'].pressed) {
        buffer_cut(&buffer);
      } else if (input.keys['V'].pressed) {
        buffer_paste(&buffer);
      }
    } else {
      if (input.char_count > 0) {
        String str = make_string(input.chars, input.char_count);
        
        if (str.data[0] == '}') {
          i32 start = seek_line_start(&buffer, buffer.cursor);
          bool only_indent = true;
          for (i32 i = start; i < buffer.cursor; i++) {
            if (get_buffer_char(&buffer, i) != ' ') {
              only_indent = false;
              break;
            }
          }
          if (only_indent && buffer.cursor >= 2) {
            buffer_remove_backward(&buffer, 2);
          }
        }
        if (str.data[0] != '\t' && 
            str.data[0] != '\b' && 
            str.data[0] != '\r') {
          buffer_insert_string(&buffer, str);
        }
      }
      
      if (input.keys[os_Keycode_ENTER].pressed) {
        i32 line_start = seek_line_start(&buffer, buffer.cursor);
        String indent_str = { .count = 1 };
        while (get_buffer_char(&buffer, 
                               line_start + indent_str.count-1) == ' ') {
          indent_str.count++;
        }
        
        
        i32 first_non_space_before = buffer.cursor;
        while (get_buffer_char(&buffer, first_non_space_before) == ' ' ||
               get_buffer_char(&buffer, first_non_space_before) == '\n') {
          first_non_space_before--;
        }
        if (get_buffer_char(&buffer, first_non_space_before) == '{') {
          indent_str.count += 2;
        }
        
        indent_str.data = scratch_push_array(char, indent_str.count);
        indent_str.data[0] = '\n';
        for (i32 i = 1; i < (i32)indent_str.count; i++) {
          indent_str.data[i] = ' ';
        }
        
        buffer_insert_string(&buffer, indent_str);
      }
      if (input.keys[os_Keycode_BACKSPACE].pressed) {
        if (buffer.cursor > 0) {
          buffer_remove_backward(&buffer, 1);
        }
      } 
      if (input.keys[os_Keycode_DELETE].pressed) {
        if (buffer.cursor < buffer.count - 1) {
          buffer_remove_forward(&buffer, 1);
        }
      }
      if (input.keys[os_Keycode_TAB].pressed) {
        String indent_str = make_string(scratch_push_array(char, 2), 2);
        for (i32 i = 0; i < (i32)indent_str.count; i++) {
          indent_str.data[i] = ' ';
        }
        
        buffer_insert_string(&buffer, indent_str);
      }
    }
    
    
    V4 bg_color = color_u32_to_v4(0xFF272822);
    gl.ClearColor(bg_color.r, bg_color.b, bg_color.g, bg_color.a);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    
    
    
    V2 cursor_p = get_screen_position_in_buffer(&font, &buffer, buffer.cursor);
    f32 screen_height_lines = window_size.y / font.line_spacing;
    {
      f32 cursor_line = -cursor_p.y / font.line_spacing;
      f32 border_top = buffer.scroll_y + 4;
      f32 border_bottom = buffer.scroll_y + screen_height_lines - 4;
      
      f32 want_scroll_y = 0;
      if (cursor_line > border_bottom) {
        want_scroll_y = cursor_line - border_bottom;
      } else if (cursor_line < border_top && buffer.scroll_y > 0) {
        want_scroll_y = cursor_line - border_top;
      }
      
      buffer.scroll_y += want_scroll_y/4;
    }
    
    V2 top_left = v2(-window_size.x*0.5f, 
                     window_size.y*0.5f);
    
    
    V2 buffer_position = v2_add(top_left,
                                v2(0, (buffer.scroll_y-1)*font.line_spacing));
    
    
    draw_rect(renderer, 
              rect2_min_size(v2_add(buffer_position, cursor_p), v2(11, font.line_spacing)), 
              v4(0, 1, 0, 1));
    
    V2 mark_p = get_screen_position_in_buffer(&font, &buffer, buffer.mark);
    draw_rect(renderer,
              rect2_min_size(v2_add(buffer_position, mark_p), v2(font.space_width, font.line_spacing)),
              v4(0, 1, 0, 0.4f));
    
    
    
    u32 green = 0xFFA6E22E;
    u32 orange = 0xFFFD911F;
    
    
    i32 first_visible_line = (i32)buffer.scroll_y;
    i32 last_visible_line = (i32)(buffer.scroll_y + screen_height_lines);
    
    push_scratch_context(); {
      Token *tokens = buffer_parse(&buffer);
      
      V2 offset = buffer_position;
      
      i32 token_count = (i32)sb_count(tokens);
      
      i32 line_index = 0;
      i32 first_visible_token = 0;
      while (first_visible_line > line_index) {
        Token t = tokens[first_visible_token++];
        if (t.kind == T_NEWLINE) {
          line_index++;
        }
      }
      
      offset.y -= renderer->state.font->line_spacing*(f32)line_index;
      
      for (i32 i = first_visible_token; i < token_count; i++) {
        Token t = tokens[i];
        if (t.kind == T_NEWLINE) {
          offset.x = buffer_position.x;
          offset.y -= renderer->state.font->line_spacing;
          line_index++;
          if (line_index > last_visible_line) {
            break;
          }
        } else if (t.kind == T_SPACE) {
          offset.x += font.space_width;
        } else {
          String token_string = buffer_part_to_string(&buffer, 
                                                      t.start, 
                                                      t.start + t.count);
          u32 color = 0xFFF8F8F2;
          
          if (t.ast_kind == A_ARGUMENT) {
            color = orange;
          } else if (t.ast_kind == A_FUNCTION) {
            color = green;
          } else if (t.ast_kind == A_TYPE) {
            color = 0xFF66D9EF;
          }else if ((t.kind >= T_KEYWORD_FIRST && 
                     t.kind <= T_KEYWORD_LAST) ||
                    (t.kind >= T_OPERATOR_FIRST && 
                     t.kind <= T_OPERATOR_LAST)) {
            color = 0xFFF92672;
          } else if (t.kind == T_INT || t.kind == T_FLOAT) {
            color = 0xFFAE81FF;
          } else if (t.kind == T_STRING || t.kind == T_CHAR) {
            color = 0xFFE6DB74;
          }
          
          V4 color_float = color_u32_to_v4(color);
          
          V2 string_offset = draw_string(renderer, token_string, 
                                         offset, color_float);
          offset = v2_add(offset, string_offset);
        }
      }
    } pop_context();
    
    
    {
      V2 rect_min = v2_add(top_left, v2(0, -(f32)font.line_spacing));
      draw_rect(renderer,
                rect2_min_size(rect_min, v2(window_size.x, font.line_spacing)),
                v4(0, 0, 0, 1));
      draw_string(renderer, const_string("fucj"), 
                  v2_add(rect_min, v2(0, font.line_spacing)), v4(1, 1, 1, 1));
    }
    
    
#if 0    
    String buffer_content = text_buffer_to_string(&buffer);
    draw_string(renderer, buffer_content, v2(buffer_position.x, buffer_position.y), v4(1, 1, 1, 1));
#endif
    
    os_blit_to_screen();
  }
  
  return 0;
}
