cmake_minimum_required(VERSION 3.17)
project(Seno)

set(CMAKE_CXX_STANDARD 14)

add_executable(Seno
        src/main.cpp
        src/Lexer.cpp
        src/Lexer.h
        src/Token.h
        src/cli/arguments.cpp
        src/cli/arguments.h)