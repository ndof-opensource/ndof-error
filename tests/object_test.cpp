// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0

#include "ndof/error/object.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory_resource>

namespace {

using TitlePath = ndof::xpath_query<"/catalog/book[2]/title">;

static_assert(ndof::detail::parsed_path<"/catalog/book[2]/title">::segment_count == 3);

template<typename Object>
Object build_xml_like_document(const typename Object::allocator_type& allocator = typename Object::allocator_type()) {
    Object catalog = Object::element("catalog", allocator);

    Object first_book = Object::element("book", allocator);
    first_book.add_attribute("id", Object::string("bk101", allocator));
    Object first_title = Object::element("title", allocator);
    first_title.add_child(Object::string("First", allocator));
    first_book.add_child(std::move(first_title));

    Object second_book = Object::element("book", allocator);
    second_book.add_attribute("id", Object::string("bk102", allocator));
    Object second_title = Object::element("title", allocator);
    second_title.add_child(Object::string("Second", allocator));
    second_book.add_child(std::move(second_title));

    catalog.add_child(std::move(first_book));
    catalog.add_child(std::move(second_book));

    return catalog;
}

template<typename Object>
Object build_mapping_document(const typename Object::allocator_type& allocator = typename Object::allocator_type()) {
    Object document = Object::mapping(allocator);
    Object users = Object::sequence(allocator);

    Object first_user = Object::mapping(allocator);
    first_user.add_member("name", Object::string("Ada", allocator));

    Object second_user = Object::mapping(allocator);
    second_user.add_member("name", Object::string("Grace", allocator));

    users.add_element(std::move(first_user));
    users.add_element(std::move(second_user));
    document.add_member("users", std::move(users));

    return document;
}

template<typename Object>
Object build_commented_mapping_document(const typename Object::allocator_type& allocator = typename Object::allocator_type()) {
    Object document = Object::mapping(allocator);
    Object users = Object::sequence(allocator);

    Object first_user = Object::mapping(allocator);
    first_user.add_member("name", Object::string("Ada", allocator));

    Object second_user = Object::mapping(allocator);
    second_user.add_member("name", Object::string("Grace", allocator));

    users.add_element(std::move(first_user));
    users.add_element(Object::comment("between users", allocator));
    users.add_element(std::move(second_user));
    document.add_member("users", std::move(users));

    return document;
}

template<typename Object>
void expect_tree_allocator(const Object& node, const typename Object::allocator_type& expected_allocator) {
    EXPECT_EQ(node.get_allocator(), expected_allocator);

    for (const auto& [_, value] : node.members()) {
        expect_tree_allocator(value, expected_allocator);
    }

    for (const auto& value : node.elements()) {
        expect_tree_allocator(value, expected_allocator);
    }

    for (const auto& value : node.children()) {
        expect_tree_allocator(value, expected_allocator);
    }

    for (const auto& [_, value] : node.attributes()) {
        expect_tree_allocator(value, expected_allocator);
    }
}

TEST(ObjectQuery, ResolvesRepeatedXmlChildrenByCompileTimePath) {
    const ndof::object document = build_xml_like_document<ndof::object>();

    const ndof::object* title = TitlePath::find_first(document);
    ASSERT_NE(title, nullptr);
    ASSERT_EQ(title->children().size(), 1U);

    const auto* value = title->children().front().as_string();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "Second");
}

TEST(ObjectQuery, ResolvesAttributesByCompileTimePath) {
    const ndof::object document = build_xml_like_document<ndof::object>();

    const ndof::object* id = ndof::find_first<"/catalog/book[2]/@id">(document);
    ASSERT_NE(id, nullptr);

    const auto* value = id->as_string();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "bk102");
}

TEST(ObjectQuery, ResolvesSequenceMembersByCompileTimePath) {
    const ndof::object document = build_mapping_document<ndof::object>();

    const ndof::object* name = ndof::find_first<"/users[2]/name">(document);
    ASSERT_NE(name, nullptr);

    const auto* value = name->as_string();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "Grace");
}

TEST(ObjectQuery, ReturnsAllRepeatedMatchesWhenNoIndexIsSpecified) {
    const ndof::object document = build_xml_like_document<ndof::object>();

    const auto matches = ndof::find_all<"/catalog/book/title">(document);
    ASSERT_EQ(matches.size(), 2U);

    const auto* first_value = matches[0]->children().front().as_string();
    const auto* second_value = matches[1]->children().front().as_string();
    ASSERT_NE(first_value, nullptr);
    ASSERT_NE(second_value, nullptr);
    EXPECT_EQ(*first_value, "First");
    EXPECT_EQ(*second_value, "Second");
}

TEST(ObjectQuery, ResolvesPathsForPmrAllocatorBackedObjects) {
    std::array<std::byte, 4096> storage{};
    std::pmr::monotonic_buffer_resource resource(storage.data(), storage.size());
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    using object_type = ndof::basic_object<allocator_type>;

    const allocator_type allocator(&resource);
    const object_type document = build_mapping_document<object_type>(allocator);

    const object_type* name = ndof::find_first<"/users[2]/name">(document);
    ASSERT_NE(name, nullptr);
    ASSERT_EQ(document.get_allocator().resource(), &resource);

    const auto* value = name->as_string();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "Grace");
}

TEST(ObjectQuery, PreservesCommentsWithoutShiftingSequenceIndices) {
    const ndof::object document = build_commented_mapping_document<ndof::object>();

    const ndof::object* name = ndof::find_first<"/users[2]/name">(document);
    ASSERT_NE(name, nullptr);

    const auto* value = name->as_string();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "Grace");

    const auto& users = document.members().front().second;
    ASSERT_EQ(users.elements().size(), 3U);
    EXPECT_EQ(users.elements()[1].type(), ndof::object::kind::comment);

    const auto* comment = users.elements()[1].as_comment();
    ASSERT_NE(comment, nullptr);
    EXPECT_EQ(*comment, "between users");
}

TEST(ObjectAllocator, MoveAcrossDifferentPmrResourcesRebindsWholeTree) {
    std::array<std::byte, 4096> source_storage{};
    std::array<std::byte, 4096> destination_storage{};
    std::pmr::monotonic_buffer_resource source_resource(source_storage.data(), source_storage.size());
    std::pmr::monotonic_buffer_resource destination_resource(destination_storage.data(), destination_storage.size());

    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    using object_type = ndof::basic_object<allocator_type>;

    const allocator_type source_allocator(&source_resource);
    const allocator_type destination_allocator(&destination_resource);
    object_type source = build_xml_like_document<object_type>(source_allocator);

    const object_type moved(std::allocator_arg, destination_allocator, std::move(source));

    EXPECT_EQ(moved.get_allocator(), destination_allocator);
    expect_tree_allocator(moved, destination_allocator);

    const object_type* title = ndof::find_first<"/catalog/book[2]/title">(moved);
    ASSERT_NE(title, nullptr);
    const auto* value = title->children().front().as_string();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "Second");
}

} // namespace