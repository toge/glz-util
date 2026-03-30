cmake_minimum_required(VERSION 3.25)
project(glz-util VERSION 0.0.1 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

## add stdc++ compiler flag
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++26" COMPILER_SUPPORTS_CXX26)
CHECK_CXX_COMPILER_FLAG("-std=c++23" COMPILER_SUPPORTS_CXX23)
if(COMPILER_SUPPORTS_CXX26)
    set(STD_CPP cxx_std_26)
elseif(COMPILER_SUPPORTS_CXX23)
    set(STD_CPP cxx_std_23)
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++23 support. Please use a different C++ compiler.")
endif()

## add warning
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
endif()

if (MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
endif()

## optimization
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
endif()
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox")
endif()

## other flag
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glaze CONFIG REQUIRED)
find_package(zfp CONFIG REQUIRED)

add_library(glz-util INTERFACE)
add_library(glz-util::glz-util ALIAS glz-util)

target_include_directories(glz-util
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(glz-util INTERFACE glaze::glaze zfp::zfp)
target_compile_features(glz-util INTERFACE ${STD_CPP})


set(GLZ_UTIL_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/glz-util")

install(TARGETS glz-util EXPORT glz-utilTargets)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/glz-util/env.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/glz-util/print.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/glz-util/tsl-hat-trie.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/glz-util/zfp.hpp"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/glz-util)

install(EXPORT glz-utilTargets
        FILE glz-utilTargets.cmake
        NAMESPACE glz-util::
        DESTINATION ${GLZ_UTIL_INSTALL_CMAKEDIR})

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/glz-utilConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/glz-utilConfig.cmake"
    INSTALL_DESTINATION ${GLZ_UTIL_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/glz-utilConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/glz-utilConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/glz-utilConfigVersion.cmake"
        DESTINATION ${GLZ_UTIL_INSTALL_CMAKEDIR})


add_executable(json_schema_codegen
    util/json_schema_codegen.cpp
    util/json_schema_codegen.hpp
)

target_link_libraries(json_schema_codegen PRIVATE glz-util::glz-util)
target_compile_features(json_schema_codegen PRIVATE ${STD_CPP})
install(TARGETS json_schema_codegen RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()
add_subdirectory(test)
