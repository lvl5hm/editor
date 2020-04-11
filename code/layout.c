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
  V2 result = v2(item->style.width.value, item->style.height.value);
  if (flag_is_set(item->style.flags, ui_IGNORE_LAYOUT) ||
      flag_is_set(item->style.flags, ui_HIDDEN)) 
  {
    result = v2_zero();
  }
  
  if (item->style.width.unit == Unit_PERCENT) {
    result.x = 0;
  }
  if (item->style.height.unit == Unit_PERCENT) {
    result.y = 0;
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

V2 ui_flex_calc_auto_dim(ui_Item *item, i32 main_axis) {
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
  
  return result;
}

void ui_flex_end(ui_Layout *layout) {
  ui_Item *item = layout->current_container;
  if (layout->current_container->parent) {
    layout->current_container = layout->current_container->parent;
  }
  
  V2 size;
  bool is_horizontal = flag_is_set(item->style.flags, ui_HORIZONTAL);
  i32 main_axis = is_horizontal ? AXIS_X : AXIS_Y;
  
  size = ui_flex_calc_auto_dim(item, main_axis);
  
  if (item->style.width.value == ui_SIZE_AUTO) {
    item->style.width.value = size.x;
    assert(item->style.width.unit == Unit_PIXELS);
  }
  if (item->style.height.value == ui_SIZE_AUTO) {
    item->style.height.value = size.y;
    assert(item->style.height.unit == Unit_PIXELS);
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

bool ui_button(ui_Layout *layout, ui_Id id, String str, Style style) {
  ui_Item *item = layout_get_item(layout, Item_Type_BUTTON, style);
  item->id = id;
  item->label = str;
  
  ui_State *state = layout_get_state(layout, id);
  bool result = state->clicked;
  
  V2 size = ui_compute_text_size(layout, str, item->style);
  if (style.width.value == ui_SIZE_AUTO) {
    item->style.width.value = size.x;
  }
  if (style.height.value == ui_SIZE_AUTO) {
    item->style.height.value = size.y;
  }
  
  return result;
}

void ui_buffer(ui_Layout *layout, Buffer_View *buffer_view, V2 *scroll, Style style) {
  ui_Item *item = layout_get_item(layout, Item_Type_BUFFER, style);
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
  
  ui_button(layout, ui_id(), panel->buffer_view.buffer->path, button_style);
  
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
  p.y -= item->style.height.value;
  Rect2 rect = rect2_min_size(p, v2(item->style.width.value, item->style.height.value));
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

void ui_flex_set_stretchy_children(ui_Item *item, i32 main_axis) {
  i32 other_axis = !main_axis;
  
  f32 fixed_size = 0;
  i32 stretch_count = 0;
  
  for (u32 child_index = 0;
       child_index < sb_count(item->children);
       child_index++) 
  {
    ui_Item *child = item->children + child_index;
    
    if (child->style.height.unit == Unit_PERCENT) {
      child->style.height.value *= item->style.height.value/100;
      child->style.height.unit = Unit_PIXELS;
    }
    if (child->style.width.unit == Unit_PERCENT) {
      child->style.width.value *= item->style.width.value/100;
      child->style.width.unit = Unit_PIXELS;
    }
    
    
    // NOTE(lvl5): dealing with min_width
    if (child->style.min_width.unit == Unit_PERCENT) {
      child->style.min_width.value *= item->style.width.value/100;
      child->style.min_width.unit = Unit_PIXELS;
    }
    if (child->style.width.value < child->style.min_width.value) {
      child->style.width.value = child->style.min_width.value;
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
      child->style.dims[other_axis].value = item->style.dims[other_axis].value;
    }
  }
  
  f32 size_per_stretch = (item->style.dims[main_axis].value - fixed_size)/stretch_count;
  
  V2 p = item->p;
  
  for (u32 child_index = 0;
       child_index < sb_count(item->children);
       child_index++) 
  {
    ui_Item *child = item->children + child_index;
    
    if (child->style.dims[main_axis].value == ui_SIZE_STRETCH) {
      child->style.dims[main_axis].value = size_per_stretch;
    }
    
    V2 child_dims = get_reported_dims(child);
    f32 dim = child_dims.e[main_axis];
    
    child->p = p;
    
    if (flag_is_set(item->style.flags, ui_ALIGN_CENTER)) {
      child->p.e[other_axis] += (item->style.dims[other_axis].value - 
                                 child->style.dims[other_axis].value)*0.5f;
    }
    
    
    if (main_axis == AXIS_X) {
      p.e[main_axis] += dim;
    } else {
      // NOTE(lvl5): y axis is multiplied by -1
      p.e[main_axis] -= dim;
    }
  }
}

ui_Item *ui_get_item_by_id(ui_Layout *layout, ui_Id id) {
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

void ui_draw_item(ui_Layout *layout, ui_Item *item, ui_Id *next_hot) {
  Rect2 rect = ui_get_rect(layout, item);
  
  switch (item->type) {
    case Item_Type_BUFFER: {
      draw_rect(layout->renderer, rect, item->style.bg_color);
      f32 border_width = item->style.border_width;
      if (border_width) {
        draw_rect_outline(layout->renderer, rect, border_width, item->style.border_color);
      }
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
      ui_State *state = layout_get_state(layout, item->id);
      
      state->clicked = ui_is_clicked(layout, item->id);
      
      u32 color = item->style.bg_color;
      if (ui_ids_equal(layout->active, item->id)) {
        color = item->style.active_bg_color;
      }
      draw_rect(layout->renderer, rect, color);
      
      if (ui_ids_equal(layout->hot, item->id)) {
        draw_rect_outline(layout->renderer, rect, 2, 0xFFFFFFFF);
      }
      
      if (point_in_rect(layout->input->mouse.p, rect)) {
        ui_Item *prev_hot = ui_get_item_by_id(layout, *next_hot);
        if (ui_get_layer(item) >= ui_get_layer(prev_hot)) {
          *next_hot = item->id;
        }
      }
      
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      draw_string(layout->renderer, item->label, p, item->style.text_color);
    } break;
    
    case Item_Type_MENU_TOGGLE_BUTTON: {
      ui_State *state = layout_get_state(layout, item->id);
      
      u32 color = item->style.bg_color;
      if (state->open || ui_ids_equal(layout->active, item->id)) {
        color = item->style.active_bg_color;
      }
      draw_rect(layout->renderer, rect, color);
      
      os_Button left = layout->input->mouse.left;
      
      if (ui_ids_equal(layout->hot, item->id)) {
        draw_rect_outline(layout->renderer, rect, 2, 0xFFFFFFFF);
      }
      
      if (ui_is_clicked(layout, item->id)) {
        state->open = !state->open;
      }
      
      // NOTE(lvl5): open the menu if clicked
      if (point_in_rect(layout->input->mouse.p, rect)) {
        ui_Item *prev_hot = ui_get_item_by_id(layout, *next_hot);
        if (ui_get_layer(item) >= ui_get_layer(prev_hot)) {
          *next_hot = item->id;
        }
      }
      
      // NOTE(lvl5): if menu is in a menu_bar, close the menu
      // that is not being hovered over
      ui_Item *menu_bar = item->parent->parent;
      if (menu_bar->type != Item_Type_MENU_BAR) {
        menu_bar = null;
      }
      
      if (!state->open && menu_bar && 
          ui_ids_equal(layout->hot, item->id)) 
      {
        for (u32 menu_index = 0; 
             menu_index < sb_count(menu_bar->children);
             menu_index++) 
        {
          ui_Item *menu_wrapper = menu_bar->children + menu_index;
          ui_Item *menu = menu_wrapper->children + 0;
          Rect2 menu_rect = ui_get_rect(layout, menu);
          
          ui_State *other_state = layout_get_state(layout, menu->id);
          if (other_state->open) {
            other_state->open = false;
            state->open = true;
          }
        }
      }
      
      // NOTE(lvl5): close the menu if a button is clicked
      ui_Item *dropdown = item->parent->children + 1;
      ui_Item **descendents = ui_scratch_get_all_descendents(dropdown);
      for (u32 i = 0; i < sb_count(descendents); i++) {
        ui_Item *child = descendents[i];
        if (child->type == Item_Type_BUTTON) {
          ui_State *button_state = layout_get_state(layout, child->id);
          if (button_state->clicked) {
            state->open = false;
            break;
          }
        }
      }
      
      
      V2 rect_size = rect2_get_size(rect);
      V2 p = v2(rect.min.x + item->style.padding_left, rect.max.y);
      
      // TODO(lvl5): make the arrow part of the layout?
      // draw the arrow
      draw_string(layout->renderer, item->label, p, 0xFFFFFFFF);
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
    } break;
    
    default: {
      draw_rect(layout->renderer, rect, item->style.bg_color);
    } break;
  }
}

void traverse_layout(ui_Layout *layout, ui_Item *root) {
  ui_Id next_hot = INVALID_UI_ID;
  
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
      switch (item->type) {
        case Item_Type_DROPDOWN_MENU: {
          // NOTE(lvl5): close the menu if clicked outside of it
          ui_Item *menu_wrapper = item;
          ui_Item *menu_button = menu_wrapper->children + 0;
          ui_State *state = layout_get_state(layout, menu_button->id);
          
          if (state->open && layout->input->mouse.left.went_up) {
            bool mouse_over_menu = false;
            
            ui_Item **descendents = ui_scratch_get_all_descendents(menu_wrapper);
            for (u32 i = 0; i < sb_count(descendents); i++) {
              ui_Item *child = descendents[i];
              Rect2 child_rect = ui_get_rect(layout, child);
              if (point_in_rect(layout->input->mouse.p, child_rect)) {
                mouse_over_menu = true;
                break;
              }
            }
            
            if (!mouse_over_menu) {
              state->open = false;
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

void ui_dropdown_menu_begin(ui_Layout *layout, ui_Id id, String label, Style style) {
  ui_flex_begin_ex(layout, style, Item_Type_DROPDOWN_MENU);
  Style toggle_style = default_button_style();
  toggle_style.bg_color = 0xFF333333;
  
  ui_Item *item = layout_get_item(layout, Item_Type_MENU_TOGGLE_BUTTON, 
                                  toggle_style);
  item->id = id;
  item->label = label;
  ui_State *state = layout_get_state(layout, id);
  
  
  V2 size = ui_compute_text_size(layout, label, item->style);
  item->style.width.value = size.x + 16;
  item->style.height.value = size.y;
  
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
}

void ui_dropdown_menu_end(ui_Layout *layout) {
  ui_flex_end(layout);
  ui_flex_end(layout);
}
