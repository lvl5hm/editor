#ifndef LVL5_STRING
#define LVL5_STRING_VERSION 0

#include "lvl5_types.h"
#include "lvl5_arena.h"
#include "lvl5_context.h"

typedef struct {
  char *data;
  u64 count;
} String;

#define const_string(const_char) make_string(const_char, array_count(const_char)-1)

String make_string(char *data, u64 count) {
  String result;
  result.data = data;
  result.count = count;
  return result;
}

String alloc_string(char *data, u64 count) {
  String result;
  result.data = alloc_array(char, count);
  copy_memory_slow(result.data, data, count);
  result.count = count;
  return result;
}

i64 find_last_index(String str, String substr, i64 start_index) {
  for (u64 i = start_index - substr.count - 1; i >= 0; i--)
  {
    u64 srcIndex = i;
    u64 testIndex = 0;
    while (str.data[srcIndex] == substr.data[testIndex])
    {
      srcIndex++;
      testIndex++;
      if (testIndex == substr.count)
      {
        return i;
      }
    }
  }
  return -1;
}

i32 find_index(String str, String substr, i32 start_index) {
  for (u32 i = start_index; i < str.count - substr.count; i++)
  {
    u32 srcIndex = i;
    u32 testIndex = 0;
    while (str.data[srcIndex] == substr.data[testIndex])
    {
      srcIndex++;
      testIndex++;
      if (testIndex == substr.count)
      {
        return i;
      }
    }
  }
  return -1;
}

b32 starts_with(String str, String substr) {
  b32 result = true;
  for (u32 i = 0; i < substr.count; i++) {
    if (str.data[i] != substr.data[i]) {
      result = false;
      break;
    }
  }
  return result;
}

String substring(String s, u64 begin, u64 end) {
  String result;
  result.data = s.data + begin;
  result.count = end - begin;
  return result;
}

String concat(String a, String b) {
  String result;
  result.data = (char *)scratch_alloc(sizeof(char)*(a.count + b.count));
  result.count = a.count + b.count;
  copy_memory_slow(result.data, a.data, a.count);
  copy_memory_slow(result.data + a.count, b.data, b.count);
  return result;
}

char *to_c_string(String a) {
  char *result = (char *)scratch_alloc(sizeof(char)*(a.count + 1));
  copy_memory_slow(result, a.data, a.count);
  result[a.count] = '\0';
  return result;
}

// zero terminator does NOT count!
i32 c_string_length(char *str) {
  i32 result = 0;
  while (*str++) result++;
  return result;
}

String from_c_string(char *c_string) {
  String result;
  result.data = c_string;
  result.count = c_string_length(c_string);
  return result;
}

b32 c_string_compare(char *a, char *b) {
  while (*a) {
    if (*a++ != *b++) {
      return false; 
    }
  }
  if (!*b) return true;
  return false;
}

b32 string_compare(String a, String b) {
  if (a.count != b.count) {
    return false;
  }
  for (u32 i = 0; i < a.count; i++) {
    if (a.data[i] != b.data[i]) {
      return false;
    }
  }
  return true;
}


#include "lvl5_math.h"
#include "stdarg.h"


i32 string_to_i32(String str) {
  i32 result = 0;
  for (u32 i = 0; i < str.count; i++) {
    i32 power = (i32)(str.count - i - 1);
    i32 a = (i32)(str.data[i] - '0');
    result += a*pow_i32(10, power);
  }
  return result;
}

String i32_to_string(Arena *arena, i32 num) {
  u64 LENGTH = 10;
  char *str = arena_push_array(arena, char, LENGTH + 1);
  String result = {0};
  
  i32 n = num;
  
  if (n == 0) {
    str[LENGTH - result.count] = '0';
    result.count = 1;
  } else {
    if (n < 0) {
      n *= -1;
    }
    
    while (n != 0) {
      str[LENGTH - result.count] = '0' + (n % 10);
      n /= 10;
      result.count++;
    }
    
    if (num < 0) {
      str[LENGTH - result.count] = '-';
      result.count++;
    }
  }
  
  result.data = str + LENGTH+1 - result.count;
  return result;
}


#if 0
String i64_to_string(Arena *arena, i64 num) {
  u64 LENGTH = 20;
  char *str = arena_push_array(arena, char, LENGTH + 1);
  String result = {0};
  
  i64 n = num;
  
  if (n == 0) {
    str[LENGTH - result.count] = '0';
    result.count = 1;
  } else {
    if (n < 0) {
      n *= -1;
    }
    
    while (n != 0) {
      str[LENGTH - result.count] = '0' + (n % 10);
      n /= 10;
      result.count++;
    }
    
    if (num < 0) {
      str[LENGTH - result.count] = '-';
      result.count++;
    }
  }
  
  result.data = str + LENGTH+1 - result.count;
  return result;
}


String f32_to_string(f32 num, i32 decimal_count) {
  i32 int_part = (i32)num;
  f32 float_part = num - int_part;
  
  String int_string = i64_to_string(int_part);
  
  i32 leading_zeros = 0;
  while (true) {
    char first_digit = (char)(float_part*10);
    if (first_digit == 0) {
      leading_zeros++;
      float_part *= 10;
    } else {
      break;
    }
  }
  
  char *str = arena_push_array(arena, char, 20);
  str[0] = '.';
  for (i32 i = 0; i < leading_zeros; i++) {
    str[i+1] = '0';
  }
  String zero_string = alloc_string(str, leading_zeros+1);
  
  float_part *= 10*(decimal_count - leading_zeros);
  String float_string = i64_to_string(arena, (i64)float_part);
  
  String result = concat(int_string, concat(zero_string, float_string));
  return result;
}

#endif


#if 0
String arena_sprintf(Arena *arena, String fmt, ...) {
  String result = {0};
  
  va_list args;
  va_start(args, fmt);
  
  
  for (u32 i = 0; i < fmt.count; i++) {
    if (fmt.data[i] == '%') {
      if (fmt.data[i+1] == 'i') {
        i64 num = va_arg(args, i64);
        String str = i64_to_string(arena, num);
        result = concat(arena, result, str);
        i += 1;
      } else if (fmt.data[i+1] == 's') {
        String str = va_arg(args, String);
        result = concat(arena, result, str);
        i += 1;
      } else if (fmt.data[i+1] == '.') {
        byte decimal_count = fmt.data[i+2] - '0';
        assert(fmt.data[i+3] == 'f');
        f32 num = va_arg(args, f32);
        String str = f32_to_string(arena, num, decimal_count);
        result = concat(arena, result, str);
        i += 3;
      } else {
        result = concat(arena, result, make_string(&fmt.data[i], 1));
      }
    } else {
      result = concat(arena, result, make_string(&fmt.data[i], 1));
    }
  }
  
  va_end(args);
  
  return result;
}
#endif


#define LVL5_STRING
#endif