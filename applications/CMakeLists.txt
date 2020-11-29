cmake_minimum_required (VERSION 3.15)

find_package(bsl REQUIRED)

add_executable(repl repl.cpp)
target_link_libraries(repl LINK_PUBLIC lsp bsl rt)
