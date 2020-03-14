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
  
  GLuint shader = gl_create_shader_from_file(const_string("shader.glsl"));
  Renderer _renderer = {0};
  Renderer *renderer = &_renderer;
  
  
  
  Font font = load_font(const_string("ubuntu_mono.ttf"), 
                        const_string("Ubuntu Mono"),
                        24);
  
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
        
        case os_Event_Type_BUTTON: {
          os_Keycode keycode = event.button.keycode;
          os_Button key = input.keys[keycode];
          
          if (input.ctrl) {
            switch (keycode) {
              case os_Keycode_SPACE: {
                if (key.went_down) {
                  buffer.mark = buffer.cursor;
                }
              } break;
              
              case 'C': {
                if (key.pressed) {
                  buffer_copy(&buffer);
                }
              } break;
              case 'X': {
                if (key.pressed) {
                  buffer_cut(&buffer);
                }
              } break;
              case 'V': {
                if (key.pressed) {
                  buffer_paste(&buffer);
                }
              } break;
            }
          } else {
            switch (keycode) {
              case os_Keycode_ARROW_RIGHT:
              case os_Keycode_ARROW_LEFT:
              case os_Keycode_ARROW_UP:
              case os_Keycode_ARROW_DOWN: {
                if (key.pressed) {
                  move_cursor_direction(&font, &buffer, keycode);
                }
              } break;
              
              case os_Keycode_ENTER: {
                if (key.pressed) buffer_newline(&buffer);
              } break;
              
              case os_Keycode_BACKSPACE: {
                if (key.pressed) buffer_remove_backward(&buffer, 1);
              } break;
              
              case os_Keycode_DELETE: {
                if (key.pressed) buffer_remove_forward(&buffer, 1);
              } break;
              
              case os_Keycode_TAB: {
                if (key.pressed) buffer_indent(&buffer);
              } break;
            }
          }
        } break;
        
        default: {} break;
      }
    }
    
    
    if (!input.ctrl && !input.alt) {
      if (input.char_count > 0) {
        String str = make_string(input.chars, input.char_count);
        buffer_input_string(&buffer, str);
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
