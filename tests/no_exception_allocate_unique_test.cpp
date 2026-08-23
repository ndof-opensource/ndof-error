#ifndef NDOF_EXCEPTIONS_ENABLED
#define NDOF_EXCEPTIONS_ENABLED 0
#endif

#include "ndof/error/allocate_unique.hpp"

#include <concepts>
#include <cstddef>
#include <expected>
#include <memory>
#include <utility>

template<class T>
struct null_allocator {
    using value_type = T;

    null_allocator() = default;

    template<class U>
    null_allocator(const null_allocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t) noexcept {
        return nullptr;
    }

    void deallocate(T*, std::size_t) noexcept {}
};

using pointer_type = std::unique_ptr<int, deleter_with_allocator<int, null_allocator<int>>>;
using result_type = decltype(
    make_unique_with_allocator<int>(std::declval<null_allocator<int>>(), 42));

static_assert(std::same_as<result_type, std::expected<pointer_type, allocation_error>>);
static_assert(!ndof::exceptions_feature_enabled());

int main() {
    auto result = make_unique_with_allocator<int>(null_allocator<int>{}, 42);
    return !result && result.error() == allocation_error::allocation_failed ? 0 : 1;
}
