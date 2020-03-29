#include "lvl5_math.h"

typedef enum {
  Flex_VERTICAL,
  Flex_HORIZONTAL,
} Flex_Enum;

#define Flex_STRETCH INFINITY
#define Flex_AUTO 0

typedef struct {
  Flex_Enum direction;
  f32 width;
  f32 height;
  f32 padding_left;
  f32 padding_right;
  u32 bg_color;
} Box;

typedef struct Item Item;
typedef struct Item {
  V2 p;
  
  Box box;
  
  Item *children;
  Item *parent;
} Item;

typedef struct {
  Item *current_container;
  Item *root;
} Layout;

Layout make_layout() {
  Layout result = {0};
  return result;
}

void ui_flex_begin(Layout *layout, Box box) {
  Item _item = {
    .box = box,
    .parent = layout->current_container,
    .children = sb_new(Item, 32),
  };
  
  Item *item = null;
  if (layout->current_container) {
    item = layout->current_container->children + sb_count(layout->current_container->children);
    sb_push(layout->current_container->children, _item);
  } else {
    item = alloc_struct(Item);
    *item = _item;
  }
  
  layout->current_container = item;
  
  if (!layout->root) {
    layout->root = item;
  }
}

void ui_flex_end(Layout *layout) {
  layout->current_container = layout->current_container->parent;
}

bool ui_button(Layout *layout, String str, Box box) {
  return false;
}

void ui_panel(Layout *layout, Box box) {
  Item _item = {
    .box = box,
    .parent = layout->current_container,
    .children = sb_new(Item, 32),
  };
  
  Item *item = null;
  if (layout->current_container) {
    item = layout->current_container->children + sb_count(layout->current_container->children);
    sb_push(layout->current_container->children, _item);
  } else {
    item = alloc_struct(Item);
    *item = _item;
  }
  
  layout->current_container = item;
  
  if (!layout->root) {
    layout->root = item;
  }
}


void resolve_autos(Item *item) {
  for (u32 child_index = 0;
       child_index < sb_count(item->children);
       child_index++) 
  {
    Item *child = item->children + child_index;
    resolve_autos(child);
    assert(child->box.height != Flex_AUTO);
  }
  
  if (item->box.direction == Flex_VERTICAL) {
    f32 total_height = 0;
    f32 total_width = 0;
    
    for (u32 child_index = 0;
         child_index < sb_count(item->children);
         child_index++) 
    {
      Item *child = item->children + child_index;
      total_height += child->box.height;
      
      if (child->box.width > total_width) {
        total_width = child->box.width;
      }
    }
    
    if (item->box.height == Flex_AUTO) {
      item->box.height = total_height;
    }
    if (item->box.width == Flex_AUTO) {
      item->box.width = total_width;
    }
  }
}

void resolve_flexes(Item *item, V2 *p, Renderer *r) {
  if (item->box.direction == Flex_VERTICAL) {
    if (item->box.bg_color == 0xFFFFFF00) {
      int fsd = 32;
    }
    
    draw_rect(r, rect2_min_size(*p, v2(item->box.width, item->box.height)),
              item->box.bg_color);
    
    f32 fixed_height = 0;
    i32 stretch_count = 0;
    for (u32 child_index = 0;
         child_index < sb_count(item->children);
         child_index++) 
    {
      Item *child = item->children + child_index;
      if (child->box.height == Flex_STRETCH) {
        stretch_count++;
      } else {
        fixed_height += child->box.height;
      }
    }
    
    f32 height_per_stretch = (item->box.height - fixed_height)/stretch_count;
    
    for (u32 child_index = 0;
         child_index < sb_count(item->children);
         child_index++) 
    {
      Item *child = item->children + child_index;
      if (child->box.height == Flex_STRETCH) {
        child->box.height = height_per_stretch;
      }
      resolve_flexes(child, p, r);
      p->y += child->box.height;
    }
  }
}

void foo(Renderer *r) {
  Layout _l = make_layout();
  Layout *l = &_l;
  
  f32 line_height = 40;
  ui_flex_begin(l, (Box){ 
                .direction = Flex_VERTICAL,
                .height = 768,
                .bg_color = 0xFFFFFFFF,
                });
  {
    ui_flex_begin(l, (Box){.height = Flex_STRETCH, .width = 100, .bg_color = 0xFFFF0000,});
    ui_flex_end(l);
    
    ui_flex_begin(l, (Box){.height = Flex_AUTO, .bg_color = 0xFFFFFF00});
    {
      ui_flex_begin(l, (Box){.height = 50, .width = 200, .bg_color = 0xFF00FF00,});
      ui_flex_end(l);
      
      ui_flex_begin(l, (Box){.height = 200, .width = 80, .bg_color = 0xFF0000FF,});
      ui_flex_end(l);
    }
    ui_flex_end(l);
  }
  ui_flex_end(l);
  
#if 0  
  ui_flex_begin(l, (Box){ 
                .direction = Flex_VERTICAL,
                .width = 1366,
                .height = 768,
                });
  {
    ui_flex_begin(l, (Box){ 
                  .direction = Flex_HORIZONTAL,
                  .height = line_height,
                  .bg_color = 0xFF0000FF,
                  });
    {
      Box button_box = (Box){
        .padding_left = 8,
        .padding_right = 8,
      };
      
      Box submenu_box = (Box){ 
        .direction = Flex_VERTICAL,
        .width = 300,
      };
      
      if (ui_button(l, const_string("file"), button_box)) {
        ui_flex_begin(l, submenu_box);
        {
          ui_button(l, const_string("open"), button_box);
          ui_button(l, const_string("save"), button_box);
          ui_button(l, const_string("settings"), button_box);
          ui_button(l, const_string("exit"), button_box);
        }
        ui_flex_end(l);
      }
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
      if (ui_button(l, const_string("panels"), button_box)) {
        ui_flex_begin(l, submenu_box);
        {
          ui_button(l, const_string("foo"), button_box);
          ui_button(l, const_string("bar"), button_box);
          ui_button(l, const_string("baz"), button_box);
        }
        ui_flex_end(l);
      }
      if (ui_button(l, const_string("commands"), button_box)) {
        ui_flex_begin(l, submenu_box);
        {
          ui_button(l, const_string("run command"), button_box);
        }
        ui_flex_end(l);
      }
    }
    ui_flex_end(l);
    
    ui_flex_begin(l, (Box){ 
                  .direction = Flex_HORIZONTAL,
                  .width = Flex_STRETCH,
                  });
    {
      ui_panel(l, (Box){ .bg_color = 0xFFFF0000, .width = Flex_STRETCH });
      ui_panel(l, (Box){ .bg_color = 0xFF00FF00, .width = Flex_STRETCH });
    }
    ui_flex_end(l);
  }
  ui_flex_end(l);
#endif
  
  V2 p = v2_mul(r->window_size, -0.5f);
  
  resolve_autos(l->root);
  resolve_flexes(l->root, &p, r);
}
