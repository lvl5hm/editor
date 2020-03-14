#ifndef EDITOR_H
#include "buffer.h"
#include <lvl5_os.h>

typedef enum {
  Command_NONE,
  Command_COPY,
  Command_PASTE,
  Command_CUT,
  Command_MOVE_CURSOR_LEFT = os_Keycode_ARROW_LEFT,
  Command_MOVE_CURSOR_RIGHT = os_Keycode_ARROW_RIGHT,
  Command_MOVE_CURSOR_UP = os_Keycode_ARROW_UP,
  Command_MOVE_CURSOR_DOWN = os_Keycode_ARROW_DOWN,
} Command;

// TODO: hashtable this shit
typedef struct {
  bool shift;
  bool ctrl;
  bool alt;
  os_Keycode keycode;
  
  Command command;
} Keybind;

typedef struct {
  Keybind *keybinds;
} Settings;

typedef struct {
  Settings settings;
  Text_Buffer buffer;
} Editor;


#define EDITOR_H
#endif