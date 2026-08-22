#include "ndof/error/allocator_support.hpp"
#include "ndof/error/configs.hpp"
#include "ndof/error/object.hpp"
#include <source_location>

namespace ndof::error {

using check_mode = ndof::build_mode;

// TODO: Move this to the core library.
namespace detail {

// template<typename T, allocator_like Allocator>
// requires allocator_aware_copy_propagating<T>
//     && allocator_compatible_with<Allocator, typename T::allocator_type>
// [[nodiscard]] T copy_considering_allocators(const T& value, const Allocator& allocator) {
//     if constexpr (std::is_constructible_v<T, std::allocator_arg_t, const Allocator&, const T&>) {
//         return T(std::allocator_arg, allocator, value);
//     } else if constexpr (std::is_constructible_v<T, const T&, const Allocator&>) {
//         return T(value, allocator);
//     } else {
//         return T(value);
//     }
// }

// TODO: Link to the design library and use icloneable.

// template<typename T, allocator_like Allocator>
// requires (!allocator_aware_copy_propagating<T>)
// [[nodiscard]] T copy_considering_allocators(const T& value, [[maybe_unused]] const Allocator&
// allocator) {
// // Question: Should this silently ignore the allocator?
//     return value;
// }

// template<typename T, allocator_like Allocator>
// requires allocator_aware_move_propagating<std::remove_cvref_t<T>>
//       && allocator_compatible_with<Allocator, typename std::remove_cvref_t<T>::allocator_type>
// [[nodiscard]] std::remove_cvref_t<T> move_considering_allocators(T&& value, const Allocator&
// allocator) {
//     using value_type = std::remove_cvref_t<T>;
//     if constexpr (std::is_constructible_v<value_type, std::allocator_arg_t, const Allocator&,
//     value_type&&>) {
//         return value_type(std::allocator_arg, allocator, std::forward<T>(value));
//     } else if constexpr (std::is_constructible_v<value_type, value_type&&, const Allocator&>) {
//         return value_type(std::forward<T>(value), allocator);
//     } else {
//         return value_type(std::forward<T>(value));
//     }
// }

// template<typename T, allocator_like Allocator>
// requires (!allocator_aware_move_propagating<std::remove_cvref_t<T>>)
// [[nodiscard]] std::remove_cvref_t<T> move_considering_allocators(T&& value, [[maybe_unused]]
// const Allocator& allocator) {
//     // Question: Should this silently ignore the allocator?
//     return std::remove_cvref_t<T>(std::forward<T>(value));
// }

// template<typename CharT, typename Traits, allocator_for<CharT> SourceAllocator,
// allocator_for<CharT> DestinationAllocator>
// [[nodiscard]] std::basic_string<CharT, Traits, DestinationAllocator> copy_considering_allocators(
//     const std::basic_string<CharT, Traits, SourceAllocator>& value,
//     const DestinationAllocator& allocator) {
//     return std::basic_string<CharT, Traits, DestinationAllocator>(value.data(), value.size(),
//     allocator);
// }

// template<typename CharT, typename Traits, allocator_for<CharT> SourceAllocator,
// allocator_for<CharT> DestinationAllocator>
// [[nodiscard]] std::basic_string<CharT, Traits, DestinationAllocator> move_considering_allocators(
//     std::basic_string<CharT, Traits, SourceAllocator>&& value,
//     const DestinationAllocator& allocator) {
//     if constexpr ((std::is_same_v<SourceAllocator, DestinationAllocator>)
//     && (value.get_allocator() == allocator)) {
//         return std::basic_string<CharT, Traits, DestinationAllocator>(std::move(value));
//     }
//     return std::basic_string<CharT, Traits, DestinationAllocator>(value.data(), value.size(),
//     allocator);
// }

// template<typename CharT, allocator_for<CharT> Allocator>
// requires allocator_for<Allocator, CharT>
// std::basic_string<CharT, std::char_traits<CharT>, Allocator>
// make_string_from_narrow(const char* text, const Allocator& allocator) {
//     std::basic_string<CharT, std::char_traits<CharT>, Allocator> output(allocator);
//     const std::string_view text_view = (text == nullptr) ? std::string_view{} :
//     std::string_view{text}; output.reserve(text_view.size()); for (const char ch : text_view) {
//         output.push_back(static_cast<CharT>(ch));
//     }
//     return output;
// }

// template<typename T>
// concept ndof_exception_derived =
//     requires { requires std::derived_from<std::remove_cvref_t<T>, exception_tag>; };

} // namespace detail

// TODO: should define stream to object operators.

// TODO: Should be IColoneable.
//       Should be Iserializable.
//       Should be IDeserializable.
//       Should be IStreamable.
//       Should be IStreamableToObject.
//       Should be IStreamableFromObject.

template <typename CharT = char> struct basic_exception : std::exception {
  public:
    basic_exception() = delete;

    basic_exception(const std::source_location& source_location_value);

    basic_exception(basic_exception&&) = delete;
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

    virtual std::expected<void, std::exception> to_object_impl(basic_object<CharT>& obj) = 0;

  protected:
    basic_exception(const basic_exception&) = delete;

  private:
    std::source_location location_;
    // TODO: Implement.
    template <ndof::allocator_like OtherAllocator>
    [[nodiscard]] std::expected<void, std::exception>
    to_object(basic_object<CharT, OtherAllocator>& obj) const noexcept;
};


// Type-independent (with respect to the captured exception type and allocator)
// base for inner-exception carriers. Holds the captured exception and provides
// the common behavior so that derived templates do not duplicate code.
template <typename CharT = char,
          ndof::allocator_like Allocator = std::allocator<CharT>>
struct basic_inner_exception : basic_exception<CharT> {
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

