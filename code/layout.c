#include "layout.h"
#include "renderer.c"

ui_Layout make_layout(Renderer *r, os_Input *input, Editor *editor) {
  ui_Layout result = {
    .renderer = r,
    .input = input,
    .editor = editor,
  };
  return result;
}


bool ui_ids_equal(ui_Id a, ui_Id b) {
  bool result = a.ptr == b.ptr;
  return result;
}

bool ui_id_valid(ui_Id id) {
  bool result = !ui_ids_equal(id, INVALID_UI_ID);
  return result;
}

u32 hash_ui_id(ui_Id id) {
  u32 result = (id.bytes[0]) ^ (id.bytes[1] * 3) ^ (id.bytes[2] * 5) ^
    (id.bytes[3] * 7) ^ (id.bytes[4] * 31) ^ (id.bytes[5] * 13) ^ (id.bytes[6] * 17)
    ^ (id.bytes[7] * 23);
  return result;
}

bool ui_is_hot(ui_Layout *layout, ui_Id id) {
  bool result = ui_id_valid(id) && ui_ids_equal(layout->hot, id);
  return result;
}

bool ui_is_active(ui_Layout *layout, ui_Id id) {
  bool result = ui_id_valid(id) && ui_ids_equal(layout->active, id);
  return result;
}

bool ui_is_interactive(ui_Layout *layout, ui_Id id) {
  bool result = ui_id_valid(id) && ui_ids_equal(layout->interactive, id);
  return result;
}


void ui_set_hot(ui_Layout *layout, ui_Id id) {
  layout->next_hot = id;
}

void ui_set_active(ui_Layout *layout, ui_Id id) {
  layout->next_active = id;
}

