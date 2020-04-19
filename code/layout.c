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

ui_Id invalid_ui_id() {
  return (ui_Id){0};
}

bool ui_ids_equal(ui_Id a, ui_Id b) {
  bool result = a.full == b.full;
  return result;
}

bool ui_id_valid(ui_Id id) {
  bool result = !ui_ids_equal(id, invalid_ui_id());
  return result;
}

u32 hash_ui_id(ui_Id id) {
  u32 result = (id.parent_ptr) ^ (id.index << 7) ^ (id.type << 13);
  return result;
}

ui_Id ui_make_child_id(ui_Item *parent, u32 child_index, Item_Type type) {
  ui_Id result = {
    .parent_ptr = (u32)(u64)parent,
    .index = (u16)child_index,
    .type = (u16)type,
  };
  return result;
}

void ui_remove_item(ui_Layout *layout, ui_Id id) {
  u32 hash = hash_ui_id(id);
  
  while (true) {
    hash = hash % array_count(layout->keys);
    ui_Id key = layout->keys[hash];
    
    if (ui_ids_equal(key, id)) {
      layout->occupancy[hash] = UI_TOMBSTONE;
      break;
    } else {
      hash++;
    }
  }
}

// can return an empty item
u32 ui_get_item_index(ui_Layout *layout, ui_Id id) {
  u32 hash = hash_ui_id(id);
  i32 first_tombstone_index = -1;
  
  while (true) {
    hash = hash % array_count(layout->keys);
    ui_Id key = layout->keys[hash];
    i8 occupancy = layout->occupancy[hash];
    
    if (occupancy == UI_TOMBSTONE) {
      if (first_tombstone_index == -1) {
        first_tombstone_index = hash;
      }
    } else if (occupancy == UI_UNOCCUPIED || ui_ids_equal(key, id)) {
      if (first_tombstone_index != -1) {
        hash = first_tombstone_index;
      }
      break;
    } else {
      hash++;
    }
  }
  
  return hash;
}

ui_Item *ui_get_item_by_id(ui_Layout *layout, ui_Id id) {
  ui_Item *result = null;
  u32 index = ui_get_item_index(layout, id);
  if (layout->occupancy[index]) {
    result = layout->values + index;
  }
  return result;
}

void ui_init_item(ui_Layout *layout, ui_Item *result, Style style) {
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
}


