---@diagnostic disable: undefined-global,undefined-field
set_project("hyperion_std")
set_version("0.1.0")

set_xmakever("2.8.7")

set_languages("cxx20")

add_rules("mode.debug", "mode.check", "mode.coverage")
add_moduledirs("xmake")
add_repositories("hyperion https://github.com/braxtons12/hyperion_packages.git")

option("hyperion_enable_tracy", function()
    set_default(false)
end)

add_requires("hyperion_platform", {
    system = false,
    external = true,
    configs = {
        languages = "cxx20",
        hyperion_enable_tracy = has_config("hyperion_enable_tracy"),
    }
})
add_requires("hyperion_mpl", {
    system = false,
    external = true,
    configs = {
        languages = "cxx20",
        hyperion_enable_tracy = has_config("hyperion_enable_tracy"),
    }
})
add_requires("hyperion_assert", {
    system = false,
    external = true,
    configs = {
        languages = "cxx20",
        hyperion_enable_tracy = has_config("hyperion_enable_tracy"),
    }
})
add_requires("boost_ut", {
    system = false,
    external = true,
    configs = {
        languages = "cxx20",
    }
})
add_requires("fmt", {
    system = false,
    external = true,
    configs = {
        languages = "cxx20",
    }
})
--add_requires("flux main", {
--    system = false,
--    external = true,
--    configs = {
--        languages = "cxx20",
--    }
--})

local hyperion_std_main_headers = {
    "$(projectdir)/include/hyperion/variant.h",
}
local hyperion_std_enum_headers = {
    "$(projectdir)/include/hyperion/variant/storage.h",
}
local hyperion_std_sources = {
    --"$(projectdir)/src/std/detail/parser.cpp",
}

target("hyperion_std", function()
    set_kind("static")
    set_languages("cxx20")
    set_default(true)

    add_includedirs("$(projectdir)/include", { public = true })
    add_headerfiles(hyperion_std_main_headers, { prefixdir = "hyperion", public = true })
    add_headerfiles(hyperion_enum_headers, { prefixdir = "hyperion/enum", public = true })
    add_files(hyperion_std_sources)

    on_config(function(target)
        import("hyperion_compiler_settings", { alias = "settings" })
        settings.set_compiler_settings(target)
    end)

    add_options("hyperion_enable_tracy", { public = true })

    add_packages("hyperion_platform", "hyperion_mpl", "fmt", { public = true })
end)

target("hyperion_std_main", function()
    set_kind("binary")
    set_languages("cxx20")
    set_default(true)

    add_files("$(projectdir)/src/main.cpp")

    add_deps("hyperion_std")

    on_config(function(target)
        import("hyperion_compiler_settings", { alias = "settings" })
        settings.set_compiler_settings(target)
    end)
    add_tests("hyperion_std_main")
end)

target("hyperion_std_tests", function()
    set_kind("binary")
    set_languages("cxx20")
    set_default(true)

    add_files("$(projectdir)/src/test_main.cpp")
    add_defines("HYPERION_ENABLE_TESTING=1")

    add_deps("hyperion_std")
    add_packages("boost_ut")

    on_config(function(target)
        import("hyperion_compiler_settings", { alias = "settings" })
        settings.set_compiler_settings(target)
    end)

    add_tests("hyperion_std_tests")
end)

target("hyperion_std_docs", function()
    set_kind("phony")
    set_default(false)
    on_build(function(_)
        local old_dir = os.curdir()
        os.cd("$(projectdir)/docs")
        os.exec("sphinx-build -M html . _build")
        os.cd(old_dir)
    end)
end)
