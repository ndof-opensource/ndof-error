#include "ndof/error/allocator_support.hpp"
#include "ndof/error/configs.hpp"
#include "ndof/error/object.hpp"
#include <source_location>
#include <expected>

namespace ndof::error {

using check_mode = ndof::build_mode;

// TODO: should define stream to object operators.

// TODO: Should be IColoneable.
//       Should be Iserializable.
//       Should be IDeserializable.
//       Should be IStreamable.
//       Should be IStreamableToObject.
//       Should be IStreamableFromObject.

// Note: This is called out at the top of the file too.

// Forward declaration, needed by result_impl below.
template <typename, typename>
struct basic_exception;

// Primary template, specialized below on whether exceptions are enabled, so
// that `result` can be defined as a single alias template regardless of the
// exceptions-enabled setting.
template <typename T,
          bool ExceptionsEnabled,
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>>
struct result_impl;

// std::expected does not support reference types (or rvalue-reference
// types) as its value_type, so when T is a reference we must wrap it in
// std::reference_wrapper for the std::expected specialization below.
// Note: rvalue references (T&&) cannot be stored via std::reference_wrapper
// either, so wrapping an rvalue reference through result_t is not supported;
// callers should return by value or use a (const) lvalue reference instead.
template <typename T>
struct result_value_type {
  using type = T;
};

template <typename T>
struct result_value_type<T&> {
  using type = std::reference_wrapper<T>;
};

// std::reference_wrapper cannot bind to rvalue references (they refer to
// temporaries), so instead of rejecting T&& outright we store the decayed
// value type by value: the rvalue is moved into the std::expected/return
// value, which is the only sound way to propagate it.
template <typename T>
struct result_value_type<T&&> {
  using type = std::decay_t<T>;
};

template <typename T>
using result_value_type_t = typename result_value_type<T>::type;

// When exceptions are disabled, error propagation is done via std::expected,
// carrying either the value T (or std::reference_wrapper<T> if T is a
// reference) or an ndof::exception on failure.
template <typename T, typename CharT, typename Traits>
struct result_impl<T, false, CharT, Traits> {
  using type = std::expected<result_value_type_t<T>,
                              ndof::error::basic_exception<CharT, Traits>>;
};

// When exceptions are enabled, error propagation is done via throwing, so the
// return type is simply T (references are passed through unwrapped, since
// throwing does not go through std::expected).
template <typename T, typename CharT, typename Traits>
struct result_impl<T, true, CharT, Traits> {
  using type = T;
};

template<typename T,
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>>
using result_t = typename result_impl<T, get_exceptions_enabled(), CharT, Traits>::type;

using void_result_t = result_t<void, ndof::default_char_t, ndof::default_char_traits_t<ndof::default_char_t>>;

template <typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>>
struct basic_exception : std::exception {
  public:

    basic_exception(const std::source_location& source_location_value);
    basic_exception() = delete;
    
    // TODO: Revisit these.
    basic_exception& operator=(const basic_exception&) = delete;
    basic_exception& operator=(basic_exception&&) = delete;
    virtual ~basic_exception() = 0;

    [[nodiscard]] const char* what() const noexcept override = 0;
    [[nodiscard]] const char* file_name() const noexcept;
    [[nodiscard]] const char* function_name() const noexcept;
    [[nodiscard]] std::uint_least32_t line() const noexcept;
    [[nodiscard]] std::uint_least32_t column() const noexcept;

    // TODO: Implement.
    void virtual rethrow() const = 0;

    virtual result_t<void, CharT, Traits> to_object_impl(basic_object<CharT, Traits>& obj) = 0;

  protected:
    basic_exception(const basic_exception&) = default;
    basic_exception(basic_exception&&) = default;

  private:
    std::source_location location_;
    // TODO: Implement.
    template <ndof::allocator_like OtherAllocator>
    [[nodiscard]] result_t<void, CharT, Traits>
    to_object(basic_object<CharT, Traits, OtherAllocator>& obj) const noexcept;
};

// Type-independent (with respect to the captured exception type and allocator)
// base for inner-exception carriers. Holds the captured exception and provides
// the common behavior so that derived templates do not duplicate code.
template <typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          ndof::allocator_like Allocator = std::allocator<CharT>>
struct basic_inner_exception : basic_exception<CharT, Traits> {
  public:
    using allocator_type = Allocator;

    basic_inner_exception() = delete;

    explicit basic_inner_exception(const std::exception_ptr& captured_exception,
                                   const Allocator& allocator = Allocator());

    basic_inner_exception(const std::exception_ptr& captured_exception,
                          const std::source_location& source_location_value,
                          const Allocator& allocator = Allocator());

    ~basic_inner_exception() override = 0;

    [[nodiscard]] const std::exception_ptr& captured_exception() const noexcept;
    [[nodiscard]] const allocator_type& get_allocator() const noexcept;
    [[nodiscard]] bool has_captured_exception() const noexcept;

