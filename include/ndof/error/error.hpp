#include <exception>
#include <string>
#include <source_location>

    // TODO: Find home for this.
    enum class BuildMode{
        undefined,
        debug, 
        release
    };

    using CheckMode = BuildMode;


namespace ndof::error {
    
    // Note: should define stream to object operators.
    //       In that case, to_string() can be removed and replaced with stream operator.
    //        
    
    // TODO: Need to make this allocator aware.  
    struct InnerException{
        // If no state, can use a function pointer instead.
        // Otherwise, we will use a ndof::function<void(std::any&)> to rethrow the exception.
   
        using RethrowFn = std::function<void(std::any&)>;
        InnerException(
            std::type_index    index, 
            // TODO: Get rid of the any here and replace with exception_ptr.
            std::any           original_exception, 
            const std::string& exception_type_name,
            const std::string& message,
            const RethrowFn&   rethrow_fn
        );

        // This will be wrapped and mapped to a rtti compile flag check.  
        // If rtti is disabled, it will resolve to an ndof::type_index.
        std::type_index       index;
        std::any              original_exception; // TODO: Fix this.  Make it exception_ptr.
        std::string           exception_type_name;
        std::string           message;
        // Replace with either a function pointer or an ndof::function.
        std::function<void()> rethrow;
    };

    struct ConditionCheckError {
        std::string_view     expression;
        std::string_view     message;
        std::source_location location;
        ContractType         contract_type;
        CheckMode            check_mode;
    };

    // TODO: This needs to be allocator aware.
    struct ErrorCondition{
        std::string  failed_condition;
        std::string  description;
        ContractType contract_type;
        CheckMode    check_mode;
    };

    using ErrorVariant = std::variant<std::monostate,ErrorCondition,InnerException>;

    // Also needs to be allocator aware.
    // TODO: Figure out a way to globally set error allocators using a singleton or something?
    struct ErrorMetadata{
        ErrorMetadata(const ConditionCheckError&, ContractType);
        ErrorMetadata(const std::source_location&, const InnerException&);
        ErrorMetadata(const std::source_location& );
        ErrorMetadata(const ErrorMetadata&)=default;

        std::string         file_name;
        std::string         function_name;
        std::uint_least32_t line;
        BuildMode           build_mode;
        ErrorVariant        details;

        // Replace this with an ndof::object type.
        // There will be conversions and adaptor protocols to map ndof::object to nlohmann::json and vice versa.
        // .. and to any other popular serialization format.
        // .. with extensible plugin protocols for other serialization formats.
        // .. Consider how std::formatter can be used to implement this.
        std::string to_string() const;
    };

    struct Exception : std::exception {
        Exception(const ErrorMetadata&);
        ~Exception() = default;
        const char* what()  const noexcept  override;
        const ErrorMetadata metadata;

    private:

        std::string message;
    };
}