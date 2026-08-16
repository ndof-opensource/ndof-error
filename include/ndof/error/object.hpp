// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0

// #include "ndof/error/fixed_string.hpp"
#include "ndof/error/allocator_support.hpp"
#include "ndof/error/fixed_string.hpp"
 
// TODO: Move this to the core library.
// TODO: Make sure the method classifier stuff specializes on noexcept.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
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
template<typename CharT, typename Allocator>
class basic_object;

// Node type enumeration
enum class node_kind {
    null,
    element,
    attribute,
    text,
    sequence,
    mapping,
    comment
};

// Node type traits
template<typename CharT, allocator_for<CharT> Allocator>
struct node_type_traits {
    using char_type = CharT;
    using allocator_type = Allocator;
    using string_type = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;
    using object_type = basic_object<CharT, Allocator>;
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
template<typename CharT, allocator_for<CharT> Allocator>
class attribute_node {
public:
    using char_type = CharT;
    using allocator_type = Allocator;
    using string_type = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

    attribute_node(const allocator_type& alloc = allocator_type())
        : name_(alloc), value_(alloc) {}
    
    attribute_node(const string_type& n, const string_type& v, const allocator_type& alloc = allocator_type())
        : name_(n, alloc), value_(v, alloc) {}

    [[nodiscard]] node_kind kind() const noexcept {
        return node_kind::attribute;
    }

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
    
    comment_node(const string_type& c, const allocator_type& alloc = allocator_type())
        : content_(c, alloc) {}

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
template<typename CharT, typename Allocator>
class basic_object;

// Node variant holding all possible node types
template<typename CharT, allocator_for<CharT> Allocator>
using node_variant = std::variant<
    std::monostate,
    text_node<CharT, Allocator>,
    attribute_node<CharT, Allocator>,
    comment_node<CharT, Allocator>,
    std::vector<basic_object<CharT, Allocator>, Allocator>
>;

// Main object class - supports XML, JSON, YAML parsing
template<typename CharT = char, typename Allocator = std::allocator<CharT>>
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
    using attributes_map = std::vector<
        std::pair<string_type, string_type>,
        typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<string_type, string_type>>
    >;
    using members_map = std::vector<
        std::pair<string_type, basic_object>,
        typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<string_type, basic_object>>
    >;
    using sequence_type = std::vector<basic_object, Allocator>;

    enum class kind : std::uint8_t {
        null = 0,
        element = 1,
        text = 2,
        sequence = 3,
        mapping = 4,
        comment = 5
    };

    // Constructors
    basic_object(const allocator_type& alloc = allocator_type())
        : name_(alloc), value_(std::monostate{}), attributes_(alloc), members_(alloc), allocator_(alloc) {}
    
    basic_object(kind k, const allocator_type& alloc = allocator_type())
        : name_(alloc), value_(std::monostate{}), attributes_(alloc), members_(alloc), allocator_(alloc), kind_(k) {
        initialize_for_kind(k);
    }

    basic_object(const string_type& n, const allocator_type& alloc = allocator_type())
        : name_(n, alloc), value_(std::monostate{}), attributes_(alloc), members_(alloc), allocator_(alloc) {}

    basic_object(const string_type& n, kind k, const allocator_type& alloc = allocator_type())
        : name_(n, alloc), value_(std::monostate{}), attributes_(alloc), members_(alloc), allocator_(alloc), kind_(k) {
        initialize_for_kind(k);
    }

    [[nodiscard]] kind type() const noexcept {
        return kind_;
    }

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

    [[nodiscard]] basic_object* add_member(const string_type& member_name, kind k) {
        members_.emplace_back(member_name, basic_object(allocator_type(), k));
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
        if (kind_ == kind::sequence) {
            return std::get<sequence_type>(value_);
        }
        static sequence_type empty_seq(allocator_);
        return empty_seq;
    }

    [[nodiscard]] const sequence_type& elements() const noexcept {
        if (kind_ == kind::sequence) {
            return std::get<sequence_type>(value_);
        }
        static const sequence_type empty_seq(allocator_);
        return empty_seq;
    }

