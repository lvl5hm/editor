#include "layout.h"
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
      if (flag_is_set(child->style.flags, ui_WIDTH_PERCENT)) {
        child_dims.x = 0;
      }
      if (flag_is_set(child->style.flags, ui_HEIGHT_PERCENT)) {
        child_dims.y = 0;
      }
      
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
      if (flag_is_set(child->style.flags, ui_WIDTH_PERCENT)) {
        child_dims.x = 0;
      }
      if (flag_is_set(child->style.flags, ui_HEIGHT_PERCENT)) {
        child_dims.y = 0;
      }
      
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
        ui_Item *child = cur->children + i;
        stack[stack_count++] = child;
        assert(stack_count <= array_count(stack));
      }
      
      sb_push(result, cur);
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
        
        if (flag_is_set(child->style.flags, ui_HEIGHT_PERCENT)) {
          child->style.height *= item->style.height/100;
        }
        if (flag_is_set(child->style.flags, ui_WIDTH_PERCENT)) {
          child->style.width *= item->style.width/100;
        }
        
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
        
        if (flag_is_set(child->style.flags, ui_HEIGHT_PERCENT)) {
          child->style.height *= item->style.height/100;
        }
        if (flag_is_set(child->style.flags, ui_WIDTH_PERCENT)) {
          child->style.width *= item->style.width/100;
        }
        
        
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
      
      ui_Item **descendents = ui_scratch_get_all_descendents(item);
      for (u32 i = 0; i < sb_count(descendents); i++) {
        ui_Item *child = descendents[i];
        
        Rect2 child_rect = ui_get_rect(layout, child);
        if (point_in_rect(layout->input->mouse.p, child_rect)) {
          mouse_inside = true;
          break;
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
  
  // check if any child was clicked
  ui_Item **descendents = ui_scratch_get_all_descendents(dropdown);
  for (u32 i = 0; i < sb_count(descendents); i++) {
    ui_Item *child = descendents[i];
    if (child->type == Item_Type_BUTTON) {
      ui_State *child_state = layout_get_state(layout, child->state_id);
      if (child_state->is_active) {
        state->is_active = false;
        break;
      }
    }
  }
  
  ui_flex_end(layout);
  ui_flex_end(layout);
}
