#ifndef LVL5_ARENA
#define LVL5_ARENA_VERSION 0

#include "lvl5_types.h"
#include "lvl5_intrinsics.h"

typedef struct {
  byte *data;
  Mem_Size size;
  Mem_Size capacity;
  
#ifdef LVL5_DEBUG
  u32 marks_taken;
#endif
} Arena;


void copy_memory_slow(void *dst, void *src, Mem_Size size) {
  for (u64 i = 0; i < size; i++) {
    ((byte *)dst)[i] = ((byte *)src)[i];
  }
}

void zero_memory_fast(void *dst, Mem_Size size) {
  assert((size & 15) == 0);
  assert(((Mem_Size)dst & 15) == 0);
  
  __m128 zero = _mm_set_ps1(0);
  for (Mem_Size i = 0; i < size; i += 16) {
    _mm_store_ps((f32 *)((byte *)dst + i), zero);
  }
}

void zero_memory_slow(void *dst, Mem_Size size) {
  for (u64 i = 0; i < size; i++) {
    ((byte *)dst)[i] = 0;
  }
} 

void arena_init(Arena *arena, void *data, Mem_Size capacity) {
  arena->data = (byte *)data;
  arena->capacity = capacity;
  arena->size = 0;
}

#define arena_push_array(arena, T, count) \
(T *)_arena_push_memory(arena, sizeof(T)*count, 16)

#define arena_push_struct(arena, T) \
(T *)_arena_push_memory(arena, sizeof(T), 16)

#define arena_push_size(arena, size) \
_arena_push_memory(arena, size, 16)

byte *_arena_push_memory(Arena *arena, Mem_Size size, Mem_Size align) {
  byte *result = 0;
  assert(arena->size + size <= arena->capacity);
  
  Mem_Size data_u64 = (Mem_Size)(arena->data + arena->size);
  Mem_Size data_u64_aligned = align_pow_2(data_u64, align);
  result = (byte *)data_u64_aligned;
  arena->size += align_pow_2(size, align);
  
  return result;
}

Mem_Size arena_get_mark(Arena *arena) {
  Mem_Size result = arena->size;
  
#ifdef LVL5_DEBUG
  arena->marks_taken++;
#endif
  
  return result;
}

void arena_set_mark(Arena *arena, Mem_Size mark) {
  assert(mark <= arena->capacity);
  arena->size = mark;
  
#ifdef LVL5_DEBUG
  arena->marks_taken--;
#endif
}

void arena_check_no_marks(Arena *arena) {
#ifdef LVL5_DEBUG
  assert(arena->marks_taken == 0);
#endif
}

void arena_init_subarena(Arena *parent, Arena *child,
                         Mem_Size capacity) {
  byte *child_memory = arena_push_array(parent, byte, capacity);
  arena_init(child, child_memory, capacity);
}

#define LVL5_ARENA
#endif