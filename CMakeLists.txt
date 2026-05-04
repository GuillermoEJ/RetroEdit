cmake_minimum_required(VERSION 3.16)
project(retroedit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(retroedit
    src/main.cpp
    src/terminal.cpp
    src/editor.cpp
    src/vcs.cpp
    src/syntax.cpp
    src/config.cpp
)

target_include_directories(retroedit PRIVATE include)
