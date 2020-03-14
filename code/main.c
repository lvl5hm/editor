#include "common.h"

#include <lvl5_files.h>
#include <stdio.h>
#include <lvl5_os.c>

typedef void Editor_Update(Os os, Editor_Memory *memory, os_Input *input);



u64 win32_get_last_write_time(String file_name) {
  WIN32_FIND_DATAA find_data;
  HANDLE file_handle = FindFirstFileA(
    to_c_string(file_name),
    &find_data);
  u64 result = 0;
  if (file_handle != INVALID_HANDLE_VALUE) {
    FindClose(file_handle);
    FILETIME write_time = find_data.ftLastWriteTime;
    
    result = write_time.dwHighDateTime;
    result = result << 32;
    result = result | write_time.dwLowDateTime;
  }
  return result;
}



os_entry_point() {
  context_init(megabytes(20));
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
    .context_info = global_context_info,
  };
  Editor_Memory memory = {
    .window = window,
    .running = true,
  };
  
  
  u64 last_game_dll_write_time = 0;
  os_Dll dll = 0;
  Editor_Update *editor_update = 0;
  
  while (memory.running) {
    String dll_path = const_string("../build/editor.dll");
    
    String lock_path = const_string("../build/lock.tmp");
    WIN32_FILE_ATTRIBUTE_DATA ignored;
    b32 lock_file_exists = 
      GetFileAttributesExA(to_c_string(lock_path), GetFileExInfoStandard, &ignored);
    
    u64 current_write_time = win32_get_last_write_time(dll_path);
    if (!lock_file_exists && 
        current_write_time &&
        last_game_dll_write_time != current_write_time) {
      if (dll) {
        os_free_dll(dll);
      }
      
      String copy_dll_path = const_string("../build/editor_temp.dll");
      
      char *src = to_c_string(dll_path);
      char *dst = to_c_string(copy_dll_path);
      b32 copy_success = CopyFileA(src, dst, false);
      assert(copy_success);
      
      dll = os_load_dll(dst);
      editor_update = (Editor_Update *)os_load_function(dll, "editor_update");
      
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
