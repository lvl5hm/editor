#include "editor.h"
#include "renderer.c"

Keybind *get_keybind(Editor *editor, os_Keycode keycode, 
                     bool shift, bool ctrl, bool alt) 
{
  Settings *settings = &editor->settings;
  Panel *panel = editor->panels + editor->active_panel_index;
  
  Keybind *result = null;
  for (u32 i = 0; i < sb_count(settings->keybinds); i++) {
    Keybind *k = settings->keybinds + i;
    if (k->keycode == keycode &&
        k->shift == shift && 
        k->ctrl == ctrl && 
        k->alt == alt &&
        (k->views & panel->type))
    {
      result = k;
      break;
    }
  }
  return result;
}

void execute_command(Os os, Editor *editor, Renderer *renderer, Command command) {
  Font *font = renderer->state.font;
  
  Panel *panel = editor->panels + editor->active_panel_index;
  Buffer_View *buffer_view = null;
  Buffer *buffer = null;
  
  switch (panel->type) {
    case Panel_Type_BUFFER: {
      buffer_view = &panel->buffer_view;
      buffer = buffer_view->buffer;
    } break;
  }
  
  switch (command) {
    case Command_SET_MARK: {
      buffer->mark = buffer->cursor;
    } break;
    case Command_COPY: {
      buffer_copy(buffer);
    } break;
    case Command_PASTE: {
      buffer_paste(buffer);
    } break;
    case Command_CUT: {
      buffer_cut(buffer);
    } break;
    
    case Command_MOVE_CURSOR_RIGHT:
    case Command_MOVE_CURSOR_UP:
    case Command_MOVE_CURSOR_DOWN:
    case Command_MOVE_CURSOR_LEFT: {
      move_cursor_direction(font, buffer, command);
    } break;
    
    case Command_LISTER_MOVE_DOWN:
    case Command_LISTER_MOVE_UP: {
      Lister *lister = &editor->current_dir_files;
      
      i32 add = 0;
      if (command == Command_LISTER_MOVE_DOWN) {
        add = 1;
      } else if (command == Command_LISTER_MOVE_UP) {
        add = -1;
      }
      lister->index = (lister->index + add) % sb_count(lister->items);
    } break;
    
    case Command_REMOVE_BACKWARD: {
      buffer_remove_backward(buffer, 1);
    } break;
    case Command_REMOVE_FORWARD: {
      buffer_remove_forward(buffer, 1);
    } break;
    case Command_NEWLINE: {
      buffer_newline(buffer);
    } break;
    case Command_TAB: {
      buffer_indent(buffer);
    } break;
    case Command_OPEN_FILE_DIALOG: {
      panel->type = Panel_Type_FILE_DIALOG_OPEN;
      
      Lister *lister = &editor->current_dir_files;
      lister->items = os.get_file_names(const_string("src"));
    } break;
    
    case Command_LISTER_SELECT: {
      // TODO(lvl5): handle file already open
      // TODO(lvl5): handle creating new file
      
      Lister *lister = &editor->current_dir_files;
      String file_name = lister->items[lister->index];
      String path = concat(const_string("src/"), file_name);
      
      os_File file = os.open_file(path);
      u64 file_size = os.get_file_size(file);
      char *file_memory = alloc_array(char, file_size);
      os.read_file(file, file_memory, 0, file_size);
      os.close_file(file);
      
      String str = make_string(file_memory, file_size);
      Buffer buffer = make_empty_buffer();
      buffer_insert_string(&buffer, str);
      set_cursor(&buffer, 0);
      sb_push(editor->buffers, buffer);
      Buffer *inserted = editor->buffers + sb_count(editor->buffers) - 1;
      
      panel->type = Panel_Type_BUFFER;
      panel->buffer_view = (Buffer_View){
        .buffer = inserted,
      };
    } break;
  }
}

