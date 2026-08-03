// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

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

template<std::size_t N>
struct fixed_string {
    char value[N]{};

    constexpr fixed_string(const char (&input)[N]) {
        std::copy_n(input, N, value);
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return std::string_view(value, N - 1);
    }
};

template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template<typename Allocator = std::allocator<std::byte>>
class basic_object {
public:
    using allocator_type = Allocator;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using char_allocator = typename allocator_traits::template rebind_alloc<char>;
    using allocated_string = std::basic_string<char, std::char_traits<char>, char_allocator>;

    enum class kind : std::uint8_t {
        null_value,
        string,
        comment,
        integer,
        floating_point,
        boolean,
        mapping,
        sequence,
        element
    };

    using value_type = basic_object;
    using member_type = std::pair<allocated_string, value_type>;
    using attribute_type = std::pair<allocated_string, value_type>;
    using member_allocator = typename allocator_traits::template rebind_alloc<member_type>;
    using child_allocator = typename allocator_traits::template rebind_alloc<value_type>;
    using attribute_allocator = typename allocator_traits::template rebind_alloc<attribute_type>;
    using scalar_type = std::variant<std::monostate, allocated_string, std::int64_t, double, bool>;
    using member_container = std::vector<member_type, member_allocator>;
    using child_container = std::vector<value_type, child_allocator>;
    using attribute_container = std::vector<attribute_type, attribute_allocator>;

    basic_object()
        : basic_object(allocator_type()) {
    }

    explicit basic_object(const allocator_type& allocator)
        : allocator_(allocator),
          name_(char_allocator(allocator)),
          members_(member_allocator(allocator)),
          elements_(child_allocator(allocator)),
          children_(child_allocator(allocator)),
          attributes_(attribute_allocator(allocator)) {
    }

    basic_object([[maybe_unused]] std::allocator_arg_t allocator_tag, const allocator_type& allocator)
        : basic_object(allocator) {
    }

    basic_object(const basic_object& other)
        : basic_object(std::allocator_arg, other.get_allocator(), other) {
    }

    template<typename OtherAllocator>
    requires (!std::is_same_v<OtherAllocator, allocator_type>)
    basic_object(const basic_object<OtherAllocator>& other)
        : basic_object(std::allocator_arg, allocator_type(), other) {
    }

        basic_object([[maybe_unused]] std::allocator_arg_t allocator_tag, const allocator_type& allocator, const basic_object& other)
        : allocator_(allocator),
          name_(char_allocator(allocator)),
          members_(member_allocator(allocator)),
          elements_(child_allocator(allocator)),
          children_(child_allocator(allocator)),
          attributes_(attribute_allocator(allocator)) {
        copy_from_other(other);
    }

    template<typename OtherAllocator>
    requires (!std::is_same_v<OtherAllocator, allocator_type>)
    basic_object([[maybe_unused]] std::allocator_arg_t allocator_tag, const allocator_type& allocator, const basic_object<OtherAllocator>& other)
        : allocator_(allocator),
          name_(char_allocator(allocator)),
          members_(member_allocator(allocator)),
          elements_(child_allocator(allocator)),
          children_(child_allocator(allocator)),
          attributes_(attribute_allocator(allocator)) {
        copy_from_other(other);
    }

    basic_object(basic_object&& other) noexcept
        : basic_object(std::allocator_arg, other.get_allocator(), std::move(other)) {
    }

    template<typename OtherAllocator>
    requires (!std::is_same_v<OtherAllocator, allocator_type>)
    basic_object(basic_object<OtherAllocator>&& other)
        : basic_object(std::allocator_arg, allocator_type(), std::move(other)) {
    }

        basic_object([[maybe_unused]] std::allocator_arg_t allocator_tag, const allocator_type& allocator, basic_object&& other)
        : allocator_(allocator),
          name_(char_allocator(allocator)),
          members_(member_allocator(allocator)),
          elements_(child_allocator(allocator)),
          children_(child_allocator(allocator)),
          attributes_(attribute_allocator(allocator)) {
        move_from_other(std::move(other));
    }

    template<typename OtherAllocator>
    requires (!std::is_same_v<OtherAllocator, allocator_type>)
    basic_object([[maybe_unused]] std::allocator_arg_t allocator_tag, const allocator_type& allocator, basic_object<OtherAllocator>&& other)
        : allocator_(allocator),
          name_(char_allocator(allocator)),
          members_(member_allocator(allocator)),
          elements_(child_allocator(allocator)),
          children_(child_allocator(allocator)),
          attributes_(attribute_allocator(allocator)) {
        move_from_other(std::move(other));
    }

