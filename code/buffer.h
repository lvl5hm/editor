#ifndef EDITOR_H
#include <lvl5_types.h>
#include "font.h"
#include <lvl5_string.h>

#define MAX_EXCHANGE_COUNT 1024

typedef struct Token Token;

typedef struct {
  String file_name;
  
  char *data;
  i32 count;
  i32 capacity;
  
  i32 cursor;
  i32 mark;
  i32 preferred_col_pos;
  
  // TODO(lvl5): exchange shouldn't be in the buffer
  char exchange[MAX_EXCHANGE_COUNT];
  i32 exchange_count;
  
  Token *tokens;
} Buffer;


String buffer_part_to_string(Buffer *, i32, i32);
char get_buffer_char(Buffer *, i32);





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


#ifdef EDITOR_SLOW

#define begin_profiler_event(name) add_profiler_event(name, Profiler_Event_Type_BEGIN)
#define end_profiler_event(name) add_profiler_event(name, Profiler_Event_Type_END)

#else
#define begin_profiler_event(name)
#define end_profiler_event(name)

#endif

#define EDITOR_H
#endif