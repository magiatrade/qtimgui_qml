# minimum require
cmake_minimum_required (VERSION 3.16)

# global solution
project(qtimgui_sln)

# Build order matters: modules and src must come before examples
add_subdirectory(modules)
add_subdirectory(src)
add_subdirectory(examples)
