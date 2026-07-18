/* ok.hpp - https://github.com/kociumba/kompound

A minimal result type with go semantics implementation for C++20 and up

SIMPLE EXAMPLE:
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

USAGE:
    This is a c++ header only template library which means it can be
    included and used anywhere.

    If using any of the config macros, the values should match across
    translation units including this header.

    NOTE: This design has close to none `clean architecture` idiot proofing
        safeguards, for example to access the value on `Result` you simply
        use the field: `r.val`. If you are not comfortable with this, then
        this library might not be a good fit for you.

REDEFINABLE MACROS:

    You can define these macros before including ok.hpp to set some configurations

    - OK_NAMESPACE: if defined all of the code in ok.hpp will be placed in in `namespace OK_NAMESPACE`
    - OK_SUPPORT_EXCEPTIONS: by default all functions in ok.hpp are marked as noexcept
        if you need to support exceptions for whatever reason define this macro which will remove noexcept
    - OK_ASSERT: define this to provide your own assert for use by ok.hpp
    - OK_FUNCTIONAL_EXTENSIONS: if defined the `Result` type will have extra functional style methods
        e.g: `value_or()`, `map()` and so on

*/

#ifndef OK_HPP
#define OK_HPP

#include <utility>

#if defined(OK_NAMESPACE)
#define ok_ns OK_NAMESPACE
#else
#define ok_ns ok
#endif  // #if defined(OK_NAMESPACE)

#if defined(OK_SUPPORT_EXCEPTIONS)
#define OK_NOEXCEPT
#else
#define OK_NOEXCEPT noexcept
#endif  // #if defined(OK_SUPPORT_EXCEPTIONS)

#if !defined(OK_ASSERT)
#include <assert.h>
#define OK_ASSERT assert
#endif  // #if !defined(OK_ASSERT)

#if defined(_MSVC_LANG)
#define CXX_STANDARD _MSVC_LANG
#else
#define CXX_STANDARD __cplusplus
#endif  // #if defined(_MSVC_LANG)

#if CXX_STANDARD < 202002L
#error "ok.hpp requires c++20 or later"
#endif  // #if CXX_STANDARD < 202002L
#undef CXX_STANDARD

namespace ok_ns {

/*
Error type wrapper, you should not need to interact with this type, instead return errors using:
    `err(...)`
*/
template <typename ERR>
struct _error {
    ERR err;
};

/*
The error type returned to you from a result, it *MIGHT NOT* hold a value

- This is truthy if the error has a value (the result has an error value)
- Can be explicitly and implicitly converted to `ERR`
*/
template <typename ERR>
struct Error {
    bool has_error;
    union {
        char nil;
        ERR err;
    };

    Error() OK_NOEXCEPT : has_error(false), nil(0) {}

    explicit Error(ERR&& e) OK_NOEXCEPT : has_error(true), err(std::move(e)) {}
    explicit Error(const ERR& e) OK_NOEXCEPT : has_error(true), err(e) {}

    Error(const Error& other) OK_NOEXCEPT : has_error(other.has_error) {
        if (has_error) {
            new (&err) ERR(other.err);
        } else {
            nil = 0;
        }
    }

    Error(Error&& other) OK_NOEXCEPT : has_error(other.has_error) {
        if (has_error) {
            new (&err) ERR(std::move(other.err));
        } else {
            nil = 0;
        }
    }

    Error& operator=(const Error& other) OK_NOEXCEPT {
        if (this == &other) return *this;
        if (has_error) err.~ERR();
        has_error = other.has_error;
        if (has_error) {
            new (&err) ERR(other.err);
        } else {
            nil = 0;
        }
        return *this;
    }

    Error& operator=(Error&& other) OK_NOEXCEPT {
        if (this == &other) return *this;
        if (has_error) err.~ERR();
        has_error = other.has_error;
        if (has_error) {
            new (&err) ERR(std::move(other.err));
        } else {
            nil = 0;
        }
        return *this;
    }

    operator bool() const OK_NOEXCEPT { return has_error; }

    operator ERR() const& OK_NOEXCEPT {
        OK_ASSERT(has_error);
        return err;
    }

    operator ERR() && OK_NOEXCEPT {
        OK_ASSERT(has_error);
        return std::move(err);
    }

    ERR& operator*() & {
        OK_ASSERT(has_error);
        return err;
    }

    const ERR& operator*() const& {
        OK_ASSERT(has_error);
        return err;
    }