void ui_set_interactive(ui_Layout *layout, ui_Id id) {
  layout->next_interactive = id;
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

ui_State *layout_get_state_ex(ui_Layout *layout, ui_Id id, bool *exists) {
  begin_profiler_function();
  ui_State *result = null;
  
  *exists = false;
  
  if (ui_id_valid(id)) {
    u32 index = get_ui_state_index(layout, id);
    result = layout->values + index;
    *exists = layout->occupancy[index];
    
    if (!layout->occupancy[index]) {
      layout->keys[index] = id;
      layout->occupancy[index] = true;
    }
  }
  
  end_profiler_function();
  return result;
}

ui_State *layout_get_state(ui_Layout *layout, ui_Id id) {
  bool ignored;
  ui_State *result = layout_get_state_ex(layout, id, &ignored);
  return result;
}


ui_Item *layout_get_item(ui_Layout *layout, Item_Type type, Style style) {
  begin_profiler_function();
  
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
  if (style.width.unit == Unit_PIXELS) {
    result->size.x = style.width.value;
  }
  if (style.height.unit == Unit_PIXELS) {
    result->size.y = style.height.value;
  }
  if (style.min_width.unit == Unit_PIXELS) {
    result->min_size.x = style.min_width.value;
  }
  if (style.min_height.unit == Unit_PIXELS) {
    result->min_size.y = style.min_height.value;
  }
  
  end_profiler_function();
  return result;
}

V2 get_reported_dims(ui_Item *item) {
  V2 result = item->size;
  if (flag_is_set(item->style.flags, ui_IGNORE_LAYOUT) ||
      flag_is_set(item->style.flags, ui_HIDDEN)) 
  {
    result = v2_zero();
  }
  
  return result;
}

ui_Item *ui_flex_begin_ex(ui_Layout *layout, Style style, Item_Type type) {
  ui_Item *item = layout_get_item(layout, type, style);
  layout->current_container = item;
  return item;
}
void ui_flex_begin(ui_Layout *layout, Style style) {
  ui_flex_begin_ex(layout, style, Item_Type_FLEX);
}

V2 ui_flex_calc_auto_dim(ui_Item *item, i32 main_axis) {
  begin_profiler_function();
  
  i32 other_axis = !main_axis;
  
  V2 result = v2_zero();
  for (u32 child_index = 0;
       child_index < sb_count(item->children);
       child_index++) 
  {
    ui_Item *child = item->children + child_index;
    V2 child_dims = get_reported_dims(child);
    
    result.e[main_axis] += child_dims.e[main_axis];
    
    if (child_dims.e[other_axis] > result.e[other_axis]) {
      result.e[other_axis] = child_dims.e[other_axis];
    }
  }
  
  end_profiler_function();
  return result;
}

void ui_flex_end(ui_Layout *layout) {
  begin_profiler_function();
  
  ui_Item *item = layout->current_container;
  if (layout->current_container->parent) {
    layout->current_container = layout->current_container->parent;
  }
  
  bool is_horizontal = flag_is_set(item->style.flags, ui_HORIZONTAL);
  i32 main_axis = is_horizontal ? AXIS_X : AXIS_Y;
  
  V2 size = ui_flex_calc_auto_dim(item, main_axis);
  
  if (item->style.width.value == ui_SIZE_AUTO) {
    item->size.x = size.x;
  }
  if (item->style.height.value == ui_SIZE_AUTO) {
    item->size.y = size.y;
  }
  
  end_profiler_function();
}

Style default_button_style() {
  Style result = {
    .text_color = 0xFFFFFFFF,
    .bg_color = 0xFF888888,
    .active_bg_color = 0xFF666666,
    .padding_left = 8,
    .padding_right = 8,
  };
  return result;
}


V2 ui_compute_text_size(ui_Layout *layout, String str, Style style) {
  begin_profiler_function();
  
  V2 result = v2(style.width.value, style.height.value);
  
  if (style.height.value == ui_SIZE_AUTO) {
    assert(style.height.unit == Unit_PIXELS);
    result.y = layout->renderer->state.font->line_height;
  }
  
  if (style.width.value == ui_SIZE_AUTO) {
    assert(style.width.unit == Unit_PIXELS);
    result.x = measure_string_width(layout->renderer, str) +
      style.padding_right + style.padding_left;
  }
  
  end_profiler_function();
  return result;
}


bool ui_is_clicked(ui_Layout *layout, ui_Id id) {
  begin_profiler_function();
  
  bool result = false;
  
  os_Button left = layout->input->mouse.left;
  if (ui_is_active(layout, id)) {
    if (left.went_up) {
      if (ui_is_hot(layout, id)) {
        result = true;
      }
      ui_set_active(layout, INVALID_UI_ID);
    }
  } else if (ui_is_hot(layout, id)) {
    if (left.went_down) {
      ui_set_active(layout, id);
    }
  }
  
  end_profiler_function();
  return result;
}



Rect2 ui_get_rect(ui_Layout *layout, ui_Item *item) {
  V2 p = v2_add(item->p, layout->p);
  p.y -= item->size.y;
  Rect2 rect = rect2_min_size(p, item->size);
  return rect;
}


void ui_widget_label(ui_Layout *layout, ui_Layout_Mode mode, ui_Item *item) {
  begin_profiler_function();
  
  switch (mode) {
    case ui_Layout_Mode_CALC_AUTOS: {
      V2 size = ui_compute_text_size(layout, item->label, item->style);
      if (item->style.width.value == ui_SIZE_AUTO) {
        item->size.x = size.x;
      }
      if (item->style.height.value == ui_SIZE_AUTO) {
        item->size.y = size.y;
      }
    } break;
    
    case ui_Layout_Mode_DRAW: {
      Rect2 rect = ui_get_rect(layout, item);
      
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      draw_string(layout->renderer, item->label, p, item->style.text_color);
    } break;
  }
  
  end_profiler_function();
}

bool ui_widget_button(ui_Layout *layout, ui_Layout_Mode mode, ui_Item *item) {
  begin_profiler_function();
  bool result = false;
  
  switch (mode) {
    case ui_Layout_Mode_INTERACT: {
      item->style.flags |= ui_FOCUSABLE;
      ui_Id id = (ui_Id){item->label.data};
      item->id = id;
      
      result = ui_is_clicked(layout, id);
      break;
    } break;
    
    case ui_Layout_Mode_CALC_AUTOS: {
      V2 size = ui_compute_text_size(layout, item->label, item->style);
      if (item->style.width.value == ui_SIZE_AUTO) {
        item->size.x = size.x;
      }
      if (item->style.height.value == ui_SIZE_AUTO) {
        item->size.y = size.y;
      }
    } break;
    
    case ui_Layout_Mode_DRAW: {
      Rect2 rect = ui_get_rect(layout, item);
      
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      draw_string(layout->renderer, item->label, p, item->style.text_color);
    } break;
  }
  
  end_profiler_function();
  return result;
}


void ui_label(ui_Layout *layout, String label, Style style) {
  ui_Item *item = layout_get_item(layout, Item_Type_LABEL, style);
  item->label = label;
  ui_widget_label(layout, ui_Layout_Mode_CALC_AUTOS, item);
}


ui_Item *ui_buffer(ui_Layout *layout, Buffer_View *buffer_view, V2 *scroll, Style style) {
  ui_Item *item = layout_get_item(layout, Item_Type_BUFFER, style);
  item->buffer_view = buffer_view;
  item->scroll = scroll;
  return item;
}

bool ui_button(ui_Layout *layout, String label, Style style) {
  ui_Item *item = layout_get_item(layout, Item_Type_BUTTON, style);
  item->label = label;
  
  bool result = ui_widget_button(layout, ui_Layout_Mode_INTERACT, item);
  ui_widget_button(layout, ui_Layout_Mode_CALC_AUTOS, item);
  return result;
}

void ui_handle_buffer_input(ui_Layout *layout, Buffer *buffer) {
  begin_profiler_function();
  
  os_Input *input = layout->input;
  if (!input->ctrl && !input->alt) {
    if (input->char_count > 0) {
      String str = make_string(input->chars, input->char_count);
      buffer_input_string(buffer, str);
    }
  }
  
  end_profiler_function();
}

bool ui_panel(ui_Layout *layout, Panel *panel, Style style) {
  begin_profiler_function();
  
  ui_Id id = (ui_Id){panel};
  
  ui_Item *item = ui_flex_begin_ex(layout, style, Item_Type_PANEL);
  item->id = id;
  
  assert(panel->type == Panel_Type_BUFFER);
  
  Style button_style = default_button_style();
  button_style.bg_color = 0xFFAAAAAA;
  button_style.width.value = ui_SIZE_STRETCH;
  button_style.text_color = 0xFF222222;
  
  ui_label(layout, panel->buffer_view.buffer->path, button_style);
  
  Style buffer_style = style;
  buffer_style.width = px(ui_SIZE_STRETCH);
  buffer_style.height = px(ui_SIZE_STRETCH);
  buffer_style.bg_color = layout->editor->settings.theme.colors[Syntax_BACKGROUND];
  buffer_style.border_width = 2;
  
  ui_buffer(layout, &panel->buffer_view, &panel->scroll, buffer_style);
  
  ui_flex_end(layout);
  
  assert(style.width.value != ui_SIZE_AUTO);
  assert(style.height.value != ui_SIZE_AUTO);
  
  if (ui_ids_equal(layout->interactive, id)) {
    ui_handle_buffer_input(layout, panel->buffer_view.buffer);
  }
  
  bool result = ui_is_clicked(layout, id);
  if (result) {
    ui_set_interactive(layout, id);
  }
  
  end_profiler_function();
  return result;
}


ui_Item **ui_scratch_get_all_descendents(ui_Item *item) {
  begin_profiler_function();
  
  push_scratch_context();
  ui_Item **result = sb_new(ui_Item *, 64);
  pop_context();
  
  ui_Item *stack[128];
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
      
      sb_push(result, cur);
    }
  }
  
  end_profiler_function();
  return result;
}