extern void editor_update(Os os, Editor_Memory *memory, os_Input *input) {
  App_State *state = (App_State *)memory->data;
  
  if(memory->reloaded) {
    global_context_info = os.context_info;
    profiler_events = os.profiler_events;
    profiler_event_capacity = os.profiler_event_capacity;
    profiler_event_count = os.profiler_event_count;
  }
  
  if (!memory->initialized) {
    memory->initialized = true;
    
    GLuint shader = gl_create_shader_from_file(os.gl, os.read_entire_file, const_string("shader.glsl"));
    Renderer *renderer = &state->renderer;
    
    
    
    state->font = os.load_font(const_string("fonts/roboto.ttf"), 
                               const_string("Roboto"),
                               24);
    
    V2 window_size = os.get_window_size(memory->window);
    init_renderer(os.gl, renderer, shader, &state->font, window_size);
    
    
    Editor *editor = &state->editor;
    
    {
      editor->buffers = sb_new(Buffer, 16);
      editor->panels = sb_new(Panel, 16);
      
      sb_push(editor->buffers, make_empty_buffer());
      Buffer *buffer = editor->buffers + sb_count(editor->buffers) - 1;
      
      os_File file = os.open_file(const_string("src/foo.c"));
      u64 file_size = os.get_file_size(file);
      char *file_memory = alloc_array(char, file_size);
      os.read_file(file, file_memory, 0, file_size);
      os.close_file(file);
      
      String str = make_string(file_memory, file_size);
      buffer_insert_string(buffer, str);
      set_cursor(buffer, 0);
      
      os_File file = os.open_file(const_string("src/foo.c"));
      u64 file_size = os.get_file_size(file);
      char *file_memory = alloc_array(char, file_size);
      os.read_file(file, file_memory, 0, file_size);
      os.close_file(file);
      
      String str = make_string(file_memory, file_size + 1);
      buffer_insert_string(buffer, str);
      set_cursor(buffer, 0);
      
      V2 ws = renderer->window_size;
      V2 bottom_left = v2(-0.5f, -0.5f);
      V2 panel_size = v2(0.5f, 1.0f);
      
      {
        Panel panel = (Panel){
          .buffer_view = (Buffer_View){
            .buffer = buffer,
          },
          .type = Panel_Type_BUFFER,
          .rect = rect2_min_size(bottom_left, panel_size),
        };
        sb_push(editor->panels, panel);
      }
      {
        Panel panel = (Panel){
          .buffer_view = (Buffer_View){
            .buffer = buffer,
          },
          .type = Panel_Type_BUFFER,
          .rect = rect2_min_size(v2(0, -0.5f), panel_size),
        };
        //sb_push(editor->panels, panel);
      }
    }
    
    
    
    editor->active_panel_index = 0;
    
    editor->settings.keybinds = sb_new(Keybind, 32);
    Keybind *keybinds = editor->settings.keybinds;
    
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_SET_MARK,
                       .keycode = os_Keycode_SPACE,
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_COPY,
                       .keycode = 'C',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_PASTE,
                       .keycode = 'V',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_CUT,
                       .keycode = 'X',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_LEFT,
                       .keycode = os_Keycode_ARROW_LEFT,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_RIGHT,
                       .keycode = os_Keycode_ARROW_RIGHT,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_UP,
                       .keycode = os_Keycode_ARROW_UP,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_DOWN,
                       .keycode = os_Keycode_ARROW_DOWN,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_REMOVE_BACKWARD,
                       .keycode = os_Keycode_BACKSPACE,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_REMOVE_FORWARD,
                       .keycode = os_Keycode_DELETE,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_NEWLINE,
                       .keycode = os_Keycode_ENTER,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_TAB,
                       .keycode = os_Keycode_TAB,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER 
                       | Panel_Type_FILE_DIALOG_OPEN,
                       .command = Command_OPEN_FILE_DIALOG,
                       .keycode = 'O',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_FILE_DIALOG_OPEN,
                       .command = Command_LISTER_MOVE_UP,
                       .keycode = os_Keycode_ARROW_UP,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_FILE_DIALOG_OPEN,
                       .command = Command_LISTER_MOVE_DOWN,
                       .keycode = os_Keycode_ARROW_DOWN,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_FILE_DIALOG_OPEN,
                       .command = Command_LISTER_SELECT,
                       .keycode = os_Keycode_ENTER,
                       }));
    
    Color_Theme monokai = (Color_Theme){
      .name = const_string("Monokai"),
    };
    monokai.colors[Syntax_BACKGROUND] = 0xFF272822;
    monokai.colors[Syntax_DEFAULT] = 0xFFF8F8F2;
    monokai.colors[Syntax_COMMENT] = 0xFF88846F;
    monokai.colors[Syntax_TYPE] = 0xFF66D9EF;
    monokai.colors[Syntax_MACRO] = 0xFFAE81FF;
    monokai.colors[Syntax_FUNCTION] = 0xFFA6E22E;
    monokai.colors[Syntax_ARG] = 0xFFFD911F;
    monokai.colors[Syntax_OPERATOR] = 0xFFF92672;
    monokai.colors[Syntax_KEYWORD] = monokai.colors[Syntax_OPERATOR];
    monokai.colors[Syntax_CURSOR] = 0xFFFFFFFF;
    monokai.colors[Syntax_NUMBER] = monokai.colors[Syntax_MACRO];
    monokai.colors[Syntax_STRING] = 0xFFE6DB74;
    editor->settings.theme = monokai;
    
    glEnable(GL_SCISSOR_TEST);
  }
  Editor *editor = &state->editor;
  Renderer *renderer = &state->renderer;
  Font *font = renderer->state.font;
  gl_Funcs gl = os.gl;
  
  profiler_event_count = 0;
  begin_profiler_event("loop");
  scratch_reset();
  
  begin_profiler_event("collect_messages");
  os.collect_messages(memory->window, input);
  end_profiler_event("collect_messages");
  
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
        
        if (key.pressed) {
          Keybind *bind = get_keybind(editor, keycode, 
                                      input->shift, input->ctrl, input->alt);
          if (bind) {
            execute_command(os, editor, renderer, bind->command);
          }
        }
      } break;
      
      default: {} break;
    }
  }
  
  
  
  
  gl.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  gl.Clear(GL_COLOR_BUFFER_BIT);
  
  V2 bottom_left = v2(-renderer->window_size.x*0.5f,
                      -renderer->window_size.y*0.5f);
  V2 top_left = v2_add(bottom_left, v2(0, renderer->window_size.y));
  
  Color_Theme theme = editor->settings.theme;
  
  {
    Panel *active_panel = editor->panels + editor->active_panel_index;
    if (active_panel->type == Panel_Type_BUFFER) {
      if (!input->ctrl && !input->alt) {
        if (input->char_count > 0) {
          String str = make_string(input->chars, input->char_count);
          buffer_input_string(active_panel->buffer_view.buffer, str);
        }
      }
    }
  }
  end_profiler_event("input");
  
  
  begin_profiler_event("render");
  u64 stamp_start = __rdtsc();
  
  u64 stamp_start = __rdtsc();
  
  for (u32 panel_index = 0; panel_index < sb_count(editor->panels); panel_index++) {
    Panel *panel = editor->panels + panel_index;
    
    V2 ws = renderer->window_size;
    Rect2 border_rect = rect2_min_max(v2_hadamard(panel->rect.min, ws),
                                      v2_hadamard(panel->rect.max, ws));
    renderer_begin_render(renderer, border_rect);
    f32 border_thickness = 3;
    
    Rect2 rect = rect2_min_max(v2(border_rect.min.x+4, border_rect.min.y+4),
                               v2(border_rect.max.x+4, border_rect.max.y+4));
    
    switch (panel->type) {
      case Panel_Type_FILE_DIALOG_OPEN: {
        Lister *lister = &editor->current_dir_files;
        u32 file_count = sb_count(lister->items);
        
        V2 pos = top_left;
        for (u32 i = 0; i < file_count; i++) {
          String file_name = lister->items[i];
          
          bool selected = (i32)i == lister->index;
          
          u32 color = theme.colors[Syntax_DEFAULT];
          if (selected) {
            color = theme.colors[Syntax_FUNCTION];
          }
          
          draw_string(renderer, file_name, pos, color);
          pos.y -= font->line_spacing;
        }
      } break;
      
      case Panel_Type_BUFFER: {
        Buffer_View *buffer_view = &panel->buffer_view;
        Buffer *buffer = buffer_view->buffer;
        
        draw_buffer_view(renderer, rect, buffer_view, &editor->settings.theme, &(editor->panels + panel_index)->scroll);
      } break;
    }
    
    draw_rect_outline(renderer, border_rect, border_thickness, 
                      editor->settings.theme.colors[Syntax_DEFAULT]);
    
    renderer_end_render(gl, renderer);
  }
  
  {
    static u64 total = 0;
    static u64 count = 0;
    
    u64 stamp_end = __rdtsc();
    u64 dur = stamp_end - stamp_start;
    total += dur;
    count++;
    
    char buffer[128];
    sprintf_s(buffer, 128, "%lld\n", total/count);
    os.debug_pring(buffer);
  }
  
  end_profiler_event("render");
#if 1
  {
    static u64 total = 0;
    static u64 count = 0;
    
    u64 stamp_end = __rdtsc();
    u64 dur = stamp_end - stamp_start;
    total += dur;
    count++;
    
    char buffer[128];
    sprintf_s(buffer, 128, "%lld\n", total/count);
    os.debug_pring(buffer);
  }
#endif
  
  
  end_profiler_event("loop");
  
  if (input->keys['P'].went_down && input->alt) {
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
  
}

