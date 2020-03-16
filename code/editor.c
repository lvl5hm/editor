#include "editor.h"
#include "buffer.c"


Keybind *get_keybind(Editor *editor, os_Keycode keycode, 
                     bool shift, bool ctrl, bool alt) 
{
  Settings *settings = &editor->settings;
  Panel *panel = editor->panels + editor->active_panel_index;
  View *view = editor->views + panel->view_index;
  
  Keybind *result = null;
  for (u32 i = 0; i < sb_count(settings->keybinds); i++) {
    Keybind *k = settings->keybinds + i;
    if (k->keycode == keycode &&
        k->shift == shift && 
        k->ctrl == ctrl && 
        k->alt == alt &&
        (k->views & view->type))
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
  View *view = editor->views + panel->view_index;
  
  Buffer *buffer = null;
  File_Dialog *file_dialog = null;
  Settings_Editor *settings_editor = null;
  switch (view->type) {
    case View_Type_BUFFER: {
      buffer = &view->buffer;
    } break;
    case View_Type_FILE_DIALOG: {
      file_dialog = &view->file_dialog;
    } break;
    case View_Type_SETTINGS: {
      settings_editor = &view->settings_editor;
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
    case Command_OPEN_FILE: {
      panel->view_index = 0; // 0 is always file dialog for now
      View *view = editor->views + panel->view_index;
      
      Lister *lister = &editor->current_dir_files;
      lister->items = os.get_file_names(const_string("src"));
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
    
    
    
    state->font = os.load_font(const_string("fonts/ubuntu_mono.ttf"), 
                               const_string("Ubuntu Mono"),
                               24);
    
    V2 window_size = os.get_window_size(memory->window);
    init_renderer(os.gl, renderer, shader, &state->font, window_size);
    
    
    Editor *editor = &state->editor;
    
    {
      editor->views = sb_new(View, 16);
      {
        Buffer buffer = {0};
        buffer.data = alloc_array(char, 128);
        buffer.capacity = 128;
        buffer.count = 0;
        buffer.cursor = 0;
        buffer.file_name = const_string("src/test.c");
        
        String *files_in_folder = os.get_file_names(const_string("src"));
        
        
        String str;
        os_File file = os.open_file(buffer.file_name);
        u64 file_size = os.get_file_size(file);
        char *file_memory = alloc_array(char, file_size + 1);
        os.read_file(file, file_memory, 0, file_size);
        os.close_file(file);
        
        str = make_string(file_memory, file_size + 1);
        str.data[file_size] = 0; // TODO: all files should end with \0
        buffer_insert_string(&buffer, str);
        set_cursor(&buffer, 0);
        
        
        
        
        
        sb_push(editor->views, ((View){
                                .type = View_Type_BUFFER,
                                .buffer = buffer,
                                }));
      }
      
      {
        Buffer buffer = {0};
        buffer.data = alloc_array(char, 128);
        buffer.capacity = 128;
        buffer.count = 0;
        buffer.cursor = 0;
        buffer.file_name = const_string("src/foo.c");
        
        String *files_in_folder = os.get_file_names(const_string("src"));
        
        
        String str;
        os_File file = os.open_file(buffer.file_name);
        u64 file_size = os.get_file_size(file);
        char *file_memory = alloc_array(char, file_size + 1);
        os.read_file(file, file_memory, 0, file_size);
        os.close_file(file);
        
        str = make_string(file_memory, file_size + 1);
        str.data[file_size] = 0; // TODO: all files should end with \0
        buffer_insert_string(&buffer, str);
        set_cursor(&buffer, 0);
        
        
        
        
        sb_push(editor->views, ((View){
                                .type = View_Type_BUFFER,
                                .buffer = buffer,
                                }));
      }
      
      
      editor->panels = sb_new(Panel, 16);
      
      V2 ws = renderer->window_size;
      V2 bottom_left = v2(-0.5f, -0.5f);
      V2 panel_size = v2(0.5f, 1.0f);
      sb_push(editor->panels, ((Panel){
                               .view_index = 0,
                               .rect = rect2_min_size(bottom_left, 
                                                      panel_size),
                               }));
      sb_push(editor->panels, ((Panel){
                               .view_index = 1,
                               .rect = rect2_min_size(v2(0, -0.5f), 
                                                      panel_size),
                               }));
    }
    
    
    
    editor->active_panel_index = 1;
    
    editor->settings.keybinds = sb_new(Keybind, 32);
    Keybind *keybinds = editor->settings.keybinds;
    
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_SET_MARK,
                       .keycode = os_Keycode_SPACE,
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_COPY,
                       .keycode = 'C',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_PASTE,
                       .keycode = 'V',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_CUT,
                       .keycode = 'X',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_LEFT,
                       .keycode = os_Keycode_ARROW_LEFT,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_RIGHT,
                       .keycode = os_Keycode_ARROW_RIGHT,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_UP,
                       .keycode = os_Keycode_ARROW_UP,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_DOWN,
                       .keycode = os_Keycode_ARROW_DOWN,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_REMOVE_BACKWARD,
                       .keycode = os_Keycode_BACKSPACE,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_REMOVE_FORWARD,
                       .keycode = os_Keycode_DELETE,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_NEWLINE,
                       .keycode = os_Keycode_ENTER,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER,
                       .command = Command_TAB,
                       .keycode = os_Keycode_TAB,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_BUFFER 
                       | View_Type_FILE_DIALOG,
                       .command = Command_OPEN_FILE,
                       .keycode = 'O',
                       .ctrl = true,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_FILE_DIALOG,
                       .command = Command_LISTER_MOVE_UP,
                       .keycode = os_Keycode_ARROW_UP,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = View_Type_FILE_DIALOG,
                       .command = Command_LISTER_MOVE_DOWN,
                       .keycode = os_Keycode_ARROW_DOWN,
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
    monokai.colors[Syntax_CURSOR] = 0xFF00FF00;
    monokai.colors[Syntax_NUMBER] = monokai.colors[Syntax_MACRO];
    monokai.colors[Syntax_STRING] = 0xFFE6DB74;
    editor->settings.theme = monokai;
    
    
    glEnable(GL_SCISSOR_TEST);
  }
  Editor *editor = &state->editor;
  Renderer *renderer = &state->renderer;
  Font *font = renderer->state.font;
  gl_Funcs gl = os.gl;
  
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
  
  
  
  end_profiler_event("input");
  
  gl.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  gl.Clear(GL_COLOR_BUFFER_BIT);
  
  V2 bottom_left = v2(-renderer->window_size.x*0.5f,
                      -renderer->window_size.y*0.5f);
  V2 top_left = v2_add(bottom_left, v2(0, renderer->window_size.y));
  
  Color_Theme theme = editor->settings.theme;
  
  {
    Panel *active_panel = editor->panels + editor->active_panel_index;
    View *active_view = editor->views + active_panel->view_index;
    if (active_view->type == View_Type_BUFFER) {
      if (!input->ctrl && !input->alt) {
        if (input->char_count > 0) {
          String str = make_string(input->chars, input->char_count);
          buffer_input_string(&active_view->buffer, str);
        }
      }
    }
  }
  
  for (u32 panel_index = 0; panel_index < sb_count(editor->panels); panel_index++) {
    Panel *panel = editor->panels + panel_index;
    View *view = editor->views + panel->view_index;
    
    V2 ws = renderer->window_size;
    Rect2 border_rect = rect2_min_max(v2_hadamard(panel->rect.min, ws),
                                      v2_hadamard(panel->rect.max, ws));
    renderer_begin_render(renderer, border_rect);
    
    f32 border_thickness = 4;
    
    Rect2 rect = rect2_min_max(v2(border_rect.min.x+4, border_rect.min.y+4),
                               v2(border_rect.max.x+4, border_rect.max.y+4));
    
    switch (view->type) {
      case View_Type_FILE_DIALOG: {
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
          
          draw_string(renderer, file_name, pos, color_u32_to_v4(color));
          pos.y -= font->line_spacing;
        }
      } break;
      
      case View_Type_BUFFER: {
        Buffer *buffer = &view->buffer;
        buffer_draw(renderer, buffer, rect, editor->settings.theme);
      } break;
    }
    
    draw_rect_outline(renderer, border_rect, border_thickness, 
                      color_u32_to_v4(editor->settings.theme.colors[Syntax_DEFAULT]));
    
    renderer_end_render(gl, renderer);
  }
}
