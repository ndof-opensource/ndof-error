#include "ndof/error/conditions_check.hpp"

#include <gtest/gtest.h>

#include <concepts>
#include <string>

namespace {

TEST(ConditionsCheck, RunsBodyBetweenPreconditionsAndPostconditions) {
    std::string order;

    auto builder = ndof::precondition_check({
        ndof::check{[&] {
            order += "pre1 ";
            return true;
        }},
        ndof::check{[&] {
            order += "pre2 ";
            return true;
        }},
    }).postcondition_check({
        ndof::check{[&](int value) {
            order += "post1 ";
            return value == 42;
        }},
        ndof::check{[&](int value) {
            order += "post2";
            return value > 0;
        }},
    });

    static_assert(std::same_as<decltype(builder), ndof::postcondition_check_builder<int, 2>>);

    const auto result = builder.body([&] {
        order += "body ";
        return 42;
    });

    EXPECT_EQ(result, 42);
    EXPECT_EQ(order, "pre1 pre2 body post1 post2");
}

TEST(ConditionsCheck, FailedPreconditionPreventsBodyExecution) {
    bool body_called = false;

    try {
        static_cast<void>(ndof::precondition_check({
            ndof::check{[] { return true; }},
            ndof::check{[] { return false; }},
        }).body([&] { body_called = true; }));
        FAIL() << "Expected a precondition_check_error";
    } catch (const ndof::precondition_check_error& error) {
        EXPECT_EQ(error.index(), 1U);
    }

    EXPECT_FALSE(body_called);
}

TEST(ConditionsCheck, FailedPostconditionReportsItsIndex) {
    try {
        static_cast<void>(ndof::precondition_check({
            ndof::check{[] { return true; }},
        }).postcondition_check({
            ndof::check{[](int) { return true; }},
            ndof::check{[](int) { return false; }},
        }).body([] { return 42; }));
        FAIL() << "Expected a postcondition_check_error";
    } catch (const ndof::postcondition_check_error& error) {
        EXPECT_EQ(error.index(), 1U);
    }
}

}  // namespace