void ui_widget_dropdown_menu(ui_Layout *layout, ui_Layout_Mode mode, ui_Item *item) {
  begin_profiler_function();
  
  switch (mode) {
    case ui_Layout_Mode_CALC_AUTOS: {
      Rect2 rect = ui_get_rect(layout, item->children + 0);
      
      ui_State *state = layout_get_state(layout, item->id);
      
      // NOTE(lvl5): close the menu if a button is clicked
      ui_Item *dropdown = item->children + 1;
      ui_Item **descendents = ui_scratch_get_all_descendents(dropdown);
      for (u32 i = 0; i < sb_count(descendents); i++) {
        ui_Item *child = descendents[i];
        if (child->type == Item_Type_BUTTON) {
          if (!flag_is_set(child->style.flags, ui_TOGGLE) && ui_is_clicked(layout, child->id)) 
          {
            state->open = false;
            break;
          }
        }
      }
      
      // NOTE(lvl5): close the menu if clicked outside of it
      bool clicked_outside = !ui_is_hot(layout, item->id);
      if (layout->input->mouse.left.went_up) {
        for (u32 i = 0; i < sb_count(descendents); i++) {
          ui_Item *child = descendents[i];
          if (ui_is_hot(layout, child->id)) {
            clicked_outside = false;
            break;
          }
        }
        if (clicked_outside) {
          state->open = false;
        }
      }
    } break;
    
    case ui_Layout_Mode_DRAW: {
#if 0      
      V2 rect_size = rect2_get_size(rect);
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      // NOTE(lvl5): draw the arrow
      {
        V2 arrow_size = v2(6, 9);
        V2 arrow_p = v2(rect.max.x - arrow_size.x*0.5f - item->style.padding_right,
                        rect.min.y + rect_size.y*0.4f);
        
        
        f32 angle = -0.25f;
        if (item->parent->style.flags & ui_HORIZONTAL) {
          angle = 0;
        }
        
        if (state->open) {
          angle += 0.5f;
        }
        
        render_save(layout->renderer);
        render_translate(layout->renderer, arrow_p);
        render_rotate(layout->renderer, angle);
        draw_arrow_outline(layout->renderer, v2_zero(), arrow_size, 2, 0xFFFFFFFF);
        
        render_restore(layout->renderer);
      }
#endif
    } break;
  }
  
  end_profiler_function();
}

