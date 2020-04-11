#include "editor.h"
#include "layout.c"


Keybind *get_keybind(Editor *editor, os_Keycode keycode, 
                     bool shift, bool ctrl, bool alt) 
{
  begin_profiler_function();
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
  end_profiler_function();
  return result;
}

Buffer *open_file_into_new_buffer(Os os, Editor *editor, String path) 
{
  begin_profiler_function();
  Buffer *buffer = make_empty_buffer(editor, path);
  
  os_File file = os.open_file(path);
  u64 file_size = os.get_file_size(file);
  char *file_memory = alloc_array(char, file_size);
  os.read_file(file, file_memory, 0, file_size);
  os.close_file(file);
  
  String str = make_string(file_memory, file_size);
  buffer_insert_string(buffer, str);
  free_memory(file_memory);
  
  set_cursor(buffer, 0);
  Buffer *inserted = editor->buffers + sb_count(editor->buffers) - 1;
  
  end_profiler_function();
  return inserted;
}

void execute_command(Editor *editor, Renderer *renderer, Command command) {
  begin_profiler_function();
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
      buffer_copy(buffer, &editor->exchange);
    } break;
    case Command_PASTE: {
      buffer_paste(buffer, &editor->exchange);
    } break;
    case Command_CUT: {
      buffer_cut(buffer, &editor->exchange);
    } break;
    
    case Command_MOVE_CURSOR_WORD_START:
    case Command_MOVE_CURSOR_WORD_END:
    case Command_MOVE_CURSOR_LINE_END:
    case Command_MOVE_CURSOR_LINE_START:
    case Command_MOVE_CURSOR_RIGHT:
    case Command_MOVE_CURSOR_UP:
    case Command_MOVE_CURSOR_DOWN:
    case Command_MOVE_CURSOR_LEFT: {
      move_cursor_direction(font, buffer, command);
    } break;
    
    case Command_LISTER_MOVE_DOWN:
    case Command_LISTER_MOVE_UP: {
#if 0
      File_Dialog;
      
      i32 count = (i32)sb_count(lister->items);
      if (command == Command_LISTER_MOVE_DOWN) {
        lister->index++;
        if (lister->index >= count) lister->index = 0;
      } else if (command == Command_LISTER_MOVE_UP) {
        lister->index--;
        if (lister->index < 0) lister->index = count-1;
      }
#endif
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
      editor->file_dialog_open = true;
      
      Context *cur = get_context();
      Context system_ctx = *cur;
      system_ctx.allocator = system_allocator;
      push_context(system_ctx);
      editor->files = global_os.get_file_names(editor->path);
      pop_context();
    } break;
    
    case Command_FILE_OPEN: {
      String file_name = editor->selected_file_name;
      if (string_compare(const_string(".."), file_name)) {
        
      } else {
        String path = concat(const_string("src/"), file_name);
        
        Buffer *buffer = get_existing_buffer(editor, path);
        if (!buffer) {
          buffer = open_file_into_new_buffer(global_os, editor, alloc_string(path.data, path.count));
        }
        
        panel->type = Panel_Type_BUFFER;
        panel->buffer_view = (Buffer_View){
          .buffer = buffer,
        };
      }
      editor->file_dialog_open = false;
    } break;
  }
  end_profiler_function();
}

extern void thread_handle_reload(Global_Context_Info *info, Os os) {
  global_context_info = info;
  
  profiler_events = os.profiler_events;
  profiler_event_capacity = os.profiler_event_capacity;
  profiler_event_count = os.profiler_event_count;
  global_os = os;
  
  for (Token_Type i = T_KEYWORD_FIRST; i <= T_KEYWORD_LAST; i++) {
    String key = Token_Kind_To_String[i];
    u32 index = keyword_map_get_index(&keyword_map, key);
    keyword_map.keys[index] = key;
    keyword_map.values[index] = i;
  }
  for (Token_Type i = T_TYPE_FIRST; i <= T_TYPE_LAST; i++) {
    String key = Token_Kind_To_String[i];
    u32 index = keyword_map_get_index(&keyword_map, key);
    keyword_map.keys[index] = key;
    keyword_map.values[index] = i;
  }
}


void draw_command_button(ui_Layout *layout, ui_Id id, String label, Command command) {
  if (ui_button(layout, id, label, default_button_style())) {
    execute_command(layout->editor, layout->renderer, command);
  }
}

