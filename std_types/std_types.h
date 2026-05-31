// this header provides simple cross-compiler std type definitions
//
// the naming resembles newer languages, and in my honest opinion is more
// pleasant to work with
//
// typedef is specifically used so that this header can be used in C and C++
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

// define to define `byte` type in the global scope for convenience
#if defined(DEFINE_BYTE)
typedef u8 byte;
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
