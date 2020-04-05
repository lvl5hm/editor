#include "editor.h"
#include "renderer.c"


ui_Layout make_layout(Renderer *r, os_Input *input) {
  ui_Layout result = {
    .renderer = r,
    .input = input,
  };
  return result;
}


#define ui_id_loop_func(l, f) (ui_Id){.call = __COUNTER__ + 1, .loop = l, .func = f}
#define ui_id_loop(l) ui_id_loop_func(l, 0)
#define ui_id_func(f) ui_id_loop_func(0, f)
#define ui_id() ui_id_loop_func(0, 0)

bool ui_ids_equal(ui_Id a, ui_Id b) {
  bool result = a.call == b.call && a.loop == b.loop && a.func == b.func;
  return result;
}

u32 hash_ui_id(ui_Id id) {
  u32 result = (id.call) ^ (id.loop << 7) ^ (id.func << 13);
  return result;
}

u32 get_ui_state_index(ui_Layout *layout, ui_Id id) {
  u32 hash = hash_ui_id(id);
  u32 result = 0;
  
  while (true) {
    hash = hash % array_count(layout->keys);
    ui_Id key = layout->keys[hash];
    
    if (!layout->occupancy[hash]) {
      result = hash;
      break;
    } else if (ui_ids_equal(key, id)) {
      result = hash;
      break;
    } else {
      hash++;
    }
  }
  
  return result;
}

bool ui_id_valid(ui_Id id) {
  bool result = !ui_ids_equal(id, INVALID_UI_ID);
  return result;
}

ui_State *layout_get_state(ui_Layout *layout, ui_Id id) {
  ui_State *result = null;
  
  if (ui_id_valid(id)) {
    u32 index = get_ui_state_index(layout, id);
    result = layout->values + index;
    if (!layout->occupancy[index]) {
      layout->keys[index] = id;
      layout->occupancy[index] = true;
    }
  }
  return result;
}


ui_Item *layout_get_item(ui_Layout *layout, Item_Type type, Style style) {
  ui_Item *result = null;
  if (layout->current_container) {
    u32 index = sb_count(layout->current_container->children);
    sb_push(layout->current_container->children, (ui_Item){0});
    result = layout->current_container->children + index;
  } else {
    result = alloc_struct(ui_Item);
    ui_Item zero_item = {0};
    *result = zero_item;
  }
  
  result->style = style;
  result->parent = layout->current_container;
  result->type = type;
  result->children = sb_new(ui_Item, 8);
  
  return result;
}

V2 get_reported_dims(ui_Item *item) {
  V2 result = v2(item->style.width, item->style.height);
  if (flag_is_set(item->style.flags, ui_IGNORE_LAYOUT) ||
      flag_is_set(item->style.flags, ui_HIDDEN)) 
  {
    result = v2_zero();
  }
  
  return result;
}

void ui_flex_begin_ex(ui_Layout *layout, Style style, Item_Type type) {
  ui_Item *item = layout_get_item(layout, type, style);
  layout->current_container = item;
}
void ui_flex_begin(ui_Layout *layout, Style style) {
  ui_flex_begin_ex(layout, style, Item_Type_FLEX);
}

void ui_flex_end(ui_Layout *layout) {
  ui_Item *item = layout->current_container;
  if (layout->current_container->parent) {
    layout->current_container = layout->current_container->parent;
  }
  
  f32 total_height = 0;
  f32 total_width = 0;
  
  bool is_horizontal = flag_is_set(item->style.flags, ui_HORIZONTAL);
  
  if (is_horizontal) {
    for (u32 child_index = 0;
         child_index < sb_count(item->children);
         child_index++) 
    {
      ui_Item *child = item->children + child_index;
      V2 child_dims = get_reported_dims(child);
      
      total_width += child_dims.x;
      
      if (child_dims.y > total_height) {
        total_height = child_dims.y;
      }
    }
  } else {
    for (u32 child_index = 0;
         child_index < sb_count(item->children);
         child_index++) 
    {
      ui_Item *child = item->children + child_index;
      V2 child_dims = get_reported_dims(child);
      
      total_height += child_dims.y;
      
      if (child_dims.x > total_width) {
        total_width = child_dims.x;
      }
    }
  }
  
  if (item->style.height == ui_SIZE_AUTO) {
    item->style.height = total_height;
  }
  if (item->style.width == ui_SIZE_AUTO) {
    item->style.width = total_width;
  }
}

