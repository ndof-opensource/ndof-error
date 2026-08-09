#include "ndof/error/configs.hpp"
#include <concepts>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// Wrapping ndof exceptions as inner exceptions preserves a rethrow chain that can
// later be rendered as a stack trace.

namespace ndof {
    using CheckMode = BuildMode;
}

namespace ndof::error {

struct exception_marker {
};

struct inner_exception_marker : exception_marker {
};

template<typename CharT, typename Allocator>
struct Exception;

template<typename CharT, typename Allocator>
struct InnerException;

namespace detail {

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

template<typename T, typename Allocator>
requires allocator_aware_copy_propagating<T>
[[nodiscard]] T copy_considering_allocators(const T& value, const Allocator& allocator) {
    if constexpr (std::is_constructible_v<T, std::allocator_arg_t, const Allocator&, const T&>) {
        return T(std::allocator_arg, allocator, value);
    } else if constexpr (std::is_constructible_v<T, const T&, const Allocator&>) {
        return T(value, allocator);
    } else {
        return T(value);
    }
}

template<typename T, typename Allocator>
[[nodiscard]] T copy_considering_allocators(const T& value, [[maybe_unused]] const Allocator& allocator) {
    return value;
}

template<typename T, typename Allocator>
requires allocator_aware_move_propagating<std::remove_cvref_t<T>>
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

template<typename T, typename Allocator>
[[nodiscard]] std::remove_cvref_t<T> move_considering_allocators(T&& value, [[maybe_unused]] const Allocator& allocator) {
    return std::remove_cvref_t<T>(std::forward<T>(value));
}

template<typename CharT, typename Traits, typename SourceAllocator, typename DestinationAllocator>
[[nodiscard]] std::basic_string<CharT, Traits, DestinationAllocator> copy_considering_allocators(
    const std::basic_string<CharT, Traits, SourceAllocator>& value,
    const DestinationAllocator& allocator) {
    return std::basic_string<CharT, Traits, DestinationAllocator>(value.data(), value.size(), allocator);
}

template<typename CharT, typename Traits, typename SourceAllocator, typename DestinationAllocator>
[[nodiscard]] std::basic_string<CharT, Traits, DestinationAllocator> move_considering_allocators(
    std::basic_string<CharT, Traits, SourceAllocator>&& value,
    const DestinationAllocator& allocator) {
    if constexpr (std::is_same_v<SourceAllocator, DestinationAllocator>) {
        if (value.get_allocator() == allocator) {
            return std::basic_string<CharT, Traits, DestinationAllocator>(std::move(value));
        }
    }
    return std::basic_string<CharT, Traits, DestinationAllocator>(value.data(), value.size(), allocator);
}

template<typename CharT, typename Allocator>
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
concept inner_exception_derived =
    requires { requires std::derived_from<std::remove_cvref_t<T>, inner_exception_marker>; };

template<typename T>
concept ndof_exception_derived =
    requires { requires std::derived_from<std::remove_cvref_t<T>, exception_marker>; };

} // namespace detail

// Note: should define stream to object operators.
//       In that case, to_string() can be removed and replaced with stream operator.

template<typename CharT = char, typename Allocator = std::allocator<CharT>>
struct Exception : std::exception, exception_marker {
public:
    using allocated_string = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

    Exception() = delete;

    Exception(
        const std::source_location& source_location_value = std::source_location::current(),
        const Allocator& allocator = Allocator());

    Exception(const Exception&) = default;
    Exception(Exception&&) = default;
    Exception& operator=(const Exception&) = default;
    Exception& operator=(Exception&&) = default;

    ~Exception() = default;

    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] const allocated_string& get_file_name() const noexcept;
    [[nodiscard]] const char* get_function_name() const noexcept;
    [[nodiscard]] std::uint_least32_t get_line() const noexcept;
    [[nodiscard]] BuildMode get_build_mode() const noexcept;
    [[nodiscard]] const allocated_string& get_message() const noexcept;

protected:
    [[nodiscard]] const std::optional<std::exception_ptr>& get_captured_exception() const noexcept;

private:
    std::source_location location;
    allocated_string file_name;
    allocated_string message;
    std::optional<std::exception_ptr> captured_exception;
};

