#include "ndof/error/configs.hpp"
#include <concepts>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>


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
// TODO: move this to the core library.
using check_mode = build_mode;

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
        typename T::pointer;
        typename T::const_pointer;
        typename T::void_pointer;
        typename T::const_void_pointer;
    } 
    && std::is_default_constructible_v<T> 
    && std::is_copy_constructible_v<T> 
    && std::is_copy_assignable_v<T> 
    && std::is_destructible_v<T>;

// TODO: add has a get_value check here somehow.

template<typename Allocator, typename ValueType>
concept allocator_for =
    ndof::allocator_like<Allocator> &&
    std::same_as<typename std::allocator_traits<Allocator>::value_type, ValueType>;

template<typename Allocator, typename ExpectedAllocator>
concept allocator_compatible_with =
    ndof::allocator_like<Allocator> &&
    ndof::allocator_like<ExpectedAllocator> &&
    std::same_as<
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

namespace ndof::error {

struct exception_tag {
};



namespace detail {

template<typename T, allocator_like Allocator>
requires allocator_aware_copy_propagating<T>
    && allocator_compatible_with<Allocator, typename T::allocator_type>
[[nodiscard]] T copy_considering_allocators(const T& value, const Allocator& allocator) {
    if constexpr (std::is_constructible_v<T, std::allocator_arg_t, const Allocator&, const T&>) {
        return T(std::allocator_arg, allocator, value);
    } else if constexpr (std::is_constructible_v<T, const T&, const Allocator&>) {
        return T(value, allocator);
    } else {
        return T(value);
    }
}

template<typename T, allocator_like Allocator>
requires (!allocator_aware_copy_propagating<T>)
[[nodiscard]] T copy_considering_allocators(const T& value, [[maybe_unused]] const Allocator& allocator) {
    return value;
}

template<typename T, allocator_like Allocator>
requires allocator_aware_move_propagating<std::remove_cvref_t<T>>
      && allocator_compatible_with<Allocator, typename std::remove_cvref_t<T>::allocator_type>
[[nodiscard]] std::remove_cvref_t<T> move_considering_allocators(T&& value, const Allocator& allocator) {
    using value_type = std::remove_cvref_t<T>;
    if constexpr (std::is_constructible_v<value_type, std::allocator_arg_t, const Allocator&, value_type&&>) {
        return value_type(std::allocator_arg, allocator, std::forward<T>(value));
    } else if constexpr (std::is_constructible_v<value_type, value_type&&, const Allocator&>) {
        return value_type(std::forward<T>(value), allocator);
    } else {
        return value_type(std::forward<T>(value));
    }
}

template<typename T, allocator_like Allocator>
requires (!allocator_aware_move_propagating<std::remove_cvref_t<T>>)
[[nodiscard]] std::remove_cvref_t<T> move_considering_allocators(T&& value, [[maybe_unused]] const Allocator& allocator) {
    // Question: Should this silently ignore the allocator?
    return std::remove_cvref_t<T>(std::forward<T>(value));
}

template<typename CharT, typename Traits, allocator_for<CharT> SourceAllocator, allocator_for<CharT> DestinationAllocator>
[[nodiscard]] std::basic_string<CharT, Traits, DestinationAllocator> copy_considering_allocators(
    const std::basic_string<CharT, Traits, SourceAllocator>& value,
    const DestinationAllocator& allocator) {
    return std::basic_string<CharT, Traits, DestinationAllocator>(value.data(), value.size(), allocator);
}

template<typename CharT, typename Traits, allocator_for<CharT> SourceAllocator, allocator_for<CharT> DestinationAllocator>
[[nodiscard]] std::basic_string<CharT, Traits, DestinationAllocator> move_considering_allocators(
    std::basic_string<CharT, Traits, SourceAllocator>&& value,
    const DestinationAllocator& allocator) {
    if constexpr ((std::is_same_v<SourceAllocator, DestinationAllocator>) 
    && (value.get_allocator() == allocator)) {
        return std::basic_string<CharT, Traits, DestinationAllocator>(std::move(value));
    }
    return std::basic_string<CharT, Traits, DestinationAllocator>(value.data(), value.size(), allocator);
}

template<typename CharT, allocator_for<CharT> Allocator>
requires allocator_for<Allocator, CharT>
std::basic_string<CharT, std::char_traits<CharT>, Allocator>
make_string_from_narrow(const char* text, const Allocator& allocator) {
    std::basic_string<CharT, std::char_traits<CharT>, Allocator> output(allocator);
    const std::string_view text_view = (text == nullptr) ? std::string_view{} : std::string_view{text};
    output.reserve(text_view.size());
    for (const char ch : text_view) {
        output.push_back(static_cast<CharT>(ch));
    }
    return output;
}

template<typename T>
concept ndof_exception_derived =
    requires { requires std::derived_from<std::remove_cvref_t<T>, exception_tag>; };

} // namespace detail

// TODO: should define stream to object operators.
 

 
struct basic_exception : std::exception, exception_tag {
public:
    basic_exception() = delete;
 
