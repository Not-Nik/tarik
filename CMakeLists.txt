cmake_minimum_required(VERSION 3.17)
project(tarik)

set(CMAKE_CXX_STANDARD 23 REQUIRED)

find_package(llvm 18.1 REQUIRED)

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
        src/lifetime/Analyser.cpp
        src/lifetime/Analyser.h
        src/lifetime/Variable.cpp
        src/lifetime/Variable.h
        src/semantic/ast/Base.h
        src/semantic/ast/Expression.h
        src/semantic/ast/Statements.h
        src/semantic/Analyser.cpp
        src/semantic/Analyser.h
        src/semantic/Macro.cpp
        src/semantic/Macro.h
        src/semantic/Path.cpp
        src/semantic/Path.h
        src/semantic/Structures.cpp
        src/semantic/Structures.h
        src/semantic/Variables.cpp
        src/semantic/Variables.h
        src/syntactic/Parser.cpp
        src/syntactic/Parser.h
        src/syntactic/Types.cpp
        src/syntactic/Types.h
        src/syntactic/ast/Base.h
        src/syntactic/ast/Expression.h
        src/syntactic/ast/Parslets.h
        src/syntactic/ast/Statements.h
        src/utf/Utf.cpp
        src/utf/Utf.h)

set(LLVM_LINK_COMPONENTS
        AArch64
        AMDGPU
        ARM
        AVR
        BPF
        Core
        Hexagon
        Lanai
        LoongArch
        MC
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
)

llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_LINK_COMPONENTS})

target_include_directories(tarik PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(tarik PRIVATE src)

target_link_libraries(tarik comperr ${LLVM_LIBS})