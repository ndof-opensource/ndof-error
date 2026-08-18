// Copyright 2026 NDOF-OS
// SPDX-License-Identifier: Apache-2.0

#ifndef NDOF_ERROR_OBJECT_QUERY_HPP
#define NDOF_ERROR_OBJECT_QUERY_HPP

// TODO: Move this to the core library.

#include <ndof/error/object.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace ndof::error {

// -----------------------------------------------------------------------------
// Compile-time XPath-like literal holder, e.g. NDOF_XPATH("/a/b/c")
// -----------------------------------------------------------------------------
template <std::size_t N>
struct xpath_literal {
    char value[N]{};

    consteval xpath_literal(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i) value[i] = str[i];
    }

    constexpr std::string_view view() const noexcept {
        return std::string_view{value, N - 1};
    }
};

namespace detail {

// Number of '/'-separated, non-empty steps in the path.
consteval std::size_t xpath_step_count(std::string_view p) {
    std::size_t count = 0;
    std::size_t i = 0;
    while (i < p.size()) {
        while (i < p.size() && p[i] == '/') ++i;
        if (i >= p.size()) break;
        ++count;
        while (i < p.size() && p[i] != '/') ++i;
    }
    return count;
}

template <std::size_t Count>
consteval std::array<std::string_view, Count> xpath_split(std::string_view p) {
    std::array<std::string_view, Count> steps{};
    std::size_t n = 0;
    std::size_t i = 0;
    while (i < p.size() && n < Count) {
        while (i < p.size() && p[i] == '/') ++i;
        if (i >= p.size()) break;
        const std::size_t begin = i;
        while (i < p.size() && p[i] != '/') ++i;
        steps[n++] = p.substr(begin, i - begin);
    }
    return steps;
}

} // namespace detail

// -----------------------------------------------------------------------------
// object_query: a compile-time constructed query over an ndof::object.
//
//   constexpr auto q = object_query<"/error/code">{};
//   std::optional<ndof::object> r = q(some_const_object_ref);
// -----------------------------------------------------------------------------


template <xpath_literal Path>
class object_query {
public:
    static constexpr std::string_view path = Path.view();
    static constexpr std::size_t step_count = detail::xpath_step_count(path);
    static constexpr std::array<std::string_view, step_count> steps =
        detail::xpath_split<step_count>(path);

    using result_type = std::optional<ndof::object>;

    constexpr object_query() noexcept = default;

    result_type operator()(const ndof::object& root) const {
        const ndof::object* current = &root;
        for (const std::string_view step : steps) {
            const ndof::object* next = resolve(*current, step);
            if (next == nullptr) return std::nullopt;
            current = next;
        }
        return *current;
    }

    result_type query(const ndof::object& root) const { return (*this)(root); }

private:
    // Resolves a single path step against an object child by name.
    static const ndof::object* resolve(const ndof::object& obj, std::string_view name) {
        return obj.find(name);
    }
};

template <xpath_literal Path>
inline constexpr object_query<Path> make_object_query() noexcept {
    return object_query<Path>{};
}

} // namespace ndof::error

#define NDOF_XPATH(literal) ::ndof::error::object_query<literal>{}

#endif // NDOF_ERROR_OBJECT_QUERY_HPP
