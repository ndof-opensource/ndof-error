// Copyright 2026 NDOF-OS
// SPDX-License-Identifier: Apache-2.0
#ifndef NDOF_ERROR_OBJECT_QUERY_HPP
#define NDOF_ERROR_OBJECT_QUERY_HPP

// TODO: Move this to the core library.
 
#include "ndof/error/allocator_support.hpp"
#include "ndof/error/object.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace ndof  {

// -----------------------------------------------------------------------------
// Compile-time XPath-like literal holder, e.g. NDOF_XPATH("/a/b/c")
// -----------------------------------------------------------------------------
template <typename CharT, std::size_t N>
struct xpath_literal {
    CharT value[N]{};

    consteval xpath_literal(const CharT (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i) value[i] = str[i];
    }

    constexpr std::basic_string_view<CharT, std::char_traits<CharT>> view() const noexcept {
        return std::basic_string_view<CharT, std::char_traits<CharT>>{value, N - 1};
    }

    using string_t = std::basic_string<CharT, std::char_traits<CharT>>;
    using string_view_t = std::basic_string_view<CharT, std::char_traits<CharT>>;
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

// Thrown when parsing stops before an object has been completely processed.
// The partially parsed object is retained so callers can inspect its state.
template <typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          allocator_like Allocator = default_allocator_t>
class object_parse_exception : public ndof::exception<CharT, Traits, Allocator> {
public:
    using object_t = ndof::object<CharT, Traits, Allocator>;
    explicit object_parse_exception(object_t last_parsed)
        : last_parsed_(std::move(last_parsed)) {}

    [[nodiscard]] const object_t& last_parsed() const noexcept {
        return last_parsed_;
    }

private:
    object_t last_parsed_;
};

// -----------------------------------------------------------------------------
// object_query: a compile-time constructed query over an ndof::object.
//
//   constexpr auto q = object_query<"/error/code">{};
//   std::optional<ndof::object> r = q(some_const_object_ref);
// -----------------------------------------------------------------------------



template <xpath_literal path>
class object_query {
public:
    using Path = decltype(path);
    using CharT = typename Path::value_type;
    using Traits = std::char_traits<CharT>;
    static constexpr std::string_view path_view =path.view() ;
    static constexpr std::size_t step_count = detail::xpath_step_count(path_view);
    static constexpr std::array<std::string_view, step_count> steps =
        detail::xpath_split<step_count>(path_view);

    using result_type = std::optional<ndof::object>;

    constexpr object_query() noexcept = default;
    explicit constexpr object_query(const typename Path::string_t& path_str);

    // TODO: Fix this.
     
    template<allocator_like Allocator>
    auto operator()(const ndof::object<CharT, Traits, Allocator>&& receiving_node) const {
        const auto query = [](auto&& obj) -> result_type {
            const auto* current = &obj;
            for (const std::string_view step : steps) {
                const auto* next = obj.resolve(*current, step);
                if (next == nullptr) return std::nullopt;
                current = next;
            }
            return *current;
        };

#if NDOF_EXCEPTIONS_ENABLED
        try {
            return query(receiving_node);
        } catch (...) {
            return std::nullopt;
        }
#else
        return query(receiving_node);
#endif

    }

    
    result_type query(const object_t& root) const { return (*this)(root); }

private:
    // Resolves a single path step against an object child by name.
 
    static const ndof::object<OtherCharT, OtherTraits, OtherAllocator>* resolve(
        const ndof::object<Char, OtherTraits, OtherAllocator>& obj,
        std::string_view name) {
        return obj.find(name);
    }   
 
};

template <xpath_literal Path>
inline constexpr object_query<Path> make_object_query() noexcept {
    return object_query<Path>{};
}

} // namespace ndof

#define NDOF_XPATH(literal) ::ndof::object_query<literal>{}

#endif // NDOF_ERROR_OBJECT_QUERY_HPP