    basic_object& operator=(const basic_object& other) {
        if (this == &other) {
            return *this;
        }

        copy_from_other(other);
        return *this;
    }

    template<typename OtherAllocator>
    requires (!std::is_same_v<OtherAllocator, allocator_type>)
    basic_object& operator=(const basic_object<OtherAllocator>& other) {
        copy_from_other(other);
        return *this;
    }

    basic_object& operator=(basic_object&& other) {
        if (this == &other) {
            return *this;
        }

        move_from_other(std::move(other));
        return *this;
    }

    template<typename OtherAllocator>
    requires (!std::is_same_v<OtherAllocator, allocator_type>)
    basic_object& operator=(basic_object<OtherAllocator>&& other) {
        move_from_other(std::move(other));
        return *this;
    }

    [[nodiscard]] static basic_object null_value(const allocator_type& allocator = allocator_type()) {
        return basic_object(kind::null_value, allocator);
    }

    [[nodiscard]] static basic_object string(std::string_view value, const allocator_type& allocator = allocator_type()) {
        basic_object result(kind::string, allocator);
        result.scalar_ = allocated_string(value, char_allocator(allocator));
        return result;
    }

    [[nodiscard]] static basic_object comment(std::string_view value, const allocator_type& allocator = allocator_type()) {
        basic_object result(kind::comment, allocator);
        result.scalar_ = allocated_string(value, char_allocator(allocator));
        return result;
    }

    [[nodiscard]] static basic_object integer(std::int64_t value, const allocator_type& allocator = allocator_type()) {
        basic_object result(kind::integer, allocator);
        result.scalar_ = value;
        return result;
    }

    [[nodiscard]] static basic_object floating_point(double value, const allocator_type& allocator = allocator_type()) {
        basic_object result(kind::floating_point, allocator);
        result.scalar_ = value;
        return result;
    }

    [[nodiscard]] static basic_object boolean(bool value, const allocator_type& allocator = allocator_type()) {
        basic_object result(kind::boolean, allocator);
        result.scalar_ = value;
        return result;
    }

    [[nodiscard]] static basic_object mapping(const allocator_type& allocator = allocator_type()) {
        return basic_object(kind::mapping, allocator);
    }

    [[nodiscard]] static basic_object sequence(const allocator_type& allocator = allocator_type()) {
        return basic_object(kind::sequence, allocator);
    }

    [[nodiscard]] static basic_object element(std::string_view name, const allocator_type& allocator = allocator_type()) {
        basic_object result(kind::element, allocator);
        result.name_ = allocated_string(name, char_allocator(allocator));
        return result;
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept {
        return allocator_;
    }

    [[nodiscard]] kind type() const noexcept {
        return type_;
    }

    [[nodiscard]] const allocated_string& name() const noexcept {
        return name_;
    }

    [[nodiscard]] const scalar_type& scalar() const noexcept {
        return scalar_;
    }

    [[nodiscard]] const member_container& members() const noexcept {
        return members_;
    }

    [[nodiscard]] const child_container& elements() const noexcept {
        return elements_;
    }

    [[nodiscard]] const child_container& children() const noexcept {
        return children_;
    }

    [[nodiscard]] const attribute_container& attributes() const noexcept {
        return attributes_;
    }

    [[nodiscard]] const allocated_string* as_string() const noexcept {
        return std::get_if<allocated_string>(&scalar_);
    }

    [[nodiscard]] const allocated_string* as_comment() const noexcept {
        if (type_ != kind::comment) {
            return nullptr;
        }
        return std::get_if<allocated_string>(&scalar_);
    }

    [[nodiscard]] const std::int64_t* as_integer() const noexcept {
        return std::get_if<std::int64_t>(&scalar_);
    }

    [[nodiscard]] const double* as_floating_point() const noexcept {
        return std::get_if<double>(&scalar_);
    }

    [[nodiscard]] const bool* as_boolean() const noexcept {
        return std::get_if<bool>(&scalar_);
    }

    basic_object& add_member(std::string_view key, basic_object value) {
        ensure_kind(kind::mapping);
        members_.emplace_back(allocated_string(key, char_allocator(allocator_)), normalize_value(std::move(value)));
        return *this;
    }

    basic_object& add_element(basic_object value) {
        ensure_kind(kind::sequence);
        elements_.push_back(normalize_value(std::move(value)));
        return *this;
    }

    basic_object& add_child(basic_object value) {
        ensure_kind(kind::element);
        children_.push_back(normalize_value(std::move(value)));
        return *this;
    }

    basic_object& add_attribute(std::string_view key, basic_object value) {
        ensure_kind(kind::element);
        attributes_.emplace_back(allocated_string(key, char_allocator(allocator_)), normalize_value(std::move(value)));
        return *this;
    }

    template<typename OtherAllocator>
    basic_object& add_member(std::string_view key, basic_object<OtherAllocator> value) {
        ensure_kind(kind::mapping);
        members_.emplace_back(allocated_string(key, char_allocator(allocator_)), normalize_value(std::move(value)));
        return *this;
    }

    template<typename OtherAllocator>
    basic_object& add_element(basic_object<OtherAllocator> value) {
        ensure_kind(kind::sequence);
        elements_.push_back(normalize_value(std::move(value)));
        return *this;
    }

    template<typename OtherAllocator>
    basic_object& add_child(basic_object<OtherAllocator> value) {
        ensure_kind(kind::element);
        children_.push_back(normalize_value(std::move(value)));
        return *this;
    }

    template<typename OtherAllocator>
    basic_object& add_attribute(std::string_view key, basic_object<OtherAllocator> value) {
        ensure_kind(kind::element);
        attributes_.emplace_back(allocated_string(key, char_allocator(allocator_)), normalize_value(std::move(value)));
        return *this;
    }

private:
    explicit basic_object(kind value, const allocator_type& allocator = allocator_type())
                : allocator_(allocator),
                    type_(value),
                    name_(char_allocator(allocator)),
                    members_(member_allocator(allocator)),
                    elements_(child_allocator(allocator)),
                    children_(child_allocator(allocator)),
                    attributes_(attribute_allocator(allocator)) {
    }

    [[nodiscard]] static scalar_type clone_scalar(const scalar_type& scalar, const allocator_type& allocator) {
        return std::visit(
            [&allocator](const auto& value) -> scalar_type {
                using value_type = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<value_type, allocated_string>) {
                    return scalar_type(std::in_place_type<allocated_string>, value, char_allocator(allocator));
                } else {
                    return scalar_type(value);
                }
            },
            scalar);
    }

