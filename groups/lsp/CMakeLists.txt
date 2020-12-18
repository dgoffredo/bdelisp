if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_library(lsp
    lspcore/lspcore_arithmeticutil.cpp
    lspcore/lspcore_builtinprocedures.cpp
    lspcore/lspcore_builtins.cpp
    lspcore/lspcore_endian.cpp
    lspcore/lspcore_environment.cpp
    lspcore/lspcore_interpreter.cpp
    lspcore/lspcore_lexer.cpp
    lspcore/lspcore_linecounter.cpp
    lspcore/lspcore_listutil.cpp
    lspcore/lspcore_nativeprocedureutil.cpp
    lspcore/lspcore_pair.cpp
    lspcore/lspcore_parser.cpp
    lspcore/lspcore_printutil.cpp
    lspcore/lspcore_procedure.cpp
    lspcore/lspcore_symbolutil.cpp
    lspcore/lspcore_userdefinedtypes.cpp
    )

find_package(bal REQUIRED)
find_package(bdl REQUIRED)
find_package(bsl REQUIRED)

target_include_directories(lsp PUBLIC lspcore)
target_link_libraries(lsp LINK_PUBLIC bal bdl bsl)
