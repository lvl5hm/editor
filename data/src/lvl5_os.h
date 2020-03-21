#ifndef LVL5_OS_H
#include "lvl5_math.h"

typedef enum {
  os_Keycode_NONE = 0,
  os_Keycode_SPACE = 0x20,
  os_Keycode_ARROW_LEFT = 0x25,
  os_Keycode_ARROW_RIGHT = 0x27,
  os_Keycode_ARROW_UP = 0x26,
  os_Keycode_ARROW_DOWN = 0x28,
  os_Keycode_ENTER = 0x0D,
  os_Keycode_LCRTL = 0x11,
  os_Keycode_ESCAPE = 0x1B,
  os_Keycode_TAB = 0x09,
  os_Keycode_LSHIFT = 0xA0,
  os_Keycode_LALT = 0xA4,
  os_Keycode_RCTRL = 0xA3,
  os_Keycode_RSHIFT = 0xA1,
  os_Keycode_RALT = 0xA5,
  os_Keycode_CAPS_LOCK = 0x14,
  os_Keycode_NUM_LOCK = 0x90,
  os_Keycode_BACKSPACE = 0x08,
  os_Keycode_F1 = 0x70,
  os_Keycode_F2 = 0x71,
  os_Keycode_F3 = 0x72,
  os_Keycode_F4 = 0x73,
  os_Keycode_F5 = 0x74,
  os_Keycode_F6 = 0x75,
  os_Keycode_F7 = 0x76,
  os_Keycode_F8 = 0x77,
  os_Keycode_F9 = 0x78,
  os_Keycode_F10 = 0x79,
  os_Keycode_F11 = 0x80,
  os_Keycode_F12 = 0x81,
  os_Keycode_DELETE = 0x2E,
  os_Keycode_SHIFT = 0x10,
  os_Keycode_CTRL = 0x11,
  os_Keycode_ALT = 0x12,
  os_Keycode_HOME = 0x24,
  os_Keycode_END = 0x23,
  
  os_Keycode_count = 128,
} os_Keycode;


typedef enum {
  os_Event_Type_NONE = 0,
  os_Event_Type_CLOSE = 0x100,
  os_Event_Type_RESIZE = 0x101,
  os_Event_Type_BUTTON = 0x102,
} os_Event_Type;

typedef void *os_Window;
typedef void *os_Dll;

typedef struct os_Button {
  bool is_down;
  bool went_down;
  bool went_up;
  bool pressed;
} os_Button;

typedef struct os_Mouse {
  os_Button left;
  os_Button middle;
  os_Button right;
  V2 p;
  f32 wheel;
} os_Mouse;

#define MAX_CHARS_PER_FRAME 16
typedef struct os_Input {
  os_Mouse mouse;
  os_Button keys[os_Keycode_count];
  bool shift;
  bool alt;
  bool ctrl;
  bool caps_lock;
  bool num_lock;
  
  char chars[MAX_CHARS_PER_FRAME];
  i32 char_count;
} os_Input;



typedef struct {
  int _;
} os_Event_Close;

typedef struct {
  int _;
} os_Event_Resize;

typedef struct {
  os_Keycode keycode;
} os_Event_Button;

typedef struct os_Event {
  union {
    os_Event_Close close;
    os_Event_Resize resize;
    os_Event_Button button;
  };
  
  os_Event_Type type;
} os_Event;

typedef struct {
  bool exists;
  u64 write_time;
} os_File_Info;

typedef void *os_File;

#define LVL5_OS_H
#endif