    ERR&& operator*() && {
        OK_ASSERT(has_error);
        return std::move(err);
    }

    ~Error() OK_NOEXCEPT {
        if (has_error) { err.~ERR(); }
    }
};

/*
Used to return only the error value from a function returning `Result<VAL, ERR>`:
    `return err(ERR());`
*/
template <typename ERR>
_error<std::decay_t<ERR>> err(ERR&& err) OK_NOEXCEPT {
    return {std::forward<ERR>(err)};
}

/*
The main `Result` type, this differs from things like rusts `Result` or `std::expected`
in the fact that it can hold both values at the same time.

This allows it to represent states that still return a partial value with an error side effect

- This is truthy when there is no error in the `Result`
- This can return one of the values or both

NOTE: `VAL` must be default constructible to achieve the go like behaviour
*/
template <typename VAL, typename ERR>
    requires std::is_default_constructible_v<VAL> && (!std::is_convertible_v<VAL, _error<ERR>>)
struct [[nodiscard("a result value can contain an error an as such should be handled")]] Result {
    VAL val;
    Error<ERR> err;

    Result(const VAL& v) OK_NOEXCEPT : val(v), err() {}
    Result(VAL&& v) OK_NOEXCEPT : val(std::move(v)), err() {}

    Result(_error<ERR>&& e) OK_NOEXCEPT : val(), err(std::move(e.err)) {}
    Result(const _error<ERR>& e) OK_NOEXCEPT : val(), err(e.err) {}

    Result(const VAL& v, const ERR& e) OK_NOEXCEPT : val(v), err(e) {}

    Result(VAL&& v, ERR&& e) OK_NOEXCEPT : val(std::move(v)), err(std::move(e)) {}

    explicit operator bool() const noexcept { return !err; }

#if defined(OK_FUNCTIONAL_EXTENSIONS)
    /*
    Applies `f` to the value and returns a new `Result` with the transformed value
    Propagates the error unchanged if this result holds an error
    */
    template <typename F>
    auto map(F&& f) && -> ok_ns::Result<std::invoke_result_t<F, VAL>, ERR> {
        if (!err)
            return std::forward<F>(f)(std::move(val));
        else
            return ok_ns::err(std::move(*err));
    }

    /*
    Applies `f` to the value and returns a new `Result` with the transformed value
    Propagates the error unchanged if this result holds an error
    */
    template <typename F>
    auto map(F&& f) const& -> ok_ns::Result<std::invoke_result_t<F, const VAL&>, ERR> {
        if (!err)
            return f(val);
        else
            return ok_ns::err(err.err);
    }

    /*
    Applies `f` to the value where `f` itself returns a `Result`
    Propagates the error unchanged if this result holds an error
    */
    template <typename F>
    auto and_then(F&& f) && -> std::invoke_result_t<F, VAL> {
        if (!err)
            return std::forward<F>(f)(std::move(val));
        else
            return ok_ns::err(std::move(*err));
    }

    /*
    Applies `f` to the value where `f` itself returns a `Result`
    Propagates the error unchanged if this result holds an error
    */
    template <typename F>
    auto and_then(F&& f) const& -> std::invoke_result_t<F, const VAL&> {
        if (!err)
            return f(val);
        else
            return ok_ns::err(err.err);
    }

    // NOTE: clojure return needs to match VAL here, i don't think anything can be done about this in c++20

    /*
    Applies `f` to the error as a recovery path, returning its result
    Propagates the value unchanged if this result holds a value
    */
    template <typename F>
    auto or_else(F&& f) && -> std::invoke_result_t<F, ERR> {
        if (err)
            return std::forward<F>(f)(std::move(*err));
        else
            return std::move(val);
    }

    /*
    Applies `f` to the error as a recovery path, returning its result
    Propagates the value unchanged if this result holds a value
    */
    template <typename F>
    auto or_else(F&& f) const& -> std::invoke_result_t<F, const ERR&> {
        if (err)
            return f(err.err);
        else
            return val;
    }

    /*
    Returns the value on success, or `fallback` if this result holds an error
    */
    VAL value_or(VAL&& fallback) && {
        if (!err)
            return std::move(val);
        else
            return std::forward<VAL>(fallback);
    }

    /*
    Returns the value on success, or `fallback` if this result holds an error
    */
    VAL value_or(const VAL& fallback) const& {
        if (!err)
            return val;
        else
            return fallback;
    }
#endif
};

}  // namespace ok_ns

#endif  // #ifndef OK_HPP
