cmake_minimum_required(VERSION 3.17)
project(Seno)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(src/libcomperr)

add_executable(Seno
        src/main.cpp
        src/Lexer.cpp
        src/Lexer.h
        src/Token.h
        src/cli/arguments.cpp
        src/cli/arguments.h
        src/Parser.cpp
        src/Parser.h src/expressions/Expression.h src/expressions/Parslets.h src/expressions/Statements.h)

target_link_libraries(Seno comperr)