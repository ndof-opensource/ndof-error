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

ndof::BuildMode getBuildMode() noexcept {
    if (kBuildModeValue == "debug") {
        return ndof::BuildMode::debug;
    }
    if (kBuildModeValue == "release") {
        return ndof::BuildMode::release;
    }
    return ndof::BuildMode::undefined;
}

std::string_view getBuildModeName() noexcept {
    switch (getBuildMode()) {
    case ndof::BuildMode::debug:
        return "debug";
    case ndof::BuildMode::release:
        return "release";
    default:
        return "undefined";
    }
}

bool isRttiEnabled() noexcept {
    return kRttiEnabled;
}

ndof::TypeIndexMode getTypeIndexMode() noexcept {
    if (kRttiEnabled) {
        return ndof::TypeIndexMode::rtti_type_index;
    }
    return ndof::TypeIndexMode::ndof_type_index;
}

std::string_view getTypeIndexModeName() noexcept {
    switch (getTypeIndexMode()) {
    case ndof::TypeIndexMode::rtti_type_index:
        return "std::type_index";
    case ndof::TypeIndexMode::ndof_type_index:
        return "ndof_type_index";
    default:
        return "unknown";
    }
}

} // namespace ndof::error
