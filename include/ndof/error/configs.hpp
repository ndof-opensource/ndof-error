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

 

[[nodiscard]] ndof::build_mode get_build_mode() noexcept;

[[nodiscard]] std::string_view get_build_mode_name() noexcept;

[[nodiscard]] bool get_rtti_enabled() noexcept;

[[nodiscard]] ndof::type_index_mode get_type_index_mode() noexcept;

[[nodiscard]] std::string_view get_type_index_mode_name() noexcept;

}  

#endif // NDOF_ERROR_CONFIGS_HPP
