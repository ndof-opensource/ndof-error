#include <exception>
#include <string>
#include <source_location>
#include <cstdint>
#include <typeindex>
#include <optional>

namespace ndof { 
    // TODO: Find home for these.
    enum class BuildMode : std::uint8_t{
        undefined,
        debug, 
        release
    };

    using CheckMode = BuildMode;
    using Type = std::type_index;
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

        const allocated_string            file_name; 
        const allocated_string            function_name;
        const std::uint_least32_t         line;
        std::optional<std::exception_ptr> inner_exception;
        BuildMode                         build_mode;


        // Replace this with an ndof::object type.
        // There will be conversions and adaptor protocols to map ndof::object to nlohmann::json and vice versa.
        // .. and to any other popular serialization format.
        // .. with extensible plugin protocols for other serialization formats.
        // .. Consider how std::formatter can be used to implement this.
        allocated_string to_string() const;

        Exception(
            // TODO: wrap this in a FileName class and a FunctionName class?
            const allocated_string& file_name,
            const allocated_string& function_name,
            std::uint_least32_t line,
            BuildMode build_mode
        ) : file_name(file_name), function_name(function_name), line(line), build_mode(build_mode) {}
        Exception(const Exception&) = default;
        Exception(Exception&&) = default;
        Exception& operator=(const Exception&) = default;
        Exception& operator=(Exception&&) = default;
 
        ~Exception() = default;
        [[nodiscard]] virtual const char* what() const noexcept override;

    private:

        mutable allocated_string message;
    };

    template<typename Allocator = std::allocator<char>>
    struct ConditionCheckException : Exception<Allocator> {
        using allocated_string 
           = std::basic_string<char, std::char_traits<char>, Allocator>;
        allocated_string     expression;
        allocated_string     message;
        CheckMode            check_mode;

        ConditionCheckException() = default;
        ConditionCheckException(const ConditionCheckException&) = default;
        ConditionCheckException(ConditionCheckException&&) = default;
        ConditionCheckException& operator=(const ConditionCheckException&) = default;
        ConditionCheckException& operator=(ConditionCheckException&&) = default;
        ~ConditionCheckException() = default;
        [[nodiscard]] const char* what() const noexcept override;
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

        using RethrowFn = void(*)();
        using allocated_string 
           = std::basic_string<char, std::char_traits<char>, Allocator>;

        ndof::Type        index;
        allocated_string  exception_type_name;
        allocated_string  message;
        RethrowFn         rethrow;

        InnerException() = default;
        InnerException(const InnerException&) = default;
        InnerException(InnerException&&) = default;
        InnerException& operator=(const InnerException&) = default;
        InnerException& operator=(InnerException&&) = default;
        ~InnerException() = default;

        [[nodiscard]] const char* what() const noexcept override;
    };
}