ui_Item *layout_push_item(ui_Layout *layout, ui_Id id, Item_Type type, Style style) {
  if (!ui_id_valid(id)) {
    ui_Item *parent = layout->current_container;
    u32 index = layout->current_container
      ? layout->current_container->current_child_index
      : 0;
    id = ui_make_child_id(parent, index, type);
  }
  
  u32 item_index = ui_get_item_index(layout, id);
  ui_Item *result = layout->values + item_index;
  
  result->current_child_index = 0;
  result->style = style;
  result->parent = layout->current_container;
  result->id = id;
  result->type = type;
  result->rendered = false;
  
  if (layout->occupancy[item_index] != UI_OCCUPIED) {
    layout->occupancy[item_index] = UI_OCCUPIED;
    layout->keys[item_index] = id;
    
    Context ctx = *get_context();
    ctx.allocator = system_allocator;
    push_context(ctx);
    result->children = sb_new(ui_Item *, 8);
    pop_context(ctx);
    
    if (layout->current_container) {
      u32 index = sb_count(layout->current_container->children);
      sb_push(layout->current_container->children, result);
    }
  }
  
  
  ui_init_item(layout, result, style);
  
  if (layout->current_container) {
    layout->current_container->current_child_index++;
  }
  
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

void ui_flex_begin_ex(ui_Layout *layout, Style style, Item_Type type) {
  ui_Item *item = layout_push_item(layout, invalid_ui_id(), type, style);
  layout->current_container = item;
}
void ui_flex_begin(ui_Layout *layout, Style style) {
  ui_flex_begin_ex(layout, style, Item_Type_FLEX);
}

V2 ui_flex_calc_auto_dim(ui_Item *item, i32 main_axis) {
  i32 other_axis = !main_axis;
  
  V2 result = v2_zero();
  for (u32 child_index = 0;
       child_index < sb_count(item->children);
       child_index++) 
  {
    ui_Item *child = item->children[child_index];
    V2 child_dims = get_reported_dims(child);
    
    result.e[main_axis] += child_dims.e[main_axis];
    
    if (child_dims.e[other_axis] > result.e[other_axis]) {
      result.e[other_axis] = child_dims.e[other_axis];
    }
  }
  
  return result;
}

void ui_flex_end(ui_Layout *layout) {
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
  
  return result;
}

void ui_label(ui_Layout *layout, String label, Style style) {
  ui_Item *item = layout_push_item(layout, invalid_ui_id(), Item_Type_LABEL, style);
  item->label = label;
  
  V2 size = ui_compute_text_size(layout, label, item->style);
  if (style.width.value == ui_SIZE_AUTO) {
    item->size.x = size.x;
  }
  if (style.height.value == ui_SIZE_AUTO) {
    item->size.y = size.y;
  }
}

bool ui_button(ui_Layout *layout, String str, Style style) {
  style.flags |= ui_FOCUSABLE;
  
  
  ui_Id id = (ui_Id){ .ptr = (u32)(u64)str.data };
  ui_Item *item = layout_push_item(layout, id, Item_Type_BUTTON, style);
  item->label = str;
  bool result = item->clicked;
  
  V2 size = ui_compute_text_size(layout, str, item->style);
  if (style.width.value == ui_SIZE_AUTO) {
    item->size.x = size.x;
  }
  if (style.height.value == ui_SIZE_AUTO) {
    item->size.y = size.y;
  }
  
  return result;
}

void ui_buffer(ui_Layout *layout, Buffer_View *buffer_view, V2 *scroll, Style style) {
  ui_Item *item = layout_push_item(layout, invalid_ui_id(), Item_Type_BUFFER, style);
  item->buffer_view = buffer_view;
  item->scroll = scroll;
}

void ui_panel(ui_Layout *layout, Panel *panel, Style style) {
  ui_flex_begin(layout, style);
  
  assert(panel->type == Panel_Type_BUFFER);
  
  Style button_style = default_button_style();
  button_style.bg_color = 0xFFAAAAAA;
  button_style.width.value = ui_SIZE_STRETCH;
  button_style.text_color = 0xFF222222;
  
  ui_label(layout, panel->buffer_view.buffer->path, button_style);
  
  Style buffer_style = (Style){
    .width = ui_SIZE_STRETCH,
    .height = ui_SIZE_STRETCH,
    .bg_color = layout->editor->settings.theme.colors[Syntax_BACKGROUND],
    .border_width = 2,
    .active_border_color = 0xFFCCCCCC,
    .border_color = 0xFF444444,
  };
  ui_buffer(layout, &panel->buffer_view, &panel->scroll, buffer_style);
  
  ui_flex_end(layout);
  
  assert(style.width.value != ui_SIZE_AUTO);
  assert(style.height.value != ui_SIZE_AUTO);
}

Rect2 ui_get_rect(ui_Layout *layout, ui_Item *item) {
  V2 p = v2_add(item->p, layout->p);
  p.y -= item->size.y;
  Rect2 rect = rect2_min_size(p, item->size);
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
      layout->active = invalid_ui_id();
    }
  } else if (ui_ids_equal(layout->hot, id)) {
    if (left.went_down) {
      layout->active = id;
    }
  }
  return result;
}

ui_Item **ui_scratch_get_all_descendents(ui_Item *item) {
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
        ui_Item *child = cur->children[i];
        stack[stack_count++] = child;
        assert(stack_count <= array_count(stack));
      }
      
      sb_push(result, cur);
    }
  }
  return result;
}

