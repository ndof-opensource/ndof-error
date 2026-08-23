#include "ndof/error/allocator_support.hpp"
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>

// TODO: Move these to core.
// TODO: Update to use try/catch blocks if exceptions are enabled, and use std::terminate() if exceptions are disabled.
template<class T>
concept bounded_array = std::is_bounded_array_v<T>;

template<class T>
concept unbounded_array = std::is_unbounded_array_v<T>;

template<class T>
struct deleter_with_allocator {
    using element_type = std::remove_extent_t<T>;
    using pointer = element_type*;

    template<class Alloc>
        requires (!std::same_as<std::remove_cvref_t<Alloc>, deleter_with_allocator> &&
                  ndof::allocator_like<Alloc>)
    explicit deleter_with_allocator(Alloc alloc, std::size_t count = 1)
        : destroy_([alloc = allocator_type<Alloc>(alloc), count](element_type* p) mutable noexcept {
            destroy(alloc, p, count);
        }) {}

    explicit deleter_with_allocator(std::size_t count = 1)
        requires requires(const element_type& object) {
            typename element_type::allocator_type;
            { object.get_allocator() } ->
                std::convertible_to<typename element_type::allocator_type>;
        }
        : destroy_([count](element_type* p) noexcept {
            if (!p)
                return;

            auto alloc = p->get_allocator();
            destroy(alloc, p, count);
        }) {}

    void operator()(element_type* p) noexcept {
        destroy_(p);
    }

private:
    template<class Alloc>
    using allocator_type =
        typename std::allocator_traits<Alloc>::template rebind_alloc<element_type>;

    template<class Alloc>
    static void destroy(Alloc& alloc, element_type* p, std::size_t count) noexcept {
        if (!p)
            return;

        using traits = std::allocator_traits<Alloc>;

        try {
            if constexpr (std::is_array_v<T>) {
                for (std::size_t i = 0; i < count; ++i)
                    traits::destroy(alloc, p + i);

                traits::deallocate(alloc, p, count);
            }
            else {
                traits::destroy(alloc, p);
                traits::deallocate(alloc, p, 1);
            }
        }
        catch (...) {
            // TODO: Consider logging the exception
            // TODO: Perhaps do something more sophisticated than just terminating the program, but for now, we will just terminate.
            std::terminate(); // Terminate the program if an exception is thrown during destruction, as throwing from a destructor can lead to undefined behavior.
        }
    }

    std::function<void(element_type*)> destroy_;
};

template<class T, class Alloc, class... Args>
    requires (!std::is_array_v<T>)
auto make_unique_with_allocator(Alloc alloc, Args&&... args)
{
    using A = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;
    using traits = std::allocator_traits<A>;

    A a{alloc};
    deleter_with_allocator<T> deleter{a};
    T* p = traits::allocate(a, 1);
    try {
        traits::construct(a, p, std::forward<Args>(args)...);
    }
    catch (...) {
        traits::destroy(a, p);
        traits::deallocate(a, p, 1);
        throw;
    }
    return std::unique_ptr<T, deleter_with_allocator<T>>{p, std::move(deleter)};
}

template<class T, class Alloc>
    requires bounded_array<T>
auto make_unique_with_allocator(Alloc alloc)
{
    using element_type = std::remove_extent_t<T>;
    using A = typename std::allocator_traits<Alloc>::template rebind_alloc<element_type>;
    using traits = std::allocator_traits<A>;

    A a{alloc};
    constexpr std::size_t count = std::extent_v<T>;
    deleter_with_allocator<T> deleter{a, count};
    element_type* p = traits::allocate(a, count);
    std::size_t constructed = 0;
    try {
        for (; constructed < count; ++constructed)
            traits::construct(a, p + constructed);
    }
    catch (...) {
        while (constructed != 0)
            traits::destroy(a, p + --constructed);
        traits::deallocate(a, p, count);
        throw;
    }
    return std::unique_ptr<T, deleter_with_allocator<T>>{p, std::move(deleter)};
}

template<class T, class Alloc>
    requires unbounded_array<T>
auto allocate_unique(Alloc alloc, std::size_t count)
{
    using element_type = std::remove_extent_t<T>;
    using A = typename std::allocator_traits<Alloc>::template rebind_alloc<element_type>;
    using traits = std::allocator_traits<A>;

    A a{alloc};
    deleter_with_allocator<T> deleter{a, count};
    element_type* p = traits::allocate(a, count);
    std::size_t constructed = 0;
    try {
        for (; constructed < count; ++constructed)
            traits::construct(a, p + constructed);
    }
    catch (...) {
        while (constructed != 0)
            traits::destroy(a, p + --constructed);
        traits::deallocate(a, p, count);
        throw;
    }
    return std::unique_ptr<T, deleter_with_allocator<T>>{p, std::move(deleter)};
}