#include "editor.h"
#include "buffer.c"


typedef struct {
  gl_Funcs gl;
  b32 (*pop_event)(os_Event*);
  V2 (*get_window_size)();
  void (*collect_messages)(os_Window, os_Input*);
  String (*read_entire_file)(String);
  Font (*load_font)(String, String, i32);
} Os;

typedef struct {
  bool initialized;
  bool reloaded;
  
  os_Window window;
  
  Renderer renderer;
  bool running;
  Text_Buffer buffer;
  Font font;
} Editor_Memory;

Keybind *get_keybind(Settings *settings, os_Keycode keycode, 
                     bool shift, bool ctrl, bool alt) 
{
  Keybind *result = null;
  for (u32 i = 0; i < sb_count(settings->keybinds); i++) {
    Keybind *k = settings->keybinds + i;
    if (k->keycode == keycode &&
        k->shift == shift && 
        k->ctrl == ctrl && 
        k->alt == alt)
    {
      result = k;
      break;
    }
  }
  return result;
}

void editor_update(Os os, Editor_Memory *memory, os_Input *input) {
  if (!memory->initialized) {
    memory->initialized = true;
    
    
    GLuint shader = gl_create_shader_from_file(os.read_entire_file, const_string("shader.glsl"));
    Renderer *renderer = &memory->renderer;
    
    
    
    memory->font = os.load_font(const_string("ubuntu_mono.ttf"), 
                                const_string("Ubuntu Mono"),
                                24);
    
    V2 window_size = os.get_window_size(memory->window);
    init_renderer(renderer, shader, &memory->font, window_size);
    
    
    Text_Buffer *buffer = &memory->buffer;
    buffer->data = alloc_array(char, 128);
    buffer->capacity = 128;
    buffer->count = 0;
    buffer->cursor = 0;
    
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
    
    buffer_insert_string(buffer, str);
    set_cursor(buffer, 0);
  }
  
  Renderer *renderer = &memory->renderer;
  Text_Buffer *buffer = &memory->buffer;
  Font *font = renderer->state.font;
  
  scratch_reset();
  os.collect_messages(memory->window, input);
  
  begin_profiler_event("input");
  os_Event event;
  while (os.pop_event(&event)) {
    switch (event.type) {
      case os_Event_Type_CLOSE: {
        memory->running = false;
      } break;
      
      case os_Event_Type_RESIZE: {
        V2 window_size = os.get_window_size(memory->window);
        V2i size = v2_to_v2i(window_size);
        gl.Viewport(0, 0, size.x, size.y);
        renderer->window_size = window_size;
      } break;
      
      case os_Event_Type_BUTTON: {
        os_Keycode keycode = event.button.keycode;
        os_Button key = input->keys[keycode];
        
        if (input->ctrl) {
          switch (keycode) {
            case os_Keycode_SPACE: {
              if (key.went_down) {
                buffer->mark = buffer->cursor;
              }
            } break;
            
            case 'C': {
              if (key.pressed) {
                buffer_copy(buffer);
              }
            } break;
            case 'X': {
              if (key.pressed) {
                buffer_cut(buffer);
              }
            } break;
            case 'V': {
              if (key.pressed) {
                buffer_paste(buffer);
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
                move_cursor_direction(font, buffer, keycode);
              }
            } break;
            
            case os_Keycode_ENTER: {
              if (key.pressed) buffer_newline(buffer);
            } break;
            
            case os_Keycode_BACKSPACE: {
              if (key.pressed) buffer_remove_backward(buffer, 1);
            } break;
            
            case os_Keycode_DELETE: {
              if (key.pressed) buffer_remove_forward(buffer, 1);
            } break;
            
            case os_Keycode_TAB: {
              if (key.pressed) buffer_indent(buffer);
            } break;
          }
        }
      } break;
      
      default: {} break;
    }
  }
  
  
  if (!input->ctrl && !input->alt) {
    if (input->char_count > 0) {
      String str = make_string(input->chars, input->char_count);
      buffer_input_string(buffer, str);
    }
  }
  
  end_profiler_event("input");
  
  V4 bg_color = color_u32_to_v4(0xFF272822);
  gl.ClearColor(bg_color.r, bg_color.b, bg_color.g, bg_color.a);
  gl.Clear(GL_COLOR_BUFFER_BIT);
  
  
  renderer->state.matrix = m4_identity();
  sb_count(renderer->items) = 0;
  
  render_scale(renderer, v3(2/renderer->window_size.x,
                            2/renderer->window_size.y, 1));
  
  V2 bottom_left = v2(-renderer->window_size.x*0.5f,
                      -renderer->window_size.y*0.5f);
  buffer_draw(renderer, buffer, rect2_min_size(bottom_left, 
                                               renderer->window_size));
  renderer_output(renderer);
}