    [[nodiscard]] static scalar_type move_scalar(scalar_type&& scalar, const allocator_type& allocator) {
        return std::visit(
            [&allocator](auto&& value) -> scalar_type {
                using value_type = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<value_type, allocated_string>) {
                    return scalar_type(std::in_place_type<allocated_string>, std::forward<value_type>(value), char_allocator(allocator));
                } else {
                    return scalar_type(std::forward<value_type>(value));
                }
            },
            std::move(scalar));
    }

    template<typename OtherAllocator>
    [[nodiscard]] static scalar_type clone_scalar(
        const typename basic_object<OtherAllocator>::scalar_type& scalar,
        const allocator_type& allocator) {
        return std::visit(
            [&allocator](const auto& value) -> scalar_type {
                using value_type = std::decay_t<decltype(value)>;
                using other_string = typename basic_object<OtherAllocator>::allocated_string;
                if constexpr (std::is_same_v<value_type, other_string>) {
                    return scalar_type(std::in_place_type<allocated_string>, value.c_str(), char_allocator(allocator));
                } else {
                    return scalar_type(value);
                }
            },
            scalar);
    }

    template<typename OtherAllocator>
    [[nodiscard]] static scalar_type move_scalar(
        typename basic_object<OtherAllocator>::scalar_type&& scalar,
        const allocator_type& allocator) {
        return std::visit(
            [&allocator](auto&& value) -> scalar_type {
                using value_type = std::decay_t<decltype(value)>;
                using other_string = typename basic_object<OtherAllocator>::allocated_string;
                if constexpr (std::is_same_v<value_type, other_string>) {
                    return scalar_type(
                        std::in_place_type<allocated_string>,
                        std::forward<value_type>(value),
                        char_allocator(allocator));
                } else {
                    return scalar_type(std::forward<value_type>(value));
                }
            },
            std::move(scalar));
    }

    // Rebinds an incoming node to this object's allocator so child/member storage
    // stays allocator-consistent even when values are created with a different allocator.
    [[nodiscard]] basic_object normalize_value(basic_object&& value) const {
        if (allocators_equal(allocator_, value.get_allocator())) {
            return std::move(value);
        }
        // TODO: Check this. If the allocators are not equal, we need to rebind the value to this object's allocator.
        // and deallocate it from the original allocator.  This needs to be a deep copy of the value, and all of its children, members, and attributes.
        return basic_object(std::allocator_arg, allocator_, std::move(value));
    }

    template<typename OtherAllocator>
    [[nodiscard]] basic_object normalize_value(const basic_object<OtherAllocator>& value) const {
        return basic_object(std::allocator_arg, allocator_, value);
    }

    template<typename OtherAllocator>
    [[nodiscard]] basic_object normalize_value(basic_object<OtherAllocator>&& value) const {
        return basic_object(std::allocator_arg, allocator_, std::move(value));
    }

    template<typename OtherAllocator>
    void copy_from_other(const basic_object<OtherAllocator>& other) {
        type_ = other.type_;
        name_ = allocated_string(other.name_, char_allocator(allocator_));
        scalar_ = clone_scalar<OtherAllocator>(other.scalar_, allocator_);

        member_container rebound_members{member_allocator(allocator_)};
        rebound_members.reserve(other.members_.size());
        for (const auto& [key, value] : other.members_) {
            rebound_members.emplace_back(
                allocated_string(key, char_allocator(allocator_)),
                basic_object(std::allocator_arg, allocator_, value));
        }

        child_container rebound_elements{child_allocator(allocator_)};
        rebound_elements.reserve(other.elements_.size());
        for (const auto& value : other.elements_) {
            rebound_elements.push_back(basic_object(std::allocator_arg, allocator_, value));
        }

        child_container rebound_children{child_allocator(allocator_)};
        rebound_children.reserve(other.children_.size());
        for (const auto& value : other.children_) {
            rebound_children.push_back(basic_object(std::allocator_arg, allocator_, value));
        }

        attribute_container rebound_attributes{attribute_allocator(allocator_)};
        rebound_attributes.reserve(other.attributes_.size());
        for (const auto& [key, value] : other.attributes_) {
            rebound_attributes.emplace_back(
                allocated_string(key, char_allocator(allocator_)),
                basic_object(std::allocator_arg, allocator_, value));
        }

        members_ = std::move(rebound_members);
        elements_ = std::move(rebound_elements);
        children_ = std::move(rebound_children);
        attributes_ = std::move(rebound_attributes);
    }

    template<typename OtherAllocator>
    void move_from_other(basic_object<OtherAllocator>&& other) {
        if constexpr (std::is_same_v<OtherAllocator, allocator_type>) {
            if (allocators_equal(allocator_, other.allocator_)) {
                type_ = other.type_;
                name_ = std::move(other.name_);
                scalar_ = std::move(other.scalar_);
                members_ = std::move(other.members_);
                elements_ = std::move(other.elements_);
                children_ = std::move(other.children_);
                attributes_ = std::move(other.attributes_);
                return;
            }
        }

        type_ = other.type_;
        name_ = allocated_string(std::move(other.name_), char_allocator(allocator_));
        scalar_ = move_scalar<OtherAllocator>(std::move(other.scalar_), allocator_);

        member_container rebound_members{member_allocator(allocator_)};
        rebound_members.reserve(other.members_.size());
        for (auto& [key, value] : other.members_) {
            rebound_members.emplace_back(
                allocated_string(std::move(key), char_allocator(allocator_)),
                basic_object(std::allocator_arg, allocator_, std::move(value)));
        }

        child_container rebound_elements{child_allocator(allocator_)};
        rebound_elements.reserve(other.elements_.size());
        for (auto& value : other.elements_) {
            rebound_elements.push_back(basic_object(std::allocator_arg, allocator_, std::move(value)));
        }

        child_container rebound_children{child_allocator(allocator_)};
        rebound_children.reserve(other.children_.size());
        for (auto& value : other.children_) {
            rebound_children.push_back(basic_object(std::allocator_arg, allocator_, std::move(value)));
        }

        attribute_container rebound_attributes{attribute_allocator(allocator_)};
        rebound_attributes.reserve(other.attributes_.size());
        for (auto& [key, value] : other.attributes_) {
            rebound_attributes.emplace_back(
                allocated_string(std::move(key), char_allocator(allocator_)),
                basic_object(std::allocator_arg, allocator_, std::move(value)));
        }

        members_ = std::move(rebound_members);
        elements_ = std::move(rebound_elements);
        children_ = std::move(rebound_children);
        attributes_ = std::move(rebound_attributes);
    }

    [[nodiscard]] static bool allocators_equal(const allocator_type& lhs, const allocator_type& rhs) {
        if constexpr (requires(const allocator_type& a, const allocator_type& b) { a == b; }) {
            return lhs == rhs;
        } else {
            return false;
        }
    }

    void ensure_kind(kind expected) {
        if (type_ == kind::null_value) {
            type_ = expected;
            return;
        }
        if (type_ != expected) {
            throw std::logic_error("ndof::object kind mismatch");
        }
    }

    [[no_unique_address]] allocator_type allocator_{};
    kind type_{kind::null_value};
    allocated_string name_;
    scalar_type scalar_;
    member_container members_;
    child_container elements_;
    child_container children_;
    attribute_container attributes_;

    template<typename>
    friend class basic_object;
};