Style default_button_style() {
  Style result = {
    .bg_color = 0xFF555555,
    .padding_left = 8,
    .padding_right = 8,
  };
  return result;
}

V2 ui_compute_text_size(ui_Layout *layout, String str, Style style) {
  V2 result = v2(style.width, style.height);
  
  if (style.height == ui_SIZE_AUTO) {
    result.y = layout->renderer->state.font->line_height;
  }
  
  if (style.width == ui_SIZE_AUTO) {
    result.x = measure_string_width(layout->renderer, str) +
      style.padding_left + style.padding_right;
  }
  
  return result;
}

bool ui_button(ui_Layout *layout, ui_Id id, String str, Style style) {
  ui_Item *item = layout_get_item(layout, Item_Type_BUTTON, style);
  item->state_id = id;
  
  ui_State *state = layout_get_state(layout, id);
  state->label = str;
  
  bool result = state->is_active;
  
  V2 size = ui_compute_text_size(layout, str, item->style);
  item->style.width = size.x;
  item->style.height = size.y;
  
  return result;
}

void ui_panel(ui_Layout *layout, Style style) {
  ui_Item *item = layout_get_item(layout, Item_Type_PANEL, style);
  assert(style.width != ui_SIZE_AUTO);
  assert(style.height != ui_SIZE_AUTO);
}

Rect2 ui_get_rect(ui_Layout *layout, ui_Item *item) {
  V2 p = v2_add(item->p, layout->p);
  p.y -= item->style.height;
  Rect2 rect = rect2_min_size(p, v2(item->style.width, item->style.height));
  return rect;
}

bool ui_is_clicked(ui_Layout *layout, ui_Id id) {
  bool result = false;
  
  os_Button left = layout->input->mouse.left;
  if (ui_ids_equal(layout->active, id)) {
    if (left.went_up) {
      if (ui_ids_equal(layout->hot, id)) {
        result = true;
      }
      layout->active = INVALID_UI_ID;
    }
  } else if (ui_ids_equal(layout->hot, id)) {
    if (left.went_down) {
      layout->active = id;
    }
  }
  return result;
}

