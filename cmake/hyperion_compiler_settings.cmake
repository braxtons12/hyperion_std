set(CMAKE_POSITION_INDEPENDENT_CODE ON)

function(hyperion_compile_settings TARGET)
    set(PUBLIC_VISIBILITY PUBLIC)
    set(ADD_PRIVATE_ITEMS TRUE)

    get_target_property(TARGET_TYPE ${TARGET} TYPE)
    if(${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
        set(PUBLIC_VISIBILITY INTERFACE)
        set(ADD_PRIVATE_ITEMS FALSE)
    endif()

    if(MSVC)
        if(${ADD_PRIVATE_ITEMS})
            target_compile_options(
                ${TARGET}
                PRIVATE
                /MP
                /sdl
            )
        endif()

        target_compile_options(
            ${TARGET}
            ${PUBLIC_VISIBILITY}
            /permissive-
            /Zc:preprocessor
            /Zc:rvalueCast
            /Zc:__cplusplus
            /EHsc
        )
        target_link_options(
            ${TARGET}
            ${PUBLIC_VISIBILITY}
            /EHsc
        )
    endif()

    if(WIN32)
        target_compile_definitions(
            ${TARGET}
            ${PUBLIC_VISIBILITY}
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            _CRT_SECURE_NO_WARNINGS
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMILER_ID STREQUAL "clang")
        target_compile_options(${TARGET} ${PUBLIC_VISIBILITY} -fsized-deallocation)
    endif()

    target_compile_features(${TARGET} ${PUBLIC_VISIBILITY} cxx_std_20)
endfunction()