template<typename CharT = char, typename Allocator = std::allocator<CharT>>
struct InnerException : Exception<CharT, Allocator>, inner_exception_marker {
    using allocated_string = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

    explicit InnerException(const Allocator& allocator = Allocator());

    template<typename CapturedException>
    requires (!std::is_same_v<std::remove_cvref_t<CapturedException>, InnerException<CharT, Allocator>>)
    explicit InnerException(CapturedException&& exception, const Allocator& allocator = Allocator());

    InnerException(const InnerException&) = default;
    InnerException(InnerException&&) = default;
    InnerException& operator=(const InnerException&) = default;
    InnerException& operator=(InnerException&&) = default;
    ~InnerException() = default;

    [[nodiscard]] const char* what() const noexcept override;

protected:
    [[nodiscard]] const std::exception_ptr& get_inner_captured_exception() const noexcept;

private:
    std::exception_ptr inner_captured_exception;
};

template<typename ExceptionType, typename CharT = char, typename Allocator = std::allocator<CharT>>
struct ExplicitInnerException : InnerException<CharT, Allocator> {
    explicit ExplicitInnerException(const Allocator& allocator = Allocator());

    template<typename ForwardedException>
    requires std::is_same_v<std::remove_cvref_t<ForwardedException>, ExceptionType>
    explicit ExplicitInnerException(
        ForwardedException&& exception,
        const Allocator& allocator = Allocator());
};

template<typename ExceptionType, typename CharT = char, typename Allocator = std::allocator<CharT>>
struct ConditionCheckException : ExplicitInnerException<ExceptionType, CharT, Allocator> {
    using allocated_string = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

    ConditionCheckException() = delete;
    ConditionCheckException(const ConditionCheckException&) = default;
    ConditionCheckException(ConditionCheckException&&) = default;
    ConditionCheckException& operator=(const ConditionCheckException&) = default;
    ConditionCheckException& operator=(ConditionCheckException&&) = default;
    ~ConditionCheckException() = default;

    [[nodiscard]] const allocated_string& get_expression() const noexcept;
    [[nodiscard]] const allocated_string& get_message() const noexcept;
    [[nodiscard]] CheckMode get_check_mode() const noexcept;
    [[nodiscard]] const char* what() const noexcept override;
 

protected:
    template<typename OtherAllocator>
    ConditionCheckException(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::source_location& location_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        CheckMode check_mode_value,
        const Allocator& allocator = Allocator());

private:
    allocated_string expression;
    allocated_string message;
    CheckMode check_mode;
};

template<typename ExceptionType, typename CharT = char, typename Allocator = std::allocator<CharT>>
struct PreConditionCheckException : ConditionCheckException<ExceptionType, CharT, Allocator> {
    PreConditionCheckException() = default;
    PreConditionCheckException(const PreConditionCheckException&) = default;
    PreConditionCheckException(PreConditionCheckException&&) = default;
    PreConditionCheckException& operator=(const PreConditionCheckException&) = default;
    PreConditionCheckException& operator=(PreConditionCheckException&&) = default;
    ~PreConditionCheckException() = default;

    template<typename OtherAllocator>
    PreConditionCheckException(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value,
        CheckMode check_mode_value,
        const Allocator& allocator = Allocator());

 
};

template<typename ExceptionType, typename CharT = char, typename Allocator = std::allocator<CharT>>
struct PostConditionCheckException : ConditionCheckException<ExceptionType, CharT, Allocator> {
    PostConditionCheckException() = default;
    PostConditionCheckException(const PostConditionCheckException&) = default;
    PostConditionCheckException(PostConditionCheckException&&) = default;
    PostConditionCheckException& operator=(const PostConditionCheckException&) = default;
    PostConditionCheckException& operator=(PostConditionCheckException&&) = default;
    ~PostConditionCheckException() = default;

