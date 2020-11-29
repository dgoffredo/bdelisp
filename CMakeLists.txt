cmake_minimum_required (VERSION 3.15)

# Shouldn't be necessary, but here we are.
set(CMAKE_CXX_STANDARD 17)

project (bdelisp)

add_subdirectory(applications)
add_subdirectory(groups)
