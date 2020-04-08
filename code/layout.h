#ifndef LAYOUT_H

#define ui_SIZE_STRETCH INFINITY
#define ui_SIZE_AUTO 0
#define ui_HORIZONTAL (1 << 0)
#define ui_ALIGN_STRETCH (1 << 1)
#define ui_IGNORE_LAYOUT (1 << 2)
#define ui_HIDDEN (1 << 3)
#define ui_WIDTH_PERCENT (1 << 4)
#define ui_HEIGHT_PERCENT (1 << 5)

typedef struct {
  u64 flags;
  
  union {
    struct {
      f32 width;
      f32 height;
    };
    f32 dims[2];
  };
  f32 padding_left;
  f32 padding_right;
  f32 padding_top;
  f32 padding_bottom;
  f32 min_width;
  f32 min_height;
  
  u32 bg_color;
  f32 layer;
} Style;



typedef enum {
  Item_Type_NONE,
  Item_Type_FLEX,
  Item_Type_BUTTON,
  Item_Type_PANEL,
  Item_Type_DROPDOWN_MENU,
  Item_Type_MENU_TOGGLE_BUTTON,
} Item_Type;

typedef struct {
  u8 func;
  u8 loop;
  u8 call;
} ui_Id;

#define INVALID_UI_ID (ui_Id){.func=0, .call = 0, .loop = 0}

typedef struct ui_Item ui_Item;
typedef struct ui_Item {
  V2 p;
  
  Item_Type type;
  ui_Id state_id;
  
  Style style;
  ui_Item *children;
  ui_Item *parent;
  
  bool rendered;
} ui_Item;


#define LAYOUT_BUTTON_MAX 512

typedef struct {
  bool is_active;
  String label;
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
  
  ui_Id hot;
  ui_Id active;
} ui_Layout;


#define LAYOUT_H
#endif