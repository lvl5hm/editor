#ifndef EDITOR_H
#include <lvl5_types.h>

#define MAX_EXCHANGE_COUNT 1024

typedef struct {
  char *data;
  i32 count;
  i32 capacity;
  
  i32 cursor;
  i32 mark;
  
  char exchange[MAX_EXCHANGE_COUNT];
  i32 exchange_count;
} Text_Buffer;

#define EDITOR_H
#endif