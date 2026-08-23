#include "ndof/error/configs.hpp"

#include <cstdint>
#include <exception>
#include <expected>
#include <memory>
#include <type_traits>

// TODO: Move these to core.
// TODO: Update to use try/catch blocks if exceptions are enabled, and use std::terminate() if exceptions are disabled.
template<class T>
concept bounded_array = std::is_bounded_array_v<T>;

template<class T>
concept unbounded_array = std::is_unbounded_array_v<T>;

enum class allocation_error : std::uint8_t {
    allocation_failed
};

namespace allocation_detail {

template<class T, class Alloc>
void destroy(Alloc& alloc, std::remove_extent_t<T>* pointer, std::size_t count) {
    using traits = std::allocator_traits<Alloc>;

    if constexpr (std::is_array_v<T>) {
        for (std::size_t i = 0; i < count; ++i)
            traits::destroy(alloc, pointer + i);

        traits::deallocate(alloc, pointer, count);
    }
    else {
        traits::destroy(alloc, pointer);
        traits::deallocate(alloc, pointer, 1);
    }
}

#if NDOF_EXCEPTIONS_ENABLED
template<class T, class Alloc>
void destroy_or_terminate(
    Alloc& alloc, std::remove_extent_t<T>* pointer, std::size_t count) noexcept {
    try {
        destroy<T>(alloc, pointer, count);
    }
    catch (...) {
        std::terminate();
    }
}

template<class T, class Alloc, class... Args>
void construct_or_rethrow(Alloc& alloc, T* pointer, Args&&... args) {
    using traits = std::allocator_traits<Alloc>;

    try {
        traits::construct(alloc, pointer, std::forward<Args>(args)...);
    }
    catch (...) {
        traits::deallocate(alloc, pointer, 1);
        throw;
    }
}

template<class T, class Alloc>
void construct_array_or_rethrow(Alloc& alloc, T* pointer, std::size_t count) {
    using traits = std::allocator_traits<Alloc>;
    std::size_t constructed = 0;

    try {
        for (; constructed < count; ++constructed)
            traits::construct(alloc, pointer + constructed);
    }
    catch (...) {
        while (constructed != 0)
            traits::destroy(alloc, pointer + --constructed);
        traits::deallocate(alloc, pointer, count);
        throw;
    }
}
#else
template<class...>
inline constexpr bool dependent_false = false;

template<class T, class Alloc>
void destroy_or_terminate(
    Alloc&, std::remove_extent_t<T>*, std::size_t) noexcept {
    static_assert(dependent_false<T>,
                  "destroy_or_terminate requires exception support");
}

template<class T, class Alloc, class... Args>
void construct_or_rethrow(Alloc&, T*, Args&&...) {
    static_assert(dependent_false<T>,
                  "construct_or_rethrow requires exception support");
}

template<class T, class Alloc>
void construct_array_or_rethrow(Alloc&, T*, std::size_t) {
    static_assert(dependent_false<T>,
                  "construct_array_or_rethrow requires exception support");
}
#endif

} // namespace allocation_detail

template<class T, class Alloc >
struct deleter_with_allocator {
    using element_type = std::remove_extent_t<T>;
    using pointer = element_type*;

    using allocator_type =
        typename std::allocator_traits<Alloc>::template rebind_alloc<element_type>;

    
    [[no_unique_address]] allocator_type alloc;
    std::size_t count = 1;

    void operator()(element_type* p) noexcept
        requires (ndof::exceptions_feature_enabled())
    {
        if (!p)
            return;

        allocation_detail::destroy_or_terminate<T>(alloc, p, count);
    }

    void operator()(element_type* p) noexcept
        requires (!ndof::exceptions_feature_enabled())
    {
        if (!p)
            return;

        allocation_detail::destroy<T>(alloc, p, count);
    }
};

template<class T, class Alloc>
using allocated_unique_t = std::unique_ptr<T, deleter_with_allocator<T, Alloc>>;

template<class T, class Alloc>
using allocation_result_t = std::conditional_t<
    ndof::exceptions_feature_enabled(),
    allocated_unique_t<T, Alloc>,
    std::expected<allocated_unique_t<T, Alloc>, allocation_error>>;

template<class T, class Alloc, class... Args>
    requires (!std::is_array_v<T>)
auto make_unique_with_allocator(Alloc alloc, Args&&... args)
    -> allocation_result_t<T, Alloc>
{
    using A = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;
    using traits = std::allocator_traits<A>;

    A a{alloc};
    T* p = traits::allocate(a, 1);

    if constexpr (ndof::exceptions_feature_enabled()) {
        allocation_detail::construct_or_rethrow(
            a, p, std::forward<Args>(args)...);
    }
    else {
        if (!p)
            return std::unexpected(allocation_error::allocation_failed);
        traits::construct(a, p, std::forward<Args>(args)...);
    }

    return allocated_unique_t<T, Alloc>{
        p, deleter_with_allocator<T, Alloc>{a}};
}

template<class T, class Alloc>
    requires bounded_array<T>
auto make_unique_with_allocator(Alloc alloc)
    -> allocation_result_t<T, Alloc>
{
    using element_type = std::remove_extent_t<T>;
    using A = typename std::allocator_traits<Alloc>::template rebind_alloc<element_type>;
    using traits = std::allocator_traits<A>;

    A a{alloc};
    constexpr std::size_t count = std::extent_v<T>;
    element_type* p = traits::allocate(a, count);

    if constexpr (ndof::exceptions_feature_enabled()) {
        allocation_detail::construct_array_or_rethrow(a, p, count);
    }
    else {
        if (!p)
            return std::unexpected(allocation_error::allocation_failed);
        for (std::size_t constructed = 0; constructed < count; ++constructed)
            traits::construct(a, p + constructed);
    }

    return allocated_unique_t<T, Alloc>{
        p, deleter_with_allocator<T, Alloc>{a, count}};
}

template<class T, class Alloc>
    requires unbounded_array<T>
auto make_unique_with_allocator(Alloc alloc, std::size_t count)
    -> allocation_result_t<T, Alloc>
{
    using element_type = std::remove_extent_t<T>;
    using A = typename std::allocator_traits<Alloc>::template rebind_alloc<element_type>;
    using traits = std::allocator_traits<A>;

    A a{alloc};
    element_type* p = traits::allocate(a, count);

    if constexpr (ndof::exceptions_feature_enabled()) {
        allocation_detail::construct_array_or_rethrow(a, p, count);
    }
    else {
        if (!p && count != 0)
            return std::unexpected(allocation_error::allocation_failed);
        for (std::size_t constructed = 0; constructed < count; ++constructed)
            traits::construct(a, p + constructed);
    }

    return allocated_unique_t<T, Alloc>{
        p, deleter_with_allocator<T, Alloc>{a, count}};
}