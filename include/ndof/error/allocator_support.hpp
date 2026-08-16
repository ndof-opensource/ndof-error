 
#include "ndof/error/configs.hpp"
#include <concepts>
#include <expected>
#include <memory>
// TODO: Conditionally include.
#include <type_traits>
 


// Note: GitHub Copilot Pro is designed for individuals who want more flexibility. 
//       This paid plan includes unlimited completions, access to a selection of models, Copilot cloud agent, 
//       and a monthly allowance of AI credits. 
//       Verified teachers, and maintainers of popular open source projects may be eligible for free access.
//                ^^^^^^^^      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Wrapping ndof exceptions as inner exceptions preserves a rethrow chain that can
// later be rendered as a stack trace.

// TODO: All exceptions should have an inner exception. Remove the link between the conditional exceptions and inherit from std::exception instead.
//      Change the optional return type of captured_exception() to std::exception_ptr instead of std::optional<std::exception_ptr>.
//      This will allow for a more consistent exception handling model and make it easier to propagate exceptions through different layers of the application.

// TODO: Does exception need to be allocator aware? If not, remove the allocator stuff from the base classes as necessary.


namespace ndof {


template<typename Allocator>
concept has_allocator_definitions = requires {
    typename std::allocator_traits<Allocator>::value_type;
    typename std::allocator_traits<Allocator>::pointer;
    typename std::allocator_traits<Allocator>::const_pointer;
    typename std::allocator_traits<Allocator>::void_pointer;
    typename std::allocator_traits<Allocator>::const_void_pointer;
};


template<typename T>
concept allocator_value_gettable =
    requires(const T& value) {
        typename T::allocator_type;
        { value.get_value() } -> std::same_as<typename T::allocator_type>;
};

template<typename T>
concept allocator_like =
    requires(const T& value) {
        typename T::value_type;
    } 
    && std::is_default_constructible_v<T> 
    && std::is_copy_constructible_v<T> 
    && std::is_copy_assignable_v<T> 
    && std::is_move_constructible_v<T>
    && std::is_move_assignable_v<T>
    && std::is_destructible_v<T>
    && std::equality_comparable<T>;

// TODO: add has a get_value check here somehow.

template<typename Allocator, typename ValueType>
concept allocator_for = 
    ndof::allocator_like<Allocator> &&
    std::same_as<typename std::allocator_traits<Allocator>::value_type, ValueType>;

template<typename Allocator, typename ExpectedAllocator>
concept allocator_compatible_with =
    ndof::allocator_like<Allocator> &&
    ndof::allocator_like<ExpectedAllocator> &&
    std::convertible_to<
        typename std::allocator_traits<Allocator>::value_type,
        typename std::allocator_traits<ExpectedAllocator>::value_type> &&
    std::constructible_from<ExpectedAllocator, const Allocator&>;

template<typename T>
concept allocator_aware_copy_propagating =
    requires {
        typename T::allocator_type;
        typename std::allocator_traits<typename T::allocator_type>::propagate_on_container_copy_assignment;
    } && std::allocator_traits<typename T::allocator_type>::propagate_on_container_copy_assignment::value;

template<typename T>
concept allocator_aware_move_propagating =
    requires {
        typename T::allocator_type;
        typename std::allocator_traits<typename T::allocator_type>::propagate_on_container_move_assignment;
    } && std::allocator_traits<typename T::allocator_type>::propagate_on_container_move_assignment::value;
 
} // namespace ndof