void ui_flex_set_stretchy_children(ui_Item *item, i32 main_axis) {
  i32 other_axis = !main_axis;
  
  f32 fixed_size = 0;
  i32 stretch_count = 0;
  
  for (u32 child_index = 0;
       child_index < sb_count(item->children);
       child_index++) 
  {
    ui_Item *child = item->children[child_index];
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
    ui_Item *child = item->children[child_index];
    
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

void ui_draw_item(ui_Layout *layout, ui_Item *item, ui_Id *next_hot) {
  Rect2 rect = ui_get_rect(layout, item);
  
  u32 color = item->style.bg_color;
  if (ui_id_valid(item->id) && ui_ids_equal(layout->active, item->id)) {
    color = item->style.active_bg_color;
  }
  draw_rect(layout->renderer, rect, color);
  
  
  f32 border_width = item->style.border_width;
  if (border_width) {
    draw_rect_outline(layout->renderer, rect, border_width, item->style.border_color);
  }
  
  switch (item->type) {
    case Item_Type_LABEL: {
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      draw_string(layout->renderer, item->label, p, item->style.text_color);
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
    
    case Item_Type_BUTTON: {
      item->clicked = ui_is_clicked(layout, item->id);
      
      if (ui_mouse_in_rect(layout, rect)) {
        ui_Item *prev_hot = ui_get_item_by_id(layout, *next_hot);
        if (!prev_hot || ui_get_layer(item) >= ui_get_layer(prev_hot)) {
          *next_hot = item->id;
        }
      }
      
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      draw_string(layout->renderer, item->label, p, item->style.text_color);
    } break;
    
    case Item_Type_MENU_TOGGLE_BUTTON: {
      u32 color = item->style.bg_color;
      if (item->open || ui_ids_equal(layout->active, item->id)) {
        color = item->style.active_bg_color;
      }
      draw_rect(layout->renderer, rect, color);
      
      os_Button left = layout->input->mouse.left;
      
      if (ui_is_clicked(layout, item->id)) {
        item->open = !item->open;
      }
      
      // NOTE(lvl5): open the menu if clicked
      if (ui_mouse_in_rect(layout, rect)) {
        ui_Item *prev_hot = ui_get_item_by_id(layout, *next_hot);
        if (!prev_hot || ui_get_layer(item) >= ui_get_layer(prev_hot)) {
          *next_hot = item->id;
        }
      }
      
      // NOTE(lvl5): if menu is in a menu_bar, close the menu
      // that is not being hovered over
      ui_Item *menu_bar = item->parent->parent;
      if (menu_bar->type != Item_Type_MENU_BAR) {
        menu_bar = null;
      }
      
      if (!item->open && menu_bar && 
          ui_ids_equal(layout->hot, item->id)) 
      {
        for (u32 menu_index = 0; 
             menu_index < sb_count(menu_bar->children);
             menu_index++) 
        {
          ui_Item *menu_wrapper = menu_bar->children[menu_index];
          ui_Item *menu = menu_wrapper->children[0];
          Rect2 menu_rect = ui_get_rect(layout, menu);
          
          ui_Item *other = ui_get_item_by_id(layout, menu->id);
          if (other->open) {
            other->open = false;
            item->open = true;
          }
        }
      }
      
      // NOTE(lvl5): close the menu if a button is clicked
      ui_Item *dropdown = item->parent->children[1];
      ui_Item **descendents = ui_scratch_get_all_descendents(dropdown);
      for (u32 i = 0; i < sb_count(descendents); i++) {
        ui_Item *child = descendents[i];
        if (child->type == Item_Type_BUTTON) {
          ui_Item *button = ui_get_item_by_id(layout, child->id);
          if (button->clicked) {
            item->open = false;
            break;
          }
        }
      }
      
      
      V2 rect_size = rect2_get_size(rect);
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      
      // TODO(lvl5): make the arrow part of the layout?
      // NOTE(lvl5): draw the arrow
      draw_string(layout->renderer, item->label, p, 0xFFFFFFFF);
      {
        V2 arrow_size = v2(6, 9);
        V2 arrow_p = v2(rect.max.x - arrow_size.x*0.5f - item->style.padding_right,
                        rect.min.y + rect_size.y*0.4f);
        
        
        f32 angle = -0.25f;
        if (item->parent->style.flags & ui_HORIZONTAL) {
          angle = 0;
        }
        
        if (item->open) {
          angle += 0.5f;
        }
        
        render_save(layout->renderer);
        render_translate(layout->renderer, arrow_p);
        render_rotate(layout->renderer, angle);
        draw_arrow_outline(layout->renderer, v2_zero(), arrow_size, 2, 0xFFFFFFFF);
        
        render_restore(layout->renderer);
      }
    } break;
  }
  
  
  if (ui_ids_equal(item->id, *next_hot) && ui_id_valid(item->id)) {
    draw_rect_outline(layout->renderer, rect, 2, 0xFFFFFFFF);
  }
}

u32 ui_get_self_index(ui_Item *item) {
  u32 result = 0;
  while (item->parent->children[result] != item) result++;
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
    
    ui_Item *sibling = cur->parent->children[self_index - 1];
    cur = sibling;
    
    if (!flag_is_set(cur->style.flags, ui_HIDDEN))  {
      if (cur->children && sb_count(cur->children)) {
        cur = cur->children[sb_count(cur->children) - 1];
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
    
    ui_Item *sibling = cur->parent->children[self_index + 1];
    cur = sibling;
    
    if (!flag_is_set(cur->style.flags, ui_HIDDEN))  {
      if (cur->children && sb_count(cur->children)) {
        cur = cur->children[0];
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

void traverse_layout(ui_Layout *layout, ui_Item *root) {
  gl_Funcs gl = global_os.gl;
  
  gl.ClearColor(0, 0, 0, 1);
  gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  Renderer *renderer = layout->renderer;
  
  Rect2 window_rect = rect2_min_size(v2_mul(renderer->window_size, -0.5f),
                                     renderer->window_size);
  
  ui_Id next_hot = layout->hot;
  if (!v2_equal(layout->ignored_mouse_p, layout->input->mouse.p)) {
    next_hot = invalid_ui_id();
  }
  
  ui_Item *stack[128];
  ui_Item *post_stack[128];
  i32 stack_count = 0;
  stack[stack_count++] = root;
  
  
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
      
      
      ui_draw_item(layout, item, &next_hot);
      
      if (ui_ids_equal(layout->hot, item->id) &&
          ui_id_valid(item->id)) 
      {
        if (layout->input->keys[os_Keycode_ARROW_DOWN].pressed) {
          layout->ignored_mouse_p = layout->input->mouse.p;
          next_hot = ui_get_next_focusable_item(item)->id;
        } else if (layout->input->keys[os_Keycode_ARROW_UP].pressed) {
          layout->ignored_mouse_p = layout->input->mouse.p;
          next_hot = ui_get_prev_focusable_item(item)->id;
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
          ui_Item *child = item->children[i];
          stack[stack_count++] = child;
        }
      }
    } else {
      switch (item->type) {
        case Item_Type_DROPDOWN_MENU: {
          // NOTE(lvl5): close the menu if clicked outside of it
          ui_Item *menu_wrapper = item;
          ui_Item *menu_button = menu_wrapper->children[0];
          
          if (menu_button->open && layout->input->mouse.left.went_up) {
            bool mouse_over_menu = false;
            
            ui_Item **descendents = ui_scratch_get_all_descendents(menu_wrapper);
            for (u32 i = 0; i < sb_count(descendents); i++) {
              ui_Item *child = descendents[i];
              Rect2 child_rect = ui_get_rect(layout, child);
              if (ui_mouse_in_rect(layout, child_rect)) {
                mouse_over_menu = true;
                break;
              }
            }
            
            if (!mouse_over_menu) {
              menu_button->open = false;
            }
          }
        } break;
      }
      if (item->style.layer) {
        render_restore(layout->renderer);
      }
    }
  }
  
  layout->hot = next_hot;
}

void ui_menu_bar_begin(ui_Layout *layout, Style style) {
  style.flags |= ui_HORIZONTAL;
  ui_flex_begin_ex(layout, style, Item_Type_MENU_BAR);
}

void ui_menu_bar_end(ui_Layout *layout) {
  ui_flex_end(layout);
}

void ui_dropdown_menu_begin(ui_Layout *layout, String label, Style style) 
{
  ui_flex_begin_ex(layout, style, Item_Type_DROPDOWN_MENU);
  Style toggle_style = default_button_style();
  toggle_style.bg_color = 0xFF333333;
  toggle_style.flags |= ui_FOCUSABLE;
  
  
  ui_Id id = { .ptr = (u32)(u64)label.data };
  ui_Item *item = layout_push_item(layout, id, Item_Type_MENU_TOGGLE_BUTTON, 
                                   toggle_style);
  item->label = label;
  
  
  V2 size = ui_compute_text_size(layout, label, item->style);
  item->size.x = size.x + 16;
  item->size.y = size.y;
  
  Style dropdown_box = (Style){
    .flags = ui_ALIGN_STRETCH|ui_IGNORE_LAYOUT,
    .bg_color = 0xFF888888,
    .layer = 1,
    .min_width = percent(100),
  };
  
  if (!item->open) {
    dropdown_box.flags |= ui_HIDDEN;
  }
  
  ui_flex_begin(layout, dropdown_box);
}

void ui_dropdown_menu_end(ui_Layout *layout) {
  ui_flex_end(layout);
  ui_flex_end(layout);
}
