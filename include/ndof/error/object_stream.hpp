#ifndef NDOF_OS_OBJECT_STREAM_HPP
#define NDOF_OS_OBJECT_STREAM_HPP

#include "object.hpp"
#include <cstddef>
#include <istream>

namespace ndof { 

    template<typename CharT = char, typename Allocator = std::allocator<std::byte>>
    struct basic_object_iostream :
        public std::basic_iostream<CharT, std::char_traits<CharT>> {
    public:
        using allocator_type = Allocator;
        using object_type = basic_object<CharT, allocator_type>;
        using char_allocator = typename std::allocator_traits<allocator_type>::template rebind_alloc<CharT>;
        using allocated_string = std::basic_string<CharT, std::char_traits<CharT>, char_allocator>;

        explicit basic_object_iostream(const object_type& object, const allocator_type& allocator = allocator_type())
            : std::basic_iostream<CharT, std::char_traits<CharT>>(nullptr),
              allocator_(allocator),
              object_(object) {
        }

    private:
        allocator_type allocator_;
        object_type object_;
    };

    using object_iostream = basic_object_iostream<>;
    

}
#endif

