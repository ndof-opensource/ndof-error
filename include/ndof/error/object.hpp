// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0

#include "ndof/error/allocator_support.hpp"
 
// TODO: Move this to the core library.
// TODO: Make sure the method classifier stuff specializes on noexcept.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// TODO: Gonna move this up into the parser space.

namespace ndof {

// Forward declarations
template<typename CharT, allocator_like Allocator>
class basic_object;

// Node type enumeration
enum class node_kind {
    undefined,
    null,
    element,
    attribute,
    text,
    sequence,
    mapping,
    comment
};

constexpr auto expected_name(node_kind kind) {
    switch (kind) {
        case node_kind::undefined: return "undefined";
        case node_kind::null: return "null";
        case node_kind::element: return "element";
        case node_kind::attribute: return "attribute";
        case node_kind::text: return "text";
        case node_kind::sequence: return "sequence";
        case node_kind::mapping: return "mapping";
        case node_kind::comment: return "comment";
        default: break;
    }
    return "invalid";
}

// Node type traits
template<typename CharT, allocator_like Allocator>
struct node_type_traits {
    using char_type = CharT;
    using allocator_type = Allocator;
    using string_type = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;
    using object_type = basic_object<CharT, Allocator>;
    using sequence_type = std::vector<object_type, Allocator>;
    using attribute_type = std::pair<string_type, string_type>;
    using attributes_map = std::vector<
        attribute_type,
        typename std::allocator_traits<Allocator>::template rebind_alloc<attribute_type>>;
};

struct mismatch_state {
    node_kind&  expected;
    std::size_t variant_index;
    std::source_location& location;
    const char* expected_name;

    std::logic_error to_exception() const {
        return std::logic_error(
            std::string("Node kind does not match variant type: expected '")
            + std::string(expected_name)
            + "', variant index "
            + std::to_string(variant_index)
            + " at "
            + location.file_name()
            + ":"
            + std::to_string(location.line())
            + ":"
            + std::to_string(location.column()));
    }
};

// Text node - represents scalar values (JSON string, number, boolean, null)
template<typename CharT, allocator_like Allocator>
class text_node {
public:
    using char_type = CharT;
    using allocator_type = Allocator;
    using string_type = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

    text_node(const allocator_type& alloc = allocator_type())
        : value_(alloc) {}

    template<typename OtherAllocator>
        requires allocator_compatible_with<OtherAllocator, allocator_type>
    text_node(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& val,
        const OtherAllocator& alloc = OtherAllocator())
        : value_(val.begin(), val.end(), allocator_type(alloc)) {}

    template<typename OtherAllocator>
        requires allocator_compatible_with<OtherAllocator, allocator_type>
    text_node(
        std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>&& val,
        const OtherAllocator& alloc = OtherAllocator())
        : value_(std::make_move_iterator(val.begin()), std::make_move_iterator(val.end()), alloc) {}

    text_node(const string_type& val, const allocator_type& alloc = allocator_type())
        : value_(val, alloc) {}

    text_node(string_type&& val, const allocator_type& alloc = allocator_type())
        : value_(std::move(val), alloc) {}

    [[nodiscard]] node_kind kind() const noexcept {
        return node_kind::text;
    }

    [[nodiscard]] const string_type& value() const noexcept {
        return value_;
    }

    void set_value(const string_type& val) {
        value_ = val;
    }

    void set_value(string_type&& val) {
        value_ = std::move(val);
    }

    [[nodiscard]] const allocator_type& get_allocator() const noexcept {
        return value_.get_allocator();
    }

private:
    string_type value_;
};

// Attribute node - represents key-value metadata
template<typename CharT, allocator_like Allocator>
class attribute_node {
public:
    using char_type = CharT;
    using allocator_type = Allocator;
    using string_type = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;


    attribute_node(const allocator_type& alloc = allocator_type())
        : name_(alloc), value_(alloc) {}

    template<typename OtherAllocator>
        requires allocator_compatible_with<OtherAllocator, allocator_type>
    attribute_node(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& name,
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& value,
        const OtherAllocator& alloc = OtherAllocator())
        : name_(name.begin(), name.end(), allocator_type(alloc)),
          value_(value.begin(), value.end(), allocator_type(alloc)) {}


    // Select the allocator rather than moving it from either source string.
    template<typename OtherAllocator>
        requires allocator_compatible_with<OtherAllocator, allocator_type>
    attribute_node(
        std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>&& name,
        std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>&& value,
        const OtherAllocator& alloc = OtherAllocator())
                : name_(std::make_move_iterator(name.begin()), std::make_move_iterator(name.end()), alloc),
                  value_(std::make_move_iterator(value.begin()), std::make_move_iterator(value.end()), alloc) {

        }

