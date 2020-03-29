#include "lvl5_math.h"

typedef enum {
  ui_VERTICAL,
  ui_HORIZONTAL,
  ui_STRETCH,
  ui_IGNORE_LAYOUT,
} ui_Enum;

#define Size_STRETCH INFINITY
#define Size_AUTO 0

typedef struct {
  ui_Enum direction;
  ui_Enum align_content;
  ui_Enum position;
  
  f32 width;
  f32 height;
  f32 padding_left;
  f32 padding_right;
  f32 padding_top;
  f32 padding_bottom;
  
  u32 bg_color;
} Box;

typedef struct Item Item;
typedef struct Item {
  V2 p;
  
  Box box;
  Item **children;
  Item *parent;
} Item;

typedef enum {
  Layout_Mode_NONE,
  Layout_Mode_RESOLVE_AUTOS,
  Layout_Mode_RESOLVE_STRETCHES,
  Layout_Mode_DRAW,
} Layout_Mode;

typedef struct {
  V2 p;
  Layout_Mode mode;
  Item *items;
  u32 current_index;
  
  Item *current_container;
  Renderer *renderer;
} Layout;

Layout make_layout(Renderer *r) {
  Layout result = {
    .items = sb_new(Item, 32),
    .renderer = r,
    .p = v2(-r->window_size.x*0.5f, r->window_size.y*0.5f),
  };
  return result;
}

Item *layout_get_item(Layout *layout, Box box) {
  Item *result = null;
  if (sb_count(layout->items) > layout->current_index) {
    result = layout->items + layout->current_index++;
  } else {
    Item _item = {
      .box = box,
      .parent = layout->current_container,
    };
    sb_push(layout->items, _item);
    result = layout->items + sb_count(layout->items) - 1;
    layout->current_index++;
    
    if (layout->current_container) {
      sb_push(layout->current_container->children, result);
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


void ui_flex_begin(Layout *layout, Box box) {
  Item *item = layout_get_item(layout, box);
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

bool ui_button(Layout *layout, String str, Box box) {
  Item *item = layout_get_item(layout, box);
  item->box.bg_color = 0xFF555555;
  
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
      draw_rect(layout->renderer, rect2_min_size(p, v2(item->box.width, -item->box.height)),
                item->box.bg_color);
      draw_string(layout->renderer, str, v2(p.x + item->box.padding_left, p.y), 0xFFFFFFFF);
    } break;
  }
  
  return false;
}

void ui_panel(Layout *layout, Box box) {
  Item *item = layout_get_item(layout, box);
  
  switch (layout->mode) {
    case Layout_Mode_DRAW: {
      V2 p = v2_add(item->p, layout->p);
      draw_rect(layout->renderer, rect2_min_size(p, v2(item->box.width, -item->box.height)),
                item->box.bg_color);
    } break;
  }
}


typedef void Layouting_Fn(Layout *, void *);
void layouting_fn(Layout *l, void *ignored) {
  Renderer *r = l->renderer;
  f32 line_height = 40;
  
  ui_flex_begin(l, (Box){ 
                .direction = ui_VERTICAL,
                .width = r->window_size.x,
                .height = r->window_size.y,
                .bg_color = 0xFF000000,
                });
  {
    ui_flex_begin(l, (Box){ 
                  .direction = ui_HORIZONTAL,
                  .bg_color = 0xFF0000FF,
                  .width = Size_STRETCH,
                  });
    {
      Box button_box = (Box){
        .padding_left = 8,
        .padding_right = 8,
      };
      
      Box submenu_button_box = (Box){
        .padding_left = 8,
        .padding_right = 8,
        .width = Size_AUTO,
      };
      
      Box submenu_box = (Box){
        .position = ui_IGNORE_LAYOUT,
        .direction = ui_VERTICAL,
        .width = Size_AUTO,
        .bg_color = 0xFF888888,
        .align_content = ui_STRETCH,
      };
      
      ui_flex_begin(l, (Box){0}); {
        if (ui_button(l, const_string("file"), button_box) || true) {
          ui_flex_begin(l, submenu_box);
          {
            ui_button(l, const_string("open"), submenu_button_box);
            ui_button(l, const_string("save"), submenu_button_box);
            ui_button(l, const_string("settings"), submenu_button_box);
            ui_button(l, const_string("exit"), submenu_button_box);
          }
          ui_flex_end(l);
        }
      } ui_flex_end(l);
      
      
      ui_flex_begin(l, (Box){0}); {
        if (ui_button(l, const_string("edit"), button_box)) {
          ui_flex_begin(l, submenu_box);
          {
            ui_button(l, const_string("copy"), button_box);
            ui_button(l, const_string("paste"), button_box);
            ui_button(l, const_string("cut"), button_box);
            ui_button(l, const_string("undo"), button_box);
            ui_button(l, const_string("redo"), button_box);
          }
          ui_flex_end(l);
        }
      } ui_flex_end(l);
      
      ui_flex_begin(l, (Box){0}); {
        if (ui_button(l, const_string("panels"), button_box)) {
          ui_flex_begin(l, submenu_box);
          {
            ui_button(l, const_string("foo"), button_box);
            ui_button(l, const_string("bar"), button_box);
            ui_button(l, const_string("baz"), button_box);
          }
          ui_flex_end(l);
        }
      } ui_flex_end(l);
      
      ui_flex_begin(l, (Box){0}); {
        if (ui_button(l, const_string("commands"), button_box)) {
          ui_flex_begin(l, submenu_box);
          {
            ui_button(l, const_string("run command"), button_box);
          }
          ui_flex_end(l);
        }
      } ui_flex_end(l);
    }
    ui_flex_end(l);
    
    ui_flex_begin(l, (Box){ 
                  .direction = ui_HORIZONTAL,
                  .width = Size_STRETCH,
                  .height = Size_STRETCH,
                  });
    {
      ui_panel(l, (Box){ .bg_color = 0x66FF0000, .width = Size_STRETCH, .height = Size_STRETCH });
      ui_panel(l, (Box){ .bg_color = 0xFF00FF00, .width = Size_STRETCH,
               .height = Size_STRETCH });
    }
    ui_flex_end(l);
  }
  ui_flex_end(l);
}

void do_layout(Layout *layout, Layouting_Fn *fn, void *data) {
  layout->mode = Layout_Mode_RESOLVE_AUTOS;
  layout->current_index = 0;
  layout->current_container = null;
  fn(layout, data);
  layout->mode = Layout_Mode_RESOLVE_STRETCHES;
  layout->current_index = 0;
  layout->current_container = null;
  fn(layout, data);
  layout->mode = Layout_Mode_DRAW;
  layout->current_index = 0;
  layout->current_container = null;
  fn(layout, data);
}

void foo(Renderer *r) {
  Layout layout = make_layout(r);
  do_layout(&layout, layouting_fn, null);
}