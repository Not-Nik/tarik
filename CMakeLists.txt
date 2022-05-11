cmake_minimum_required(VERSION 3.17)
project(tarik)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(llvm 13 REQUIRED)

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
        src/lexical/Lexer.cpp
        src/lexical/Lexer.h
        src/lexical/Token.h
        src/semantic/Analyser.cpp
        src/semantic/Analyser.h
        src/syntactic/Parser.cpp
        src/syntactic/Parser.h
        src/syntactic/expressions/Base.h
        src/syntactic/expressions/Expression.h
        src/syntactic/expressions/Parslets.h
        src/syntactic/expressions/Statements.h
        src/syntactic/expressions/Types.h
        src/syntactic/expressions/Types.cpp
        src/util/Util.cpp
        src/util/Util.h)

llvm_map_components_to_libnames(llvm_libs
        aarch64asmparser aarch64codegen aarch64info aarch64utils
        aggressiveinstcombine all amdgpuasmparser amdgpucodegen amdgpudesc amdgpudisassembler
        amdgpuinfo amdgpuutils analysis armasmparser armcodegen armdesc armdisassembler arminfo
        armutils asmparser asmprinter avrasmparser avrcodegen avrdesc avrdisassembler avrinfo
        binaryformat bitreader bitstreamreader bitwriter bpfasmparser bpfcodegen bpfdesc
        bpfdisassembler bpfinfo cfguard codegen core coroutines coverage debuginfocodeview
        debuginfodwarf debuginfogsym debuginfomsf debuginfopdb demangle dlltooldriver dwarflinker
        dwp executionengine extensions filecheck frontendopenacc frontendopenmp fuzzmutate
        globalisel hexagonasmparser hexagoncodegen hexagondesc hexagondisassembler hexagoninfo
        instcombine instrumentation interfacestub interpreter ipo irreader jitlink lanaiasmparser
        lanaicodegen lanaidesc lanaidisassembler lanaiinfo libdriver lineeditor linker lto mc mca
        mcdisassembler mcjit mcparser mipsasmparser mipscodegen mipsdesc mipsdisassembler mipsinfo
        mirparser msp430asmparser msp430codegen msp430desc msp430disassembler msp430info native
        nativecodegen nvptxcodegen nvptxdesc nvptxinfo objcarcopts object objectyaml option orcjit
        orcshared orctargetprocess passes powerpcasmparser powerpccodegen powerpcdesc
        powerpcdisassembler powerpcinfo profiledata remarks riscvasmparser riscvcodegen riscvdesc
        riscvdisassembler riscvinfo runtimedyld scalaropts selectiondag sparcasmparser sparccodegen
        sparcdesc sparcdisassembler sparcinfo support symbolize systemzasmparser systemzcodegen
        systemzdesc systemzdisassembler systemzinfo tablegen target textapi transformutils vectorize
        webassemblyasmparser webassemblycodegen webassemblydesc webassemblydisassembler
        webassemblyinfo webassemblyutils windowsmanifest x86asmparser x86codegen x86desc
        x86disassembler x86info xcorecodegen xcoredesc xcoredisassembler xcoreinfo xray)

target_include_directories(tarik PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(tarik PRIVATE src)

target_link_libraries(tarik comperr ${llvm_libs})