    attribute_node(const string_type& name, const string_type& value, const allocator_type& alloc = allocator_type())
        : name_(name, alloc), value_(value, alloc) {}

    [[nodiscard]] const string_type& name() const noexcept {
        return name_;
    }

    [[nodiscard]] const string_type& value() const noexcept {
        return value_;
    }

    void set_name(const string_type& n) {
        name_ = n;
    }

    void set_value(const string_type& v) {
        value_ = v;
    }

    [[nodiscard]] const allocator_type& get_allocator() const noexcept {
        return name_.get_allocator();
    }

private:
    string_type name_;
    string_type value_;
};

// Comment node - represents metadata/comments (XML comments, YAML comments)
template<typename CharT, typename Allocator>
class comment_node {
public:
    using char_type = CharT;
    using allocator_type = Allocator;
    using string_type = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

    comment_node(const allocator_type& alloc = allocator_type())
        : content_(alloc) {}

    template<typename OtherAllocator>
        requires allocator_compatible_with<OtherAllocator, allocator_type>
    comment_node(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& c,
        const OtherAllocator& alloc = OtherAllocator())
        : content_(c.begin(), c.end(), allocator_type(alloc)) {}

    template<typename OtherAllocator>
        requires allocator_compatible_with<OtherAllocator, allocator_type>
    comment_node(
        std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>&& c,
        const OtherAllocator& alloc = OtherAllocator())
        : content_(std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()), alloc) {}

    comment_node(const string_type& c, const allocator_type& alloc = allocator_type())
        : content_(c, alloc) {}

    comment_node(string_type&& c, const allocator_type& alloc = allocator_type())
        : content_(std::move(c), alloc) {}

    [[nodiscard]] node_kind kind() const noexcept {
        return node_kind::comment;
    }

    [[nodiscard]] const string_type& content() const noexcept {
        return content_;
    }

    void set_content(const string_type& c) {
        content_ = c;
    }

    [[nodiscard]] const allocator_type& get_allocator() const noexcept {
        return content_.get_allocator();
    }

private:
    string_type content_;
};

// Forward declaration of basic_object for variant
template<typename CharT, allocator_like  Allocator>
class basic_object;

// Node variant holding all possible node types
template<typename CharT, allocator_like  Allocator>
using node_variant = std::variant<
    std::monostate,
    text_node<CharT, Allocator>,
    attribute_node<CharT, Allocator>,
    comment_node<CharT, Allocator>,
    std::vector<basic_object<CharT, Allocator>, Allocator>
>;



// TODO: Update the template argument list to accept a CharTraits template parameter for the string type, so that we can support custom character traits.
//       That should be done throughout the entire library, so that we can support custom character traits for the string type.
// Main object class - supports XML, JSON, YAML parsing
template<typename CharT = char, allocator_like Allocator = std::allocator<CharT>>
class basic_object {
public:
    using char_type = CharT;
    using allocator_type = Allocator;
    using string_type = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;
    using string_view_type = std::basic_string_view<CharT, std::char_traits<CharT>>;
    using text_node_type = text_node<CharT, Allocator>;
    using attribute_node_type = attribute_node<CharT, Allocator>;
    using comment_node_type = comment_node<CharT, Allocator>;
    using variant_type = node_variant<CharT, Allocator>;
    using attribute_type = std::pair<string_type, string_type>;
    using attributes_map = std::vector<
        attribute_type,
        typename std::allocator_traits<Allocator>::template rebind_alloc<attribute_type>>;
    using members_map = std::vector<
        std::pair<string_type, basic_object>,
        typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<string_type, basic_object>>
    >;
    using sequence_type = std::vector<basic_object, Allocator>;

    
    // Constructors
    basic_object(const allocator_type& alloc = allocator_type())
        : name_(alloc), value_(std::monostate{}), attributes_(alloc), members_(alloc), allocator_(alloc) {}

    basic_object(node_kind k, const allocator_type& alloc = allocator_type())
        : name_(alloc), value_(std::monostate{}), attributes_(alloc), members_(alloc), allocator_(alloc) {
        initialize_for_kind(k);
    }

    template<typename OtherAllocator>
        requires allocator_compatible_with<OtherAllocator, allocator_type>
    basic_object(
        const std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>& n,
        const OtherAllocator& alloc = OtherAllocator())
        : name_(n.begin(), n.end(), allocator_type(alloc)), value_(std::monostate{}),
          attributes_(allocator_type(alloc)), members_(allocator_type(alloc)), allocator_(alloc) {}

    template<typename OtherAllocator>
        requires allocator_compatible_with<OtherAllocator, allocator_type>
    basic_object(
        std::basic_string<CharT, std::char_traits<CharT>, OtherAllocator>&& n,
        const OtherAllocator& alloc = OtherAllocator())
        : name_(std::make_move_iterator(n.begin()), std::make_move_iterator(n.end()), allocator_type(alloc)),
          value_(std::monostate{}), attributes_(allocator_type(alloc)), members_(allocator_type(alloc)),
          allocator_(alloc) {}

