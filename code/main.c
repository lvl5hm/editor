#include <lvl5_os.h>
#include <lvl5_files.h>
#include "font.c"
#include "renderer.c"
#include "editor.c"

os_entry_point() {
  context_init(kilobytes(128));
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
  String str = const_string(
    "void buffer_copy(Text_Buffer *buffer) {\n"
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
    "}\n;");
  
  buffer_insert_string(&buffer, str);
  set_cursor(&buffer, 0);
  
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
        if (str.data[0] != '\b' && str.data[0] != '\r') {
          buffer_insert_string(&buffer, str);
        }
      }
      
      if (input.keys[os_Keycode_ENTER].pressed) {
        buffer_insert_string(&buffer, const_string("\n"));
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
    }
    
    
    gl.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    
    
    V2 buffer_position = v2(-window_size.x*0.5f, window_size.y*0.5f);
    
    V2 cursor_p = get_screen_position_in_buffer(&font, &buffer, buffer.cursor);
    draw_rect(renderer, 
              rect2_min_size(v2_add(buffer_position, cursor_p), v2(11, font.line_spacing)), 
              v4(0, 1, 0, 1));
    
    V2 mark_p = get_screen_position_in_buffer(&font, &buffer, buffer.mark);
    draw_rect(renderer,
              rect2_min_size(v2_add(buffer_position, mark_p), v2(11, font.line_spacing)),
              v4(0, 1, 0, 0.4f));
    
    String buffer_content = text_buffer_to_string(&buffer);
    draw_string(renderer, buffer_content, v2(buffer_position.x, buffer_position.y), v4(1, 1, 1, 1));
    
    os_blit_to_screen();
  }
  
  return 0;
}
