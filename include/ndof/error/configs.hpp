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

enum class BuildMode : std::uint8_t {
    undefined,
    debug,
    release
};

#if defined(NDOF_RTTI_ENABLED) && (NDOF_RTTI_ENABLED)
using Type = std::type_index;
#else
using Type = ndof_type_index;
#endif

enum class TypeIndexMode : std::uint8_t {
    ndof_type_index,
    rtti_type_index
};

template<typename T>
[[nodiscard]] Type getType() noexcept {
#if defined(NDOF_RTTI_ENABLED) && (NDOF_RTTI_ENABLED)
    return std::type_index(typeid(T));
#else
    return ndof_type_index::of<T>();
#endif
}

} // namespace ndof

namespace ndof::error {

[[nodiscard]] ndof::BuildMode getBuildMode() noexcept;

[[nodiscard]] std::string_view getBuildModeName() noexcept;

[[nodiscard]] bool isRttiEnabled() noexcept;

[[nodiscard]] ndof::TypeIndexMode getTypeIndexMode() noexcept;

[[nodiscard]] std::string_view getTypeIndexModeName() noexcept;

} // namespace ndof::error
