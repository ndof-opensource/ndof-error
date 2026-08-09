// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <compare>
#include <cstddef>
#include <functional>
#include <type_traits>

namespace ndof {

namespace detail {

template<typename T>
struct ndof_type_tag {
    static inline const int value = 0;
};

} // namespace detail

struct ndof_type_index {
    using value_type = const void*;

    constexpr ndof_type_index() noexcept = default;

    explicit constexpr ndof_type_index(value_type value) noexcept
        : value_(value) {
    }

    template<typename T>
    [[nodiscard]] static ndof_type_index of() noexcept {
        using normalized_type = std::remove_cvref_t<T>;
        return ndof_type_index(static_cast<value_type>(&detail::ndof_type_tag<normalized_type>::value));
    }

    [[nodiscard]] constexpr value_type value() const noexcept {
        return value_;
    }

    friend constexpr bool operator==(ndof_type_index lhs, ndof_type_index rhs) noexcept = default;
    friend constexpr auto operator<=>(ndof_type_index lhs, ndof_type_index rhs) noexcept = default;

private:
    value_type value_ = nullptr;
};

} // namespace ndof

namespace std {

template<>
struct hash<ndof::ndof_type_index> {
    std::size_t operator()(ndof::ndof_type_index value) const noexcept {
        return std::hash<ndof::ndof_type_index::value_type>{}(value.value());
    }
};

} // namespace std
