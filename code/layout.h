#ifndef LAYOUT_H

#include "lvl5_math.h"
#include "buffer.h"

#define ui_SIZE_STRETCH INFINITY
#define ui_SIZE_AUTO 0
#define ui_HORIZONTAL (1 << 0)
#define ui_ALIGN_STRETCH (1 << 1)
#define ui_IGNORE_LAYOUT (1 << 2)
#define ui_HIDDEN (1 << 3)
#define ui_ALIGN_CENTER (1 << 4)
#define ui_FOCUSABLE (1 << 5)
#define ui_FOCUS_TRAP (1 << 6)
#define ui_TOGGLE (1 << 7)

typedef enum {
  Unit_PIXELS,
  Unit_PERCENT,
  Unit_FRACTIONS,
} Unit;

typedef struct {
  f32 value;
  Unit unit;
} Dim;

Dim px(f32 v) {
  return (Dim){v, Unit_PIXELS};
}

Dim percent(f32 v) {
  return (Dim){v, Unit_PERCENT};
}

Dim fr(f32 v) {
  return (Dim){v, Unit_FRACTIONS};
}

typedef struct {
  u64 flags;
  
  union {
    struct {
      Dim width;
      Dim height;
    };
    Dim dims[2];
  };
  
  f32 padding_left;
  f32 padding_right;
  f32 padding_top;
  f32 padding_bottom;
  Dim min_width;
  Dim min_height;
  
  f32 border_width;
  u32 border_color;
  u32 active_border_color;
  
  u32 text_color;
  u32 bg_color;
  u32 active_bg_color;
  f32 layer;
} Style;



typedef enum {
  Item_Type_NONE,
  Item_Type_FLEX,
  Item_Type_BUTTON,
  Item_Type_PANEL,
  Item_Type_BUFFER,
  Item_Type_DROPDOWN_MENU,
  Item_Type_MENU_BAR,
  Item_Type_LABEL,
} Item_Type;

typedef union {
  void *ptr;
  byte bytes[8];
} ui_Id;

#define INVALID_UI_ID (ui_Id){NULL}

typedef struct ui_Item ui_Item;
typedef struct ui_Item {
  V2 p;
  V2 size;
  V2 min_size;
  
  Item_Type type;
  ui_Id id;
  
  Style style;
  ui_Item *children;
  ui_Item *parent;
  
  bool rendered;
  
  // stuff that needs to be remembered until the end of a frame to draw
  String label;
  Buffer_View *buffer_view;
  V2 *scroll;
} ui_Item;


typedef enum {
  ui_Layout_Mode_INTERACT,
  ui_Layout_Mode_CALC_AUTOS,
  ui_Layout_Mode_DRAW,
  ui_Layout_Mode_CHILDREN_FINISHED,
} ui_Layout_Mode;



typedef struct Buffer_View {
  Buffer *buffer;
  i32 visible_cursor;
  i32 preferred_col_pos;
  i32 visible_mark;
  
  bool is_single_line;
} Buffer_View;



#define LAYOUT_BUTTON_MAX 512

typedef union {
  bool open;
  struct {
    Buffer_View buffer_view;
    Buffer buffer;
    V2 scroll;
  } panel;
} ui_State;

typedef struct {
  V2 p;
  
  ui_Id keys[LAYOUT_BUTTON_MAX];
  ui_State values[LAYOUT_BUTTON_MAX];
  bool occupancy[LAYOUT_BUTTON_MAX];
  u32 count;
  
  ui_Item *current_container;
  
  Renderer *renderer;
  os_Input *input;
  Editor *editor;
  
  ui_Id hot;
  ui_Id active;
  ui_Id interactive;
  
  ui_Id next_hot;
  ui_Id next_active;
  
  V2 ignored_mouse_p;
} ui_Layout;


#define LAYOUT_H
#endif