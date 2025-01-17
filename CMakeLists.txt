cmake_minimum_required(VERSION 3.25)
project(hyperion_std LANGUAGES CXX VERSION 0.1.0)

include(CTest)

option(HYPERION_ENABLE_TRACY "Enables Profiling with Tracy" OFF)
option(HYPERION_USE_FETCH_CONTENT "Enables FetchContent usage for getting dependencies" ON)

set(HYPERION_ENABLE_TRACY
    ${HYPERION_ENABLE_TRACY}
    CACHE
    INTERNAL
    "force dependencies's `HYPERION_ENABLE_TRACY` to the value of this top-level project")
set(HYPERION_USE_FETCH_CONTENT
    ${HYPERION_USE_FETCH_CONTENT}
    CACHE
    INTERNAL
    "force depedencies's `HYPERION_USE_FETCH_CONTENT` to the value of this top-level project")

if(HYPERION_USE_FETCH_CONTENT)
    include(FetchContent)
endif()

if(${HYPERION_USE_FETCH_CONTENT})
    FetchContent_Declare(hyperion_platform
        GIT_REPOSITORY "https://github.com/braxtons12/hyperion_platform"
        GIT_TAG "v0.5.3"
        SYSTEM
        )
    FetchContent_MakeAvailable(hyperion_platform)

    FetchContent_Declare(hyperion_mpl
        GIT_REPOSITORY "https://github.com/braxtons12/hyperion_mpl"
        GIT_TAG "v0.8.6"
        SYSTEM
        )
    FetchContent_MakeAvailable(hyperion_mpl)

    FetchContent_Declare(hyperion_assert
        GIT_REPOSITORY "https://github.com/braxtons12/hyperion_assert"
        GIT_TAG "v0.1.0"
        SYSTEM
        )
    FetchContent_MakeAvailable(hyperion_mpl)

    FetchContent_Declare(boost_ut
        GIT_REPOSITORY "https://github.com/boost-ext/ut"
        GIT_TAG "v2.1.0"
        SYSTEM
        )
    FetchContent_MakeAvailable(boost_ut)

    FetchContent_Declare(fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt"
        GIT_TAG "11.0.2"
        SYSTEM
        )
    FetchContent_MakeAvailable(fmt)

    # FetchContent_Declare(flux
    #     GIT_REPOSITORY "https://github.com/tcbrindle/flux"
    #     GIT_TAG "main"
    #     SYSTEM
    #     )
    # FetchContent_MakeAvailable(flux)
else()
    find_package(hyperion_platform REQUIRED)
    find_package(hyperion_mpl REQUIRED)
    find_package(ut REQUIRED)
    find_package(fmt REQUIRED)
    # find_package(flux REQUIRED)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/hyperion_compiler_settings.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/hyperion_enable_warnings.cmake)

set(HYPERION_STD_INCLUDE_DIRECTORIES
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
set(HYPERION_STD_INCLUDE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/include/hyperion"
)
set(HYPERION_STD_HEADERS
    "${HYPERION_STD_INCLUDE_PATH}/enum.h"
    "${HYPERION_STD_INCLUDE_PATH}/enum/storage.h"
)

add_library(hyperion_std STATIC)
add_library(hyperion::std ALIAS hyperion_std)
target_include_directories(
    hyperion_std
    PUBLIC
    ${HYPERION_STD_INCLUDE_DIRECTORIES}
)
target_sources(
    hyperion_std
    PUBLIC
    ${HYPERION_STD_HEADERS}
)
target_link_libraries(
    hyperion_std
    PUBLIC
    hyperion::platform
    hyperion::mpl
    fmt::fmt
    #flux::flux
)

hyperion_compile_settings(hyperion_std)
hyperion_enable_warnings(hyperion_std)

add_executable(hyperion_std_main ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(hyperion_std_main
    PRIVATE
    hyperion::std
)

hyperion_compile_settings(hyperion_std_main)
hyperion_enable_warnings(hyperion_std_main)

add_test(NAME hyperion_std_main
         COMMAND hyperion_std_main)

add_executable(hyperion_std_tests ${CMAKE_CURRENT_SOURCE_DIR}/src/test_main.cpp)
target_link_libraries(hyperion_std_tests
    PRIVATE
    hyperion::std
    Boost::ut
)
target_compile_definitions(
    hyperion_std_tests
    PUBLIC
    BOOST_UT_DISABLE_MODULE=1
    HYPERION_ENABLE_TESTING=1
)

hyperion_compile_settings(hyperion_std_tests)
hyperion_enable_warnings(hyperion_std_tests)

add_test(NAME hyperion_std_tests
         COMMAND hyperion_std_tests)

set(HYPERION_STD_DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs/_build/html")
set(HYPERION_STD_DOXYGEN_HTML "${HYPERION_STD_DOXYGEN_OUTPUT_DIR}/index.html")

set(HYPERION_STD_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs")
set(HYPERION_STD_DOCS_FILES
    "${HYPERION_STD_DOCS_DIR}/index.rst"
    "${HYPERION_STD_DOCS_DIR}/quick_start.rst"
)

add_custom_command(
    OUTPUT ${HYPERION_STD_DOXYGEN_HTML}
    DEPENDS ${HYPERION_STD_HEADERS} ${DOCS_FILES}
    COMMAND sphinx-build -M html . _build
    WORKING_DIRECTORY ${HYPERION_STD_DOCS_DIR}
    MAIN_DEPENDENCY
    COMMENT "Generating documentation with doxygen, sphinx, and breathe"
    VERBATIM
)

add_custom_target(hyperion_std_doxygen DEPENDS ${DOXYGEN_HTML})

include(GNUInstallDirs)
install(DIRECTORY ${HYPERION_STD_DOXYGEN_OUTPUT_DIR} DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY include/ DESTINATION include)
install(TARGETS hyperion_std DESTINATION lib)
