#include <exception>
#include <string>
#include <source_location>
#include <cstdint>
#include <typeindex>
#include <type_traits>
#include <optional>
#include <string_view>
#include <stdexcept>
#include <utility>

namespace ndof { 
    // TODO: Find home for these.
    enum class BuildMode : std::uint8_t{
        undefined,
        debug, 
        release
    };

    using CheckMode = BuildMode;
    using Type = std::type_index;

    template<typename Allocator = std::allocator<char>>
    struct FileName {
        std::basic_string<char, std::char_traits<char>, Allocator> value;
    };

    template<typename Allocator = std::allocator<char>>
    struct FunctionName {
        std::basic_string<char, std::char_traits<char>, Allocator> value;
    };

}

namespace ndof::error {
    
    // Note: should define stream to object operators.
    //       In that case, to_string() can be removed and replaced with stream operator.
    //        
    


    // TODO: Consider create a project default config library that  

    // TODO: Consider making this type support wide character strings.
    // TODO: Or have it in a config file, to define things like debug mode, character types, default allocators, etc.

    template<typename Allocator = std::allocator<char>>
    struct Exception : std::exception {
        using allocated_string 
           = std::basic_string<char, std::char_traits<char>, Allocator>;

        // Replace this with an ndof::object type.
        // There will be conversions and adaptor protocols to map ndof::object to nlohmann::json and vice versa.
        // .. and to any other popular serialization format.
        // .. with extensible plugin protocols for other serialization formats.
        // .. Consider how std::formatter can be used to implement this.
        allocated_string to_string() const;

        Exception(
            // TODO: wrap this in a FileName class and a FunctionName class?
            const FileName<Allocator>& file_name,
            const FunctionName<Allocator>& function_name,
            std::uint_least32_t line,
            BuildMode build_mode
        ) : file_name(file_name), function_name(function_name), line(line), build_mode(build_mode) {}
        Exception(const Exception&) = default;
        Exception(Exception&&) = default;
        Exception& operator=(const Exception&) = default;
        Exception& operator=(Exception&&) = default;
 
        ~Exception() = default;
        [[nodiscard]] const char* what() const noexcept override;

        // TODO: Make available only in builds where exceptions are enabled.
        void rethrow() const{
            if (! this->inner_exception.has_value()) {
                throw std::runtime_error("No inner exception to rethrow.");
            }
            std::rethrow_exception(this->inner_exception.value());
        }

        public:
        [[nodiscard]] const allocated_string& get_file_name() const noexcept {
            return file_name.value;
        }

        [[nodiscard]] const allocated_string& get_function_name() const noexcept {
            return function_name.value;
        }

        [[nodiscard]] std::uint_least32_t get_line() const noexcept {
            return line;
        }

        [[nodiscard]] BuildMode get_build_mode() const noexcept {
            return build_mode;
        }

        [[nodiscard]] const allocated_string& get_message() const noexcept {
            // TODO: Implement this.  It perhaps should return an ndof::object type instead of a string.
            return "implement me";
        }

        private:
        // TODO: Remove this, or replace it with an ndof::object type.
        mutable allocated_string message;
        FileName<Allocator>               file_name; 
        FunctionName<Allocator>           function_name;
        std::uint_least32_t               line;
        std::optional<std::exception_ptr> inner_exception;
        BuildMode                         build_mode;
    };

    template<typename Allocator = std::allocator<char>>
    struct ConditionCheckException : Exception<Allocator> {
        using allocated_string 
           = std::basic_string<char, std::char_traits<char>, Allocator>;

        ConditionCheckException() = delete;
        ConditionCheckException(const ConditionCheckException&) = default;
        ConditionCheckException(ConditionCheckException&&) = default;
        ConditionCheckException& operator=(const ConditionCheckException&) = default;
        ConditionCheckException& operator=(ConditionCheckException&&) = default;
        ~ConditionCheckException() = default;

        [[nodiscard]] const allocated_string& get_expression() const noexcept {
            return expression;
        }

        [[nodiscard]] const allocated_string& get_message() const noexcept {
            return message;
        }

        [[nodiscard]] CheckMode get_check_mode() const noexcept {
            return check_mode;
        }

        [[nodiscard]] const char* what() const noexcept override;

