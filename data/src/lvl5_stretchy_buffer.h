#ifndef LVL5_STRETCHY_BUFFER
#define LVL5_STRETCHY_BUFFER_VERSION 0

#include "lvl5_arena.h"
#include "lvl5_context.h"

typedef struct {
  u32 count;
  u32 capacity;
  Allocator allocator;
  void *allocator_data;
} sb_Header;


#ifndef LVL5_STRETCHY_BUFFER_GROW_FACTOR
#define LVL5_STRETCHY_BUFFER_GROW_FACTOR 2
#endif


#define __get_header(arr) ((sb_Header *)(arr)-1)
#define sb_count(arr) (__get_header(arr)->count)
#define sb_capacity(arr) (__get_header(arr)->capacity)

#define __need_grow(arr) (!arr || (sb_count(arr) == sb_capacity(arr)))
#define sb_new(T, capacity) __sb_init(capacity, sizeof(T))
void *__sb_init(u32 capacity, Mem_Size item_size) {
  Context *ctx = get_context();
  sb_Header header = {0};
  header.count = 0;
  header.capacity = capacity;
  header.allocator = ctx->allocator;
  header.allocator_data = ctx->allocator_data;
  
  byte *memory = header.allocator(Alloc_Op_ALLOC,
                                  sizeof(sb_Header) + item_size*capacity,
                                  header.allocator_data, null, 0, 16);
  *(sb_Header *)memory = header;
  return memory + sizeof(sb_Header);
}

void sb_free(void *arr) {
  sb_Header *header = (sb_Header *)arr - 1;
  header->allocator(Alloc_Op_FREE, 0, header->allocator_data, header, 0, 16);
}

#define sb_push(arr, item) (__need_grow(arr) ? __grow(&(arr), sizeof(item)) : 0, (arr)[sb_count(arr)++] = (item))

void *__grow(void *arr_ptr_, Mem_Size item_size) {
  void **arr_ptr = (void **)arr_ptr_;
  void *arr = *arr_ptr;
  assert(arr);
  
  sb_Header *header = __get_header(arr);
  
  i32 new_capacity = header->capacity*LVL5_STRETCHY_BUFFER_GROW_FACTOR;
  u32 header_size = sizeof(sb_Header);
  byte *result = header->allocator(Alloc_Op_ALLOC, new_capacity*item_size + header_size, header->allocator_data, null, 0, 16) + header_size;
  copy_memory_slow(result, arr, header->capacity*item_size);
  
  sb_Header *new_header = __get_header(result);
  *new_header = *header;
  new_header->capacity = new_capacity;
  
  *arr_ptr = result;
  
  return 0;
}

#define LVL5_STRETCHY_BUFFER
#endif