void draw_layout(ui_Layout *layout, Renderer *r, os_Input *input) {
  push_scratch_context();
  
  Editor *editor = layout->editor;
  
  f32 line_height = 40;
  ui_Layout *l = layout;
  layout->current_container = null;
  l->p = v2(-r->window_size.x*0.5f, r->window_size.y*0.5f);
  
  ui_flex_begin(l, (Style){ 
                .flags = ui_ALIGN_CENTER,
                .width = px(r->window_size.x),
                .height = px(r->window_size.y),
                .bg_color = 0xFF000000,
                }); {
    Style button_box = default_button_style();
    Style menu_bar_style = (Style){
      .bg_color = 0xFF333333,
      .width = ui_SIZE_STRETCH,
    };
    
    ui_menu_bar_begin(l, menu_bar_style); {
      ui_dropdown_menu_begin(l, ui_id(), const_string("file"), (Style){0}); {
        draw_command_button(l, ui_id(), const_string("new"), Command_OPEN_FILE_DIALOG);
        draw_command_button(l, ui_id(), const_string("open"), Command_OPEN_FILE_DIALOG);
        ui_button(l, ui_id(), const_string("save"), button_box);
        ui_button(l, ui_id(), const_string("exit"), button_box);
        
        ui_dropdown_menu_begin(l, ui_id(), const_string("settings"), (Style){
                               .flags = ui_HORIZONTAL,
                               }); 
        {
          ui_button(l, ui_id(), const_string("color theme"), button_box);
          ui_button(l, ui_id(), const_string("options"), button_box);
        } ui_dropdown_menu_end(l);
      } ui_dropdown_menu_end(l);
      
      ui_dropdown_menu_begin(l, ui_id(), const_string("edit"), (Style){0}); {
        draw_command_button(l, ui_id(), const_string("copy"), Command_COPY);
        draw_command_button(l, ui_id(), const_string("paste"), Command_PASTE);
        draw_command_button(l, ui_id(), const_string("cut"), Command_CUT);
        ui_button(l, ui_id(), const_string("undo"), button_box);
        ui_button(l, ui_id(), const_string("redo"), button_box);
      } ui_dropdown_menu_end(l);
      
      ui_dropdown_menu_begin(l, ui_id(), const_string("panels"), (Style){0}); {
        ui_button(l, ui_id(), const_string("foo"), button_box);
        ui_button(l, ui_id(), const_string("bar"), button_box);
        ui_button(l, ui_id(), const_string("baz"), button_box);
      } ui_dropdown_menu_end(l);
      
      ui_dropdown_menu_begin(l, ui_id(), const_string("commands"), (Style){0}); {
        ui_button(l, ui_id(), const_string("run command"), button_box);
      } ui_dropdown_menu_end(l);
    } ui_menu_bar_end(l);
    
    
    // NOTE(lvl5): file lister
    if (editor->file_dialog_open) {
      ui_flex_begin(l, (Style){
                    .flags = ui_IGNORE_LAYOUT|ui_ALIGN_STRETCH,
                    .layer = 2,
                    .width = 400,
                    });
      
      for (u8 i = 0; i < sb_count(editor->files); i++) {
        if (ui_button(l, ui_id_loop(i), editor->files[i], button_box)) {
          editor->selected_file_name = editor->files[i];
          execute_command(l->editor, l->renderer, Command_FILE_OPEN);
        }
      }
      
      ui_flex_end(l);
    }
    
    ui_flex_begin(l, (Style){ 
                  .flags = ui_HORIZONTAL,
                  .width = px(ui_SIZE_STRETCH),
                  .height = px(ui_SIZE_STRETCH),
                  }); 
    {
      ui_panel(l, layout->editor->panels + 0, (Style){ 
               .bg_color = 0xFFFF0000, 
               .width = px(ui_SIZE_STRETCH), 
               .height = px(ui_SIZE_STRETCH),
               });
      ui_panel(l, layout->editor->panels + 1, (Style){ 
               .bg_color = 0xFFFF0000, 
               .width = px(ui_SIZE_STRETCH), 
               .height = px(ui_SIZE_STRETCH),
               });
    } ui_flex_end(l);
  } ui_flex_end(l);
  
  
  traverse_layout(l, l->current_container);
  
  pop_context();
}


