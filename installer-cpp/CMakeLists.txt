cmake_minimum_required(VERSION 3.25)
project(flarialinstaller)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(/MT /MP)

add_executable(flarialinstaller WIN32 main.cpp)
target_sources(flarialinstaller PRIVATE icon.rc)

target_link_libraries(flarialinstaller PRIVATE dwrite.lib d2d1.lib urlmon.lib windowscodecs.lib shlwapi.lib)
add_subdirectory(11Zip) # Path to the 11zip
target_link_libraries(flarialinstaller PUBLIC elzip)

# Default to statically-linked runtime.
if("${MSVC_RUNTIME}" STREQUAL "")
    set(MSVC_RUNTIME "static")

    if(${MSVC_RUNTIME} STREQUAL "static")
        message(STATUS
                "MSVC -> forcing use of statically-linked runtime."
                )
        foreach(variable ${variables})
            if(${variable} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
            endif()
        endforeach()
    else()
        message(STATUS
                "MSVC -> forcing use of dynamically-linked runtime."
                )
        foreach(variable ${variables})
            if(${variable} MATCHES "/MT")
                string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
            endif()
        endforeach()
    endif()
endif()