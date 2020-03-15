#include "common.h"

#include <lvl5_files.h>
#include <stdio.h>
#include <lvl5_os.c>
#include <lvl5_arena.h>

typedef void Editor_Update(Os os, Editor_Memory *memory, os_Input *input);



os_entry_point() {
  context_init(megabytes(20));
  profiler_event_capacity = 1000000;
  profiler_events = alloc_array(Profiler_Event, profiler_event_capacity);
  
  
  gl_Funcs gl;
  os_Window window = os_create_window(&gl);
  
  os_Input input = {0};
  
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
    
    .context_info = global_context_info,
    .profiler_event_capacity = profiler_event_capacity,
    .profiler_events = profiler_events,
    .profiler_event_count = profiler_event_count,
  };
  
  Mem_Size memory_size = megabytes(20);
  
  Editor_Memory memory = {
    .window = window,
    .running = true,
    .data = alloc_array(byte, memory_size),
    .size = memory_size,
  };
  
  zero_memory_slow(memory.data, memory.size);
  
  u64 last_game_dll_write_time = 0;
  os_Dll dll = 0;
  Editor_Update *editor_update = 0;
  
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
      
      last_game_dll_write_time = current_write_time;
      memory.reloaded = true;
    } else {
      memory.reloaded = false;
    }
    
    
    begin_profiler_event("loop");
    
    
    editor_update(os, &memory, &input);
    
    end_profiler_event("loop");
    
    if (profiler_event_count && false) {
      FILE *file;
      errno_t err = fopen_s(&file, "profile.json", "wb");
      
      char begin[] = "[\n";
      fwrite(begin, array_count(begin)-1, 1, file);
      
      for (i32 i = 0; i < profiler_event_count; i++) {
        Profiler_Event *event = profiler_events + i;
        char str[512];
        i32 str_count = sprintf_s(str, 512, "  {\"name\": \"%s\", \"cat\": \"PERF\", \"ts\": %lld, \"ph\": \"%c\", \"pid\": 1, \"tid\": 1},\n", event->name, event->stamp, event->type == Profiler_Event_Type_BEGIN ? 'B' : 'E');
        fwrite(str, str_count, 1, file);
      }
      fseek(file, -2, SEEK_CUR);
      
      char end[] = "\n]";
      fwrite(end, array_count(end)-1, 1, file);
      
      fclose(file);
    }
    
    
    os_blit_to_screen();
  }
  
  return 0;
}