    protected:
        template<typename OtherAllocator>
        ConditionCheckException(
            const std::basic_string<char, std::char_traits<char>, OtherAllocator>& expression,
            const std::basic_string<char, std::char_traits<char>, OtherAllocator>& message,
            const std::source_location& location,
            CheckMode check_mode
        );

    private:
        allocated_string expression;
        allocated_string message;
        CheckMode check_mode;
    };
     
    template<typename Allocator = std::allocator<char>>
    struct PreConditionCheckException : ConditionCheckException<Allocator> {
        PreConditionCheckException() = default;
        PreConditionCheckException(const PreConditionCheckException&) = default;
        PreConditionCheckException(PreConditionCheckException&&) = default;
        PreConditionCheckException& operator=(const PreConditionCheckException&) = default;
        PreConditionCheckException& operator=(PreConditionCheckException&&) = default;
        ~PreConditionCheckException() = default;
        template<typename OtherAllocator>
        // Question: Do we need deduction guides for this to work?
        PreConditionCheckException(
            const std::basic_string<char, std::char_traits<char>, OtherAllocator>& expression,
            const std::basic_string<char, std::char_traits<char>, OtherAllocator>& message,
            const std::source_location& location,
            CheckMode check_mode
        );

        [[nodiscard]] const char* what() const noexcept override;
    };

    template<typename Allocator = std::allocator<char>>
    struct PostConditionCheckException : ConditionCheckException<Allocator> {
        PostConditionCheckException() = default;
        PostConditionCheckException(const PostConditionCheckException&) = default;
        PostConditionCheckException(PostConditionCheckException&&) = default;
        PostConditionCheckException& operator=(const PostConditionCheckException&) = default;
        PostConditionCheckException& operator=(PostConditionCheckException&&) = default;
        ~PostConditionCheckException() = default;
        template<typename OtherAllocator>
        PostConditionCheckException(
            const std::basic_string<char, std::char_traits<char>, OtherAllocator>& expression,
            const std::basic_string<char, std::char_traits<char>, OtherAllocator>& message,
            const std::source_location& location,
            CheckMode check_mode
        );
    };

    template<typename Allocator = std::allocator<char>>
    struct InvariantConditionCheckException : ConditionCheckException<Allocator> {
        InvariantConditionCheckException() = default;
        InvariantConditionCheckException(const InvariantConditionCheckException&) = default;
        InvariantConditionCheckException(InvariantConditionCheckException&&) = default;
        InvariantConditionCheckException& operator=(const InvariantConditionCheckException&) = default;
        InvariantConditionCheckException& operator=(InvariantConditionCheckException&&) = default;
        ~InvariantConditionCheckException() = default;
        template<typename OtherAllocator>
        InvariantConditionCheckException(
            const std::basic_string<char, std::char_traits<char>, OtherAllocator>& expression,
            const std::basic_string<char, std::char_traits<char>, OtherAllocator>& message,
            const std::source_location& location,
            CheckMode check_mode
        );

        [[nodiscard]] const char* what() const noexcept override;
    };

    template<typename Allocator = std::allocator<char>>
    struct InnerException : Exception<Allocator> {

 
        using allocated_string 
           = std::basic_string<char, std::char_traits<char>, Allocator>;

        InnerException() = default;

        template<typename CapturedException>
        // Question: Is same or is derived from instead? 
        requires (!std::is_same_v<std::remove_cvref_t<CapturedException>, InnerException<Allocator>>)
        explicit InnerException(CapturedException&& exception)
        // Note: Didn't know about std::make_exception_ptr.
            : captured_exception(std::make_exception_ptr(std::forward<CapturedException>(exception))) {
        }

        InnerException(const InnerException&) = default;
        InnerException(InnerException&&) = default;
        InnerException& operator=(const InnerException&) = default;
        InnerException& operator=(InnerException&&) = default;
        ~InnerException() = default;

        [[nodiscard]] const char* what() const noexcept override;

    protected:
        [[nodiscard]] const std::exception_ptr& get_captured_exception() const noexcept {
            return captured_exception;
        }

    private:
        std::exception_ptr captured_exception;
    };

    template<typename ExceptionType, typename Allocator = std::allocator<char>>
    struct ExplicitInnerException : InnerException<Allocator> {

