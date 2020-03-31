#include "editor.h"
#include "renderer.c"


Layout make_layout(Renderer *r, os_Input *input) {
  Layout result = {
    .renderer = r,
    .input = input,
  };
  return result;
}

Item *layout_get_item(Layout *layout, Box box, i32 id) {
  Item *result = layout->items + id;;
  
  if (layout->mode == Layout_Mode_RESOLVE_AUTOS) {
    result->box = box;
    result->parent = layout->current_container;
    result->id = id;
    
    if (layout->current_container) {
      sb_push(layout->current_container->children, result);
    }
  } else {
    if (result->id != id) {
      result = null;
    }
  }
  
  
  return result;
}

V2 get_reported_dims(Item *item) {
  V2 result = v2(item->box.width, item->box.height);
  if (item->box.position == ui_IGNORE_LAYOUT) {
    result = v2_zero();
  }
  
  return result;
}

#define INVALID_ID -1
Item NULL_ITEM = {
  .id = INVALID_ID,
};

#define ui_flex_begin(layout, box) _ui_flex_begin(layout, box, __COUNTER__)
void _ui_flex_begin(Layout *layout, Box box, i32 id) {
  Item *item = layout_get_item(layout, box, id);
  if (!item) {
    NULL_ITEM.parent = layout->current_container;
    layout->current_container = &NULL_ITEM;
    return;
  }
  layout->current_container = item;
  
  switch (layout->mode) {
    case Layout_Mode_RESOLVE_AUTOS: {
      item->children = sb_new(Item, 8);
    } break;
    
    case Layout_Mode_RESOLVE_STRETCHES: {
      if (item->box.direction == ui_VERTICAL) {
        f32 fixed_height = 0;
        i32 stretch_count = 0;
        for (u32 child_index = 0;
             child_index < sb_count(item->children);
             child_index++) 
        {
          Item *child = item->children[child_index];
          
          if (child->box.height == Size_STRETCH) {
            stretch_count++;
          } else {
            fixed_height += child->box.height;
          }
          
          if (child->box.width == Size_STRETCH ||
              item->box.align_content == ui_STRETCH) {
            child->box.width = item->box.width;
          }
        }
        
        f32 height_per_stretch = (item->box.height - fixed_height)/stretch_count;
        
        V2 p = item->p;
        
        for (u32 child_index = 0;
             child_index < sb_count(item->children);
             child_index++) 
        {
          Item *child = item->children[child_index];
          child->p = p;
          if (child->box.height == Size_STRETCH) {
            child->box.height = height_per_stretch;
          }
          p.y -= child->box.height;
        }
      } else if (item->box.direction == ui_HORIZONTAL) {
        f32 fixed_width = 0;
        i32 stretch_count = 0;
        for (u32 child_index = 0;
             child_index < sb_count(item->children);
             child_index++) 
        {
          Item *child = item->children[child_index];
          
          if (child->box.width == Size_STRETCH) {
            stretch_count++;
          } else {
            fixed_width += child->box.width;
          }
          
          if (child->box.height == Size_STRETCH) {
            child->box.height = item->box.height;
          }
        }
        
        f32 width_per_stretch = (item->box.width - fixed_width)/stretch_count;
        
        V2 p = item->p;
        for (u32 child_index = 0;
             child_index < sb_count(item->children);
             child_index++) 
        {
          Item *child = item->children[child_index];
          child->p = p;
          if (child->box.width == Size_STRETCH) {
            child->box.width = width_per_stretch;
          }
          p.x += child->box.width;
        }
      }
    } break;
    
    case Layout_Mode_DRAW: {
      V2 p = v2_add(item->p, layout->p);
      draw_rect(layout->renderer, rect2_min_size(p, v2(item->box.width, -item->box.height)),
                item->box.bg_color);
    } break;
  }
}

void ui_flex_end(Layout *layout) {
  Item *item = layout->current_container;
  layout->current_container = layout->current_container->parent;
  
  if (item == &NULL_ITEM) {
    return;
  }
  
  switch (layout->mode) {
    case Layout_Mode_RESOLVE_AUTOS: {
      f32 total_height = 0;
      f32 total_width = 0;
      
      if (item->box.direction == ui_VERTICAL) {
        for (u32 child_index = 0;
             child_index < sb_count(item->children);
             child_index++) 
        {
          Item *child = item->children[child_index];
          V2 child_dims = get_reported_dims(child);
          
          total_height += child_dims.y;
          
          if (child_dims.x > total_width) {
            total_width = child_dims.x;
          }
        }
      } else if (item->box.direction == ui_HORIZONTAL) {
        for (u32 child_index = 0;
             child_index < sb_count(item->children);
             child_index++) 
        {
          Item *child = item->children[child_index];
          V2 child_dims = get_reported_dims(child);
          
          total_width += child_dims.x;
          
          if (child_dims.y > total_height) {
            total_height = child_dims.y;
          }
        }
      }
      
      if (item->box.height == Size_AUTO) {
        item->box.height = total_height;
      }
      if (item->box.width == Size_AUTO) {
        item->box.width = total_width;
      }
    } break;
    
    case Layout_Mode_RESOLVE_STRETCHES: {
      
    } break;
    
    case Layout_Mode_DRAW: {
    } break;
  }
}

