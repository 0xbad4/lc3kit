cmake_minimum_required(VERSION 3.28)

project(lc3kit
    VERSION     1.0.0
    DESCRIPTION "LC-3 Virtual Machine and Assembler"
    LANGUAGES   CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------ library target
# header-only - no sources to compile

add_library(lc3kit INTERFACE)
add_library(lc3kit::lc3kit ALIAS lc3kit)

target_include_directories(lc3kit INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(lc3kit INTERFACE cxx_std_20)

# ------------ install
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS lc3kit
    EXPORT  lc3kitTargets
)

install(DIRECTORY include/lc3kit
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT lc3kitTargets
    FILE        lc3kitTargets.cmake
    NAMESPACE   lc3kit::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lc3kit
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/lc3kitConfigVersion.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/lc3kitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lc3kitConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lc3kit
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/lc3kitConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lc3kitConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lc3kit
)
