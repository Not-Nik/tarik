cmake_minimum_required(VERSION 3.17)
project(tarik)

find_package(llvm 21.1 REQUIRED)

set(CMAKE_CXX_STANDARD 23 REQUIRED)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_subdirectory(src/libcomperr)

add_compile_options(-pedantic -Wall -Wextra -Wpedantic)

set(TARIK_COMPILER_SOURCES
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
        src/utf/Utf.h
        src/Version.h
        src/System.cpp
        src/System.h
        src/tlib/Serialise.cpp
        src/tlib/Serialise.h
        src/tlib/Export.cpp
        src/tlib/Export.h)

add_library(tarik-lib STATIC ${TARIK_COMPILER_SOURCES})

add_executable(tarik src/main.cpp)

add_executable(tarik-testing
        src/testing/TestMain.cpp
        src/testing/Testing.cpp
        src/testing/Testing.h)

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
        SPIRV
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

target_include_directories(tarik-lib SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(tarik-lib PUBLIC src)

target_link_libraries(tarik-lib comperr ${LLVM_LIBS})

target_link_libraries(tarik tarik-lib)
target_link_libraries(tarik-testing tarik-lib)

install(TARGETS tarik
        RUNTIME DESTINATION bin)