    template<typename OtherAllocator>
    PostConditionCheckException(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value,
        CheckMode check_mode_value,
        const Allocator& allocator = Allocator());
};

template<typename ExceptionType, typename CharT = char, typename Allocator = std::allocator<CharT>>
struct InvariantConditionCheckException : ConditionCheckException<ExceptionType, CharT, Allocator> {
    InvariantConditionCheckException() = default;
    InvariantConditionCheckException(const InvariantConditionCheckException&) = default;
    InvariantConditionCheckException(InvariantConditionCheckException&&) = default;
    InvariantConditionCheckException& operator=(const InvariantConditionCheckException&) = default;
    InvariantConditionCheckException& operator=(InvariantConditionCheckException&&) = default;
    ~InvariantConditionCheckException() = default;

    template<typename OtherAllocator>
    InvariantConditionCheckException(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value,
        CheckMode check_mode_value,
        const Allocator& allocator = Allocator());

 
};

template<typename ExceptionType, typename CharT = char, typename Allocator = std::allocator<CharT>>
void throw_ndof_exception(
    ExceptionType&& exception,
    const Allocator& allocator = Allocator()) {

    using captured_exception_type = std::remove_cvref_t<ExceptionType>;
    if constexpr (detail::ndof_exception_derived<captured_exception_type>) {
        throw ExplicitInnerException<captured_exception_type, CharT, Allocator>(
            std::forward<ExceptionType>(exception),
            allocator);
    } else if constexpr (detail::inner_exception_derived<captured_exception_type>) {
        throw std::forward<ExceptionType>(exception);
    } else {
        throw ExplicitInnerException<captured_exception_type, CharT, Allocator>(
            std::forward<ExceptionType>(exception),
            allocator);
    }
}

template<typename CharT, typename Allocator>
Exception<CharT, Allocator>::Exception(
    const std::source_location& source_location_value,
    const Allocator& allocator)
    : location(source_location_value),
      file_name(detail::make_string_from_narrow<CharT, Allocator>(source_location_value.file_name(), allocator)),
      message(detail::make_string_from_narrow<CharT, Allocator>("implement me", allocator)),
      captured_exception(std::nullopt) {
}

template<typename CharT, typename Allocator>
const typename Exception<CharT, Allocator>::allocated_string&
Exception<CharT, Allocator>::get_file_name() const noexcept {
    return file_name;
}

template<typename CharT, typename Allocator>
const char* Exception<CharT, Allocator>::get_function_name() const noexcept {
    return location.function_name();
}

template<typename CharT, typename Allocator>
std::uint_least32_t Exception<CharT, Allocator>::get_line() const noexcept {
    return location.line();
}

template<typename CharT, typename Allocator>
BuildMode Exception<CharT, Allocator>::get_build_mode() const noexcept {
    return getBuildMode();
}

template<typename CharT, typename Allocator>
const typename Exception<CharT, Allocator>::allocated_string&
Exception<CharT, Allocator>::get_message() const noexcept {
    return message;
}

template<typename CharT, typename Allocator>
const std::optional<std::exception_ptr>& Exception<CharT, Allocator>::get_captured_exception() const noexcept {
    return captured_exception;
}

template<typename CharT, typename Allocator>
const char* Exception<CharT, Allocator>::what() const noexcept {
    return "ndof::error::Exception";
}

template<typename CharT, typename Allocator>
InnerException<CharT, Allocator>::InnerException(const Allocator& allocator)
    : Exception<CharT, Allocator>(std::source_location::current(), allocator),
      inner_captured_exception(std::make_exception_ptr(std::runtime_error("no captured exception"))) {
}

template<typename CharT, typename Allocator>
template<typename CapturedException>
requires (!std::is_same_v<std::remove_cvref_t<CapturedException>, InnerException<CharT, Allocator>>)
InnerException<CharT, Allocator>::InnerException(CapturedException&& exception, const Allocator& allocator)
    : Exception<CharT, Allocator>(std::source_location::current(), allocator),
      inner_captured_exception(std::make_exception_ptr(std::forward<CapturedException>(exception))) {
}

