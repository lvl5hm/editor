#ifndef FONT_H
#include <lvl5_types.h>

typedef struct {
  Bitmap bmp;
  Rect2i *rects;
  i32 count;
} Texture_Atlas;

typedef struct {
  Texture_Atlas atlas;
  char first_codepoint;
  i32 codepoint_count;
  V2 *origins;
  i8 *advance;
  i8 line_spacing;
} Font;


#define FONT_H
#endif