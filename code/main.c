#include "editor.c"
#include <lvl5_files.h>
#include <stdio.h>
#include <lvl5_os.c>

extern void editor_update(Os os, Editor_Memory *memory, os_Input *input);



os_entry_point() {
  context_init(megabytes(20));
  os_Window window = os_create_window();
  
#if 0  
  Keybind keybinds[] = {
    (Keybind){
      .command = Command_COPY,
      .keycode = 'C',
      .ctrl = true,
    },
    (Keybind){
      .command = Command_PASTE,
      .keycode = 'V',
      .ctrl = true,
    },
    (Keybind){
      .command = Command_CUT,
      .keycode = 'X',
      .ctrl = true,
    },
    (Keybind){
      .command = Command_MOVE_CURSOR_LEFT,
      .keycode = os_Keycode_ARROW_LEFT,
    },
    (Keybind){
      .command = Command_MOVE_CURSOR_RIGHT,
      .keycode = os_Keycode_ARROW_RIGHT,
    },
    (Keybind){
      .command = Command_MOVE_CURSOR_UP,
      .keycode = os_Keycode_ARROW_UP,
    },
    (Keybind){
      .command = Command_MOVE_CURSOR_DOWN,
      .keycode = os_Keycode_ARROW_DOWN,
    },
  };
#endif
  
  
  os_Input input = {0};
  
  Os os = {
    .gl = gl,
    .pop_event = os_pop_event,
    .get_window_size = os_get_window_size,
    .collect_messages = os_collect_messages,
    .read_entire_file = os_read_entire_file,
    .load_font = os_load_font,
  };
  Editor_Memory memory = {
    .window = window,
    .running = true,
  };
  
  
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
