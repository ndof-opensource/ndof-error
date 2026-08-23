#include <exception>
#include <memory>
#include <type_traits>

// TODO: Move these to core.
template<class T>
concept bounded_array = std::is_bounded_array_v<T>;

template<class T>
concept unbounded_array = std::is_unbounded_array_v<T>;

template<class T, class Alloc>
struct allocator_deleter {
    using element_type = std::remove_extent_t<T>;

    using allocator_type =
        typename std::allocator_traits<Alloc>::template rebind_alloc<element_type>;

    allocator_type alloc;
    std::size_t count = 1;

    void operator()(element_type* p) noexcept {
        if (!p)
            return;

        using traits = std::allocator_traits<allocator_type>;

        try{
            if constexpr (unbounded_array<T>) {
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
};

template<class T, class Alloc, class... Args>
auto allocate_unique(Alloc alloc, Args&&... args)
{
    using A = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

    using traits = std::allocator_traits<A>;

    A a{alloc};

    T* p = traits::allocate(a, 1);

    try {
        traits::construct(a, p, std::forward<Args>(args)...);
    }
    catch (...) {
        traits::deallocate(a, p, 1);
        throw;
    }

    return std::unique_ptr<T, allocator_deleter<T, Alloc>>{p, allocator_deleter<T, Alloc>{a}};
}