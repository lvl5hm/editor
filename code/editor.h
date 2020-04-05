#ifndef EDITOR_H
#include "buffer.h"
#include "common.h"
#include "renderer.h"
#include "lvl5_stretchy_buffer.h"

typedef enum {
  Command_NONE,
  Command_COPY,
  Command_SET_MARK,
  Command_PASTE,
  Command_CUT,
  Command_MOVE_CURSOR_LEFT,
  Command_MOVE_CURSOR_RIGHT,
  Command_MOVE_CURSOR_UP,
  Command_MOVE_CURSOR_DOWN,
  Command_MOVE_CURSOR_LINE_START,
  Command_MOVE_CURSOR_LINE_END,
  Command_MOVE_CURSOR_WORD_START,
  Command_MOVE_CURSOR_WORD_END,
  Command_REMOVE_BACKWARD,
  Command_REMOVE_FORWARD,
  Command_NEWLINE,
  Command_TAB,
  Command_OPEN_FILE_DIALOG,
  Command_LISTER_MOVE_UP,
  Command_LISTER_MOVE_DOWN,
  Command_LISTER_SELECT,
  Command_SAVE_BUFFER,
} Command;

typedef struct Color_Theme {
  String name;
  u32 colors[Syntax_count];
} Color_Theme;



typedef enum {
  Panel_Type_NONE,
  Panel_Type_BUFFER,
  Panel_Type_FILE_DIALOG_OPEN,
  Panel_Type_FILE_DIALOG_NEW,
  Panel_Type_SETTINGS,
} Panel_Type;


// TODO: hashtable this shit
typedef struct {
  bool shift;
  bool ctrl;
  bool alt;
  os_Keycode keycode;
  Panel_Type views;
  
  Command command;
} Keybind;

typedef struct {
  Keybind *keybinds;
  Color_Theme theme;
} Settings;

typedef struct {
  String *items;
  i32 index;
} Lister;


typedef struct Buffer_View {
  Buffer *buffer;
  i32 visible_cursor;
  i32 preferred_col_pos;
  i32 visible_mark;
} Buffer_View;

typedef struct {
  String path;
  Lister files;
} File_Dialog;

typedef struct {
  union {
    Buffer_View buffer_view;
    File_Dialog file_dialog;
  };
  
  Panel_Type type;
  Rect2 rect;
  String name;
  V2 scroll;
} Panel;




#define ui_SIZE_STRETCH INFINITY
#define ui_SIZE_AUTO 0
#define ui_HORIZONTAL (1 << 0)
#define ui_ALIGN_STRETCH (1 << 1)
#define ui_IGNORE_LAYOUT (1 << 2)
#define ui_HIDDEN (1 << 3)

typedef struct {
  u64 flags;
  
  f32 width;
  f32 height;
  f32 padding_left;
  f32 padding_right;
  f32 padding_top;
  f32 padding_bottom;
  
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


typedef struct Editor {
  Buffer *buffers;
  Panel *panels;
  i32 active_panel_index;
  
  Settings settings;
  Exchange exchange;
  
  i32 generation;
  
  i32 menu_index;
  ui_Layout layout;
} Editor;

#include "renderer.h"

typedef struct {
  Renderer renderer;
  Font font;
  Editor editor;
} App_State;


#define EDITOR_H
#endif