#define ui_button(layout, str, box) _ui_button(layout, str, box, __COUNTER__)
bool _ui_button(Layout *layout, String str, Box box, i32 id) {
  Item *item = layout_get_item(layout, box, id);
  if (!item) return false;
  item->box.bg_color = 0xFF555555;
  
  bool result = false;
  
  switch (layout->mode) {
    case Layout_Mode_RESOLVE_AUTOS: {
      if (item->box.height == Size_AUTO) {
        item->box.height = layout->renderer->state.font->line_height;
      }
      if (item->box.width == Size_AUTO) {
        item->box.width = measure_string_width(layout->renderer, str) +
          item->box.padding_left + item->box.padding_right;
      }
    } break;
    
    case Layout_Mode_DRAW: {
      V2 p = v2_add(item->p, layout->p);
      V2 rect_p = v2(p.x, p.y - item->box.height);
      Rect2 rect = rect2_min_size(rect_p, 
                                  v2(item->box.width, item->box.height));
      draw_rect(layout->renderer, rect, item->box.bg_color);
      if (point_in_rect(layout->input->mouse.p, rect)) {
        draw_rect_outline(layout->renderer, rect, 2, 0xFFFFFFFF);
        if (layout->input->mouse.left.went_down) {
          result = true;
        }
      }
      
      draw_string(layout->renderer, str, v2(p.x + item->box.padding_left, p.y), 0xFFFFFFFF);
    } break;
  }
  
  return result;
}

#define ui_panel(layout, box) _ui_panel(layout, box, __COUNTER__)
void _ui_panel(Layout *layout, Box box, i32 id) {
  Item *item = layout_get_item(layout, box, id);
  if (!item) return;
  
  switch (layout->mode) {
    case Layout_Mode_DRAW: {
      V2 p = v2_add(item->p, layout->p);
      draw_rect(layout->renderer, rect2_min_size(p, v2(item->box.width, -item->box.height)),
                item->box.bg_color);
    } break;
  }
}


typedef void Layouting_Fn(Layout *, void *);
void layouting_fn(Layout *l, Editor *editor) {
  Renderer *r = l->renderer;
  f32 line_height = 40;
  
  ui_flex_begin(l, ((Box){ 
                    .direction = ui_VERTICAL,
                    .width = r->window_size.x,
                    .height = r->window_size.y,
                    .bg_color = 0xFF000000,
                    })); {
    ui_flex_begin(l, ((Box){ 
                      .direction = ui_HORIZONTAL,
                      .bg_color = 0xFF0000FF,
                      .width = Size_STRETCH,
                      })); {
      Box button_box = (Box){
        .padding_left = 8,
        .padding_right = 8,
      };
      
      Box submenu_box = (Box){
        .position = ui_IGNORE_LAYOUT,
        .direction = ui_VERTICAL,
        .width = Size_AUTO,
        .bg_color = 0xFF888888,
        .align_content = ui_STRETCH,
      };
      
      
      l->renderer->state.z = 1.0f;
      
      ui_flex_begin(l, (Box){0}); {
        if (ui_button(l, const_string("file"), button_box)) {
          editor->menu_index = 0;
        }
        if (editor->menu_index == 0) {
          ui_flex_begin(l, submenu_box); {
            ui_button(l, const_string("open"), button_box);
            ui_button(l, const_string("save"), button_box);
            ui_button(l, const_string("settings"), button_box);
            ui_button(l, const_string("exit"), button_box);
          } ui_flex_end(l);
        }
      } ui_flex_end(l);
      
      
      ui_flex_begin(l, (Box){0}); {
        if (ui_button(l, const_string("edit"), button_box)) {
          editor->menu_index = 1;
        }
        if (editor->menu_index == 1) {
          ui_flex_begin(l, submenu_box); {
            ui_button(l, const_string("copy"), button_box);
            ui_button(l, const_string("paste"), button_box);
            ui_button(l, const_string("cut"), button_box);
            ui_button(l, const_string("undo"), button_box);
            ui_button(l, const_string("redo"), button_box);
          } ui_flex_end(l);
        }
      } ui_flex_end(l);
      
      ui_flex_begin(l, (Box){0}); {
        if (ui_button(l, const_string("panels"), button_box)) {
          editor->menu_index = 2;
        }
        if (editor->menu_index == 2) {
          ui_flex_begin(l, submenu_box); {
            ui_button(l, const_string("foo"), button_box);
            ui_button(l, const_string("bar"), button_box);
            ui_button(l, const_string("baz"), button_box);
          } ui_flex_end(l);
        }
      } ui_flex_end(l);
      
      ui_flex_begin(l, (Box){0}); {
        if (ui_button(l, const_string("commands"), button_box)) {
          editor->menu_index = 3;
        }
        if (editor->menu_index == 3) {
          ui_flex_begin(l, submenu_box); {
            ui_button(l, const_string("run command"), button_box);
          } ui_flex_end(l);
        }
      } ui_flex_end(l);
      
      l->renderer->state.z = 0;
    } ui_flex_end(l);
    
    ui_flex_begin(l, ((Box){ 
                      .direction = ui_HORIZONTAL,
                      .width = Size_STRETCH,
                      .height = Size_STRETCH,
                      })); {
      ui_panel(l, ((Box){ 
                   .bg_color = 0xFFFF0000, 
                   .width = Size_STRETCH, 
                   .height = Size_STRETCH,
                   }));
      ui_panel(l, ((Box){ .bg_color = 0xFF00FF00, .width = Size_STRETCH,
                   .height = Size_STRETCH }));
    } ui_flex_end(l);
  } ui_flex_end(l);
}

