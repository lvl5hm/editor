#include "editor.h"

Keybind *get_keybind(Settings *settings, os_Keycode keycode, 
                     bool shift, bool ctrl, bool alt) 
{
  Keybind *result = null;
  for (u32 i = 0; i < sb_count(settings->keybinds); i++) {
    Keybind *k = settings->keybinds + i;
    if (k->keycode == keycode &&
        k->shift == shift && 
        k->ctrl == ctrl && 
        k->alt == alt)
    {
      result = k;
      break;
    }
  }
  return result;
}
