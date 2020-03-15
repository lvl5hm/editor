#ifndef EDITOR_H
#include "buffer.h"
#include "common.h"

typedef enum {
  Command_NONE,
  Command_COPY,
  Command_PASTE,
  Command_CUT,
  Command_MOVE_CURSOR_LEFT = os_Keycode_ARROW_LEFT,
  Command_MOVE_CURSOR_RIGHT = os_Keycode_ARROW_RIGHT,
  Command_MOVE_CURSOR_UP = os_Keycode_ARROW_UP,
  Command_MOVE_CURSOR_DOWN = os_Keycode_ARROW_DOWN,
  Command_REMOVE_BACKWARD,
  Command_REMOVE_FORWARD,
  Command_NEWLINE,
  Command_TAB,
} Command;

typedef enum {
  Text_Color_Type_BACKGROUND,
  Text_Color_Type_DEFAULT,
  Text_Color_Type_COMMENT,
  Text_Color_Type_TYPE,
  Text_Color_Type_MACRO,
  Text_Color_Type_FUNCTION,
  Text_Color_Type_ARG,
  Text_Color_Type_OPERATOR,
  Text_Color_Type_KEYWORD,
  Text_Color_Type_CURSOR,
  Text_Color_Type_NUMBER,
  Text_Color_Type_STRING,
  
  Text_Color_Type_count,
} Text_Color_Type;

typedef struct {
  String name;
  u32 colors[Text_Color_Type_count];
} Color_Theme;


typedef enum {
  View_Type_NONE,
  View_Type_BUFFER,
  View_Type_SETTINGS,
  View_Type_OPEN_FILE,
  View_Type_CREATE_FILE,
} View_Type;


// TODO: hashtable this shit
typedef struct {
  bool shift;
  bool ctrl;
  bool alt;
  os_Keycode keycode;
  View_Type type;
  
  Command command;
} Keybind;

typedef struct {
  Keybind *keybinds;
  Color_Theme theme;
} Settings;

typedef struct {
  Settings settings;
  Text_Buffer buffer;
  View_Type view_type;
} Editor;

typedef struct {
  Renderer renderer;
  Font font;
  Editor editor;
} Editor_State;

#define EDITOR_H
#endif