void ui_dropdown_menu_begin(ui_Layout *layout, String label, Style style) {
  begin_profiler_function();
  
  ui_Item *menu = ui_flex_begin_ex(layout, style, Item_Type_DROPDOWN_MENU);
  menu->id = (ui_Id){label.data};
  ui_State *state = layout_get_state(layout, menu->id);
  
  
  Style toggle_style = default_button_style();
  toggle_style.bg_color = 0xFF333333;
  toggle_style.flags |= ui_FOCUSABLE | ui_TOGGLE;
  
  if (ui_button(layout, label, toggle_style)) {
    state->open = !state->open;
  }
  
  Style dropdown_box = (Style){
    .flags = ui_ALIGN_STRETCH|ui_IGNORE_LAYOUT,
    .bg_color = 0xFF888888,
    .layer = 1,
    .min_width = percent(100),
  };
  if (!state->open) {
    dropdown_box.flags |= ui_HIDDEN;
  }
  ui_flex_begin(layout, dropdown_box);
  
  end_profiler_function();
}

void ui_dropdown_menu_end(ui_Layout *layout) {
  ui_flex_end(layout);
  
  ui_widget_dropdown_menu(layout, ui_Layout_Mode_CALC_AUTOS, layout->current_container);
  ui_flex_end(layout);
}


void ui_input_text(ui_Layout *layout, String str, Style style) {
  begin_profiler_function();
  
  ui_Id id = (ui_Id){str.data};
  assert(ui_id_valid(id));
  
  
  bool exists;
  ui_State *state = layout_get_state_ex(layout, id, &exists);
  if (!exists) {
    state->panel.buffer = buffer_make_empty();
    state->panel.buffer_view = (Buffer_View){
      .buffer = &state->panel.buffer,
      .is_single_line = true,
    };
    buffer_insert_string(&state->panel.buffer, const_string("test"));
    //layout->interactive = id;
  }
  state->panel.scroll = v2_zero();
  
  style.flags |= ui_FOCUSABLE;
  ui_Item *item = ui_buffer(layout, &state->panel.buffer_view,
                            &state->panel.scroll, style);
  item->id = id;
  item->buffer_view = &state->panel.buffer_view;
  
  
  if (ui_is_clicked(layout, id)) {
    layout->interactive = id;
  }
  
  if (ui_ids_equal(layout->interactive, id)) {
    ui_handle_buffer_input(layout, item->buffer_view->buffer);
  }
  
  end_profiler_function();
}

