// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0


#include <cstdint>
#include <string_view>

// Captures a boolean test value together with the original expression text.
// Example: auto [passed, name] = NDOF_CAPTURE_BOOL_TEST(x > 0);
#define NDOF_CAPTURE_BOOL_TEST(test_expression) static_cast<bool>(test_expression), #test_expression

 
#if defined(NDOF_RTTI_ENABLED) && (NDOF_RTTI_ENABLED)
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
[[nodiscard]] type_token type_token_of() noexcept {
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
