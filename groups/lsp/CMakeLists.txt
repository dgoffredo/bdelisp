add_library(lsp
    lspcore/lspcore_lexer.cpp
    lspcore/lspcore_linecounter.cpp
    lspcore/lspcore_listutil.cpp
    lspcore/lspcore_pair.cpp
    lspcore/lspcore_symbolutil.cpp
    lspcore/lspcore_userdefinedtypes.cpp
    )

find_package(bal REQUIRED)
find_package(bdl REQUIRED)
find_package(bsl REQUIRED)

target_include_directories(lsp PUBLIC lspcore)
target_link_libraries(lsp LINK_PUBLIC bal bdl bsl)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()