void ui_flex_set_stretchy_children(ui_Item *item, i32 main_axis) {
  begin_profiler_function();
  
  i32 other_axis = !main_axis;
  
  f32 fixed_size = 0;
  i32 stretch_count = 0;
  
  for (u32 child_index = 0;
       child_index < sb_count(item->children);
       child_index++) 
  {
    ui_Item *child = item->children + child_index;
    if (flag_is_set(child->style.flags, ui_HIDDEN)) {
      continue;
    }
    
    
    if (child->style.height.unit == Unit_PERCENT) {
      child->size.y = child->style.height.value*item->size.y/100;
    }
    if (child->style.width.unit == Unit_PERCENT) {
      child->size.x = child->style.width.value*item->size.x/100;
    }
    
    
    // NOTE(lvl5): dealing with min_width
    if (child->style.min_width.unit == Unit_PERCENT) {
      child->min_size.x = child->style.min_width.value*item->size.x/100;
    }
    if (child->size.x < child->min_size.x) {
      child->size.x = child->min_size.x;
    }
    
    V2 child_dims = get_reported_dims(child);
    
    if (child->style.dims[main_axis].value == ui_SIZE_STRETCH) {
      stretch_count++;
    } else {
      fixed_size += child_dims.e[main_axis];
    }
    
    if (child->style.dims[other_axis].value == ui_SIZE_STRETCH ||
        flag_is_set(item->style.flags, ui_ALIGN_STRETCH))
    {
      child->size.e[other_axis] = item->size.e[other_axis];
    }
  }
  
  f32 size_per_stretch = (item->size.e[main_axis] - fixed_size)/stretch_count;
  
  V2 p = item->p;
  
  for (u32 child_index = 0;
       child_index < sb_count(item->children);
       child_index++) 
  {
    ui_Item *child = item->children + child_index;
    
    if (child->style.dims[main_axis].value == ui_SIZE_STRETCH) {
      child->size.e[main_axis] = size_per_stretch;
    }
    
    V2 child_dims = get_reported_dims(child);
    f32 dim = child_dims.e[main_axis];
    
    child->p = p;
    
    if (flag_is_set(item->style.flags, ui_ALIGN_CENTER)) {
      child->p.e[other_axis] += (item->size.e[other_axis] - 
                                 child->size.e[other_axis])*0.5f;
    }
    
    if (main_axis == AXIS_X) {
      p.e[main_axis] += dim;
    } else {
      // NOTE(lvl5): y axis is multiplied by -1
      p.e[main_axis] -= dim;
    }
  }
  
  end_profiler_function();
}

ui_Item *ui_get_item_by_id(ui_Layout *layout, ui_Id id) {
  begin_profiler_function();
  
  ui_Item *root = layout->current_container;
  while (root->parent) root = root->parent;
  
  ui_Item **descendents = ui_scratch_get_all_descendents(root);
  
  ui_Item *result = null;
  for (u32 i = 0; i < sb_count(descendents); i++) {
    if (ui_ids_equal(descendents[i]->id, id)) {
      result = descendents[i];
      break;
    }
  }
  
  end_profiler_function();
  return result;
}

f32 ui_get_layer(ui_Item *item) {
  f32 layer = item->style.layer;
  while (item->parent) {
    item = item->parent;
    if (item->style.layer) {
      layer = item->style.layer;
    }
  }
  return layer;
}

bool ui_mouse_in_rect(ui_Layout *layout, Rect2 rect) {
  bool result = false;
  if (!v2_equal(layout->ignored_mouse_p, layout->input->mouse.p)) {
    result = point_in_rect(layout->input->mouse.p, rect);
  }
  return result;
}

