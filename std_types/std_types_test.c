#define DEFINE_BYTE
#include "std_types.h"

int main(void) {
    // fixed-width integers
    i8  a = -1;   u8  b = 1;
    i16 c = -1;   u16 d = 1;
    i32 e = -1;   u32 f = 1U;
    i64 g = -1LL; u64 h = 1ULL;

    // floating point
    f32 i = 1.0f;
    f64 j = 1.0;

    // pointer / size types
    intptr  k = (intptr)&a;
    uintptr l = (uintptr)&b;
    isize   m = (isize)-1;
    usize   n = (usize)1;

    // byte
    byte o = 0xFF;

    // boolean
    bool p = true;
    bool q = false;

    // touch each type
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    (void)g; (void)h; (void)i; (void)j; (void)k; (void)l;
    (void)m; (void)n; (void)o; (void)p; (void)q;

    return 0;
}
