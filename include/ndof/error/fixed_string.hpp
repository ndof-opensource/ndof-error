#ifndef NDOF_ERROR_FIXED_STRING_HPP
#define NDOF_ERROR_FIXED_STRING_HPP

#include <cstddef>
#include <array>
#include <string_view>
#include <algorithm>

namespace ndof {

template <typename CharT, std::size_t N>
struct fixed_string {
 
    constexpr fixed_string() = delete;

    constexpr fixed_string(const CharT (&str)[N]) {
        std::copy_n(str, N, data.begin());
    }

    [[nodiscard]] constexpr std::size_t length() const {
        return N-1;
    }

    constexpr bool operator==(const fixed_string& other) const {
        return std::equal(data.begin(), data.begin() + length(),
                         other.data.begin(), other.data.begin() + other.length());
    }

    [[nodiscard]] constexpr std::string_view view() const {
        return std::string_view(data.data(), length());
    }

    private:
    std::array<CharT, N> data{};
};

// Deduction guide for fixed_string
template <typename CharT, std::size_t N>
fixed_string(const CharT (&)[N]) -> fixed_string<CharT, N>;

}
#endif

