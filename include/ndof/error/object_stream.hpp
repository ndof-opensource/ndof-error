#ifndef NDOF_OS_OBJECT_STREAM_HPP
#define NDOF_OS_OBJECT_STREAM_HPP

#include "object.hpp"
#include <ostream>

namespace ndof { 

    template<typename CharT = char, typename Allocator = std::allocator<CharT>>
    struct basic_object_ostream :
        public std::basic_ostream<CharT, std::char_traits<CharT>> {
    public:
        using allocator_type = Allocator;
        using allocated_string = std::basic_string<CharT, std::char_traits<CharT>, allocator_type>;

        basic_object_ostream(const basic_object<CharT, allocator_type>& object, const allocator_type& allocator = allocator_type())
            : allocator_(allocator), object_(object) {
        }

    private:
        allocator_type allocator_;
        basic_object<CharT, allocator_type> object_;
    };
    

}
#endif

