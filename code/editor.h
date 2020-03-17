#ifndef EDITOR_H
#include "buffer.h"
#include "common.h"

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
  Command_REMOVE_BACKWARD,
  Command_REMOVE_FORWARD,
  Command_NEWLINE,
  Command_TAB,
  Command_OPEN_FILE_DIALOG,
  Command_LISTER_MOVE_UP,
  Command_LISTER_MOVE_DOWN,
  Command_LISTER_SELECT,
} Command;

typedef enum {
  Syntax_BACKGROUND,
  Syntax_DEFAULT,
  Syntax_COMMENT,
  Syntax_TYPE,
  Syntax_MACRO,
  Syntax_FUNCTION,
  Syntax_ARG,
  Syntax_OPERATOR,
  Syntax_KEYWORD,
  Syntax_CURSOR,
  Syntax_NUMBER,
  Syntax_STRING,
  
  Syntax_count,
} Syntax;

typedef struct {
  String name;
  u32 colors[Syntax_count];
} Color_Theme;



typedef enum {
  View_Type_NONE,
  View_Type_BUFFER,
  View_Type_FILE_DIALOG,
  View_Type_SETTINGS,
} View_Type;


// TODO: hashtable this shit
typedef struct {
  bool shift;
  bool ctrl;
  bool alt;
  os_Keycode keycode;
  View_Type views;
  
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




typedef struct {
  i32 view_index;
  Rect2 rect;
} Panel;


typedef enum {
  File_Dialog_Type_NONE,
  File_Dialog_Type_OPEN,
  File_Dialog_Type_NEW,
} File_Dialog_Type;

typedef struct {
  int _;
} File_Dialog;

typedef struct {
  int _;
} Settings_Editor;

typedef struct {
  union {
    Buffer buffer;
    File_Dialog file_dialog;
    Settings_Editor settings_editor;
  };
  View_Type type;
} View;

typedef struct {
  Panel *panels;
  i32 active_panel_index;
  
  View *views;
  
  Settings settings;
  String current_dir;
  Lister current_dir_files;
} Editor;

typedef struct {
  Renderer renderer;
  Font font;
  Editor editor;
} App_State;

#define EDITOR_H
#endif