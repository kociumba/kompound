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