extern void editor_update(Os os, Editor_Memory *memory, os_Input *input) {
  App_State *state = (App_State *)memory->data;
  
  profiler_event_count = 0;
  
  Context *_old_context = get_context();
  Context ctx = *_old_context;
  ctx.allocator = system_allocator;
  push_context(ctx);
  scratch_reset();
  
  
  
  if (!memory->initialized) {
    u64 stamp_start = __rdtsc();
    
    memory->initialized = true;
    
    GLuint shader = gl_create_shader_from_file(os.gl, os.read_entire_file, const_string("shader.glsl"));
    Renderer *renderer = &state->renderer;
    
    
    
    state->font = os.load_font(const_string("fonts/inconsolata.ttf"), 
                               const_string("Inconsolata"),
                               26);
    
    V2 window_size = os.get_window_size(memory->window);
    init_renderer(os.gl, renderer, shader, &state->font, window_size);
    
    
    Editor *editor = &state->editor;
    Editor zero_editor = {0};
    *editor = zero_editor;
    
    {
      editor->buffers = sb_new(Buffer, 16);
      editor->panels = sb_new(Panel, 16);
      editor->layout = make_layout(renderer, input, editor);
      editor->path = const_string("src");
      
      Buffer *buffer = make_empty_buffer(editor, const_string("<scratch>"));
      
      
      V2 ws = renderer->window_size;
      V2 bottom_left = v2(-0.5f, -0.5f);
      V2 panel_size = v2(1.0f, 1.0f);
      
      {
        Panel panel = (Panel){
          .buffer_view = (Buffer_View){
            .buffer = buffer,
          },
          .type = Panel_Type_BUFFER,
        };
        sb_push(editor->panels, panel);
      }
      {
        Panel panel = (Panel){
          .buffer_view = (Buffer_View){
            .buffer = buffer,
          },
          .type = Panel_Type_BUFFER,
        };
        sb_push(editor->panels, panel);
      }
    }
    
    
    
    editor->active_panel_index = 0;
    editor->menu_index = -1;
    
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
                       .command = Command_MOVE_CURSOR_LINE_START,
                       .keycode = os_Keycode_HOME,
                       }));
    sb_push(keybinds, ((Keybind){
                       .views = Panel_Type_BUFFER,
                       .command = Command_MOVE_CURSOR_LINE_END,
                       .keycode = os_Keycode_END,
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
                       .views = Panel_Type_BUFFER,
                       .command = Command_OPEN_FILE_DIALOG,
                       .keycode = 'O',
                       .ctrl = true,
                       }));
#if 0
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
#endif
    
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
    monokai.colors[Syntax_ENUM_MEMBER] = monokai.colors[Syntax_STRING];
    editor->settings.theme = monokai;
    
    
#if 0    
    Lister *lister = &editor->current_dir_files;
    lister->items = os.get_file_names(const_string("src"));
    for (u32 i = 0; i < sb_count(lister->items); i++) {
      String file_name = lister->items[i];
      String path = concat(const_string("src/"), file_name);
      
      Buffer *buffer = get_existing_buffer(editor, file_name);
      if (!buffer) {
        buffer = open_file_into_new_buffer(os, editor, path);
        buffer->file_name = alloc_string(file_name.data, file_name.count);
      }
    }
#endif
    
    
    
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
    
  }
  
  //glEnable(GL_DEPTH_TEST);
  //glDisable(GL_DEPTH_TEST);
  
  Editor *editor = &state->editor;
  Renderer *renderer = &state->renderer;
  Font *font = renderer->state.font;
  gl_Funcs gl = os.gl;
  
  
  
  
  os.collect_messages(memory->window, input);
  
  
  
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
            execute_command(editor, renderer, bind->command);
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
  
  
  gl.ClearColor(0, 0, 0, 1);
  gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  
  V2 ws = renderer->window_size;
  f32 menu_height = font->line_height;
  
  
  renderer_begin_render(renderer, rect2_min_size(v2_mul(ws, -0.5f), ws));
  draw_layout(&editor->layout, renderer, input);
  renderer_end_render(gl, renderer);
  
  
  
  static bool true_once = false;
  
  if (true_once) {
    true_once = false;
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
  
  pop_context();
}

