#ifndef NDOF_OS_OBJECT_STREAM_HPP
#define NDOF_OS_OBJECT_STREAM_HPP

#include "object.hpp"
#include <ostream>

namespace ndof { 

    template<typename CharT = char, typename Allocator = std::allocator<CharT>>
    struct ObjectStream :
        public std::basic_ostream<CharT, std::char_traits<CharT>> {
    public:
        using allocator_type = Allocator;
        using allocated_string = std::basic_string<char, std::char_traits<char>, allocator_type>;

        ObjectStream(const basic_object<allocator_type>& obj, const allocator_type& allocator = allocator_type())
            : obj_(obj), allocator_(allocator) {
        }

    private:
        allocator_type allocator_;
        basic_object<allocator_type> obj_;
    };
    

}
#endif

