#ifndef LVL5_FILES
#include <lvl5_context.h>
#include <stdio.h>


typedef struct {
  i32 width;
  i32 height;
  u32 *data;
} Bitmap;

#pragma pack(push, 1)
typedef struct {
  u32 info_header_size;
  i32 width;
  i32 height;
  i16 planes;
  i16 bits_per_pixel;
  u32 compression;
  u32 image_size;
  i32 x_pixels_per_meter;
  i32 y_pixels_per_meter;
  u32 colors_used;
  u32 important_colors;
  u32 red_mask;
  u32 green_mask;
  u32 blue_mask;
  u32 alpha_mask;
  u32 cs_type;
  i32 red_x;
  i32 red_y;
  i32 red_z;
  i32 green_x;
  i32 green_y;
  i32 green_z;
  i32 blue_x;
  i32 blue_y;
  i32 blue_z;
  u32 gamma_red;
  u32 gamma_green;
  u32 gamma_blue;
} Bmp_Info_Header;

typedef struct {
  u16 signature;
  u32 file_size;
  u32 reserved;
  u32 data_offset;
  Bmp_Info_Header info;
} Bmp_File_Header;

typedef struct {
  byte red;
  byte green;
  byte blue;
  byte reserved;
} Bmp_File_Color;
#pragma pack(pop)

#define BMP_SIGNATURE (('B' << 0) | ('M' << 8))

Bitmap bmp_load(char *file_name) {
  FILE *file;
  fopen_s(&file, file_name, "rb");
  fseek(file, 0, SEEK_END);
  u32 file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  byte *file_data = alloc(file_size);
  fread(file_data, file_size, 1, file);
  fclose(file);
  
  Bmp_File_Header *header = (Bmp_File_Header *)file_data;
  assert(header->signature == BMP_SIGNATURE);
  assert(header->file_size == file_size);
  assert(header->info.planes == 1);
  assert(header->info.bits_per_pixel == 32);
  
  Bitmap result = {0};
  result.data = (u32 *)(file_data + header->data_offset);
  result.width = header->info.width;
  result.height = header->info.height;
  
  return result;
}

void bmp_save(char *file_name, Bitmap bmp) {
  Bmp_Info_Header info = {0};
  info.info_header_size = 124;
  info.width = bmp.width;
  info.height = bmp.height;
  info.planes = 1;
  info.bits_per_pixel = 32;
  info.compression = 3;
  info.image_size = bmp.width*bmp.height*4;
  info.x_pixels_per_meter = 11811;
  info.y_pixels_per_meter = 11811;
  info.colors_used = 0;
  info.important_colors = 0;
  info.red_mask = 0x00FF0000;
  info.green_mask = 0x0000FF00;
  info.blue_mask = 0x000000FF;
  info.alpha_mask = 0xFF000000;
  info.cs_type = 0x73524742;
	
  Bmp_File_Header header = {0};
  header.signature = BMP_SIGNATURE;
  header.data_offset = 138;
  header.file_size = header.data_offset + info.image_size;
  header.reserved = 0;
  header.info = info;
  
  FILE *file;
  fopen_s(&file, file_name, "wb");
  fwrite(&header, sizeof(header), 1, file);
  fseek(file, header.data_offset, SEEK_SET);
  fwrite(bmp.data, info.image_size, 1, file);
  
  fclose(file);
}





Bitmap make_empty_bitmap(i32 width, i32 height) {
  Bitmap result;
  result.width = width;
  result.height = height;
  
  Mem_Size size = align_pow_2(width*height*sizeof(u32), 16);
  result.data = (u32 *)alloc(size);
  zero_memory_fast(result.data, size);
  return result;
}







// fonts
#include <lvl5_math.h>

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
  i8 *kerning;
  i8 line_spacing;
  i8 line_height;
  i8 descent;
} Font;


Texture_Atlas texture_atlas_make_from_bitmaps(Bitmap *bitmaps, i32 bitmap_count, i32 atlas_width) {
  Texture_Atlas result = {0};
  result.rects = alloc_array(Rect2i, bitmap_count);
  result.count = bitmap_count;
  
  i32 total_height = 0;
  
  i32 row_height = 0;
  i32 row_width = 0;
  
  i32 PADDING = 2;
  
  for (i32 i = 0; i < bitmap_count; i++) {
    Bitmap bmp = bitmaps[i];
    if (row_width + bmp.width > atlas_width) {
      total_height += row_height;
      row_width = 0;
      row_height = 0;
    }
    
    result.rects[i] = rect2i_min_size(v2i(row_width, total_height),
                                      v2i(bmp.width, bmp.height));
    row_width += bmp.width + PADDING;
    if (bmp.height > row_height) {
      row_height = bmp.height;
    }
  }
  
  total_height += row_height;
  result.bmp = make_empty_bitmap(atlas_width, total_height);
  
  for (i32 i = 0; i < bitmap_count; i++) {
    Rect2i rect = result.rects[i];
    Bitmap bmp = bitmaps[i];
    
    for (i32 y = 0; y < bmp.height; y++) {
      for (i32 x = 0; x < bmp.width; x++) {
        result.bmp.data[(rect.min.y + y)*result.bmp.width + rect.min.x + x] = bmp.data[y*bmp.width + x];
      }
    }
  }
  
  
  return result;
}


i32 font_get_char(Font *font, char c) {
  assert(c >= font->first_codepoint && c <= font->first_codepoint + font->codepoint_count);
  i32 result = c - font->first_codepoint;
  return result;
}

i8 font_get_advance(Font *font, char a, char b) {
  i32 a_index = font_get_char(font, a);
  
  i8 result = font->advance[a_index];
  if (b >= font->first_codepoint && b <= font->first_codepoint + font->codepoint_count)
  {
    i32 b_index = font_get_char(font, b);
    result += font->kerning[a_index*font->codepoint_count + b_index];
  }
  
  return result;
}


#define LVL5_FILES
#endif