    // TODO: Review these.
    basic_exception(const basic_exception&) = default;
    basic_exception(basic_exception&&) = default;
    basic_exception& operator=(const basic_exception&) = default;
    basic_exception& operator=(basic_exception&&) = default;

    ~basic_exception() = default;

    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] const char* file_name() const noexcept;
    [[nodiscard]] const char* function_name() const noexcept;
    [[nodiscard]] std::uint_least32_t line() const noexcept;
 
private:
    std::source_location location_;
    std::optional<std::exception_ptr> captured_exception_;
};

template<typename ExceptionType, typename CharT = char, ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
struct basic_explicit_inner_exception : basic_exception {
    explicit basic_explicit_inner_exception(const Allocator& allocator = Allocator());

    template<typename ForwardedException>
    requires std::is_same_v<std::remove_cvref_t<ForwardedException>, ExceptionType>
    explicit basic_explicit_inner_exception(
        ForwardedException&& exception,
        const std::source_location& source_location_value,
        const Allocator& allocator = Allocator());
};

template<typename ExceptionType, typename CharT = char, ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
struct basic_condition_check_exception : basic_explicit_inner_exception<ExceptionType, CharT, Allocator> {
    using allocated_string = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

    basic_condition_check_exception() = delete;
    basic_condition_check_exception(const basic_condition_check_exception&) = default;
    basic_condition_check_exception(basic_condition_check_exception&&) = default;
    basic_condition_check_exception& operator=(const basic_condition_check_exception&) = default;
    basic_condition_check_exception& operator=(basic_condition_check_exception&&) = default;
    ~basic_condition_check_exception() = default;

    [[nodiscard]] const allocated_string& expression() const noexcept;
    [[nodiscard]] const allocated_string& message() const noexcept;
    [[nodiscard]] ndof::check_mode check_mode() const noexcept;
    [[nodiscard]] const char* what() const noexcept override;
 

protected:
    template<ndof::allocator_for<CharT> OtherAllocator>
    basic_condition_check_exception(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::source_location& location_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        ndof::check_mode check_mode_value,
        const Allocator& allocator = Allocator());

private:
    allocated_string expression_;
    allocated_string message_;
    ndof::check_mode check_mode_;
};

template<typename ExceptionType, typename CharT = char, ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
struct basic_precondition_check_exception : basic_condition_check_exception<ExceptionType, CharT, Allocator> {
    basic_precondition_check_exception() = default;
    basic_precondition_check_exception(const basic_precondition_check_exception&) = default;
    basic_precondition_check_exception(basic_precondition_check_exception&&) = default;
    basic_precondition_check_exception& operator=(const basic_precondition_check_exception&) = default;
    basic_precondition_check_exception& operator=(basic_precondition_check_exception&&) = default;
    ~basic_precondition_check_exception() = default;

    template<ndof::allocator_for<CharT> OtherAllocator>
    basic_precondition_check_exception(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value,
        ndof::check_mode check_mode_value,
        const Allocator& allocator = Allocator());

 
};

template<typename ExceptionType, typename CharT = char, ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
struct basic_postcondition_check_exception : basic_condition_check_exception<ExceptionType, CharT, Allocator> {
    basic_postcondition_check_exception() = default;
    basic_postcondition_check_exception(const basic_postcondition_check_exception&) = default;
    basic_postcondition_check_exception(basic_postcondition_check_exception&&) = default;
    basic_postcondition_check_exception& operator=(const basic_postcondition_check_exception&) = default;
    basic_postcondition_check_exception& operator=(basic_postcondition_check_exception&&) = default;
    ~basic_postcondition_check_exception() = default;

    template<ndof::allocator_for<CharT> OtherAllocator>
    basic_postcondition_check_exception(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value,
        ndof::check_mode check_mode_value,
        const Allocator& allocator = Allocator());
};

template<typename ExceptionType, typename CharT = char, ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
struct basic_invariant_condition_check_exception : basic_condition_check_exception<ExceptionType, CharT, Allocator> {
    basic_invariant_condition_check_exception() = default;
    basic_invariant_condition_check_exception(const basic_invariant_condition_check_exception&) = default;
    basic_invariant_condition_check_exception(basic_invariant_condition_check_exception&&) = default;
    basic_invariant_condition_check_exception& operator=(const basic_invariant_condition_check_exception&) = default;
    basic_invariant_condition_check_exception& operator=(basic_invariant_condition_check_exception&&) = default;
    ~basic_invariant_condition_check_exception() = default;

