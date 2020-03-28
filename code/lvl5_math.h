#ifndef LVL5_MATH

#include "lvl5_types.h"
#include <math.h>


#define PI 3.14159265359f

#define U8_MIN 0x0
#define U16_MIN 0x0
#define U32_MIN 0x0
#define U64_MIN 0x0
#define U8_MAX 0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF

#define I8_MAX 0x7F
#define I16_MAX 0x7FFF
#define I32_MAX 0x7FFFFFFF
#define I64_MAX 0x7FFFFFFFFFFFFFFF
#define I8_MIN -I8_MAX-1
#define I16_MIN -I16_MAX-1
#define I32_MIN -I32_MAX-1
#define I64_MIN -I64_MAX-1

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) > (b) ? (b) : (a))

i32 max_i32(i32 a, i32 max) {
  i32 result = a > max ? a : max;
  return result;
}

f32 max_f32(f32 a, f32 max) {
  f32 result = a > max ? a : max;
  return result;
}

u32 max_u32(u32 a, u32 max) {
  u32 result = a > max ? a : max;
  return result;
}



i32 min_i32(i32 a, i32 min) {
  i32 result = a < min ? a : min;
  return result;
}

f32 min_f32(f32 a, f32 min) {
  f32 result = a < min ? a : min;
  return result;
}

u32 min_u32(u32 a, u32 min) {
  u32 result = a < min ? a : min;
  return result;
}



i32 clamp_i32(i32 a, i32 min, i32 max) {
  i32 result = max_i32(min_i32(a, max), min);
  return result;
}

f32 clamp_f32(f32 a, f32 min, f32 max) {
  f32 result = max_f32(min_f32(a, max), min);
  return result;
}

i32 pow_i32(i32 a, i32 n) {
  assert(n >= 0);
  i32 result = 1;
  while (n) {
    result *= a;
    n--;
  }
  return result;
}

f32 sin_f32(f32 x) {
  f32 result = sinf(x*2*PI);
  return result;
}

f32 cos_f32(f32 a) {
  f32 result = cosf(a*2*PI);
  return result;
}

f32 tan_f32(f32 a) {
  f32 result = tanf(a*2*PI);
  return result;
}

f32 atan2_f32(f32 y, f32 x) {
  f32 result = atan2f(y, x)/(2*PI);
  return result;
}

f32 sqrt_f32(f32 a) {
  f32 result = sqrtf(a);
  return result;
}

f32 sqr_f32(f32 a) {
  f32 result = a*a;
  return result;
}

i16 abs_i16(i16 a) {
  i16 result = a > 0 ? a : -a;
  return result;
}

f32 sign_f32(f32 a) {
  f32 result = 0;
  if (a > 0) 
    result = 1;
  else if (a < 0) 
    result = -1;
  return result;
}

i32 sign_i32(i32 a) {
  i32 result = 0;
  if (a > 0) result = 1;
  if (a < 0) result = -1;
  return result;
}

f32 abs_f32(f32 a) {
  f32 result = a > 0 ? a : -a;
  return result;
}

i32 round_f32_i32(f32 a) {
  i32 result = (i32)roundf(a);
  return result;
}

i32 floor_f32_i32(f32 a) {
  i32 result = (i32)floorf(a);
  return result;
}

f32 round_f32(f32 a) {
  f32 result = roundf(a);
  return result;
}

f32 ceil_f32(f32 a) {
  f32 result = ceilf(a);
  return result;
}

i32 ceil_f32_i32(f32 a) {
  i32 result = (i32)ceilf(a);
  return result;
}

i16 round_f32_i16(f32 a) {
  i16 result = (i16)roundf(a);
  return result;
}



// v2

typedef struct {
  f32 x;
  f32 y;
} V2;

V2 v2(f32 x, f32 y) {
  V2 result;
  result.x = x;
  result.y = y;
  return result;
}

