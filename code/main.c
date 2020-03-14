#include <lvl5_os.h>
#include <lvl5_files.h>
#include "font.c"
#include "renderer.c"
#include "buffer.c"
#include <stdio.h>

u64 performance_frequency;

f64 win32_get_time() {
  LARGE_INTEGER time_li;
  QueryPerformanceCounter(&time_li);
  f64 result = ((f64)time_li.QuadPart/(f64)performance_frequency);
  return result;
}




os_entry_point() {
  context_init(megabytes(20));
  os_Window window = os_create_window();
  
  
  LARGE_INTEGER performance_frequency_li;
  QueryPerformanceFrequency(&performance_frequency_li);
  performance_frequency = performance_frequency_li.QuadPart;
  
  // NOTE(lvl5): windows font stuff
  //Font font = load_font(const_string("inconsolata.ttf"));
  Font font = load_font(const_string("comic.ttf"), 
                        const_string("Comic Sans MS"),
                        30);
  GLuint shader = gl_create_shader_from_file(const_string("shader.glsl"));
  Renderer _renderer = {0};
  Renderer *renderer = &_renderer;
  
  
  V2 window_size = os_get_window_size(window);
  init_renderer(renderer, shader, &font, window_size);
  
  
  Text_Buffer buffer = {0};
  buffer.data = alloc_array(char, 128);
  buffer.capacity = 128;
  buffer.count = 0;
  buffer.cursor = 0;
  //arena_init(&buffer.token_arena, )
  
  
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
  
  buffer_insert_string(&buffer, str);
  set_cursor(&buffer, 0);
  
  i32 thing = get_buffer_pos(&buffer, 20);
  if (thing > 40) {
    OutputDebugStringA("fdsff");
  }
  
  
  b32 running = true;
  f64 prev_time = win32_get_time();
  
  while (running) {
    begin_profiler_event("loop");
    
    scratch_reset();
    os_collect_messages(window);
    
    begin_profiler_event("input");
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
        move_cursor_direction(&font, &buffer, code);
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
    
    end_profiler_event("input");
    
    V4 bg_color = color_u32_to_v4(0xFF272822);
    gl.ClearColor(bg_color.r, bg_color.b, bg_color.g, bg_color.a);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    
    
    renderer->state.matrix = m4_identity();
    sb_count(renderer->items) = 0;
    
    render_scale(renderer, v3(2/window_size.x, 2/window_size.y, 1));
    
    V2 bottom_left = v2(-window_size.x*0.5f, -window_size.y*0.5f);
    buffer_draw(renderer, &buffer, rect2_min_size(bottom_left, window_size));
    renderer_output(renderer);
    
    
    end_profiler_event("loop");
    
    if (profiler_event_count && false) {
      FILE *file;
      errno_t err = fopen_s(&file, "profile.json", "wb");
      
      char begin[] = "[\n";
      fwrite(begin, array_count(begin)-1, 1, file);
      
      for (i32 i = 0; i < profiler_event_count; i++) {
        Profiler_Event *event = profiler_events + i;
        char str[512];
        i32 str_count = sprintf_s(str, 512, "  {\"name\": \"%s\", \"cat\": \"PERF\", \"ts\": %lld, \"ph\": \"%c\", \"pid\": 1, \"tid\": 1},\n", event->name, event->stamp, event->type == Profiler_Event_Type_BEGIN ? 'B' : 'E');
        fwrite(str, str_count, 1, file);
      }
      fseek(file, -2, SEEK_CUR);
      
      char end[] = "\n]";
      fwrite(end, array_count(end)-1, 1, file);
      
      fclose(file);
    }
    
    
    os_blit_to_screen();
  }
  
  return 0;
}
