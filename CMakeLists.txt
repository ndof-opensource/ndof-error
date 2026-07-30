cmake_minimum_required(VERSION 3.28)

project(ndof-error
    VERSION 0.1.0
    DESCRIPTION "Design-by-contract and error-handling framework for the ndof framework family"
    HOMEPAGE_URL "https://github.com/ndof-opensource/ndof-error"
    LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# This project does not use C++20 modules; CMake 3.28+ module scanning also
# injects compiler flags that clang-tidy cannot parse.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(NDOF_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
# OFF by default so source builds by consumers never break on a newer
# compiler's warnings; the team's CMake presets and CI turn it ON.
option(NDOF_WERROR "Treat compiler warnings as errors" OFF)

add_library(error
    src/version.cpp)
add_library(ndof::error ALIAS error)

set_target_properties(error PROPERTIES
    OUTPUT_NAME ndof-error)

target_compile_features(error PUBLIC cxx_std_23)

target_include_directories(error PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(error PRIVATE
    NDOF_LIBRARY_NAME="ndof-error"
    NDOF_LIBRARY_VERSION="${PROJECT_VERSION}")

target_compile_options(error PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- $<$<BOOL:${NDOF_WERROR}>:/WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion -Wshadow $<$<BOOL:${NDOF_WERROR}>:-Werror>>)

if(NDOF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---- installation (plain CMake package; no package-manager assumptions) -----
install(TARGETS error EXPORT ndof-error-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ndof-error-targets
    NAMESPACE ndof::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ndof-error)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ndof-error-config-version.cmake
    COMPATIBILITY SameMinorVersion)

configure_package_config_file(
    cmake/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ndof-error-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ndof-error)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ndof-error-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ndof-error-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ndof-error)