V2 v2_add(V2 a, V2 b) {
  V2 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

V2 v2_sub(V2 a, V2 b) {
  V2 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

V2 v2_invert(V2 a) {
  V2 result;
  result.x = 1.0f/a.x;
  result.y = 1.0f/a.y;
  return result;
}

f32 v2_dot(V2 a, V2 b) {
  f32 result = a.x*b.x + a.y*b.y;
  return result;
}

V2 v2_hadamard(V2 a, V2 b) {
  V2 result;
  result.x = a.x*b.x;
  result.y = a.y*b.y;
  return result;
}

V2 v2_mul(V2 a, f32 s) {
  V2 result;
  result.x = a.x*s;
  result.y = a.y*s;
  return result;
}

V2 v2_div(V2 a, f32 s) {
  V2 result;
  result.x = a.x/s;
  result.y = a.y/s;
  return result;
}

V2 v2_i(i32 x, i32 y) {
  V2 result;
  result.x = (f32)x;
  result.y = (f32)y;
  return result;
}

V2 v2_perp(V2 a) {
  V2 result = v2(-a.y, a.x);
  return result;
}

f32 v2_length_sqr(V2 a) {
  f32 result = sqr_f32(a.x) + sqr_f32(a.y);
  return result;
}

f32 v2_length(V2 a) {
  f32 result = sqrt_f32(v2_length_sqr(a));
  return result;
}

V2 v2_unit(V2 a) {
  V2 result = v2_div(a, v2_length(a));
  return result;
}

f32 v2_project_s(V2 a, V2 b) {
  f32 len = v2_length(b);
  assert(len);
  f32 result = v2_dot(a, b)/len;
  return result;
}

V2 v2_project(V2 a, V2 b) {
  f32 len_sqr = v2_length_sqr(b);
  assert(len_sqr);
  V2 result = v2_mul(b, v2_dot(a, b)/len_sqr);
  return result;
}

V2 v2_rotate(V2 a, f32 angle) {
  V2 result;
  result.x = a.x*cos_f32(-angle) + a.y*sin_f32(-angle);
  result.y = -a.x*sin_f32(-angle) + a.y*cos_f32(-angle);
  return result;
}


V2 v2_negate(V2 a) {
  V2 result = v2_mul(a, -1);
  return result;
}

V2 v2_perp_direction(V2 a, V2 b) {
  V2 result = v2_perp(a);
  if (v2_dot(result, b) < 0) {
    result = v2_negate(result);
  }
  return result;
}

V2 v2_zero() {return v2(0, 0);}
V2 v2_right() {return v2(1, 0);}
V2 v2_left() {return v2(-1, 0);}
V2 v2_up() {return v2(0, 1);}
V2 v2_down() {return v2(0, -1);}


typedef struct {
  f32 min;
  f32 max;
} Range;

Range inverted_infinity_range() {
  Range result;
  result.min = INFINITY;
  result.max = -INFINITY;
  return result;
}

// v3

typedef union {
  struct {
    f32 x;
    f32 y;
    f32 z;
  };
  struct {
    f32 r;
    f32 g;
    f32 b;
  };
  struct {
    V2 xy;
  };
} V3;


V3 v3(f32 x, f32 y, f32 z) {
  V3 result;
  result.x = x;
  result.y = y;
  result.z = z;
  return result;
}

V3 v3_add(V3 a, V3 b) {
  V3 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  return result;
}

V3 v3_sub(V3 a, V3 b) {
  V3 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  return result;
}

V3 v3_invert(V3 a) {
  V3 result;
  result.x = 1.0f/a.x;
  result.y = 1.0f/a.y;
  result.z = 1.0f/a.z;
  return result;
}

f32 v3_dot(V3 a, V3 b) {
  f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
  return result;
}

V3 v3_hadamard(V3 a, V3 b) {
  V3 result;
  result.x = a.x*b.x;
  result.y = a.y*b.y;
  result.z = a.z*b.z;
  return result;
}

V3 v3_mul(V3 a, f32 s) {
  V3 result;
  result.x = a.x*s;
  result.y = a.y*s;
  result.z = a.z*s;
  return result;
}

V3 v3_div(V3 a, f32 s) {
  V3 result;
  result.x = a.x/s;
  result.y = a.y/s;
  result.z = a.z/s;
  return result;
}

V3 v2_to_v3(V2 a, f32 z) {
  V3 result;
  result.x = a.x;
  result.y = a.y;
  result.z = z;
  return result;
}


V3 v3_negate(V3 a) {
  V3 result;
  result.x = -a.x;
  result.y = -a.y;
  result.z = -a.z;
  return result;
}

f32 v3_length_sqr(V3 a) {
  f32 result = sqr_f32(a.x) + sqr_f32(a.y) + sqr_f32(a.z);
  return result;
}

f32 v3_length(V3 a) {
  f32 result = sqrt_f32(v3_length_sqr(a));
  return result;
}

V3 v3_unit(V3 a) {
  f32 length = v3_length(a);
  assert(length);
  V3 result = v3_div(a, length);
  return result;
}

V3 v3_cross(V3 a, V3 b) {
  V3 result;
  result.x = a.y*b.z - a.z*b.y;
  result.y = a.z*b.x - a.x*b.z;
  result.z = a.x*b.y - a.y*b.x;
  return result;
}


V3 v3_zero() {return v3(0, 0, 0);}
V3 v3_right() {return v3(1, 0, 0);}
V3 v3_left() {return v3(-1, 0, 0);}
V3 v3_up() {return v3(0, 1, 0);}
V3 v3_down() {return v3(0, -1, 0);}
V3 v3_forward() {return v3(0, 0, 1);}
V3 v3_backward() {return v3(0, 0, -1);}


// V4
typedef union {
  struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
  };
  struct {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
  };
  struct {
    V2 xy;
    V2 zw;
  };
  struct {
    V3 xyz;
  };
  struct {
    V3 rgb;
  };
  struct {
    f32 inner;
    V3 outer;
  };
} V4;

V4 v4(f32 x, f32 y, f32 z, f32 w) {
  V4 result;
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
  return result;
}

V4 v4_add(V4 a, V4 b) {
  V4 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  result.w = a.w + b.w;
  return result;
}

V4 v4_sub(V4 a, V4 b) {
  V4 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  result.w = a.w - b.w;
  return result;
}

V4 v4_hadamard(V4 a, V4 b) {
  V4 result;
  result.x = a.x*b.x;
  result.y = a.y*b.y;
  result.z = a.z*b.z;
  result.w = a.w*b.w;
  return result;
}

V4 v4_mul(V4 a, f32 s) {
  V4 result;
  result.x = a.x*s;
  result.y = a.y*s;
  result.z = a.z*s;
  result.w = a.w*s;
  return result;
}

f32 v4_dot(V4 a, V4 b) {
  f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
  return result;
}

V4 v4_negate(V4 a) {
  V4 result = v4_mul(a, -1);
  return result;
}

V4 v4_div(V4 a, f32 s) {
  V4 result;
  result.x = a.x/s;
  result.y = a.y/s;
  result.z = a.z/s;
  result.w = a.w/s;
  return result;
}

V4 v2_to_v4(V2 v, f32 z, f32 w) {
  V4 result;
  result.xy = v;
  result.z = z;
  result.w = w;
  return result;
}

// v2i

typedef struct {
  i32 x;
  i32 y;
} V2i;

V2i v2i(i32 x, i32 y) {
  V2i result;
  result.x = x;
  result.y = y;
  return result;
}

V2i v2_to_v2i(V2 v) {
  V2i result;
  result.x = (i32)v.x;
  result.y = (i32)v.y;
  return result;
}

V2 v2i_to_v2(V2i v) {
  V2 result;
  result.x = (f32)v.x;
  result.y = (f32)v.y;
  return result;
}

V2i v2i_add(V2i a, V2i b) {
  V2i result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

V2i v2i_sub(V2i a, V2i b) {
  V2i result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

V2i v2i_mul(V2i a, i32 s) {
  V2i result;
  result.x = a.x*s;
  result.y = a.y*s;
  return result;
}

V2i v2i_div(V2i a, i32 s) {
  V2i result;
  result.x = a.x/s;
  result.y = a.y/s;
  return result;
}

V2i v2i_hadamard(V2i a, V2i b) {
  V2i result;
  result.x = a.x*b.x;
  result.y = a.y*b.y;
  return result;
}


// mat2

typedef union {
  f32 e[4];
  struct {
    f32 e00; f32 e10;
    f32 e01; f32 e11;
  };
} M2;

M2 m2(f32 e00, f32 e10,
      f32 e01, f32 e11) {
  M2 result;
  result.e00 = e00;
  result.e10 = e10;
  result.e01 = e01;
  result.e11 = e11;
  return result;
}

M2 m2_mul_m2(M2 a, M2 b) {
  M2 result;
  result.e00 = a.e00*b.e00 + a.e10*b.e01;
  result.e10 = a.e00*b.e10 + a.e10*b.e11;
  result.e01 = a.e01*b.e00 + a.e11*b.e01;
  result.e11 = a.e01*b.e10 + a.e11*b.e11;
  return result;
}

V2 m2_mul_v2(M2 m, V2 v) {
  V2 result;
  result.x = v.x*m.e00 + v.x*m.e10;
  result.y = v.y*m.e01 + v.y*m.e11;
  return result;
}

f32 m2_det(M2 m) {
  f32 result = m.e00*m.e11 - m.e10*m.e01;
  return result;
}

// mat3
typedef union {
  f32 e[9];
  struct {
    f32 e00; f32 e10; f32 e20;
    f32 e01; f32 e11; f32 e21;
    f32 e02; f32 e12; f32 e22;
  };
} M3;

M3 m3(f32 e00, f32 e10, f32 e20,
      f32 e01, f32 e11, f32 e21,
      f32 e02, f32 e12, f32 e22) {
  M3 result;
  result.e00 = e00;
  result.e10 = e10;
  result.e20 = e20;
  result.e01 = e01;
  result.e11 = e11;
  result.e21 = e21;
  result.e02 = e02;
  result.e12 = e12;
  result.e22 = e22;
  return result;
}

f32 M3_det(M3 m) {
  f32 result = m.e00*m.e11*m.e22 + m.e10*m.e21*m.e02 + m.e20*m.e01*m.e12 -
    m.e20*m.e11*m.e02 - m.e10*m.e01*m.e22 - m.e00*m.e21*m.e12;
  return result;
}

M3 m3_mul_3(M3 a, M3 b) {
  M3 result;
  result.e00 = a.e00*b.e00 + a.e10*b.e01 + a.e20*b.e02;
  result.e10 = a.e00*b.e10 + a.e10*b.e11 + a.e20*b.e12;
  result.e20 = a.e00*b.e20 + a.e10*b.e21 + a.e20*b.e22;
  
  result.e01 = a.e01*b.e00 + a.e11*b.e01 + a.e21*b.e02;
  result.e11 = a.e01*b.e10 + a.e11*b.e11 + a.e21*b.e12;
  result.e21 = a.e01*b.e20 + a.e11*b.e21 + a.e21*b.e22;
  
  result.e02 = a.e02*b.e00 + a.e12*b.e01 + a.e22*b.e02;
  result.e12 = a.e02*b.e10 + a.e12*b.e11 + a.e22*b.e12;
  result.e22 = a.e02*b.e20 + a.e12*b.e21 + a.e22*b.e22;
  return result;
}


// mat4
typedef union {
  f32 e[16];
  struct {
    f32 e00; f32 e10; f32 e20; f32 e30;
    f32 e01; f32 e11; f32 e21; f32 e31;
    f32 e02; f32 e12; f32 e22; f32 e32;
    f32 e03; f32 e13; f32 e23; f32 e33;
  };
} M4;

M4 m4(f32 e00, f32 e10, f32 e20, f32 e30,
      f32 e01, f32 e11, f32 e21, f32 e31,
      f32 e02, f32 e12, f32 e22, f32 e32,
      f32 e03, f32 e13, f32 e23, f32 e33) {
  M4 result;
  result.e00 = e00;
  result.e10 = e10;
  result.e20 = e20;
  result.e30 = e30,
  
  result.e01 = e01;
  result.e11 = e11;
  result.e21 = e21;
  result.e31 = e31;
  
  result.e02 = e02;
  result.e12 = e12;
  result.e22 = e22;
  result.e32 = e32;
  
  result.e03 = e03;
  result.e13 = e13;
  result.e23 = e23;
  result.e33 = e33;
  
  return result;
}

M2 m4_to_m2(M4 m) {
  M2 result;
  result.e00 = m.e00;
  result.e01 = m.e01;
  result.e10 = m.e10;
  result.e11 = m.e11;
  return result;
}

V4 m4_mul_v4(M4 m, V4 v) {
  V4 result;
  result.x = v.x*m.e00 + v.y*m.e10 + v.z*m.e20 + v.w*m.e30;
  result.y = v.x*m.e01 + v.y*m.e11 + v.z*m.e21 + v.w*m.e31;
  result.z = v.x*m.e02 + v.y*m.e12 + v.z*m.e22 + v.w*m.e32;
  result.w = v.x*m.e03 + v.y*m.e13 + v.z*m.e23 + v.w*m.e33;
  return result;
}


M4 m4_mul_m4(M4 a, M4 b) {
  M4 result;
  
  result.e00 = a.e00*b.e00 + a.e10*b.e01 + a.e20*b.e02 + a.e30*b.e03;
  result.e10 = a.e00*b.e10 + a.e10*b.e11 + a.e20*b.e12 + a.e30*b.e13;
  result.e20 = a.e00*b.e20 + a.e10*b.e21 + a.e20*b.e22 + a.e30*b.e23;
  result.e30 = a.e00*b.e30 + a.e10*b.e31 + a.e20*b.e32 + a.e30*b.e33;
  
  result.e01 = a.e01*b.e00 + a.e11*b.e01 + a.e21*b.e02 + a.e31*b.e03;
  result.e11 = a.e01*b.e10 + a.e11*b.e11 + a.e21*b.e12 + a.e31*b.e13;
  result.e21 = a.e01*b.e20 + a.e11*b.e21 + a.e21*b.e22 + a.e31*b.e23;
  result.e31 = a.e01*b.e30 + a.e11*b.e31 + a.e21*b.e32 + a.e31*b.e33;
  
  result.e02 = a.e02*b.e00 + a.e12*b.e01 + a.e22*b.e02 + a.e32*b.e03;
  result.e12 = a.e02*b.e10 + a.e12*b.e11 + a.e22*b.e12 + a.e32*b.e13;
  result.e22 = a.e02*b.e20 + a.e12*b.e21 + a.e22*b.e22 + a.e32*b.e23;
  result.e32 = a.e02*b.e30 + a.e12*b.e31 + a.e22*b.e32 + a.e32*b.e33;
  
  result.e03 = a.e03*b.e00 + a.e13*b.e01 + a.e23*b.e02 + a.e33*b.e03;
  result.e13 = a.e03*b.e10 + a.e13*b.e11 + a.e23*b.e12 + a.e33*b.e13;
  result.e23 = a.e03*b.e20 + a.e13*b.e21 + a.e23*b.e22 + a.e33*b.e23;
  result.e33 = a.e03*b.e30 + a.e13*b.e31 + a.e23*b.e32 + a.e33*b.e33;
  
  return result;
}

M4 m4_identity() {
  M4 result = m4(1, 0, 0, 0,
                 0, 1, 0, 0,
                 0, 0, 1, 0,
                 0, 0, 0, 1);
  return result;
}

V4 v3_to_v4(V3 a, f32 w) {
  V4 result;
  result.xyz = a;
  result.w = w;
  return result;
}

V3 v3_outer(V3 a, V3 b) {
  V3 result;
  result.x = a.x*b.y - b.x*a.y;
  result.y = a.x*b.z - b.x*a.z;
  result.z = a.y*b.z - b.y*a.z;
  return result;
}


V4 v3_geometric(V3 a, V3 b) {
  V4 result;
  result.inner = v3_dot(a, b);
  result.outer = v3_outer(a, b);
  
  return result;
}

M4 m4_scaled(V3 scale) {
  M4 scale_m = m4(scale.x, 0,         0,         0,
                  0,       scale.y,   0,         0,
                  0,       0,         scale.z,   0,
                  0,       0,         0,         1.0f);
  
  return scale_m;
}

M4 m4_scale(M4 m, V3 scale) {
  M4 result = m4_mul_m4(m4_scaled(scale), m);
  return result;
}

M4 m4_rotated(f32 angle) {
  f32 cos = cos_f32(-angle);
  f32 sin = sin_f32(-angle);
  M4 rotate_m = m4(cos,  sin,  0,    0,
                   -sin, cos,  0,    0,
                   0,    0,    1.0f, 0,
                   0,    0,    0,    1.0f);
  return rotate_m;
}

M4 m4_rotate(M4 m, f32 angle) {
  M4 result = m4_mul_m4(m4_rotated(angle), m);
  return result;
}

M4 m4_translated(V3 p) {
  M4 translate_m = m4(1.0f, 0,    0,    p.x,
                      0,    1.0f, 0,    p.y,
                      0,    0,    1.0f, p.z,
                      0,    0,    0,    1.0f);
  return translate_m;
}

M4 m4_translate(M4 m, V3 p) {
  M4 result = m4_mul_m4(m4_translated(p), m);
  return result;
}



M4 m4_untranslate(M4 a, V3 p) {
  M4 result = a;
  
  result.e30 = a.e00*p.x + a.e10*p.y + a.e20*p.z + a.e30;
  result.e31 = a.e01*p.x + a.e11*p.y + a.e21*p.z + a.e31;
  result.e32 = a.e02*p.x + a.e12*p.y + a.e22*p.z + a.e32;
  result.e33 = a.e03*p.x + a.e13*p.y + a.e23*p.z + a.e33;
  return result;
}

M4 m4_unscale(M4 a, V3 scale) {
  M4 result = a;
  
  result.e00 = a.e00*scale.x;
  result.e10 = a.e10*scale.y;
  result.e20 = a.e20*scale.z;
  
  result.e01 = a.e01*scale.x;
  result.e11 = a.e11*scale.y;
  result.e21 = a.e21*scale.z;
  
  result.e02 = a.e02*scale.x;
  result.e12 = a.e12*scale.y;
  result.e22 = a.e22*scale.z;
  
  result.e03 = a.e03*scale.x;
  result.e13 = a.e13*scale.y;
  result.e23 = a.e23*scale.z;
  
  return result;
}

M4 m4_unrotate(M4 a, f32 angle) {
  M4 result = a;
  f32 cos = cos_f32(-angle);
  f32 sin = sin_f32(-angle);
  
  result.e00 = a.e00*cos + a.e10*-sin;
  result.e10 = a.e00*sin + a.e10*cos;
  
  result.e01 = a.e01*cos + a.e11*-sin;
  result.e11 = a.e01*sin + a.e11*cos;
  
  result.e02 = a.e02*cos + a.e12*-sin;
  result.e12 = a.e02*sin + a.e12*cos;
  
  result.e03 = a.e03*cos + a.e13*-sin;
  result.e13 = a.e03*sin + a.e13*cos;
  
  return result;
}


M4 m4_transpose(M4 m) {
  M4 result;
  for (i32 y = 0; y < 4; y++) {
    for (i32 x = 0; x < 4; x++) {
      result.e[y*4 + x] = m.e[x*4 + y];
    }
  }
  return result;
}

M4 m4_orthographic(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
  M4 result = m4(2/(r-l),      0,            0,             0,
                 0,            2/(t-b),      0,             0,
                 0,            0,            -2/(f-n),      0,
                 -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n),  1);
  return result;
}

M4 M4_perspective(f32 fov_angle, f32 f, f32 n) {
  f32 v = 1.0f/(n*tan_f32(fov_angle*0.5f));
  f32 a = f/(f - n);
  f32 b = -n*a;
  M4 result = m4(v, 0, 0,  0,
                 0, v, 0,  0,
                 0, 0, a, -1,
                 0, 0, b,  0);
  return result;
}

M4 m4_inverse(M4 m) {
  f32 A2323 = m.e22 * m.e33 - m.e23 * m.e32;
  f32 A1323 = m.e21 * m.e33 - m.e23 * m.e31;
  f32 A1223 = m.e21 * m.e32 - m.e22 * m.e31;
  f32 A0323 = m.e20 * m.e33 - m.e23 * m.e30;
  f32 A0223 = m.e20 * m.e32 - m.e22 * m.e30;
  f32 A0123 = m.e20 * m.e31 - m.e21 * m.e30;
  f32 A2313 = m.e12 * m.e33 - m.e13 * m.e32;
  f32 A1313 = m.e11 * m.e33 - m.e13 * m.e31;
  f32 A1213 = m.e11 * m.e32 - m.e12 * m.e31;
  f32 A2312 = m.e12 * m.e23 - m.e13 * m.e22;
  f32 A1312 = m.e11 * m.e23 - m.e13 * m.e21;
  f32 A1212 = m.e11 * m.e22 - m.e12 * m.e21;
  f32 A0313 = m.e10 * m.e33 - m.e13 * m.e30;
  f32 A0213 = m.e10 * m.e32 - m.e12 * m.e30;
  f32 A0312 = m.e10 * m.e23 - m.e13 * m.e20;
  f32 A0212 = m.e10 * m.e22 - m.e12 * m.e20;
  f32 A0113 = m.e10 * m.e31 - m.e11 * m.e30;
  f32 A0112 = m.e10 * m.e21 - m.e11 * m.e20;
  
  f32 det = 1 / (m.e00 * (m.e11 * A2323 - m.e12 * A1323 + m.e13 * A1223)
                 - m.e01 * (m.e10 * A2323 - m.e12 * A0323 + m.e13 * A0223)
                 + m.e02 * (m.e10 * A1323 - m.e11 * A0323 + m.e13 * A0123)
                 - m.e03 * (m.e10 * A1223 - m.e11 * A0223 + m.e12 * A0123));
  
  M4 result = m4(det * (m.e11 * A2323 - m.e12 * A1323 + m.e13 * A1223),
                 det * -(m.e01 * A2323 - m.e02 * A1323 + m.e03 * A1223),
                 det * (m.e01 * A2313 - m.e02 * A1313 + m.e03 * A1213),
                 det * -(m.e01 * A2312 - m.e02 * A1312 + m.e03 * A1212),
                 det * -(m.e10 * A2323 - m.e12 * A0323 + m.e13 * A0223),
                 det * (m.e00 * A2323 - m.e02 * A0323 + m.e03 * A0223),
                 det * -(m.e00 * A2313 - m.e02 * A0313 + m.e03 * A0213),
                 det * (m.e00 * A2312 - m.e02 * A0312 + m.e03 * A0212),
                 det * (m.e10 * A1323 - m.e11 * A0323 + m.e13 * A0123),
                 det * -(m.e00 * A1323 - m.e01 * A0323 + m.e03 * A0123),
                 det * (m.e00 * A1313 - m.e01 * A0313 + m.e03 * A0113),
                 det * -(m.e00 * A1312 - m.e01 * A0312 + m.e03 * A0112),
                 det * -(m.e10 * A1223 - m.e11 * A0223 + m.e12 * A0123),
                 det * (m.e00 * A1223 - m.e01 * A0223 + m.e02 * A0123),
                 det * -(m.e00 * A1213 - m.e01 * A0213 + m.e02 * A0113),
                 det * (m.e00 * A1212 - m.e01 * A0212 + m.e02 * A0112));
  return result;
}


// rect2

typedef struct {
  V2 min;
  V2 max;
} Rect2;

Rect2 rect2_min_max(V2 min, V2 max) {
  Rect2 result;
  result.min = min;
  result.max = max;
  return result;
}

Rect2 rect2_min_size(V2 min, V2 size) {
  Rect2 result;
  result.min = min;
  result.max = v2_add(min, size);
  return result;
}

Rect2 rect2_center_size(V2 center, V2 size) {
  Rect2 result;
  V2 half_size = v2_mul(size, 0.5f);
  result.min = v2_sub(center, half_size);
  result.max = v2_add(center, half_size);
  return result;
}

Rect2 rect2_inverted_infinity() {
  Rect2 result;
  result.min = v2(INFINITY, INFINITY);
  result.max = v2(-INFINITY, -INFINITY);
  return result;
}

V2 rect2_get_size(Rect2 r) {
  V2 result = v2_sub(r.max, r.min);
  return result;
}

V2 rect2_get_center(Rect2 r) {
  V2 result = v2_add(r.min, v2_mul(rect2_get_size(r), 0.5f));
  return result;
}

Rect2 rect2_translate(Rect2 r, V2 v) {
  Rect2 result;
  result.min = v2_add(r.min, v);
  result.max = v2_add(r.max, v);
  return result;
}

Rect2 rect2_apply_m4(Rect2 r, M4 m) {
  Rect2 result;
  result.min = m4_mul_v4(m, v2_to_v4(r.min, 1, 1)).xy;
  result.max = m4_mul_v4(m, v2_to_v4(r.max, 1, 1)).xy;
  return result;
}

// rect2i

typedef struct {
  V2i min;
  V2i max;
} Rect2i;

Rect2i rect2i_min_max(V2i min, V2i max) {
  Rect2i result;
  result.min = min;
  result.max = max;
  return result;
}

Rect2i rect2i_min_size(V2i min, V2i size) {
  Rect2i result;
  result.min = min;
  result.max = v2i_add(min, size);
  return result;
}

Rect2i rect2i_center_size(V2i center, V2i size) {
  Rect2i result;
  V2i half_size = v2i_div(size, 2);
  result.min = v2i_sub(center, half_size);
  result.max = v2i_add(center, half_size);
  return result;
}

V2i rect2i_get_size(Rect2i r) {
  V2i result = v2i_sub(r.max, r.min);
  return result;
}

Rect2 rect2i_to_rect2(Rect2i r) {
  Rect2 result;
  result.min = v2i_to_v2(r.min);
  result.max = v2i_to_v2(r.max);
  return result;
}




// NOTE(lvl5): misc
f32 lerp_f32(f32 a, f32 b, f32 c) {
  f32 result = a*(1 - c) + b*c;
  return result;
}

V2 lerp_v2(V2 a, V2 b, V2 c) {
  V2 result;
  result.x = lerp_f32(a.x, b.x, c.x);
  result.y = lerp_f32(a.y, b.y, c.y);
  return result;
}


V3 lerp_v3(V3 a, V3 b, V3 c) {
  V3 result;
  result.x = lerp_f32(a.x, b.x, c.x);
  result.y = lerp_f32(a.y, b.y, c.y);
  result.z = lerp_f32(a.z, b.z, c.z);
  return result;
}


V4 lerp_v4(V4 a, V4 b, V4 c) {
  V4 result;
  result.x = lerp_f32(a.x, b.x, c.x);
  result.y = lerp_f32(a.y, b.y, c.y);
  result.z = lerp_f32(a.z, b.z, c.z);
  result.w = lerp_f32(a.w, b.w, c.w);
  return result;
}


// NOTE(lvl5): misc
b32 point_in_circle(V2 point, V2 origin, f32 radius) {
  b32 result = false;
  
  f32 dist_sqr = v2_length_sqr(v2_sub(point, origin));
  if (dist_sqr < sqr_f32(radius)) {
    result = true;
  }
  return result;
}

b32 point_in_rect(V2 point, Rect2 rect) {
  b32 result = point.x > rect.min.x &&
    point.x <= rect.max.x &&
    point.y > rect.min.y &&
    point.y <= rect.max.y;
  return result;
}


V4 color_linear_to_srgb(V4 color) {
  V4 result;
  result.r = sqrt_f32(color.r);
  result.g = sqrt_f32(color.g);
  result.b = sqrt_f32(color.b);
  result.a = color.a;
  return result;
}

V4 color_srgb_to_linear(V4 color) {
  V4 result;
  result.r = sqr_f32(color.r);
  result.g = sqr_f32(color.g);
  result.b = sqr_f32(color.b);
  result.a = color.a;
  return result;
}

u32 color_u32(u8 r, u8 g, u8 b, u8 a) {
  u32 result = (r << 16)|(g << 8)|(b << 0)|(a << 24);
  return result;
}

u32 color_v4_to_u32(V4 color) {
  u8 r = (u8)(color.r*255);
  u8 g = (u8)(color.g*255);
  u8 b = (u8)(color.b*255);
  u8 a = (u8)(color.a*255);
  u32 result = (r << 16)|(g << 8)|(b << 0)|(a << 24);
  return result;
}

V4 color_u32_to_v4(u32 color) {
  V4 result;
  result.r = ((0x00FF0000 & color) >> 16)/255.0f;
  result.g = ((0x0000FF00 & color) >> 8)/255.0f;
  result.b = ((0x000000FF & color) >> 0)/255.0f;
  result.a = ((0xFF000000 & color) >> 24)/255.0f;
  return result;
}

u32 color_invert(u32 color) {
  u8 a = ((0xFF000000 & color) >> 24);
  u32 result = 0xFFFFFFFF - color | a << 24;
  return result;
}


#define LVL5_MATH
#endif