    [[nodiscard]] basic_object* add_element() {
        if (kind_ != kind::sequence) {
            kind_ = kind::sequence;
            value_ = sequence_type(allocator_);
        }
        auto& seq = std::get<sequence_type>(value_);
        seq.emplace_back(allocator_);
        return &seq.back();
    }

    [[nodiscard]] basic_object* add_element(kind k) {
        if (kind_ != kind::sequence) {
            kind_ = kind::sequence;
            value_ = sequence_type(allocator_);
        }
        auto& seq = std::get<sequence_type>(value_);
        seq.emplace_back(allocator_type(), k);
        return &seq.back();
    }

    [[nodiscard]] bool remove_element(std::size_t index) {
        if (kind_ != kind::sequence || index >= std::get<sequence_type>(value_).size()) {
            return false;
        }
        auto& seq = std::get<sequence_type>(value_);
        seq.erase(seq.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    [[nodiscard]] basic_object* get_element(std::size_t index) {
        if (kind_ != kind::sequence || index >= std::get<sequence_type>(value_).size()) {
            return nullptr;
        }
        return &std::get<sequence_type>(value_)[index];
    }

    [[nodiscard]] const basic_object* get_element(std::size_t index) const {
        if (kind_ != kind::sequence || index >= std::get<sequence_type>(value_).size()) {
            return nullptr;
        }
        return &std::get<sequence_type>(value_)[index];
    }

    // Text value operations
    [[nodiscard]] std::optional<string_type> text_value() const {
        if (kind_ == kind::text) {
            return std::get<text_node_type>(value_).value();
        }
        return std::nullopt;
    }

    void set_text_value(const string_type& val) {
        kind_ = kind::text;
        value_ = text_node_type(val, allocator_);
    }

    void set_text_value(string_type&& val) {
        kind_ = kind::text;
        value_ = text_node_type(std::move(val), allocator_);
    }

    // Comment operations
    [[nodiscard]] std::optional<string_type> comment() const {
        if (kind_ == kind::comment) {
            return std::get<comment_node_type>(value_).content();
        }
        return std::nullopt;
    }

    void set_comment(const string_type& content) {
        kind_ = kind::comment;
        value_ = comment_node_type(content, allocator_);
    }

    // Visitor support
    template<typename Visitor>
    [[nodiscard]] decltype(auto) visit(Visitor&& v) {
        return visit_impl(std::forward<Visitor>(v), std::integral_constant<bool, std::is_const_v<Visitor>>{});
    }

    template<typename Visitor>
    [[nodiscard]] decltype(auto) visit(Visitor&& v) const {
        switch (kind_) {
            case kind::null:
                return v.visit_null(*this);
            case kind::element:
                return v.visit_element(*this);
            case kind::text:
                return v.visit_text(*this);
            case kind::sequence:
                return v.visit_sequence(*this);
            case kind::mapping:
                return v.visit_mapping(*this);
            case kind::comment:
                return v.visit_comment(*this);
        }
        throw std::logic_error("Invalid node kind");
    }

private:
    void initialize_for_kind(kind k) {
        switch (k) {
            case kind::null:
                value_ = std::monostate{};
                break;
            case kind::sequence:
                value_ = sequence_type(allocator_);
                break;
            case kind::text:
                value_ = text_node_type(allocator_);
                break;
            case kind::comment:
                value_ = comment_node_type(allocator_);
                break;
            case kind::element:
            case kind::mapping:
            default:
                value_ = std::monostate{};
                break;
        }
        kind_ = k;
    }

    template<typename Visitor>
    [[nodiscard]] decltype(auto) visit_impl(Visitor&& v, std::false_type) {
        switch (kind_) {
            case kind::null:
                return v.visit_null(*this);
            case kind::element:
                return v.visit_element(*this);
            case kind::text:
                return v.visit_text(*this);
            case kind::sequence:
                return v.visit_sequence(*this);
            case kind::mapping:
                return v.visit_mapping(*this);
            case kind::comment:
                return v.visit_comment(*this);
        }
        throw std::logic_error("Invalid node kind");
    }

    string_type name_;
    variant_type value_;
    attributes_map attributes_;
    members_map members_;
    allocator_type allocator_;
    kind kind_{kind::null};
};

// Type aliases
using object = basic_object<char, std::allocator<char>>;
using wobject = basic_object<wchar_t, std::allocator<wchar_t>>;

};
namespace detail {

template<typename CharT, typename Traits, typename Alloc>
[[nodiscard]] bool matches_path_name(
    const std::basic_string<CharT, Traits, Alloc>& value,
    std::string_view path_name) {
    if constexpr (std::is_same_v<CharT, char>) {
        return value == path_name;
    } else {
        if (value.size() != path_name.size()) {
            return false;
        }
        for (std::size_t i = 0; i < path_name.size(); ++i) {
            if (value[i] != static_cast<CharT>(static_cast<unsigned char>(path_name[i]))) {
                return false;
            }
        }
        return true;
    }
}

template<typename Object, typename Pointer>
using pointer_vector = std::vector<Pointer, typename std::allocator_traits<typename Object::allocator_type>::template rebind_alloc<Pointer>>;

template<std::size_t MaxNameLength>
struct path_segment {
    std::array<char, MaxNameLength + 1> name{};
    std::size_t name_length{};
    std::size_t index{};
    bool has_index{};
    bool is_attribute{};