void ui_draw_item(ui_Layout *layout, ui_Item *item) {
  begin_profiler_function();
  
  Rect2 rect = ui_get_rect(layout, item);
  
  u32 color = item->style.bg_color;
  if (ui_is_active(layout, item->id)) {
    color = item->style.active_bg_color;
  }
  draw_rect(layout->renderer, rect, color);
  
  
  f32 border_width = item->style.border_width;
  if (border_width) {
    draw_rect_outline(layout->renderer, rect, border_width, item->style.border_color);
  }
  
  
  if (ui_id_valid(item->id) && ui_mouse_in_rect(layout, rect)) {
    ui_Item *prev_hot = ui_get_item_by_id(layout, layout->next_hot);
    if (ui_get_layer(item) >= ui_get_layer(prev_hot)) {
      ui_set_hot(layout, item->id);
    }
  }
  
  switch (item->type) {
    case Item_Type_LABEL: {
      ui_widget_label(layout, ui_Layout_Mode_DRAW, item);
    } break;
    
    case Item_Type_BUTTON: {
      ui_widget_button(layout, ui_Layout_Mode_DRAW, item);
    } break;
    
    case Item_Type_BUFFER: {
      f32 border_width = item->style.border_width;
      rect.min.x += border_width;
      rect.min.y += border_width;
      rect.max.x -= border_width;
      rect.max.y -= border_width;
      
      draw_buffer_view(layout->renderer,
                       rect,
                       item->buffer_view,
                       &layout->editor->settings.theme,
                       item->scroll);
    } break;
    
    case Item_Type_DROPDOWN_MENU: {
      ui_widget_dropdown_menu(layout, ui_Layout_Mode_DRAW, item);
    } break;
  }
  
  
  if (ui_is_hot(layout, item->id)) {
    draw_rect_outline(layout->renderer, rect, 2, 0xFFFFFFFF);
  }
  
  end_profiler_function();
}

u32 ui_get_self_index(ui_Item *item) {
  u32 result = 0;
  while (item->parent->children + result != item) result++;
  return result;
}


ui_Item *ui_get_prev_focusable_item(ui_Item *item) {
  ui_Item *result = item;
  ui_Item *cur = item;
  
  while (true) {
    if (!cur->parent) goto end;
    i32 self_index = ui_get_self_index(cur);
    
    while (self_index == 0) {
      if (flag_is_set(cur->parent->style.flags, ui_FOCUS_TRAP)) {
        self_index = sb_count(cur->parent->children);
      } else {
        cur = cur->parent;
        if (!cur->parent) goto end;
        self_index = ui_get_self_index(cur);
      }
    }
    
    ui_Item *sibling = cur->parent->children + self_index - 1;
    cur = sibling;
    
    if (!flag_is_set(cur->style.flags, ui_HIDDEN))  {
      if (cur->children && sb_count(cur->children)) {
        cur = cur->children + sb_count(cur->children) - 1;
      }
      
      if (flag_is_set(cur->style.flags, ui_FOCUSABLE)) {
        result = cur;
        goto end;
      }
    }
  }
  end:
  return result;
}

ui_Item *ui_get_next_focusable_item(ui_Item *item) {
  ui_Item *result = item;
  ui_Item *cur = item;
  
  while (true) {
    if (!cur->parent) goto end;
    i32 self_index = ui_get_self_index(cur);
    
    while (self_index == (i32)sb_count(cur->parent->children) - 1) {
      if (flag_is_set(cur->parent->style.flags, ui_FOCUS_TRAP)) {
        self_index = -1;
      } else {
        cur = cur->parent;
        if (!cur->parent) goto end;
        self_index = ui_get_self_index(cur);
      }
    }
    
    ui_Item *sibling = cur->parent->children + self_index + 1;
    cur = sibling;
    
    if (!flag_is_set(cur->style.flags, ui_HIDDEN))  {
      if (cur->children && sb_count(cur->children)) {
        cur = cur->children + 0;
      }
      
      if (flag_is_set(cur->style.flags, ui_FOCUSABLE)) {
        result = cur;
        goto end;
      }
    }
  }
  end:
  return result;
}

