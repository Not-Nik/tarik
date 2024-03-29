cmake_minimum_required(VERSION 3.17)
project(tarik)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

find_package(llvm 17 REQUIRED)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_subdirectory(src/libcomperr)

add_compile_options(-pedantic -Wall -Wextra -Wpedantic)

add_executable(tarik
        src/main.cpp
        src/Testing.cpp
        src/Testing.h
        src/cli/Arguments.cpp
        src/cli/Arguments.h
        src/codegen/LLVM.cpp
        src/codegen/LLVM.h
        src/error/Bucket.cpp
        src/error/Bucket.h
        src/error/Error.cpp
        src/error/Error.h
        src/lexical/Lexer.cpp
        src/lexical/Lexer.h
        src/lexical/Token.cpp
        src/lexical/Token.h
        src/semantic/Analyser.cpp
        src/semantic/Analyser.h
        src/semantic/Path.cpp
        src/semantic/Path.h
        src/syntactic/Parser.cpp
        src/syntactic/Parser.h
        src/syntactic/expressions/Base.h
        src/syntactic/expressions/Expression.h
        src/syntactic/expressions/Parslets.h
        src/syntactic/expressions/Statements.h
        src/syntactic/expressions/Types.cpp
        src/syntactic/expressions/Types.h)

set(LLVM_LINK_COMPONENTS
        AArch64
        AMDGPU
        ARM
        asmparser
        AVR
        BPF
        Core
        ExecutionEngine
        Hexagon
        Interpreter
        Lanai
        LoongArch
        MC
        mcjit
        Mips
        MSP430
        nativecodegen
        NVPTX
        PowerPC
        RISCV
        Sparc
        Support
        SystemZ
        VE
        WebAssembly
        X86
        XCore
        objcarcopts
        scalaropts
        transformutils
        irreader
)

llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_LINK_COMPONENTS})

target_include_directories(tarik PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(tarik PRIVATE src)

target_link_libraries(tarik comperr ${LLVM_LIBS})