    basic_object(const string_type& n, const allocator_type& alloc = allocator_type())
        : name_(n, alloc), value_(std::monostate{}), attributes_(alloc), members_(alloc), allocator_(alloc) {}

    basic_object(string_type&& n, const allocator_type& alloc = allocator_type())
        : name_(std::move(n), alloc), value_(std::monostate{}), attributes_(alloc), members_(alloc), allocator_(alloc) {}

   

    [[nodiscard]] const string_type& name() const noexcept {
        return name_;
    }

    void set_name(const string_type& n) {
        name_ = n;
    }

    [[nodiscard]] const allocator_type& allocator() const noexcept {
        return allocator_;
    }

    // Attribute operations
    [[nodiscard]] const attributes_map& attributes() const noexcept {
        return attributes_;
    }

    [[nodiscard]] attributes_map& attributes() noexcept {
        return attributes_;
    }

    void add_attribute(const string_type& attr_name, const string_type& attr_value) {
        attributes_.emplace_back(attr_name, attr_value);
    }

    void add_attribute(string_type&& attr_name, string_type&& attr_value) {
        attributes_.emplace_back(std::move(attr_name), std::move(attr_value));
    }

    [[nodiscard]] bool remove_attribute(const string_view_type& attr_name) {
        auto it = std::find_if(attributes_.begin(), attributes_.end(),
            [attr_name](const auto& pair) { return pair.first == attr_name; });
        if (it != attributes_.end()) {
            attributes_.erase(it);
            return true;
        }
        return false;
    }

