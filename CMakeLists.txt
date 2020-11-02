cmake_minimum_required(VERSION 3.17)
project(tarik)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(MANU_EXIT)

add_subdirectory(src/libcomperr)
add_subdirectory(src/magic_enum)

add_executable(tarik
        src/main.cpp
        src/Lexer.cpp
        src/Lexer.h
        src/Token.h
        src/cli/arguments.cpp
        src/cli/arguments.h
        src/Parser.cpp
        src/Parser.h
        src/expressions/Expression.h
        src/expressions/Parslets.h
        src/expressions/Statements.h
        src/expressions/Types.h src/testing.h)

target_link_libraries(tarik comperr)
target_link_libraries(tarik magic_enum)