#include "common.h"

#include "lvl5_files.h"
#include <stdio.h>
#include "lvl5_os.c"
#include "lvl5_arena.h"

typedef void Editor_Update(Os, Editor_Memory *, os_Input *);
typedef void Thread_Handle_Reload(Global_Context_Info *, Os);

#define THREAD_COUNT 4
#define THREAD_QUEUE_CAPACITY 256



typedef struct {
  Worker *fn;
  void *data;
} Thread_Queue_Entry;

typedef struct Thread_Queue {
  Thread_Queue_Entry entries[THREAD_QUEUE_CAPACITY];
  volatile i32 read_cursor;
  volatile i32 write_cursor;
  void *semaphore;
} Thread_Queue;

typedef struct {
  i32 thread_index;
  Thread_Queue *queue;
  bool need_reload;
} Thread_Info;

void queue_add(Thread_Queue *queue, Worker *fn, void *data) {
  while (true) {
    i32 cur_index = queue->write_cursor;
    i32 next_index = (cur_index + 1) % THREAD_QUEUE_CAPACITY;
    
    if (InterlockedCompareExchange(
      (volatile LONG *)&queue->write_cursor,
      next_index,
      cur_index) == cur_index)
    {
      Thread_Queue_Entry *entry = queue->entries + cur_index;
      entry->fn = fn;
      entry->data = data;
      ReleaseSemaphore(queue->semaphore, 1, null);
      break;
    }
  }
}

bool queue_do_next_entry(Thread_Queue *queue) {
  bool result = false;
  
  i32 cur_index = queue->read_cursor;
  i32 next_index = (cur_index + 1) % THREAD_QUEUE_CAPACITY;
  
  if (cur_index != queue->write_cursor) {
    if (InterlockedCompareExchange(
      (volatile LONG *)&queue->read_cursor,
      next_index,
      cur_index) == cur_index)
    {
      Thread_Queue_Entry *entry = queue->entries + cur_index;
      entry->fn(entry->data);
    }
  }
  
  return result;
}

globalvar Thread_Handle_Reload *thread_handle_reload;

DWORD WINAPI thread_proc(void *void_info) {
  Thread_Info *info = (Thread_Info *)void_info;
  Thread_Queue *queue = info->queue;
  
  context_init(megabytes(2));
  
  while (true) {
    if (info->need_reload) {
      thread_handle_reload(global_context_info, global_os);
      info->need_reload = false;
    }
    bool did_entry = queue_do_next_entry(queue);
    if (!did_entry) {
      WaitForSingleObject(queue->semaphore, INFINITE);
    }
  }
}

os_entry_point() {
  context_init(megabytes(20));
  profiler_event_capacity = 1000000;
  profiler_events = alloc_array(Profiler_Event, profiler_event_capacity);
  
  Thread_Info infos[THREAD_COUNT] = {0};
  Thread_Queue queue = {
    .semaphore = CreateSemaphore(null, 0, THREAD_QUEUE_CAPACITY, null),
  };
  {
    for (i32 thread_index = 0; thread_index < THREAD_COUNT; thread_index++) {
      Thread_Info *info = infos + thread_index;
      info->thread_index = thread_index;
      info->queue = &queue;
      
      DWORD thread_id;
      CreateThread(null, 0, thread_proc, info, 0, &thread_id);
      
    }
  }
  
  gl_Funcs gl;
  os_Window window = os_create_window(&gl);
  
  {
    String path = const_string("D:/word/code/editor/data/src/main.c");
    char * dir = to_c_string(os_get_parent_dir(path));
    char * file = to_c_string(os_get_file_name_from_path(path));
    char * ext = to_c_string(os_get_file_ext(path));
    char * base = to_c_string(os_get_file_base(path));
    int foo = 32;
  }
  
  os_Input input = {0};
  
  Thread_Queue *thread_queue = &queue;
  
  Os os = {
    .gl = gl,
    .pop_event = os_pop_event,
    .get_window_size = os_get_window_size,
    .collect_messages = os_collect_messages,
    .read_entire_file = os_read_entire_file,
    .load_font = os_load_font,
    .open_file = os_open_file,
    .get_file_names = os_get_file_names,
    .close_file = os_close_file,
    .read_file = os_read_file,
    .get_file_size = os_get_file_size,
    .debug_pring = OutputDebugStringA,
    
    .thread_queue = thread_queue,
    .queue_add = queue_add,
    
    .context_info = global_context_info,
    .profiler_event_capacity = profiler_event_capacity,
    .profiler_events = profiler_events,
    .profiler_event_count = profiler_event_count,
  };
  global_os = os;
  
  Mem_Size memory_size = megabytes(20);
  
  Editor_Memory memory = {
    .window = window,
    .running = true,
    .data = alloc_array(byte, memory_size),
    .size = memory_size,
  };
  
  zero_memory_slow(memory.data, memory.size);
  
  u64 last_game_dll_write_time = 0;
  os_Dll dll = null;
  Editor_Update *editor_update = null;
  thread_handle_reload = null;
  
  while (memory.running) {
    String lock_path = const_string("../build/lock.tmp");
    String dll_path = const_string("../build/editor.dll");
    bool lock_file_exists = os_get_file_info(lock_path).exists;
    u64 current_write_time = os_get_file_info(dll_path).write_time;
    
    if (!lock_file_exists && 
        current_write_time &&
        last_game_dll_write_time != current_write_time) {
      if (dll) {
        os_free_dll(dll);
      }
      
      String copy_dll_path = const_string("../build/editor_temp.dll");
      
      bool copy_success = os_copy_file(copy_dll_path, dll_path);
      assert(copy_success);
      
      dll = os_load_dll(copy_dll_path);
      editor_update = (Editor_Update *)
        os_load_function(dll, const_string("editor_update"));
      thread_handle_reload = (Thread_Handle_Reload *)
        os_load_function(dll, const_string("thread_handle_reload"));
      
      thread_handle_reload(global_context_info, global_os);
      for (i32 i = 0; i < THREAD_COUNT; i++) {
        Thread_Info *info = infos + i;
        info->need_reload = true;
      }
      
      last_game_dll_write_time = current_write_time;
      memory.reloaded = true;
    } else {
      memory.reloaded = false;
    }
    
    
    editor_update(os, &memory, &input);
    
    os_blit_to_screen();
  }
  
  return 0;
}
