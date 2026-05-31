# std_types.h

A very simple C/C++ header providing std type definitions

The types are named in a way more closely resembling newer languages e.g. i64, f32

If you want to define a `byte` type in the global scope as a `u8`,  define `DEFINE_BYTE` before including this header

The header also includes some compile time checks for the width of the defined types, these are active only
if you version of C or C++ supports some kind of `static_assert`

## test

There is a very simple test also provided for this, you can run it with `make test`, it will work on windows
with a correctly set up enviornment but there can be issues
