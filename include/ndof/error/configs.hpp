// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef NDOF_ERROR_CONFIGS_HPP
#define NDOF_ERROR_CONFIGS_HPP

// TODO: Check the character set that was used to compile the library and provide a way to query it at runtime.
//       We can use this setting as the default character set for the library and provide a way to override it at runtime if necessary.
//       Probably by defining ndof::object to be ndof::basic_object<ndof::default_char_type> and providing a way to change the default_char_type at runtime.
//       Same with ndof::exception and ndof::basic_exception<ndof::default_char_type>.

#include <cstdint>
#include <string_view>

// #include <expected> May be included below if exceptions are disabled. This is a C++23 feature, so we need to check for compiler support and provide a fallback if necessary.


// Captures a boolean test value together with the original expression text.
// Example: auto [passed, name] = NDOF_CAPTURE_BOOL_TEST(x > 0);
#define NDOF_CAPTURE_BOOL_TEST(test_expression) static_cast<bool>(test_expression), #test_expression

#ifndef NDOF_RTTI_ENABLED
//-fno-rtti (GCC/Clang) or /GR- (MSVC)
#if defined(__cpp_rtti)
#define NDOF_RTTI_ENABLED 1
#elif defined(_MSC_VER)
#if defined(_CPPRTTI)
#define NDOF_RTTI_ENABLED 1
#else
#define NDOF_RTTI_ENABLED 0
#endif
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__GXX_RTTI)
#define NDOF_RTTI_ENABLED 1
#else
#define NDOF_RTTI_ENABLED 0
#endif
#else
#define NDOF_RTTI_ENABLED 0
#endif
#endif

#if defined(NDOF_RTTI_ENABLED) 
#include <typeindex>
using type_token = std::type_index;
#else
#include "ndof/error/type_index.hpp"
using type_token = ndof_type_index;
#endif

namespace ndof {

enum class build_mode : std::uint8_t {
    undefined,
    debug,
    release
};
 

enum class type_index_mode : std::uint8_t {
    ndof_type_index,
    rtti_type_index
};

template<typename T>
[[nodiscard]] constexpr type_token type_token_of() noexcept {
#if defined(NDOF_RTTI_ENABLED) && (NDOF_RTTI_ENABLED)
    return std::type_index(typeid(T));
#else
    return ndof_type_index::for_type<T>();
#endif
}

// Determine the default character type based on standard compiler switches / the standard.
// - On Windows with _UNICODE (or UNICODE) defined, wchar_t is the conventional default.
// - If the compiler supports char8_t (C++20, __cpp_char8_t), prefer it as the "native" UTF-8 char type.
// - Otherwise fall back to plain char.
#ifndef NDOF_DEFAULT_CHAR_TYPE
#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
#define NDOF_DEFAULT_CHAR_TYPE wchar_t
#elif defined(__cpp_char8_t)
#define NDOF_DEFAULT_CHAR_TYPE char8_t
#else
#define NDOF_DEFAULT_CHAR_TYPE char
#endif
#endif


using default_char_t = NDOF_DEFAULT_CHAR_TYPE;

#ifndef NDOF_DEFAULT_CHAR_TRAITS_TYPE
#define NDOF_DEFAULT_CHAR_TRAITS_TYPE std::char_traits<default_char_t>
#endif

using default_char_traits_t = NDOF_DEFAULT_CHAR_TRAITS_TYPE;

using default_string_view = std::basic_string_view<default_char_t, default_char_traits_t>;

[[nodiscard]] consteval ndof::build_mode get_build_mode() noexcept;

[[nodiscard]] consteval default_string_view get_build_mode_name() noexcept;

[[nodiscard]] consteval bool get_rtti_enabled() noexcept;

[[nodiscard]] consteval ndof::type_index_mode get_type_index_mode() noexcept;

[[nodiscard]] consteval default_string_view get_type_index_mode_name() noexcept;

// Determine whether C++ exceptions are enabled based on standard compiler switches.
#ifndef NDOF_EXCEPTIONS_ENABLED
#if defined(__cpp_exceptions)
#define NDOF_EXCEPTIONS_ENABLED 1
#elif defined(_MSC_VER) && defined(_CPPUNWIND)
#define NDOF_EXCEPTIONS_ENABLED 1
#else
#define NDOF_EXCEPTIONS_ENABLED 0
#endif
#endif

class basic_exception;

#if defined(NDOF_EXCEPTIONS_ENABLED) && (NDOF_EXCEPTIONS_ENABLED)

// When exceptions are enabled, error propagation is done via throwing, so the
// return type is simply T.
template<typename T>
using result = T;

#else

// TODO: make sure this is noted at the top of the file.
#include <expected>

// When exceptions are disabled, error propagation is done via std::expected,
// carrying either the value T or an ndof::exception on failure.
template<typename T>
using result = std::expected<T, ndof::basic_exception>;

#endif


}  

#endif // NDOF_ERROR_CONFIGS_HPP