    template<ndof::allocator_for<CharT> OtherAllocator>
    basic_invariant_condition_check_exception(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value,
        ndof::check_mode check_mode_value,
        const Allocator& allocator = Allocator());

 
};

template<typename ExceptionType, typename CharT = char, ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
void throw_exception(
    ExceptionType&& exception,
    std::source_location source_location_value = std::source_location::current(),
    const Allocator& allocator = Allocator()) {

    using captured_exception_type = std::remove_cvref_t<ExceptionType>;
    if constexpr (detail::ndof_exception_derived<captured_exception_type>) {
        throw basic_explicit_inner_exception<captured_exception_type, CharT, Allocator>(
            std::forward<ExceptionType>(exception), 
            source_location_value,
            allocator);
    } else {
        throw basic_explicit_inner_exception<captured_exception_type, CharT, Allocator>(
            std::forward<ExceptionType>(exception),
            allocator);
    }
}
 
// TODO: Reinvestigate how to hide the allocator from the user.
//       Manage the allocator the user passed in internally. 

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
template<typename ForwardedException>
requires std::is_same_v<std::remove_cvref_t<ForwardedException>, ExceptionType>
basic_explicit_inner_exception<ExceptionType, CharT, Allocator>::basic_explicit_inner_exception(
    ForwardedException&& exception,
    [[maybe_unused]] const std::source_location& source_location_value,
    const Allocator& allocator)
    : basic_exception(std::forward<ForwardedException>(exception), allocator) {
        // TODO: Fix the initialization of the base class, vis-a-vis the source_location_value. The base class constructor does not currently accept a source location parameter, which is needed to properly initialize the exception with the correct context.
     
}

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
const typename basic_condition_check_exception<ExceptionType, CharT, Allocator>::allocated_string&
basic_condition_check_exception<ExceptionType, CharT, Allocator>::expression() const noexcept {
    return expression_;
}

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
const typename basic_condition_check_exception<ExceptionType, CharT, Allocator>::allocated_string&
basic_condition_check_exception<ExceptionType, CharT, Allocator>::message() const noexcept {
    return message_;
}

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
ndof::check_mode basic_condition_check_exception<ExceptionType, CharT, Allocator>::check_mode() const noexcept {
    return check_mode_;
}

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
const char* basic_condition_check_exception<ExceptionType, CharT, Allocator>::what() const noexcept {
    // TODO: Implement this properly.
    //       The current implementation simply returns the base class's what() message, which is not informative enough for condition check exceptions. A more detailed message should be constructed that includes the expression, message, and check mode information.
    return basic_exception<CharT, Allocator>::what();
}

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
template<ndof::allocator_for<CharT> OtherAllocator>
basic_condition_check_exception<ExceptionType, CharT, Allocator>::basic_condition_check_exception(
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
    // TODO: Fix this.
    [[maybe_unused]] const std::source_location& location_value,
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
    ndof::check_mode check_mode_value,
    const Allocator& allocator)
    : basic_explicit_inner_exception<ExceptionType, CharT, Allocator>(allocator),
            expression_(detail::copy_considering_allocators(expression_value, allocator)),
            message_(detail::copy_considering_allocators(message_value, allocator)),
      check_mode_(check_mode_value) {
}

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
template<ndof::allocator_for<CharT> OtherAllocator>
basic_precondition_check_exception<ExceptionType, CharT, Allocator>::basic_precondition_check_exception(
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
    const std::source_location& location_value,
    ndof::check_mode check_mode_value,
    const Allocator& allocator)
    : basic_condition_check_exception<ExceptionType, CharT, Allocator>(
          expression_value,
          location_value,
          message_value,
          check_mode_value,
          allocator) {
}

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
template<ndof::allocator_for<CharT> OtherAllocator>
basic_postcondition_check_exception<ExceptionType, CharT, Allocator>::basic_postcondition_check_exception(
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
    const std::source_location& location_value,
    ndof::check_mode check_mode_value,
    const Allocator& allocator)
    : basic_condition_check_exception<ExceptionType, CharT, Allocator>(
          expression_value,
          location_value,
          message_value,
          check_mode_value,
          allocator) {
}

template<typename ExceptionType, typename CharT, ndof::allocator_for<CharT> Allocator>
template<ndof::allocator_for<CharT> OtherAllocator>
basic_invariant_condition_check_exception<ExceptionType, CharT, Allocator>::basic_invariant_condition_check_exception(
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
    const std::source_location& location_value,
    ndof::check_mode check_mode_value,
    const Allocator& allocator)
    : basic_condition_check_exception<ExceptionType, CharT, Allocator>(
          expression_value,
          location_value,
          message_value,
          check_mode_value,
          allocator) {
}
 

 
}