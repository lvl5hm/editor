#ifndef BUFFER_H
#include <lvl5_types.h>
#include "font.h"
#include <lvl5_string.h>
#include "parser.h"

#define MAX_EXCHANGE_COUNT 1024
typedef struct {
  char data[MAX_EXCHANGE_COUNT];
  i32 count;
} Exchange;

typedef struct Editor Editor;

typedef struct Buffer {
  String file_name;
  
  char *data;
  i32 count;
  i32 capacity;
  
  i32 cursor;
  i32 mark;
  i32 preferred_col_pos;
  
  Editor *editor;
  struct {
    Arena arena;
    String *dependencies;
    Syntax *colors;
    Token *tokens;
  } cache;
} Buffer;


String buffer_part_to_string(Buffer *, i32, i32);
char get_buffer_char(Buffer *, i32);

#define BUFFER_H
#endif