void traverse_layout(ui_Layout *layout, ui_Item *item) {
  if (flag_is_set(item->style.flags, ui_HIDDEN)) {
    return;
  }
  
  f32 prev_z = layout->renderer->state.z;
  if (item->style.layer)
    layout->renderer->state.z = item->style.layer;
  
  Rect2 rect = ui_get_rect(layout, item);
  draw_rect(layout->renderer, rect, item->style.bg_color);
  
  switch (item->type) {
    case Item_Type_BUTTON: {
      ui_State *state = layout_get_state(layout, item->state_id);
      
      
      if (point_in_rect(layout->input->mouse.p, rect)) {
        layout->hot = item->state_id;
        draw_rect_outline(layout->renderer, rect, 2, 0xFFFFFFFF);
      }
      
      state->is_active = ui_is_clicked(layout, item->state_id);
      
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      draw_string(layout->renderer, state->label, p, 0xFFFFFFFF);
    } break;
    
    case Item_Type_MENU_TOGGLE_BUTTON: {
      ui_State *state = layout_get_state(layout, item->state_id);
      
      os_Button left = layout->input->mouse.left;
      if (point_in_rect(layout->input->mouse.p, rect)) {
        layout->hot = item->state_id;
        draw_rect_outline(layout->renderer, rect, 2, 0xFFFFFFFF);
      }
      
      if (ui_is_clicked(layout, item->state_id)) {
        state->is_active = !state->is_active;
      }
      
      V2 rect_size = rect2_get_size(rect);
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      
      // draw the arrow
      draw_string(layout->renderer, state->label, p, 0xFFFFFFFF);
      {
        V2 arrow_size = v2(6, 9);
        V2 arrow_p = v2(rect.max.x - arrow_size.x*0.5f - item->style.padding_right,
                        rect.min.y + rect_size.y*0.4f);
        
        
        f32 angle = -0.25f;
        if (item->parent->style.flags & ui_HORIZONTAL) {
          angle = 0;
        }
        
        render_save(layout->renderer);
        render_translate(layout->renderer, arrow_p);
        render_rotate(layout->renderer, angle);
        draw_arrow_outline(layout->renderer, v2_zero(), arrow_size, 2, 0xFFFFFFFF);
        
        render_restore(layout->renderer);
      }
    } break;
  }
  
  bool is_horizontal = flag_is_set(item->style.flags, ui_HORIZONTAL);
  
  if (item->children && sb_count(item->children)) {
    if (!is_horizontal) {
      f32 fixed_height = 0;
      i32 stretch_count = 0;
      for (u32 child_index = 0;
           child_index < sb_count(item->children);
           child_index++) 
      {
        ui_Item *child = item->children + child_index;
        
        if (child->style.height == ui_SIZE_STRETCH) {
          stretch_count++;
        } else {
          fixed_height += child->style.height;
        }
        
        if (child->style.width == ui_SIZE_STRETCH ||
            flag_is_set(item->style.flags, ui_ALIGN_STRETCH))
        {
          child->style.width = item->style.width;
        }
      }
      
      f32 height_per_stretch = (item->style.height - fixed_height)/stretch_count;
      
      V2 p = item->p;
      
      for (u32 child_index = 0;
           child_index < sb_count(item->children);
           child_index++) 
      {
        ui_Item *child = item->children + child_index;
        child->p = p;
        if (child->style.height == ui_SIZE_STRETCH) {
          child->style.height = height_per_stretch;
        }
        p.y -= child->style.height;
        
        traverse_layout(layout, child);
      }
    } else {
      f32 fixed_width = 0;
      i32 stretch_count = 0;
      for (u32 child_index = 0;
           child_index < sb_count(item->children);
           child_index++) 
      {
        ui_Item *child = item->children + child_index;
        
        if (child->style.width == ui_SIZE_STRETCH) {
          stretch_count++;
        } else {
          fixed_width += child->style.width;
        }
        
        if (child->style.height == ui_SIZE_STRETCH) {
          child->style.height = item->style.height;
        }
      }
      
      f32 width_per_stretch = (item->style.width - fixed_width)/stretch_count;
      
      V2 p = item->p;
      for (u32 child_index = 0;
           child_index < sb_count(item->children);
           child_index++) 
      {
        ui_Item *child = item->children + child_index;
        child->p = p;
        if (child->style.width == ui_SIZE_STRETCH) {
          child->style.width = width_per_stretch;
        }
        p.x += child->style.width;
        
        traverse_layout(layout, child);
      }
    }
  }
  
  // after all children rects have been calculated
  switch (item->type) {
    case Item_Type_DROPDOWN_MENU: {
      assert(sb_count(item->children) == 2);
      ui_Item *button = item->children + 0;
      ui_State *state = layout_get_state(layout, button->state_id);
      
      bool mouse_inside = false;
      
      ui_Item *stack[16];
      i32 stack_count = 0;
      stack[stack_count++] = item;
      
      while (stack_count) {
        ui_Item *cur = stack[--stack_count];
        
        if (!flag_is_set(cur->style.flags, ui_HIDDEN)) {
          for (u32 i = 0; i < sb_count(cur->children); i++) {
            ui_Item *child = cur->children + i;
            stack[stack_count++] = child;
            assert(stack_count <= array_count(stack));
          }
          
          Rect2 child_rect = ui_get_rect(layout, cur);
          if (point_in_rect(layout->input->mouse.p, child_rect)) {
            mouse_inside = true;
            break;
          }
        }
      }
      
      if (!mouse_inside) {
        state->is_active = false;
      }
    } break;
  }
  
  layout->renderer->state.z = prev_z;
}

void ui_menu_bar_begin(ui_Layout *layout) {
  ui_flex_begin(layout, (Style){
                .flags = ui_HORIZONTAL,
                .bg_color = 0xFF0000FF,
                .width = ui_SIZE_STRETCH,
                });
}

void ui_menu_bar_end(ui_Layout *layout) {
  ui_flex_end(layout);
}

