#ifndef LVL5_CONTEXT

#include "lvl5_types.h"
#include "lvl5_arena.h"

typedef enum {
  Alloc_Op_NONE,
  Alloc_Op_ALLOC,
  Alloc_Op_FREE,
  Alloc_Op_FREE_ALL,
} Alloc_Op;

#define ALLOCATOR(name) byte *(name)(Alloc_Op type, Mem_Size size, void *allocator_data, void *old_ptr, Mem_Size *old_size, Mem_Size align)
typedef ALLOCATOR(*Allocator);

typedef struct {
  Allocator allocator;
  void *allocator_data;
  
  Arena *scratch;
} Context;

typedef struct {
  Context stack[32];
  i32 stack_count;
} Global_Context_Info;

globalvar thread_local Global_Context_Info *global_context_info = null;

Context *get_context() {
  Context *result = global_context_info->stack + global_context_info->stack_count - 1;
  return result;
}

void push_context(Context ctx) {
  global_context_info->stack[global_context_info->stack_count++] = ctx;
}

void pop_context() {
  global_context_info->stack_count--;
}

#define alloc_struct(T) (T *)alloc(sizeof(T))
#define alloc_array(T, count) (T *)alloc(sizeof(T)*count)
byte *alloc(Mem_Size size) {
  Context *ctx = get_context();
  byte *result = ctx->allocator(Alloc_Op_ALLOC, size, ctx->allocator_data, null, 0, 16);
  return result;
}

void free_memory(void *ptr) {
  Context *ctx = get_context();
  ctx->allocator(Alloc_Op_FREE, 0, ctx->allocator_data, ptr, 0, 0);
}

ALLOCATOR(arena_allocator) {
  Context *ctx = get_context();
  Arena *arena = (Arena *)allocator_data;
  
  byte *result = 0;
  switch (type) {
    case Alloc_Op_ALLOC: {
      result = _arena_push_memory(arena, size, align);
    } break;
    
    case Alloc_Op_FREE_ALL: {
      arena->size = 0;
    } break;
    
    
    invalid_default_case;
  }
  
  return result;
}

ALLOCATOR(scratch_allocator) {
  Context *ctx = get_context();
  
  byte *result = null;
  switch (type) {
    case Alloc_Op_ALLOC: {
      result = arena_allocator(type, size, 
                               ctx->scratch, old_ptr,
                               old_size, align);
    } break;
    
    case Alloc_Op_FREE: {
    } break;
    
    
    invalid_default_case;
  }
  return result;
  
  return result;
}

#define scratch_push_struct(T) (T *)scratch_alloc(sizeof(T))
#define scratch_push_array(T, count) (T *)scratch_alloc(sizeof(T)*count)

byte *scratch_alloc(Mem_Size size) {
  byte *result = scratch_allocator(Alloc_Op_ALLOC, size, null, null, 0, 16);
  return result;
}

Mem_Size scratch_get_mark() {
  Context *ctx = get_context();
  return arena_get_mark(ctx->scratch);
}

void scratch_set_mark(Mem_Size mark) {
  Context *ctx = get_context();
  arena_set_mark(ctx->scratch, mark);
}

void scratch_reset() {
  Context *ctx = get_context();
  ctx->scratch->size = 0;
}

void push_scratch_context() {
  Context *ctx = get_context();
  Context ctx2 = *ctx;
  ctx2.allocator = scratch_allocator;
  push_context(ctx2);
}

void push_arena_context(Arena *arena) {
  Context ctx = *get_context();
  ctx.allocator = arena_allocator;
  ctx.allocator_data = arena;
  push_context(ctx);
}



ALLOCATOR(system_allocator) {
  byte *result = null;
  switch (type) {
    case Alloc_Op_ALLOC: {
      result = (byte *)malloc(size);
    } break;
    
    case Alloc_Op_FREE: {
      free(old_ptr);
    } break;
    
    invalid_default_case;
  }
  return result;
}


void context_init(Mem_Size scratch_size) {
  Global_Context_Info *info = calloc(1, sizeof(Global_Context_Info));
  global_context_info = info;
  
  Context default_ctx = {0};
  default_ctx.allocator = system_allocator;
  Arena *scratch = malloc(sizeof(Arena));
  arena_init(scratch, malloc(scratch_size), scratch_size);
  default_ctx.scratch = scratch;
  
  push_context(default_ctx);
}


#define LVL5_CONTEXT
#endif
