# ok.hpp

This is a c++20 implementation of my take on a `result` type this has more go like design
than c++ `std::expected` or rusts `Result`

`ok.hpp` is a single header library and does not have an implementation macro
due to being written in c++

As is typically the case with header only libraries, most documentation is in the 
opening comment and most functions/structures have doc comments

Simple usage:

```c
    #define OK_FUNCTIONAL_EXTENSIONS
    #include "ok.hpp"
    #include <print>

    using namespace ok;

    Result<int, const char*> error_func() { return err("error"); }
    Result<int, const char*> val_func() { return 69; }
    Result<int, const char*> both_func() { return {420, "partial error"}; }

    int main() {
        auto [_, err1] = error_func();
        if (err1) { std::println("returned error: '{}'", *err1); }

        auto [val, err2] = val_func();
        if (err2) { std::println("returned error: '{}'", *err2); }
        std::println("returned value: '{}'", val);

        auto def_val = error_func().value_or(420);
        std::println("returned value or default: '{}'", def_val);

        return 0;
    }
```