    std::expected<void, std::exception> to_object_impl(basic_object<CharT>& obj) override;

  private:
    std::exception_ptr captured_exception_;
    Allocator allocator_;
};


template <typename ExceptionType, typename CharT = char,
          ndof::allocator_like Allocator = std::allocator<CharT>>
struct basic_explicit_inner_exception : basic_inner_exception<CharT, Allocator> {

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

    [[nodiscard]] std::expected<void, std::exception>
    to_object(ndof::basic_object<CharT>& obj) const override;
};

template <typename ExceptionType, typename CharT = char, typename Allocator = std::allocator<CharT>>
struct basic_condition_check_exception
    : basic_explicit_inner_exception<ExceptionType, CharT, Allocator> {
    using allocated_string = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

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
    [[nodiscard]] ndof::check_mode check_mode() const noexcept;
    [[nodiscard]] const char* what() const noexcept override;

  protected:
    template <ndof::allocator_for<CharT> OtherAllocator>
    basic_condition_check_exception(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::source_location& location_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        ndof::check_mode check_mode_value, const Allocator& allocator = Allocator());

  private:
    allocated_string expression_;
    allocated_string message_;
    ndof::check_mode check_mode_;
};

template <typename ExceptionType, typename CharT = char,
          ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
struct basic_precondition_check_exception
    : basic_condition_check_exception<ExceptionType, CharT, Allocator> {
    basic_precondition_check_exception() = default;
    basic_precondition_check_exception(const basic_precondition_check_exception&) = default;
    basic_precondition_check_exception(basic_precondition_check_exception&&) = default;
    basic_precondition_check_exception&
    operator=(const basic_precondition_check_exception&) = default;
    basic_precondition_check_exception& operator=(basic_precondition_check_exception&&) = default;
    ~basic_precondition_check_exception() = default;

    template <ndof::allocator_for<CharT> OtherAllocator>
    basic_precondition_check_exception(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value, ndof::check_mode check_mode_value,
        const Allocator& allocator = Allocator());
};

template <typename ExceptionType, typename CharT = char,
          ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
struct basic_postcondition_check_exception
    : basic_condition_check_exception<ExceptionType, CharT, Allocator> {
    basic_postcondition_check_exception() = default;
    basic_postcondition_check_exception(const basic_postcondition_check_exception&) = default;
    basic_postcondition_check_exception(basic_postcondition_check_exception&&) = default;
    basic_postcondition_check_exception&
    operator=(const basic_postcondition_check_exception&) = default;
    basic_postcondition_check_exception& operator=(basic_postcondition_check_exception&&) = default;
    ~basic_postcondition_check_exception() = default;

    template <ndof::allocator_for<CharT> OtherAllocator>
    basic_postcondition_check_exception(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value, ndof::check_mode check_mode_value,
        const Allocator& allocator = Allocator());
};

template <typename ExceptionType, typename CharT = char,
          ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
struct basic_invariant_condition_check_exception
    : basic_condition_check_exception<ExceptionType, CharT, Allocator> {
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

    template <ndof::allocator_for<CharT> OtherAllocator>
    basic_invariant_condition_check_exception(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& expression_value,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& message_value,
        const std::source_location& location_value, ndof::check_mode check_mode_value,
        const Allocator& allocator = Allocator());
};

// TODO: Revisit.
template <typename ExceptionType, typename CharT = char,
          ndof::allocator_for<CharT> Allocator = std::allocator<CharT>>
void throw_exception(ExceptionType&& exception,
                     std::source_location source_location_value = std::source_location::current(),
                     const Allocator& allocator = Allocator()) {

    // using captured_exception_type = std::remove_cvref_t<ExceptionType>;
    // if constexpr (detail::ndof_exception_derived<captured_exception_type>) {
    //     throw basic_explicit_inner_exception<captured_exception_type, CharT, Allocator>(
    //         std::forward<ExceptionType>(exception),
    //         source_location_value,
    //         allocator);
    // } else {
    //     throw basic_explicit_inner_exception<captured_exception_type, CharT, Allocator>(
    //         std::forward<ExceptionType>(exception),
    //         allocator);
}
} // namespace ndof::error

} // namespace ndof::error