template<typename CharT, typename Allocator>
const std::exception_ptr& InnerException<CharT, Allocator>::get_inner_captured_exception() const noexcept {
    return inner_captured_exception;
}

template<typename CharT, typename Allocator>
const char* InnerException<CharT, Allocator>::what() const noexcept {
    return Exception<CharT, Allocator>::what();
}

template<typename ExceptionType, typename CharT, typename Allocator>
ExplicitInnerException<ExceptionType, CharT, Allocator>::ExplicitInnerException(const Allocator& allocator)
    : InnerException<CharT, Allocator>(allocator) {
}

template<typename ExceptionType, typename CharT, typename Allocator>
template<typename ForwardedException>
requires std::is_same_v<std::remove_cvref_t<ForwardedException>, ExceptionType>
ExplicitInnerException<ExceptionType, CharT, Allocator>::ExplicitInnerException(
    ForwardedException&& exception,
    const Allocator& allocator)
    : InnerException<CharT, Allocator>(std::forward<ForwardedException>(exception), allocator) {
}

template<typename ExceptionType, typename CharT, typename Allocator>
const typename ConditionCheckException<ExceptionType, CharT, Allocator>::allocated_string&
ConditionCheckException<ExceptionType, CharT, Allocator>::get_expression() const noexcept {
    return expression;
}

template<typename ExceptionType, typename CharT, typename Allocator>
const typename ConditionCheckException<ExceptionType, CharT, Allocator>::allocated_string&
ConditionCheckException<ExceptionType, CharT, Allocator>::get_message() const noexcept {
    return message;
}

template<typename ExceptionType, typename CharT, typename Allocator>
CheckMode ConditionCheckException<ExceptionType, CharT, Allocator>::get_check_mode() const noexcept {
    return check_mode;
}

template<typename ExceptionType, typename CharT, typename Allocator>
const char* ConditionCheckException<ExceptionType, CharT, Allocator>::what() const noexcept {
    return Exception<CharT, Allocator>::what();
}

template<typename ExceptionType, typename CharT, typename Allocator>
template<typename OtherAllocator>
ConditionCheckException<ExceptionType, CharT, Allocator>::ConditionCheckException(
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
    [[maybe_unused]] const std::source_location& location_value,
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
    CheckMode check_mode_value,
    const Allocator& allocator)
    : ExplicitInnerException<ExceptionType, CharT, Allocator>(allocator),
            expression(detail::copy_considering_allocators(expression_value, allocator)),
            message(detail::copy_considering_allocators(message_value, allocator)),
      check_mode(check_mode_value) {
}

template<typename ExceptionType, typename CharT, typename Allocator>
template<typename OtherAllocator>
PreConditionCheckException<ExceptionType, CharT, Allocator>::PreConditionCheckException(
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
    const std::source_location& location_value,
    CheckMode check_mode_value,
    const Allocator& allocator)
    : ConditionCheckException<ExceptionType, CharT, Allocator>(
          expression_value,
          location_value,
          message_value,
          check_mode_value,
          allocator) {
}

template<typename ExceptionType, typename CharT, typename Allocator>
template<typename OtherAllocator>
PostConditionCheckException<ExceptionType, CharT, Allocator>::PostConditionCheckException(
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
    const std::source_location& location_value,
    CheckMode check_mode_value,
    const Allocator& allocator)
    : ConditionCheckException<ExceptionType, CharT, Allocator>(
          expression_value,
          location_value,
          message_value,
          check_mode_value,
          allocator) {
}

template<typename ExceptionType, typename CharT, typename Allocator>
template<typename OtherAllocator>
InvariantConditionCheckException<ExceptionType, CharT, Allocator>::InvariantConditionCheckException(
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
    const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
    const std::source_location& location_value,
    CheckMode check_mode_value,
    const Allocator& allocator)
    : ConditionCheckException<ExceptionType, CharT, Allocator>(
          expression_value,
          location_value,
          message_value,
          check_mode_value,
          allocator) {
}
 

} // namespace ndof::error
