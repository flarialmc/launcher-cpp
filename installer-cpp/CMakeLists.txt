cmake_minimum_required(VERSION 3.25)
project(flarialinstaller)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(/MP)

add_executable(flarialinstaller WIN32 main.cpp)
target_sources(flarialinstaller PRIVATE icon.rc)

target_link_libraries(flarialinstaller PRIVATE dwrite.lib d2d1.lib urlmon.lib windowscodecs.lib shlwapi.lib)
add_subdirectory(11Zip) # Path to the 11zip
target_link_libraries(flarialinstaller PUBLIC elzip)