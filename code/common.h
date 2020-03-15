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
  os_File (*open_file)(String);
  String *(*get_file_names)(String);
  void (*close_file)(os_File);
  void (*read_file)(os_File, void*, u64, u64);
  u64 (*get_file_size)(os_File);
  
  Global_Context_Info *context_info;
  Profiler_Event *profiler_events;
  i32 profiler_event_capacity;
  i32 profiler_event_count;
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