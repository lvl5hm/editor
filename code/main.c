#include "common.h"

#include <lvl5_files.h>
#include <stdio.h>
#include <lvl5_os.c>

typedef void Editor_Update(Os os, Editor_Memory *memory, os_Input *input);



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
  
  
  os_Dll dll = os_load_dll("../build/editor.dll");
  Editor_Update *editor_update = (Editor_Update *)os_load_function(dll, "editor_update");
  
  while (memory.running) {
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