    [[nodiscard]] std::optional<string_type> get_attribute(const string_view_type& attr_name) const {
        auto it = std::find_if(attributes_.begin(), attributes_.end(),
            [attr_name](const auto& pair) { return pair.first == attr_name; });
        if (it != attributes_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Member/mapping operations
    [[nodiscard]] const members_map& members() const noexcept {
        return members_;
    }

    [[nodiscard]] members_map& members() noexcept {
        return members_;
    }

    [[nodiscard]] basic_object* add_member(const string_type& member_name) {
        members_.emplace_back(member_name, basic_object(allocator_));
        return &members_.back().second;
    }

    [[nodiscard]] basic_object* add_member(const string_type& member_name, node_kind k) {
        members_.emplace_back(member_name, basic_object(k, allocator_type()));
        return &members_.back().second;
    }

    [[nodiscard]] bool remove_member(const string_view_type& member_name) {
        auto it = std::find_if(members_.begin(), members_.end(),
            [member_name](const auto& pair) { return pair.first == member_name; });
        if (it != members_.end()) {
            members_.erase(it);
            return true;
        }
        return false;
    }

    [[nodiscard]] basic_object* get_member(const string_view_type& member_name) {
        auto it = std::find_if(members_.begin(), members_.end(),
            [member_name](const auto& pair) { return pair.first == member_name; });
        if (it != members_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    [[nodiscard]] const basic_object* get_member(const string_view_type& member_name) const {
        auto it = std::find_if(members_.begin(), members_.end(),
            [member_name](const auto& pair) { return pair.first == member_name; });
        if (it != members_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Sequence/array operations
    [[nodiscard]] sequence_type& elements() noexcept {
        if (std::holds_alternative<sequence_type>(value_)) {
            return std::get<sequence_type>(value_);
        }
        static sequence_type empty_seq(allocator_);
        return empty_seq;
    }

    [[nodiscard]] const sequence_type& elements() const noexcept {
        if (std::holds_alternative<sequence_type>(value_)) {
            return std::get<sequence_type>(value_);
        }
        static const sequence_type empty_seq(allocator_);
        return empty_seq;
    }

    [[nodiscard]] basic_object* add_element() {
        if (!std::holds_alternative<sequence_type>(value_)) {
            value_ = sequence_type(allocator_);
        }
        auto& seq = std::get<sequence_type>(value_);
        seq.emplace_back(allocator_);
        return &seq.back();
    }

    [[nodiscard]] basic_object* add_element(node_kind k) {
        if (!std::holds_alternative<sequence_type>(value_)) {
            value_ = sequence_type(allocator_);
        }
        auto& seq = std::get<sequence_type>(value_);
        seq.emplace_back(allocator_type(), k);
        return &seq.back();
    }

    [[nodiscard]] bool remove_element(std::size_t index) {
        if (!std::holds_alternative<sequence_type>(value_) || index >= std::get<sequence_type>(value_).size()) {
            return false;
        }
        auto& seq = std::get<sequence_type>(value_);
        seq.erase(seq.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    [[nodiscard]] basic_object* get_element(std::size_t index) {
        const auto elements = get_node<sequence_type>();
        if (!elements || index >= (*elements)->size()) {
            return nullptr;
        }
        return &(*elements)->operator[](index);
    }

    [[nodiscard]] const basic_object* get_element(std::size_t index) const {
        const auto elements = get_node<sequence_type>();
        if (!elements || index >= (*elements)->size()) {
            return nullptr;
        }
        return &(*elements)->operator[](index);
    }

    // Text value operations
    [[nodiscard]] std::optional<string_type> text_value() const {
        if (const auto* text = get_node<text_node_type>()) {
            return text->value();
        }
        return std::nullopt;
    }

    void set_text_value(const string_type& val) {
        value_ = text_node_type(val, allocator_);
    }

    void set_text_value(string_type&& val) {
        value_ = text_node_type(std::move(val), allocator_);
    }

    template<typename Node>
    [[nodiscard]] std::optional<Node*> get_node() noexcept {
        return std::get_if<Node>(&value_);
    }

    template<typename Node>
    [[nodiscard]] std::optional<const Node*> get_node() const noexcept {
        std::optional<const Node*> result;
        if (const auto* ptr = std::get_if<Node>(&value_)) {
            result = ptr;
        }
        return result;
    }

    // Comment operations
    [[nodiscard]] std::optional<string_type> comment() const {
        // TODO: Return the comment content if the node is a comment node, otherwise return std::nullopt.
        if (const auto* comment = get_node<comment_node_type>()) {
            return comment->content();
        }
        return std::nullopt;
    }

    void set_comment(const string_type& content) {
        value_ = comment_node_type(content, allocator_);
    }

private:
    // TODO: Update this to check for the exception compiler flag, otherwise, return an ndof_return which will be used to
    //       redefine the return type based on whether exceptions are enabled or not. 
    //       If no exceptions, the return type will be an expected<T, E> type, otherwise it will be T, in this case, void.

 
    node_kind get_node_kind(const node_variant<CharT, Allocator>& value_)  noexcept {
        if (std::holds_alternative<std::monostate>(value_)) {
            if (!members_.empty()) {
                return node_kind::mapping;
            }
            if (!name_.empty() || !attributes_.empty()) {
                return node_kind::element;
            }
            return node_kind::null;
        }
        if (std::holds_alternative<text_node_type>(value_)) {
            return node_kind::text;
        }
        if (std::holds_alternative<attribute_node_type>(value_)) {
            return node_kind::attribute;
        }
        if (std::holds_alternative<comment_node_type>(value_)) {
            return node_kind::comment;
        }
        if (std::holds_alternative<sequence_type>(value_)) {
            return node_kind::sequence;
        }
        return node_kind::null;
    }

public:
    // Visitor support
    template<typename Visitor>
    [[nodiscard]] decltype(auto) visit(Visitor&& v) {
        return visit_impl(std::forward<Visitor>(v), std::integral_constant<bool, std::is_const_v<Visitor>>{});
    }


private:
    void initialize_for_kind(node_kind k) {
        switch (k) {
            case node_kind::undefined:
                throw std::logic_error("node_kind::undefined is not a valid kind for initialization");
            case node_kind::null:
            case node_kind::element:
            case node_kind::mapping:
                value_ = std::monostate{};
                return;
            case node_kind::text:
                value_ = text_node_type(allocator_);
                return;
            case node_kind::attribute:
                value_ = attribute_node_type(allocator_);
                return;
            case node_kind::sequence:
                value_ = sequence_type(allocator_);
                return;
            case node_kind::comment:
                value_ = comment_node_type(allocator_);
                return;
        }
        throw std::logic_error("Invalid node kind");
    }

    template<typename Visitor>
    [[nodiscard]] decltype(auto) visit_impl(Visitor&& v) {
        // TODO: Put these in a map.
        switch (get_node_kind(v)) {
            case node_kind::null:
                return v.visit_null(*this);
            case node_kind::element:
                return v.visit_element(*this);
            case node_kind::attribute:
                return v.visit_text(*this);
            case node_kind::text:
                return v.visit_text(*this);
            case node_kind::sequence:
                return v.visit_sequence(*this);
            case node_kind::mapping:
                return v.visit_mapping(*this);
            case node_kind::comment:
                return v.visit_comment(*this);
        }
        throw std::logic_error("Invalid node kind");
    }

    string_type name_;
    variant_type value_;
    attributes_map attributes_;
    members_map members_;
    allocator_type allocator_;
};

// Type aliases
using object = basic_object<char, std::allocator<char>>;
using wobject = basic_object<wchar_t, std::allocator<wchar_t>>;

};
