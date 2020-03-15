#ifndef EDITOR_COMMON_H

#include <lvl5_os.h>
#include <lvl5_types.h>
#include <lvl5_opengl.h>
#include "renderer.h"

typedef struct {
  gl_Funcs gl;
  b32 (*pop_event)(os_Event*);
  V2 (*get_window_size)();
  void (*collect_messages)(os_Window, os_Input*);
  String (*read_entire_file)(String);
  Font (*load_font)(String, String, i32);
  
  Global_Context_Info *context_info;
} Os;

typedef struct {
  bool initialized;
  bool reloaded;
  bool running;
  os_Window window;
  
  byte *data;
  Mem_Size size;
} Editor_Memory;


#define EDITOR_COMMON_H
#endif