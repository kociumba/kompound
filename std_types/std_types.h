// this header provides simple cross-compiler std type definitions
//
// the naming resembles newer languages, and in my honest opinion is more
// pleasant to work with
//
// typedef is specifically used so that this header can be used in C and C++
//
// maximum and minimum values are defined for all numeric typed defined in this
// header, the simpler way to access them is through the convenience macros
// max_of and min_of
//
// if the language version supports some kind of static_assert, width checks are
// also provided for the defined types

#ifndef STD_TYPES_H
#define STD_TYPES_H

#if (defined(__LP64__) || defined(_LP64) || defined(_WIN64) ||                 \
     (defined(__x86_64__) && !defined(__ILP32__)) || defined(__aarch64__) ||   \
     defined(__ppc64__) || defined(__powerpc64__) ||                           \
     (defined(__riscv) && defined(__riscv_xlen) && (__riscv_xlen == 64)))
#define STD_TYPES_ARCH_64BIT
#endif

#if defined(_MSC_VER)

typedef __int8 i8;
typedef unsigned __int8 u8;
typedef __int16 i16;
typedef unsigned __int16 u16;
typedef __int32 i32;
typedef unsigned __int32 u32;
typedef __int64 i64;
typedef unsigned __int64 u64;

typedef float f32;
typedef double f64;

#if defined(STD_TYPES_ARCH_64BIT)
typedef __int64 intptr;
typedef unsigned __int64 uintptr;
typedef unsigned __int64 usize;
typedef __int64 isize;
#else
typedef __int32 intptr;
typedef unsigned __int32 uintptr;
typedef unsigned __int32 usize;
typedef __int32 isize;
#endif

#elif defined(__GNUC__) || defined(__clang__)

typedef __INT8_TYPE__ i8;
typedef __UINT8_TYPE__ u8;
typedef __INT16_TYPE__ i16;
typedef __UINT16_TYPE__ u16;
typedef __INT32_TYPE__ i32;
typedef __UINT32_TYPE__ u32;
typedef __INT64_TYPE__ i64;
typedef __UINT64_TYPE__ u64;

typedef float f32;
typedef double f64;

typedef __INTPTR_TYPE__ intptr;
typedef __UINTPTR_TYPE__ uintptr;

typedef __PTRDIFF_TYPE__ isize;
typedef __SIZE_TYPE__ usize;

#else
#error "std_types.h does not support this compiler"
#endif

// numeric limits for integer types
#define i8_min (-128)
#define i8_max (127)

#define u8_min (0)
#define u8_max (255)

#define i16_min (-32768)
#define i16_max (32767)

#define u16_min (0)
#define u16_max (65535)

#define i32_min (-i32_max - 1)
#define i32_max (2147483647)

#define u32_min (0)
#define u32_max (4294967295U)

#define i64_min (-i64_max - 1)
#define i64_max (9223372036854775807)

#define u64_min (0)
#define u64_max (18446744073709551615ULL)

// numeric limits for floating point types
#define f32_min (1.175494351e-38F)
#define f32_max (3.402823466e+38F)

#define f64_min (2.2250738585072014e-308)
#define f64_max (1.7976931348623158e+308)

// numeric limits for pointer types
#if defined(STD_TYPES_ARCH_64BIT)
#define isize_min i64_min
#define isize_max i64_max

#define usize_min u64_min
#define usize_max u64_max

#define intptr_min i64_min
#define intptr_max i64_max

#define uintptr_min u64_min
#define uintptr_max u64_max
#else
#define isize_min i32_min
#define isize_max i32_max

#define usize_min u32_min
#define usize_max u32_max

#define intptr_min i32_min
#define intptr_max i32_max

#define uintptr_min u32_min
#define uintptr_max u32_max
#endif

// type limit macros, only work with numeric types defined in this header
#define max_of(type) type##_max
#define min_of(type) type##_min

// define to define `byte` type in the global scope for convenience
#if defined(DEFINE_BYTE)
typedef u8 byte;

#define byte_min u8_min
#define byte_max u8_max
#endif

// C parity with C++
#if !defined(__cplusplus)
#if (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 202311L))
#if !defined(__bool_true_false_are_defined)
typedef _Bool bool;

#if !defined(true)
#define true 1
#endif

#if !defined(false)
#define false 0
#endif

#define __bool_true_false_are_defined 1
#endif
#endif
#endif

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define STD_TYPES_size_assert(type, size)                                      \
  static_assert(sizeof(type) == size, #type " is not " #size " byte(s)")
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)

#define STD_TYPES_size_assert(type, size)                                      \
  _Static_assert(sizeof(type) == size, #type " is not " #size " byte(s)")
#endif

#if defined(STD_TYPES_size_assert)
STD_TYPES_size_assert(i8, 1);
STD_TYPES_size_assert(u8, 1);
STD_TYPES_size_assert(i16, 2);
STD_TYPES_size_assert(u16, 2);
STD_TYPES_size_assert(i32, 4);
STD_TYPES_size_assert(u32, 4);
STD_TYPES_size_assert(i64, 8);
STD_TYPES_size_assert(u64, 8);

STD_TYPES_size_assert(f32, 4);
STD_TYPES_size_assert(f64, 8);

#if defined(STD_TYPES_ARCH_64BIT)
STD_TYPES_size_assert(intptr, 8);
STD_TYPES_size_assert(uintptr, 8);
STD_TYPES_size_assert(usize, 8);
STD_TYPES_size_assert(isize, 8);
#else
STD_TYPES_size_assert(intptr, 4);
STD_TYPES_size_assert(uintptr, 4);
STD_TYPES_size_assert(usize, 4);
STD_TYPES_size_assert(isize, 4);
#endif

// define if targeting a platform that does not use 1 byte bools
#if !defined(DO_NOT_CHECK_SIZEOF_BOOL)
STD_TYPES_size_assert(bool, 1);
#endif

#undef STD_TYPES_size_assert
#endif

#if defined(STD_TYPES_ARCH_64BIT)
#undef STD_TYPES_ARCH_64BIT
#endif

#endif // STD_TYPES_H
