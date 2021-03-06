#ifndef EDITOR_H
#include "buffer.h"
#include "common.h"
#include "renderer.h"
#include "lvl5_stretchy_buffer.h"
#include "layout.h"

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
  Command_FILE_OPEN,
  Command_SAVE_BUFFER,
} Command;

typedef struct Color_Theme {
  String name;
  u32 colors[Syntax_count];
} Color_Theme;



typedef enum {
  Panel_Type_NONE,
  Panel_Type_BUFFER,
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
  union {
    Buffer_View buffer_view;
  };
  
  Panel_Type type;
  Rect2 rect;
  String name;
  V2 scroll;
} Panel;


typedef struct Editor {
  String path;
  String *files;
  bool file_dialog_open;
  String selected_file_name;
  
  Buffer *buffers;
  Panel *panels;
  i32 active_panel_index;
  
  Settings settings;
  Exchange exchange;
  
  i32 generation;
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


