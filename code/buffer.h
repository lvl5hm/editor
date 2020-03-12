#ifndef EDITOR_H
#include <lvl5_types.h>

#define MAX_EXCHANGE_COUNT 1024

typedef struct {
  char *data;
  i32 count;
  i32 capacity;
  
  i32 cursor;
  i32 mark;
  
  f32 scroll_y;
  
  char exchange[MAX_EXCHANGE_COUNT];
  i32 exchange_count;
} Text_Buffer;


String buffer_part_to_string(Text_Buffer *, i32, i32);
char get_buffer_char(Text_Buffer *, i32);


#define begin_profiler_event(name)
void _begin_profiler_event(char *);
#define end_profiler_event(name)
void _end_profiler_event(char *);

#define EDITOR_H
#endif