    [[nodiscard]] constexpr std::string_view name_view() const noexcept {
        return std::string_view(name.data(), name_length);
    }
};

consteval void fail_parse(std::string_view message) {
    throw std::logic_error(std::string(message));
}

[[nodiscard]] consteval std::size_t count_segments(std::string_view path) {
    std::size_t count = 0;
    std::size_t offset = 0;
    while (offset < path.size()) {
        while (offset < path.size() && path[offset] == '/') {
            ++offset;
        }
        if (offset >= path.size()) {
            break;
        }
        ++count;
        while (offset < path.size() && path[offset] != '/') {
            ++offset;
        }
    }
    return count;
}

[[nodiscard]] consteval std::size_t max_segment_name_length(std::string_view path) {
    std::size_t max_length = 0;
    std::size_t offset = 0;
    while (offset < path.size()) {
        while (offset < path.size() && path[offset] == '/') {
            ++offset;
        }
        if (offset >= path.size()) {
            break;
        }

        if (path[offset] == '@') {
            ++offset;
        }

        std::size_t name_length = 0;
        while (offset < path.size() && path[offset] != '/' && path[offset] != '[') {
            ++name_length;
            ++offset;
        }

        max_length = std::max(max_length, name_length);
        while (offset < path.size() && path[offset] != '/') {
            ++offset;
        }
    }
    return max_length;
}

[[nodiscard]] consteval std::size_t parse_index(std::string_view path, std::size_t& offset) {
    if (offset >= path.size() || path[offset] != '[') {
        fail_parse("expected '[' in path segment index");
    }

    ++offset;
    if (offset >= path.size() || path[offset] < '0' || path[offset] > '9') {
        fail_parse("path segment index must contain digits");
    }

    std::size_t value = 0;
    while (offset < path.size() && path[offset] >= '0' && path[offset] <= '9') {
        value = (value * 10U) + static_cast<std::size_t>(path[offset] - '0');
        ++offset;
    }

    if (offset >= path.size() || path[offset] != ']') {
        fail_parse("expected closing ']' in path segment index");
    }
    ++offset;

    if (value == 0) {
        fail_parse("XPath-style indices are 1-based and must be greater than zero");
    }

    return value - 1U;
}

template<std::size_t SegmentCount, std::size_t MaxNameLength>
[[nodiscard]] consteval auto build_segments(std::string_view path) {
    std::array<path_segment<MaxNameLength>, SegmentCount> result{};
    std::size_t segment_index = 0;
    std::size_t offset = 0;

    while (offset < path.size()) {
        while (offset < path.size() && path[offset] == '/') {
            ++offset;
        }
        if (offset >= path.size()) {
            break;
        }

        auto& segment = result[segment_index++];
        if (path[offset] == '@') {
            segment.is_attribute = true;
            ++offset;
        }

        while (offset < path.size() && path[offset] != '/' && path[offset] != '[') {
            if (segment.name_length >= MaxNameLength) {
                fail_parse("path segment exceeds supported compile-time name length");
            }
            segment.name[segment.name_length++] = path[offset++];
        }

        if (offset < path.size() && path[offset] == '[') {
            segment.index = parse_index(path, offset);
            segment.has_index = true;
        }

        if (segment.is_attribute && segment.has_index) {
            fail_parse("attribute segments cannot be indexed");
        }

        if (offset < path.size() && path[offset] != '/') {
            fail_parse("unsupported path syntax");
        }
    }

    return result;
}

template<ndof::fixed_string Path>
struct parsed_path {
    static constexpr auto raw = Path.view();
    static constexpr std::size_t segment_count = count_segments(raw);
    static constexpr std::size_t max_name_length = max_segment_name_length(raw);
    static constexpr auto segments = build_segments<segment_count, max_name_length>(raw);
};

template<typename Result>
void append_if_present(Result& result, const typename Result::value_type candidate) {
    if (candidate != nullptr) {
        result.push_back(candidate);
    }
}

template<typename Node>
[[nodiscard]] bool is_query_metadata(const Node& node) {
    using object_type = std::remove_const_t<Node>;
    return node.type() == object_type::kind::comment;
}

template<typename Node>
[[nodiscard]] auto find_named_attribute(Node& current, std::string_view name)
    -> std::conditional_t<std::is_const_v<Node>, const typename std::remove_const_t<Node>::value_type*, typename std::remove_const_t<Node>::value_type*> {
    for (auto& [attribute_name, value] : current.attributes()) {
        if (matches_path_name(attribute_name, name)) {
            return &value;
        }
    }
    return nullptr;
}

template<typename Node, typename Result>
void append_named_children(Node& current, std::string_view name, std::optional<std::size_t> index, Result& result) {
    std::size_t match_index = 0;
    for (auto& child : current.children()) {
        if (!matches_path_name(child.name(), name)) {
            continue;
        }

        if (!index.has_value()) {
            result.push_back(&child);
            continue;
        }

        if (match_index == *index) {
            result.push_back(&child);
            return;
        }
        ++match_index;
    }
}

template<typename Node, typename Result>
void append_sequence_items(Node& current, std::optional<std::size_t> index, Result& result) {
    using object_type = std::remove_const_t<Node>;

    if (current.type() != object_type::kind::sequence) {
        return;
    }

    if (index.has_value()) {
        std::size_t semantic_index = 0;
        for (auto& item : current.elements()) {
            if (is_query_metadata(item)) {
                continue;
            }
            if (semantic_index == *index) {
                result.push_back(&item);
                return;
            }
            ++semantic_index;
        }
        return;
    }

    for (auto& item : current.elements()) {
        if (is_query_metadata(item)) {
            continue;
        }
        result.push_back(&item);
    }
}

template<typename Node, typename Result>
void append_named_member(Node& current, std::string_view name, std::optional<std::size_t> index, Result& result) {
    using object_type = std::remove_const_t<Node>;

    for (auto& [member_name, value] : current.members()) {
        if (!matches_path_name(member_name, name)) {
            continue;
        }

        if (!index.has_value()) {
            result.push_back(&value);
            return;
        }

        if (value.type() == object_type::kind::sequence) {
            append_sequence_items(value, index, result);
            return;
        }

        if (*index == 0) {
            result.push_back(&value);
        }
        return;
    }
}

template<typename Node, std::size_t MaxNameLength, typename Result>
void append_matches_for_segment(Node& current, const path_segment<MaxNameLength>& segment, Result& result) {
    using object_type = std::remove_const_t<Node>;
    const auto name = segment.name_view();
    const auto index = segment.has_index ? std::optional<std::size_t>(segment.index) : std::nullopt;

    if (segment.is_attribute) {
        append_if_present(result, find_named_attribute(current, name));
        return;
    }

    if (name.empty()) {
        append_sequence_items(current, index, result);
        return;
    }

    if (!index.has_value() && current.type() == object_type::kind::element && matches_path_name(current.name(), name)) {
        result.push_back(&current);
    }

    append_named_member(current, name, index, result);
    append_named_children(current, name, index, result);
}

template<fixed_string Path, std::size_t SegmentIndex>
struct path_evaluator {
    template<typename Pointer, typename CharT, typename Allocator>
    [[nodiscard]] static auto run(const pointer_vector<ndof::basic_object<CharT, Allocator>, Pointer>& current_nodes, const Allocator& allocator) {
        using object_type = basic_object<CharT, Allocator>;
        using result_type = pointer_vector<object_type, Pointer>;

        result_type next_nodes{typename result_type::allocator_type(allocator)};
        const auto& segment = parsed_path<Path>::segments[SegmentIndex];
        for (Pointer current : current_nodes) {
            append_matches_for_segment(*current, segment, next_nodes);
        }

        if constexpr (SegmentIndex + 1U == parsed_path<Path>::segment_count) {
            return next_nodes;
        } else {
            return path_evaluator<Path, SegmentIndex + 1U>::template run<Pointer, CharT, Allocator>(next_nodes, allocator);
        }
    }
};

} // namespace detail

template<fixed_string Path>
struct xpath_query {
    template<typename CharT, typename Allocator>
    [[nodiscard]] static auto find_all(basic_object<CharT, Allocator>& root) {
        using object_type = basic_object<CharT, Allocator>;
        using result_type = detail::pointer_vector<object_type, object_type*>;

        if constexpr (detail::parsed_path<Path>::segment_count == 0) {
            result_type matches(typename result_type::allocator_type(root.allocator()));
            matches.push_back(&root);
            return matches;
        } else {
            result_type roots(typename result_type::allocator_type(root.allocator()));
            roots.push_back(&root);
            return detail::path_evaluator<Path, 0>::template run<object_type*, CharT, Allocator>(roots, root.allocator());
        }
    }

