cmake_minimum_required(VERSION 3.17)
project(tarik)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(MANU_EXIT)

add_subdirectory(src/libcomperr)

add_compile_options(-pedantic -Wall -Wextra -Wpedantic)

add_executable(tarik
        src/main.cpp
        src/Testing.h
        src/Testing.cpp
        src/lexical/Lexer.cpp
        src/lexical/Lexer.h
        src/lexical/Token.h
        src/cli/arguments.cpp
        src/cli/arguments.h
        src/syntactic/Parser.cpp
        src/syntactic/Parser.h
        src/syntactic/expressions/Expression.h
        src/syntactic/expressions/Parslets.h
        src/syntactic/expressions/Statements.h
        src/syntactic/expressions/Types.h
        src/syntactic/expressions/Types.cpp
        src/util/Util.cpp src/util/Util.h
        src/semantic/Analyser.cpp
        src/semantic/Analyser.h)

target_include_directories(tarik PRIVATE src)

target_link_libraries(tarik comperr)