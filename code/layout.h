#ifndef LAYOUT_H

#define ui_SIZE_STRETCH INFINITY
#define ui_SIZE_AUTO 0
#define ui_HORIZONTAL (1 << 0)
#define ui_ALIGN_STRETCH (1 << 1)
#define ui_IGNORE_LAYOUT (1 << 2)
#define ui_HIDDEN (1 << 3)
#define ui_ALIGN_CENTER (1 << 4)
#define ui_FOCUSABLE (1 << 5)
#define ui_FOCUS_TRAP (1 << 6)


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
  
  u32 text_color;
  u32 bg_color;
  f32 layer;
  
  V2 translate;
  V2 scale;
  f32 rotate;
  
  
  u32 active_border_color;
  u32 active_bg_color;
} Style;

// normal style
// hot style
// active style
// focused style
// transition animations??


typedef enum {
  Item_Type_NONE,
  Item_Type_FLEX,
  Item_Type_BUTTON,
  Item_Type_PANEL,
  Item_Type_BUFFER,
  Item_Type_DROPDOWN_MENU,
  Item_Type_MENU_TOGGLE_BUTTON,
  Item_Type_MENU_BAR,
  Item_Type_LABEL,
} Item_Type;

typedef union {
  struct {
    u32 ptr;
  };
  struct {
    u32 parent_ptr;
    u16 index;
    u16 type;
  };
  u64 full;
} ui_Id;

typedef struct ui_Item ui_Item;
typedef struct ui_Item {
  V2 p;
  V2 size;
  V2 min_size;
  
  Item_Type type;
  
  Style style;
  ui_Item **children;
  ui_Item *parent;
  
  ui_Id id;
  
  bool used; // if the item was not used last frame, remove it from the hashtable
  bool rendered;
  
  // stuff that needs to be remembered until the end of a frame to draw
  String label;
  Buffer_View *buffer_view;
  V2 *scroll;
  
  // state
  bool open;
  bool clicked;
  
  
  // temp stuff
  u32 current_child_index;
} ui_Item;


#define LAYOUT_ITEM_MAX 512

#define UI_UNOCCUPIED 0
#define UI_OCCUPIED 1
#define UI_TOMBSTONE 2

typedef struct {
  V2 p;
  
  ui_Id keys[LAYOUT_ITEM_MAX];
  ui_Item values[LAYOUT_ITEM_MAX];
  i8 occupancy[LAYOUT_ITEM_MAX];
  
  u32 count;
  
  ui_Item *current_container;
  
  Renderer *renderer;
  os_Input *input;
  Editor *editor;
  
  ui_Id hot;
  ui_Id active;
  
  V2 ignored_mouse_p;
} ui_Layout;


#define LAYOUT_H
#endif