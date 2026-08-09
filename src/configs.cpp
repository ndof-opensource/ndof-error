// Copyright 2026 The ndof Authors
// SPDX-License-Identifier: Apache-2.0

#include "ndof/error/configs.hpp"

#include <string_view>

namespace ndof::error {

namespace {

#if defined(NDEBUG)
constexpr std::string_view kBuildModeValue = "release";
#else
constexpr std::string_view kBuildModeValue = "debug";
#endif

#if defined(NDOF_RTTI_ENABLED) && (NDOF_RTTI_ENABLED)
constexpr bool kRttiEnabled = true;
#else
constexpr bool kRttiEnabled = false;
#endif

} // namespace

ndof::build_mode build_mode() noexcept {
    if (kBuildModeValue == "debug") {
        return ndof::build_mode::debug;
    }
    if (kBuildModeValue == "release") {
        return ndof::build_mode::release;
    }
    return ndof::build_mode::undefined;
}

std::string_view build_mode_name() noexcept {
    switch (build_mode()) {
    case ndof::build_mode::debug:
        return "debug";
    case ndof::build_mode::release:
        return "release";
    default:
        return "undefined";
    }
}

bool rtti_enabled() noexcept {
    return kRttiEnabled;
}

ndof::type_index_mode type_index_mode() noexcept {
    if (kRttiEnabled) {
        return ndof::type_index_mode::rtti_type_index;
    }
    return ndof::type_index_mode::ndof_type_index;
}

std::string_view type_index_mode_name() noexcept {
    switch (type_index_mode()) {
    case ndof::type_index_mode::rtti_type_index:
        return "std::type_index";
    case ndof::type_index_mode::ndof_type_index:
        return "ndof_type_index";
    default:
        return "unknown";
    }
}

} // namespace ndof::error
