// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string_view>

#include "ndof/error/type_index.hpp"

#if defined(NDOF_RTTI_ENABLED) && (NDOF_RTTI_ENABLED)
#include <typeindex>
#include <typeinfo>
#endif

namespace ndof {

enum class build_mode : std::uint8_t {
    undefined,
    debug,
    release
};

#if defined(NDOF_RTTI_ENABLED) && (NDOF_RTTI_ENABLED)
using type_token = std::type_index;
#else
using type_token = ndof_type_index;
#endif

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

} // namespace ndof

namespace ndof::error {

[[nodiscard]] ndof::build_mode build_mode() noexcept;

[[nodiscard]] std::string_view build_mode_name() noexcept;

[[nodiscard]] bool rtti_enabled() noexcept;

[[nodiscard]] ndof::type_index_mode type_index_mode() noexcept;

[[nodiscard]] std::string_view type_index_mode_name() noexcept;

} // namespace ndof::error