    template<typename CharT, typename Allocator>
    [[nodiscard]] static auto find_all(const basic_object<CharT, Allocator>& root) {
        using object_type = basic_object<CharT, Allocator>;
        using result_type = detail::pointer_vector<object_type, const object_type*>;

        if constexpr (detail::parsed_path<Path>::segment_count == 0) {
            result_type matches(typename result_type::allocator_type(root.allocator()));
            matches.push_back(&root);
            return matches;
        } else {
            result_type roots(typename result_type::allocator_type(root.allocator()));
            roots.push_back(&root);
            return detail::path_evaluator<Path, 0>::template run<const object_type*, CharT, Allocator>(roots, root.allocator());
        }
    }

    template<typename CharT, typename Allocator>
    [[nodiscard]] static basic_object<CharT, Allocator>* find_first(basic_object<CharT, Allocator>& root) {
        auto matches = find_all(root);
        return matches.empty() ? nullptr : matches.front();
    }

    template<typename CharT, typename Allocator>
    [[nodiscard]] static const basic_object<CharT, Allocator>* find_first(const basic_object<CharT, Allocator>& root) {
        auto matches = find_all(root);
        return matches.empty() ? nullptr : matches.front();
    }
};

template<fixed_string Path, typename CharT, typename Allocator>
[[nodiscard]] auto find_all(basic_object<CharT, Allocator>& root) {
    return xpath_query<Path>::find_all(root);
}

template<fixed_string Path, typename CharT, typename Allocator>
[[nodiscard]] auto find_all(const basic_object<CharT, Allocator>& root) {
    return xpath_query<Path>::find_all(root);
}

template<fixed_string Path, typename CharT, typename Allocator>
[[nodiscard]] basic_object<CharT, Allocator>* find_first(basic_object<CharT, Allocator>& root) {
    return xpath_query<Path>::find_first(root);
}

template<fixed_string Path, typename CharT, typename Allocator>
[[nodiscard]] const basic_object<CharT, Allocator>* find_first(const basic_object<CharT, Allocator>& root) {
    return xpath_query<Path>::find_first(root);
}

} // namespace ndof