void do_layout(Layout *layout, Layouting_Fn *fn, void *data) {
  layout->p = v2_zero();
  layout->current_container = null;
  V2 ws = layout->renderer->window_size;
  layout->p = v2(-ws.x*0.5f, ws.y*0.5f),
  
  layout->mode = Layout_Mode_RESOLVE_AUTOS;
  layout->current_container = null;
  fn(layout, data);
  
  layout->mode = Layout_Mode_RESOLVE_STRETCHES;
  layout->current_container = null;
  fn(layout, data);
  
  layout->mode = Layout_Mode_DRAW;
  layout->current_container = null;
  fn(layout, data);
}

void foo(Layout *layout, Renderer *r, os_Input *input, Editor *editor) {
  do_layout(layout, layouting_fn, editor);
}


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

void execute_command(Os os, Editor *editor, Renderer *renderer, Command command) {
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
#if 0
      panel->type = Panel_Type_FILE_DIALOG_OPEN;
      
      Lister *lister = &editor->current_dir_files;
      lister->items = os.get_file_names(const_string("src"));
#endif
    } break;
    
    case Command_LISTER_SELECT: {
      // TODO(lvl5): handle file already open
      // TODO(lvl5): handle creating new file
      
#if 0
      Lister *lister = &editor->current_dir_files;
      String file_name = lister->items[lister->index];
      if (string_compare(const_string(".."), file_name)) {
        
      } else {
        String path = concat(const_string("src/"), file_name);
        
        Buffer *buffer = get_existing_buffer(editor, path);
        if (!buffer) {
          buffer = open_file_into_new_buffer(os, editor, alloc_string(path.data, path.count));
        }
        
        panel->type = Panel_Type_BUFFER;
        panel->buffer_view = (Buffer_View){
          .buffer = buffer,
        };
      }
#endif
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
      editor->layout = make_layout(renderer, input);
      
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
  
  
  gl.ClearColor(0, 0, 0, 1);
  gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  
  V2 ws = renderer->window_size;
  f32 menu_height = font->line_height;
  
#if 0  
  glEnable(GL_SCISSOR_TEST);
  for (u32 panel_index = 0; panel_index < sb_count(editor->panels); panel_index++) {
    Panel *panel = editor->panels + panel_index;
    
    
    
    Rect2 border_rect = rect2_min_max(v2_hadamard(panel->rect.min, ws),
                                      v2_hadamard(panel->rect.max, ws));
    
    renderer_begin_render(renderer, border_rect);
    border_rect.max.y -= menu_height;
    f32 border_thickness = 3;
    f32 header_height = font->line_height;
    
    Rect2 rect = rect2_min_max(v2(border_rect.min.x+border_thickness, 
                                  border_rect.min.y+border_thickness),
                               v2(border_rect.max.x-border_thickness, border_rect.max.y-header_height));
    
    draw_rect(renderer, rect, theme.colors[Syntax_BACKGROUND]);
    
    String panel_name = {0};
    
    switch (panel->type) {
      case Panel_Type_FILE_DIALOG_OPEN: {
#if 0
        Lister *lister = &editor->current_dir_files;
        u32 file_count = sb_count(lister->items);
        
        V2 pos = v2(rect.min.x, rect.max.y);
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
#endif
      } break;
      
      case Panel_Type_BUFFER: {
        Buffer_View *buffer_view = &panel->buffer_view;
        Buffer *buffer = buffer_view->buffer;
        
        draw_buffer_view(renderer, rect, buffer_view, &editor->settings.theme, &(editor->panels + panel_index)->scroll);
        panel_name = buffer->path;
      } break;
    }
    
    {
      V2 panel_size = rect2_get_size(border_rect);
      V2 header_size = v2(panel_size.x, header_height);
      Rect2 header = rect2_min_size(v2(border_rect.min.x, 
                                       border_rect.max.y - header_height),
                                    header_size);
      draw_rect(renderer, header, theme.colors[Syntax_COMMENT]);
      draw_string(renderer, panel_name, v2(rect.min.x, header.max.y), theme.colors[Syntax_DEFAULT]);
    }
    
    draw_rect_outline(renderer, border_rect, border_thickness, 
                      editor->settings.theme.colors[Syntax_COMMENT]);
    
    renderer_end_render(gl, renderer);
  }
  
  glDisable(GL_SCISSOR_TEST);
  renderer_begin_render(renderer, rect2_min_size(v2_zero(), ws));
  {
    V2 menu_size = v2(ws.x, menu_height);
    Rect2 menu_rect = rect2_min_size(v2(-ws.x*0.5f, ws.y*0.5f - menu_height),
                                     menu_size);
    draw_rect(renderer, menu_rect, theme.colors[Syntax_BACKGROUND]);
    
    f32 menu_item_padding = 8;
    String menu_items[] = {
      arr_string("file"),
      arr_string("edit"),
      arr_string("panels"),
      arr_string("commands"),
      arr_string("about"),
    };
    
    V2 offset = rect2_top_left(menu_rect);
    for (i32 i = 0; i < array_count(menu_items); i++) {
      String name = menu_items[i];
      f32 string_width = measure_string_width(renderer, name);
      f32 rect_width = menu_item_padding*2 + string_width;
      
      Rect2 rect = rect2_min_size(v2_add(offset, v2(0, -menu_height)), 
                                  v2(rect_width, menu_height));
      
      bool mouse_in_menu = false;
      
      if (point_in_rect(input->mouse.p, rect)) {
        mouse_in_menu = true;
        
        draw_rect_outline(renderer, rect, 2, theme.colors[Syntax_COMMENT]);
        if (input->mouse.left.went_down) {
          editor->menu_index = i;
        }
      }
      
      
      if (editor->menu_index == i) {
        draw_rect(renderer, rect, theme.colors[Syntax_COMMENT]);
        for (u32 j = 0; j < array_count(menu_items); j++) {
          String name = menu_items[j];
          f32 string_width = 200;
          V2 min = v2(offset.x, 
                      offset.y - menu_height- (j+1)*renderer->state.font->line_height);
          
          Rect2 submenu_rect = 
            rect2_min_size(min, 
                           v2(string_width, 
                              renderer->state.font->line_height));
          
          if (point_in_rect(input->mouse.p, submenu_rect)) {
            draw_rect(renderer, submenu_rect, theme.colors[Syntax_COMMENT]);
            mouse_in_menu = true;
          } else {
            draw_rect(renderer, submenu_rect, 0xFF444444);
          }
          
          min.y += renderer->state.font->line_height;
          min.x += menu_item_padding;
          draw_string(renderer, name, min, theme.colors[Syntax_DEFAULT]);
        }
      }
      
      if (!mouse_in_menu && editor->menu_index == i) {
        editor->menu_index = -1;
      }
      
      
      draw_string(renderer, name, v2_add(offset, v2(menu_item_padding, 0)),
                  theme.colors[Syntax_DEFAULT]);
      
      offset = v2_add(offset, v2(rect_width, 0));
    }
  }
#endif
  
  renderer_begin_render(renderer, rect2_min_size(v2_mul(ws, -0.5f), ws));
  
  
  foo(&editor->layout, renderer, input, editor);
  
  renderer_end_render(gl, renderer);
  
  
  
  static bool true_once = true;
  
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

