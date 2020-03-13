#include "font.h"

Bitmap make_empty_bitmap(i32 width, i32 height) {
  Bitmap result;
  result.width = width;
  result.height = height;
  
  Mem_Size size = align_pow_2(width*height*sizeof(u32), 16);
  result.data = (u32 *)alloc(size);
  zero_memory_fast(result.data, size);
  return result;
}


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


Font load_font(String file_name, String font_name_str, i32 font_size) {
  Font font = {0};
  
  {
    HDC device_context = CreateCompatibleDC(GetDC(null));
    char *font_name = to_c_string(font_name_str);
    b32 font_added = AddFontResourceExA(to_c_string(file_name), FR_PRIVATE, 0);
    assert(font_added);
    
    HFONT win_font = CreateFontA(
      font_size, 0,
      0, 0,
      FW_NORMAL, //weight
      FALSE, //italic
      FALSE, //underline
      FALSE, // strikeout
      DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS,
      CLIP_DEFAULT_PRECIS,
      ANTIALIASED_QUALITY,
      DEFAULT_PITCH|FF_DONTCARE,
      font_name);
    
    u32 *font_buffer_pixels = null;
    
    i32 font_buffer_width = 256;
    i32 font_buffer_height = 256;
    
    BITMAPINFO font_buffer_bi = {0};
    font_buffer_bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    font_buffer_bi.bmiHeader.biWidth = font_buffer_width;
    font_buffer_bi.bmiHeader.biHeight = font_buffer_height;
    font_buffer_bi.bmiHeader.biPlanes = 1;
    font_buffer_bi.bmiHeader.biBitCount = 32;
    font_buffer_bi.bmiHeader.biCompression = BI_RGB;
    font_buffer_bi.bmiHeader.biSizeImage = 0;
    font_buffer_bi.bmiHeader.biXPelsPerMeter = 0;
    font_buffer_bi.bmiHeader.biYPelsPerMeter = 0;
    font_buffer_bi.bmiHeader.biClrUsed = 0;
    font_buffer_bi.bmiHeader.biClrImportant = 9;
    
    HBITMAP font_buffer = CreateDIBSection(
      device_context,
      &font_buffer_bi,
      DIB_RGB_COLORS,
      (void **)&font_buffer_pixels,
      0, 0);
    SelectObject(device_context, font_buffer);
    SelectObject(device_context, win_font);
    
    
    b32 metrics_bytes = GetOutlineTextMetrics(device_context, 0, null);
    OUTLINETEXTMETRIC *metric = (OUTLINETEXTMETRIC *)scratch_push_array(byte, metrics_bytes);
    GetOutlineTextMetrics(device_context, metrics_bytes, metric);
    
    
    
    SetBkColor(device_context, RGB(0, 0, 0));
    
    char first_codepoint = ' ';
    char last_codepoint = '~';
    
    i32 codepoint_count = last_codepoint - first_codepoint + 1;
    Bitmap *codepoint_bitmaps = scratch_push_array(Bitmap, codepoint_count + 1);
    // 1 extra bitmap for white pixel
    
    font = (Font){
      .first_codepoint = first_codepoint,
      .advance = alloc_array(i8, codepoint_count),
      .kerning = alloc_array(i8, codepoint_count*codepoint_count),
      .origins = alloc_array(V2, codepoint_count),
      .codepoint_count = codepoint_count,
    };
    zero_memory_slow_(font.advance, sizeof(i8)*codepoint_count);
    zero_memory_slow_(font.kerning, sizeof(i8)*codepoint_count*codepoint_count);
    
    ABC *abcs = scratch_push_array(ABC, font.codepoint_count);
    GetCharABCWidthsW(device_context, font.first_codepoint,
                      font.first_codepoint + codepoint_count, abcs);
    
    for (char codepoint_index = 0;
         codepoint_index < codepoint_count;
         codepoint_index++)
    {
      wchar_t codepoint = first_codepoint + codepoint_index;
      
      b32 bg_blitted = PatBlt(device_context, 
                              0, 0, font_buffer_width, font_buffer_height, BLACKNESS);
      assert(bg_blitted);
      SetTextColor(device_context, RGB(255, 255, 255));
      b32 written_text = TextOutW(device_context, 0, 0, &codepoint, 1);
      
      i32 min_x = 10000;
      i32 min_y = 10000;
      i32 max_x = -10000;
      i32 max_y = -10000;
      
      if (codepoint == ' ') {
        min_x = 0;
        min_y = 0;
        max_x = 20;
        max_y = 20;
      } else {
        u32 *pixel = font_buffer_pixels;
        for (i32 y = 0; y < font_buffer_height; y++) {
          for (i32 x = 0; x < font_buffer_width; x++) {
            u32 color_ref = *(pixel++);
            if (color_ref != 0) {
              if (x < min_x) min_x = x;
              if (x > max_x) max_x = x;
              if (y < min_y) min_y = y;
              if (y > max_y) max_y = y;
            }
          }
        }
        min_x--;
        min_y--;
        max_x++;
        max_y++;
      }
      
      
      Bitmap bmp = make_empty_bitmap(max_x - min_x, max_y - min_y);
      
      for (i32 y = 0; y < bmp.height; y++) {
        for (i32 x = 0; x < bmp.width; x++) {
          u32 src_pixel = font_buffer_pixels[(min_y + y)*font_buffer_width + min_x + x];
          u8 intensity = (u8)((src_pixel & 0x00FF0000) >> 16);
          u32 new_pixel = color_u32(0xFF, 0xFF, 0xFF, intensity);
          bmp.data[y*bmp.width + x] = new_pixel;
        }
      }
      
      codepoint_bitmaps[codepoint_index] = bmp;
      
      ABC abc = abcs[codepoint_index];
      
      i8 total_width = (i8)(abc.abcA + abc.abcB + abc.abcC);
      font.advance[codepoint_index] = total_width;
      font.origins[codepoint_index] = v2((f32)min_x, (f32)min_y - font_buffer_height);
    }
    
    Bitmap white_bitmap = make_empty_bitmap(2, 2);
    white_bitmap.data[0] = 0xFFFFFFFF;
    white_bitmap.data[1] = 0xFFFFFFFF;
    white_bitmap.data[2] = 0xFFFFFFFF;
    white_bitmap.data[3] = 0xFFFFFFFF;
    codepoint_bitmaps[codepoint_count] = white_bitmap;
    
    font.line_spacing = (i8)(metric->otmLineGap + metric->otmAscent - metric->otmDescent);
    font.atlas = texture_atlas_make_from_bitmaps(codepoint_bitmaps,
                                                 codepoint_count+1,
                                                 512);
    
    bmp_save("font.bmp", font.atlas.bmp);
    
    DWORD kerning_pair_count = GetKerningPairs(device_context, I32_MAX, null);
    KERNINGPAIR *kerning_pairs = scratch_push_array(KERNINGPAIR, 
                                                    kerning_pair_count);
    GetKerningPairs(device_context, kerning_pair_count, kerning_pairs);
    for (DWORD i = 0; i < kerning_pair_count; i++) {
      KERNINGPAIR pair = kerning_pairs[i];
      assert(pair.wFirst >= font.first_codepoint);
      
      char first = (char)pair.wFirst - font.first_codepoint;
      char second = (char)pair.wSecond - font.first_codepoint;
      assert(pair.iKernAmount < I8_MAX);
      assert(pair.iKernAmount > I8_MIN);
      font.kerning[first*codepoint_count + second] += (i8)pair.iKernAmount;
    }
  }
  
  return font;
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