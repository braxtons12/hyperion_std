Quick Start Guide
*****************

hyperion::std supports both CMake and XMake, and incorporating it in your project is quick and
easy.

CMake
-----

hyperion::std is easily incorporated into a raw CMake project with :cmake:`FetchContent` or
other methods like :cmake:`add_subdirectory`\. Example for :cmake:`FetchContent`\:

.. code-block:: cmake
    :caption: CMakeLists.txt
    :linenos:

    # Include FetchContent so we can use it
    include(FetchContent)

    # Declare the dependency on hyperion-utils and make it available for use
    FetchContent_Declare(hyperion_std
        GIT_REPOSITORY "https://github.com/braxtons12/hyperion_std"
        GIT_TAG "v0.1.0")
    FetchContent_MakeAvailable(hyperion_std)

    # For this example, we create an executable target and link hyperion::std to it
    add_executable(MyExecutable "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    target_link_libraries(MyExecutable PRIVATE hyperion::std)

Note that hyperion::std depends on
`hyperion::platform <https://github.com/braxtons12/hyperion_platform>`_\, for platform and feature
detection macros and other core utilities,
`hyperion::mpl <https://github.com/braxtons12/hyperion_mpl>`_\, for metaprogramming,
`hyperion::assert <https://github.com/braxtons12/hyperion_asser>`_\, for runtime assertions,
`fmtlib <https://github.com/fmtlib/fmt>`_\, for string formatting,
and `doctest <https://github.com/doctest/doctest>`_\, for testing.

The hyperion libraries used as dependencies have several optional features with optional
dependencies, and hyperion::std exposes those settings to its users as well (see the relevant
documentation, e.g. the
`hyperion::platform documentation <https://braxtons12.github.io/hyperion_platform/quick_start.html>`_
for how to enable these).

By default, :cmake:`FetchContent` will be used to obtain these dependencies, but you can disable
this by setting :cmake:`HYPERION_USE_FETCH_CONTENT` to :cmake:`OFF`\, in which case you will need to
make sure each package is findable via CMake's :cmake:`find_package`\.

XMake
-----

XMake is a new(er) Lua-based build system with integrated package management. It is the preferred
way to use Hyperion packages. Example:

.. code-block:: lua
    :caption: xmake.lua
    :linenos:

    set_project("my_project")

    -- add the hyperion_packages git repository as an XMake package repository/registry
    add_repositories("hyperion https://github.com/braxtons12/hyperion_packages.git")

    -- add hyperion_std as a required dependency for the project
    add_requires("hyperion_std", {
        system = false,
        external = true,
    })
    
    -- For this example, we create an executable target and link hyperion::std to it
    target("my_executable")
        set_kind("binary")
        add_packages("hyperion_std")

Note that with XMake, hyperion::std requires the same dependencies as with the CMake build system.
Third-party dependencies will be pulled from xmake-repo, the package repository/registry for XMake,
and dependencies on other hyperion libraries will be pulled from github via the 
`hyperion package repository/registry for xmake <https://github.com/braxtons12/hyperion_packages>`_\.