using object = basic_object<>;

namespace detail {

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

template<fixed_string Path>
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
        if (attribute_name == name) {
            return &value;
        }
    }
    return nullptr;
}

template<typename Node, typename Result>
void append_named_children(Node& current, std::string_view name, std::optional<std::size_t> index, Result& result) {
    std::size_t match_index = 0;
    for (auto& child : current.children()) {
        if (child.name() != name) {
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
        if (member_name != name) {
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

    if (!index.has_value() && current.type() == object_type::kind::element && current.name() == name) {
        result.push_back(&current);
    }

    append_named_member(current, name, index, result);
    append_named_children(current, name, index, result);
}

template<fixed_string Path, std::size_t SegmentIndex>
struct path_evaluator {
    template<typename Pointer, typename Allocator>
    [[nodiscard]] static auto run(const pointer_vector<basic_object<Allocator>, Pointer>& current_nodes, const Allocator& allocator) {
        using object_type = basic_object<Allocator>;
        using result_type = pointer_vector<object_type, Pointer>;

        result_type next_nodes{typename result_type::allocator_type(allocator)};
        const auto& segment = parsed_path<Path>::segments[SegmentIndex];
        for (Pointer current : current_nodes) {
            append_matches_for_segment(*current, segment, next_nodes);
        }

        if constexpr (SegmentIndex + 1U == parsed_path<Path>::segment_count) {
            return next_nodes;
        } else {
            return path_evaluator<Path, SegmentIndex + 1U>::run(next_nodes, allocator);
        }
    }
};

} // namespace detail

template<fixed_string Path>
struct xpath_query {
    template<typename Allocator>
    [[nodiscard]] static auto find_all(basic_object<Allocator>& root) {
        using object_type = basic_object<Allocator>;
        using result_type = detail::pointer_vector<object_type, object_type*>;

        if constexpr (detail::parsed_path<Path>::segment_count == 0) {
            result_type matches(typename result_type::allocator_type(root.get_allocator()));
            matches.push_back(&root);
            return matches;
        } else {
            result_type roots(typename result_type::allocator_type(root.get_allocator()));
            roots.push_back(&root);
            return detail::path_evaluator<Path, 0>::run(roots, root.get_allocator());
        }
    }

    template<typename Allocator>
    [[nodiscard]] static auto find_all(const basic_object<Allocator>& root) {
        using object_type = basic_object<Allocator>;
        using result_type = detail::pointer_vector<object_type, const object_type*>;

        if constexpr (detail::parsed_path<Path>::segment_count == 0) {
            result_type matches(typename result_type::allocator_type(root.get_allocator()));
            matches.push_back(&root);
            return matches;
        } else {
            result_type roots(typename result_type::allocator_type(root.get_allocator()));
            roots.push_back(&root);
            return detail::path_evaluator<Path, 0>::run(roots, root.get_allocator());
        }
    }

    template<typename Allocator>
    [[nodiscard]] static basic_object<Allocator>* find_first(basic_object<Allocator>& root) {
        auto matches = find_all(root);
        return matches.empty() ? nullptr : matches.front();
    }

    template<typename Allocator>
    [[nodiscard]] static const basic_object<Allocator>* find_first(const basic_object<Allocator>& root) {
        auto matches = find_all(root);
        return matches.empty() ? nullptr : matches.front();
    }
};

template<fixed_string Path, typename Allocator>
[[nodiscard]] auto find_all(basic_object<Allocator>& root) {
    return xpath_query<Path>::find_all(root);
}

template<fixed_string Path, typename Allocator>
[[nodiscard]] auto find_all(const basic_object<Allocator>& root) {
    return xpath_query<Path>::find_all(root);
}

template<fixed_string Path, typename Allocator>
[[nodiscard]] basic_object<Allocator>* find_first(basic_object<Allocator>& root) {
    return xpath_query<Path>::find_first(root);
}

template<fixed_string Path, typename Allocator>
[[nodiscard]] const basic_object<Allocator>* find_first(const basic_object<Allocator>& root) {
    return xpath_query<Path>::find_first(root);
}

} // namespace ndof