void ui_begin(ui_Layout *layout) {
  V2 ws = layout->renderer->window_size;
  layout->current_container = null;
  layout->p = v2(-ws.x*0.5f, ws.y*0.5f);
  
  if (v2_equal(layout->ignored_mouse_p, layout->input->mouse.p)) {
    layout->next_hot = layout->hot;
  } else {
    layout->next_hot = INVALID_UI_ID;
  }
}


void ui_end(ui_Layout *layout) {
  begin_profiler_function();
  
  gl_Funcs gl = global_os.gl;
  
  gl.ClearColor(0, 0, 0, 1);
  gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  Renderer *renderer = layout->renderer;
  
  Rect2 window_rect = rect2_min_size(v2_mul(renderer->window_size, -0.5f),
                                     renderer->window_size);
  
  assert(layout->current_container->parent == null);
  
  ui_Item *stack[128];
  ui_Item *post_stack[128];
  i32 stack_count = 0;
  stack[stack_count++] = layout->current_container;
  
  
  while (stack_count) {
    ui_Item *item = stack[--stack_count];
    
    if (!item->rendered) {
      if (flag_is_set(item->style.flags, ui_HIDDEN)) {
        continue;
      }
      
      if (item->style.layer) {
        render_save(layout->renderer);
        layout->renderer->state.z = item->style.layer;
      }
      
      
      ui_draw_item(layout, item);
      
      if (ui_is_hot(layout, item->id) &&
          ui_id_valid(item->id)) 
      {
        if (layout->input->keys[os_Keycode_ARROW_DOWN].pressed) {
          layout->ignored_mouse_p = layout->input->mouse.p;
          ui_set_hot(layout, ui_get_next_focusable_item(item)->id);
        } else if (layout->input->keys[os_Keycode_ARROW_UP].pressed) {
          layout->ignored_mouse_p = layout->input->mouse.p;
          ui_set_hot(layout, ui_get_prev_focusable_item(item)->id);
        }
      }
      
      
      if (item->children && sb_count(item->children) > 0) {
        // return to the item after visiting all descendents
        item->rendered = true;
        stack[stack_count++] = item;
        
        bool is_horizontal = flag_is_set(item->style.flags, ui_HORIZONTAL);
        i32 main_axis = is_horizontal ? AXIS_X : AXIS_Y;
        
        ui_flex_set_stretchy_children(item, main_axis);
        
        u32 i = sb_count(item->children);
        while (i > 0) {
          i--;
          ui_Item *child = item->children + i;
          stack[stack_count++] = child;
        }
      }
    } else {
      if (item->style.layer) {
        render_restore(layout->renderer);
      }
    }
  }
  
  layout->hot = layout->next_hot;
  layout->active = layout->next_active;
  layout->interactive = layout->next_interactive;
  
  end_profiler_function();
}

void ui_menu_bar_begin(ui_Layout *layout, Style style) {
  style.flags |= ui_HORIZONTAL;
  ui_flex_begin_ex(layout, style, Item_Type_MENU_BAR);
}

void ui_menu_bar_end(ui_Layout *layout) {
  begin_profiler_function();
  
  // NOTE(lvl5): close the menu
  // that is not being hovered over
  ui_Item *menu_bar = layout->current_container;
  
  ui_State *open_menu_state = null;
  ui_State *hot_menu_state = null;
  for (u32 menu_index = 0; 
       menu_index < sb_count(menu_bar->children);
       menu_index++)
  {
    ui_Item *menu = menu_bar->children + menu_index;
    
    ui_State *state = layout_get_state(layout, menu->id);
    if (ui_is_hot(layout, menu->id)) {
      hot_menu_state = state;
    }
    if (state->open) {
      open_menu_state = state;
    }
  }
  
  if (hot_menu_state && open_menu_state &&
      hot_menu_state != open_menu_state) 
  {
    open_menu_state->open = false;
    hot_menu_state->open = true;
  }
  
  ui_flex_end(layout);
  
  end_profiler_function();
}
