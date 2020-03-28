#ifndef EDITOR_COMMON_H

#include "lvl5_os.h"
#include "lvl5_types.h"
#include "lvl5_opengl.h"
#include "lvl5_files.h"


typedef enum {
  Profiler_Event_Type_NONE,
  Profiler_Event_Type_BEGIN,
  Profiler_Event_Type_END,
} Profiler_Event_Type;

typedef struct {
  u64 stamp;
  char *name;
  Profiler_Event_Type type;
} Profiler_Event;

Profiler_Event *profiler_events = null;
i32 profiler_event_capacity = 0;
i32 profiler_event_count = 0;


void add_profiler_event(char *name, Profiler_Event_Type type) {
  if (profiler_event_count < profiler_event_capacity) {
    Profiler_Event *event = profiler_events + profiler_event_count++;
    event->stamp = __rdtsc();
    event->name = name;
    event->type = type;
  }
}


#define begin_profiler_function() add_profiler_event(__FUNCTION__, Profiler_Event_Type_BEGIN)


#define end_profiler_function() add_profiler_event(__FUNCTION__, Profiler_Event_Type_END)


#ifdef EDITOR_SLOW



#else
#define assert

#endif


typedef struct Thread_Queue Thread_Queue;
typedef void Worker(void *);

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
  void (*debug_pring)(char *);
  
  // threads
  Thread_Queue *thread_queue;
  void (*queue_add)(Thread_Queue *, Worker *, void *);
  
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

Os global_os;

#define EDITOR_COMMON_H
#endif