        // Note: This was added to dismiss a clang-tidy warning about not properly moving
        //       exception before forwarding it to the base class constructor.
        template<typename ForwardedException>
        requires std::is_same_v<std::remove_cvref_t<ForwardedException>, ExceptionType>
        explicit ExplicitInnerException(ForwardedException&& exception)
            : InnerException<Allocator>(std::forward<ForwardedException>(exception)) {
        }

    };

    template<typename ExceptionType, typename Allocator = std::allocator<char>>
    void throw_ndof_exception(ExceptionType&& exception) {
        using captured_exception_type = std::remove_cvref_t<ExceptionType>;
        throw ExplicitInnerException<captured_exception_type, Allocator>(std::forward<ExceptionType>(exception));
    }

    namespace detail {

        [[nodiscard]] inline std::string_view build_mode_to_string(const BuildMode mode) noexcept {
            switch (mode) {
            case BuildMode::debug:
                return "debug";
            case BuildMode::release:
                return "release";
            case BuildMode::undefined:
            default:
                return "undefined";
            }
        }

    } // namespace detail

    template<typename Allocator>
    auto Exception<Allocator>::to_string() const -> allocated_string {
        allocated_string output;
        output.reserve(96);
        output += "Exception{";
        output += "file=";
        output += get_file_name();
        output += ", function=";
        output += get_function_name();
        output += ", line=";
        const auto line_text = std::to_string(get_line());
        output += line_text.c_str();
        output += ", mode=";
        output += detail::build_mode_to_string(get_build_mode()).data();
        output += "}";
        return output;
    }

    template<typename Allocator>
    const char* Exception<Allocator>::what() const noexcept {
        message = to_string();
        return message.c_str();
    }

    template<typename Allocator>
    template<typename OtherAllocator>
    ConditionCheckException<Allocator>::ConditionCheckException(
        const std::basic_string<char, std::char_traits<char>, OtherAllocator>& expression_value,
        const std::basic_string<char, std::char_traits<char>, OtherAllocator>& message_value,
        const std::source_location& location,
        CheckMode check_mode_value)
        : Exception<Allocator>(
              FileName<Allocator>{allocated_string(location.file_name())},
              FunctionName<Allocator>{allocated_string(location.function_name())},
              static_cast<std::uint_least32_t>(location.line()),
              static_cast<BuildMode>(check_mode_value)),
          expression(expression_value.c_str()),
          message(message_value.c_str()),
          check_mode(check_mode_value) {
    }

    template<typename Allocator>
    const char* ConditionCheckException<Allocator>::what() const noexcept {
        return Exception<Allocator>::what();
    }

    template<typename Allocator>
    template<typename OtherAllocator>
    PreConditionCheckException<Allocator>::PreConditionCheckException(
        const std::basic_string<char, std::char_traits<char>, OtherAllocator>& expression,
        const std::basic_string<char, std::char_traits<char>, OtherAllocator>& message,
        const std::source_location& location,
        CheckMode check_mode)
        : ConditionCheckException<Allocator>(expression, message, location, check_mode) {
    }

    template<typename Allocator>
    const char* PreConditionCheckException<Allocator>::what() const noexcept {
        return ConditionCheckException<Allocator>::what();
    }

    template<typename Allocator>
    template<typename OtherAllocator>
    PostConditionCheckException<Allocator>::PostConditionCheckException(
        const std::basic_string<char, std::char_traits<char>, OtherAllocator>& expression,
        const std::basic_string<char, std::char_traits<char>, OtherAllocator>& message,
        const std::source_location& location,
        CheckMode check_mode)
        : ConditionCheckException<Allocator>(expression, message, location, check_mode) {
    }

    template<typename Allocator>
    template<typename OtherAllocator>
    InvariantConditionCheckException<Allocator>::InvariantConditionCheckException(
        const std::basic_string<char, std::char_traits<char>, OtherAllocator>& expression,
        const std::basic_string<char, std::char_traits<char>, OtherAllocator>& message,
        const std::source_location& location,
        CheckMode check_mode)
        : ConditionCheckException<Allocator>(expression, message, location, check_mode) {
    }

    template<typename Allocator>
    const char* InvariantConditionCheckException<Allocator>::what() const noexcept {
        return ConditionCheckException<Allocator>::what();
    }

    template<typename Allocator>
    const char* InnerException<Allocator>::what() const noexcept {
        return Exception<Allocator>::what();
    }
}