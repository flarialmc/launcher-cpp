cmake_minimum_required(VERSION 3.25)
project(flarialminimal)

set(CMAKE_CXX_STANDARD 23)

add_executable(flarialminimal WIN32 main.cpp)
target_sources(flarialminimal PRIVATE icon.rc)


target_include_directories(flarialminimal PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include/")

add_library(sussy SHARED IMPORTED GLOBAL)
set_target_properties(sussy PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/discord-rpc.lib")

target_link_libraries(flarialminimal PRIVATE dwrite.lib d2d1.lib urlmon.lib windowscodecs.lib shlwapi.lib)
target_link_libraries(flarialminimal PUBLIC sussy)