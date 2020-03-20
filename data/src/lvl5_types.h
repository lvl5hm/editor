#ifndef LVL5_TYPES

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

typedef i32 b32;
typedef u8 byte;
typedef i8 bool;

typedef u64 Mem_Size;

#define thread_local __declspec(thread)

#define false 0
#define true 1
#define null 0

#define kilobytes(n) (1024LL*(n))
#define megabytes(n) (1024LL*kilobytes(n))
#define gigabytes(n) (1024LL*megabytes(n))
#define terabytes(n) (1024LL*gigabytes(n))

#define assert(expr) if (!(expr)) *(int *)0 = 120;
#define invalid_default_case default: assert(false); break;

#define align_pow_2(value, align) (((value) + (align)-1) & ~((align)-1))

#define array_count(arr) (sizeof(arr)/sizeof(*(arr)))

#define offsetof(T, m) ((Mem_Size)(void *)&(((T *)0)->m))

#define globalvar static

#define LVL5_TYPES
#endif