    [[nodiscard]] const char* what() const noexcept override;

    void rethrow() const override;

    // TODO: figure out how to specialize for standard exceptions.
    result_t<void, CharT, Traits> to_object_impl(basic_object<CharT, Traits>& obj) override;

  private:
    std::exception_ptr captured_exception_;
    Allocator allocator_;
};

template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          ndof::allocator_like Allocator = std::allocator<CharT>>
struct basic_explicit_inner_exception : basic_inner_exception<CharT, Traits, Allocator> {

    explicit basic_explicit_inner_exception(std::exception_ptr const& captured_exception,
                                            const Allocator& allocator = Allocator());
    ~basic_explicit_inner_exception() override = default;

    template <typename ForwardedException>
        requires std::is_same_v<std::remove_cvref_t<ForwardedException>, ExceptionType>
    explicit basic_explicit_inner_exception(ForwardedException&& exception,
                                            const std::source_location& source_location_value,
                                            const Allocator& allocator = Allocator());

    // TODO: Implement.
    template <allocator_compatible_with<Allocator> OtherAllocator>
    explicit basic_explicit_inner_exception(const std::exception_ptr& captured_exception,
                                            const std::source_location& source_location_value,
                                            const OtherAllocator& allocator = OtherAllocator());

    // Fix: 
    static_assert(false,"fix this.");
    [[nodiscard]]  
    virtual result_t<void, CharT, Traits>
    to_object(ndof::basic_object<CharT, Traits>& obj) const override;
    
 
};

// TODO: Implement partial specializations for each standard exception.
//       In implementing them, create an inner class or something.
//       Partial specialization definitions outside of the declaration don't work apparently.

template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          typename Allocator = std::allocator<CharT>>
struct basic_condition_check_exception
    : basic_explicit_inner_exception<ExceptionType, CharT, Traits, Allocator> {
    using allocated_string = std::basic_string<CharT, Traits, Allocator>;

    basic_condition_check_exception() = delete;
    basic_condition_check_exception(const basic_condition_check_exception&) = default;
    basic_condition_check_exception(basic_condition_check_exception&&) = default;
    basic_condition_check_exception& operator=(const basic_condition_check_exception&) = default;
    basic_condition_check_exception& operator=(basic_condition_check_exception&&) = default;
    ~basic_condition_check_exception() = default;

    // TODO: Consider string_view for expression and message parameters.
    //       This would allow for more flexibility in passing string-like objects without requiring
    //       a specific string type.
    [[nodiscard]] const allocated_string& expression() const noexcept;
    [[nodiscard]] const allocated_string& message() const noexcept;
    [[nodiscard]] ndof::error::check_mode check_mode() const noexcept;
    [[nodiscard]] const char* what() const noexcept override;

  protected:
    template <ndof::allocator_like OtherAllocator>
    basic_condition_check_exception(
        const std::basic_string<CharT, Traits, OtherAllocator>& expression_value,
        const std::source_location& location_value,
        const std::basic_string<CharT, Traits, OtherAllocator>& message_value,
        ndof::error::check_mode check_mode_value, const Allocator& allocator = Allocator());

  private:
    allocated_string expression_;
    allocated_string message_;
    ndof::error::check_mode check_mode_;
};

template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          ndof::allocator_like Allocator = std::allocator<CharT>>
struct basic_precondition_check_exception
    : basic_condition_check_exception<ExceptionType, CharT, Traits, Allocator> {
    basic_precondition_check_exception() = default;
    basic_precondition_check_exception(const basic_precondition_check_exception&) = default;
    basic_precondition_check_exception(basic_precondition_check_exception&&) = default;
    basic_precondition_check_exception&
    operator=(const basic_precondition_check_exception&) = default;
    basic_precondition_check_exception& operator=(basic_precondition_check_exception&&) = default;
    ~basic_precondition_check_exception() = default;

    template <ndof::allocator_like OtherAllocator>
    basic_precondition_check_exception(
        const std::basic_string<CharT, Traits, OtherAllocator>& expression_value,
        const std::basic_string<CharT, Traits, OtherAllocator>& message_value,
        const std::source_location& location_value, ndof::error::check_mode check_mode_value,
        const Allocator& allocator = Allocator());
};

template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          ndof::allocator_like Allocator = std::allocator<CharT>>
struct basic_postcondition_check_exception
    : basic_condition_check_exception<ExceptionType, CharT, Traits, Allocator> {
    basic_postcondition_check_exception() = default;
    basic_postcondition_check_exception(const basic_postcondition_check_exception&) = default;
    basic_postcondition_check_exception(basic_postcondition_check_exception&&) = default;
    basic_postcondition_check_exception&
    operator=(const basic_postcondition_check_exception&) = default;
    basic_postcondition_check_exception& operator=(basic_postcondition_check_exception&&) = default;
    ~basic_postcondition_check_exception() = default;

    template <ndof::allocator_like OtherAllocator>
    basic_postcondition_check_exception(
        const std::basic_string<CharT, Traits, OtherAllocator>& expression_value,
        const std::basic_string<CharT, Traits, OtherAllocator>& message_value,
        const std::source_location& location_value, ndof::error::check_mode check_mode_value,
        const Allocator& allocator = Allocator());
};

template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          ndof::allocator_like Allocator = std::allocator<CharT>>
struct basic_invariant_condition_check_exception
    : basic_condition_check_exception<ExceptionType, CharT, Traits, Allocator> {
    basic_invariant_condition_check_exception() = default;
    basic_invariant_condition_check_exception(const basic_invariant_condition_check_exception&) =
        default;
    basic_invariant_condition_check_exception(basic_invariant_condition_check_exception&&) =
        default;
    basic_invariant_condition_check_exception&
    operator=(const basic_invariant_condition_check_exception&) = default;
    basic_invariant_condition_check_exception&
    operator=(basic_invariant_condition_check_exception&&) = default;
    ~basic_invariant_condition_check_exception() = default;

    template <ndof::allocator_like OtherAllocator>
    basic_invariant_condition_check_exception(
        const std::basic_string<CharT, Traits, OtherAllocator>& expression_value,
        const std::basic_string<CharT, Traits, OtherAllocator>& message_value,
        const std::source_location& location_value, ndof::error::check_mode check_mode_value,
        const Allocator& allocator = Allocator());
};

// TODO: Discuss.  The behavior will change if the exception type passed in is not_allocator_aware.
template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          ndof::allocator_like Allocator = std::allocator<CharT>>
    requires(
        get_exceptions_enabled() &&
        std::derived_from<std::remove_cvref_t<ExceptionType>, ndof::error::basic_exception<CharT, Traits>> &&
        !ndof::allocator_aware<std::remove_cvref_t<ExceptionType>>)
[[nodiscard]] auto
generate_or_throw_ndof_exception(ExceptionType& exception,
                    std::source_location source_location_value = std::source_location::current(),
                    const Allocator& allocator = Allocator()) {

    using captured_exception_type = std::remove_cvref_t<ExceptionType>;
    return basic_explicit_inner_exception<captured_exception_type, CharT, Traits, Allocator>(
        std::forward<ExceptionType>(exception), source_location_value, allocator);
}

template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>>
    requires(
        get_exceptions_enabled() &&
        std::derived_from<std::remove_cvref_t<ExceptionType>, ndof::error::basic_exception<CharT, Traits>> &&
        ndof::allocator_aware<std::remove_cvref_t<ExceptionType>>)
[[nodiscard]] auto
generate_or_throw_ndof_exception(ExceptionType& exception,
                    std::source_location source_location_value = std::source_location::current(),
                    const std::remove_cvref_t<decltype(exception.get_allocator())>& allocator =
                        std::remove_cvref_t<decltype(exception.get_allocator())>()) {

    using captured_exception_type = std::remove_cvref_t<ExceptionType>;
    using allocator_type = std::remove_cvref_t<decltype(exception.get_allocator())>;
    return basic_explicit_inner_exception<captured_exception_type, CharT, Traits, allocator_type>(
        std::forward<ExceptionType>(exception), source_location_value, allocator);
}

template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>,
          ndof::allocator_like Allocator = std::allocator<CharT>>
    requires(
        get_exceptions_enabled() &&
        !std::derived_from<std::remove_cvref_t<ExceptionType>, ndof::error::basic_exception<CharT, Traits>> &&
        !ndof::allocator_aware<std::remove_cvref_t<ExceptionType>>)
void generate_or_throw_ndof_exception(
    ExceptionType& exception,
    std::source_location source_location_value = std::source_location::current(),
    const Allocator& allocator = Allocator()) {

    using captured_exception_type = std::remove_cvref_t<ExceptionType>;
    throw basic_explicit_inner_exception<captured_exception_type, CharT, Traits, Allocator>(
        std::forward<ExceptionType>(exception), source_location_value, allocator);
}

template <typename ExceptionType, 
          typename CharT = ndof::default_char_t,
          typename Traits = ndof::default_char_traits_t<CharT>>
    requires(
        get_exceptions_enabled() &&
        !std::derived_from<std::remove_cvref_t<ExceptionType>, ndof::error::basic_exception<CharT, Traits>> &&
        ndof::allocator_aware<std::remove_cvref_t<ExceptionType>>)
void generate_or_throw_ndof_exception(
    ExceptionType& exception,
    std::source_location source_location_value = std::source_location::current()) {

    using captured_exception_type = std::remove_cvref_t<ExceptionType>;
    using allocator_type = std::remove_cvref_t<decltype(exception.get_allocator())>;
    throw basic_explicit_inner_exception<captured_exception_type, CharT, Traits, allocator_type>(
        std::forward<ExceptionType>(exception), source_location_value, exception.get_allocator());
}

} // namespace ndof::error