#ifndef EDITOR_H
#include <lvl5_types.h>
#include "font.h"
#include <lvl5_string.h>

#define MAX_EXCHANGE_COUNT 1024

typedef struct Token Token;

typedef struct {
  char *data;
  i32 count;
  i32 capacity;
  
  i32 cursor;
  i32 preferred_col_pos;
  i32 mark;
  
  f32 scroll_y;
  
  char exchange[MAX_EXCHANGE_COUNT];
  i32 exchange_count;
  
  Token *tokens;
  
  bool context_menu;
} Text_Buffer;


String buffer_part_to_string(Text_Buffer *, i32, i32);
char get_buffer_char(Text_Buffer *, i32);





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

Profiler_Event profiler_events[10000000];
i32 profiler_event_count = 0;


void add_profiler_event(char *name, Profiler_Event_Type type) {
  if (profiler_event_count < array_count(profiler_events)) {
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