void ui_dropdown_menu_begin(ui_Layout *layout, ui_Id id, String label, Style style) {
  ui_flex_begin_ex(layout, style, Item_Type_DROPDOWN_MENU);
  
  bool result = false;
  ui_Item *item = layout_get_item(layout, Item_Type_MENU_TOGGLE_BUTTON, 
                                  default_button_style());
  item->state_id = id;
  
  ui_State *state = layout_get_state(layout, id);
  state->label = label;
  
  result = state->is_active;
  
  V2 size = ui_compute_text_size(layout, label, item->style);
  item->style.width = size.x + 16;
  item->style.height = size.y;
  
  Style dropdown_box = (Style){
    .flags = ui_ALIGN_STRETCH|ui_IGNORE_LAYOUT,
    .bg_color = 0xFF888888,
    .layer = 1,
  };
  
  if (!result) {
    dropdown_box.flags |= ui_HIDDEN;
  }
  
  ui_flex_begin(layout, dropdown_box);
}

void ui_dropdown_menu_end(ui_Layout *layout) {
  ui_Item *dropdown = layout->current_container;
  ui_Item *menu = dropdown->parent;
  assert(sb_count(menu->children) == 2);
  ui_Item *menu_toggle = menu->children + 0;
  ui_State *state = layout_get_state(layout, menu_toggle->state_id);
  if (state->is_active) {
    int fo = 321;
  }
  
  // check if any child was clicked
  ui_Item *stack[16];
  i32 stack_count = 0;
  stack[stack_count++] = dropdown;
  
  while (stack_count) {
    ui_Item *cur = stack[--stack_count];
    
    if (!flag_is_set(cur->style.flags, ui_HIDDEN)) {
      for (u32 i = 0; i < sb_count(cur->children); i++) {
        ui_Item *child = cur->children + i;
        stack[stack_count++] = child;
        assert(stack_count <= array_count(stack));
      }
      
      if (cur->type == Item_Type_BUTTON) {
        ui_State *child_state = layout_get_state(layout, cur->state_id);
        if (child_state->is_active) {
          state->is_active = false;
          break;
        }
      }
    }
  }
  
  ui_flex_end(layout);
  ui_flex_end(layout);
}

void foo(ui_Layout *layout, Renderer *r, os_Input *input, Editor *editor) {
  push_scratch_context();
  
  f32 line_height = 40;
  ui_Layout *l = layout;
  layout->current_container = null;
  l->p = v2(-r->window_size.x*0.5f, r->window_size.y*0.5f);
  
  ui_flex_begin(l, (Style){ 
                .width = r->window_size.x,
                .height = r->window_size.y,
                .bg_color = 0xFF000000,
                }); {
    
    Style submenu_box = (Style){
      .flags = ui_IGNORE_LAYOUT|ui_ALIGN_STRETCH,
      .bg_color = 0xFF888888,
    };
    Style button_box = default_button_style();
    
    ui_menu_bar_begin(l); {
      ui_dropdown_menu_begin(l, ui_id(), const_string("file"), (Style){0}); {
        ui_button(l, ui_id(), const_string("new"), button_box);
        ui_button(l, ui_id(), const_string("open"), button_box);
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
        ui_button(l, ui_id(), const_string("copy"), button_box);
        ui_button(l, ui_id(), const_string("paste"), button_box);
        ui_button(l, ui_id(), const_string("cut"), button_box);
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
    
    ui_flex_begin(l, (Style){ 
                  .flags = ui_HORIZONTAL,
                  .width = ui_SIZE_STRETCH,
                  .height = ui_SIZE_STRETCH,
                  }); {
      ui_panel(l, (Style){ 
               .bg_color = 0xFFFF0000, 
               .width = ui_SIZE_STRETCH, 
               .height = ui_SIZE_STRETCH,
               });
      ui_panel(l, (Style){ 
               .bg_color = 0xFF00FF00, 
               .width = ui_SIZE_STRETCH,
               .height = ui_SIZE_STRETCH,
               });
    } ui_flex_end(l);
  } ui_flex_end(l);
  
  
  traverse_layout(l, l->current_container);
  
  pop_context();
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

