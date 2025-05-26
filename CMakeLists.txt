cmake_minimum_required(VERSION 3.5)
project(Exact)

set(CMAKE_CXX_STANDARD 20)
find_package(Boost 1.71.0 REQUIRED) # 1.81 for the fastest hash maps

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_CXX_FLAGS_TESTRUNS "${CMAKE_CXX_FLAGS_TESTRUNS} -O3")

if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR
        CMAKE_BUILD_TYPE STREQUAL "Release" OR
        CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR
        CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR
        CMAKE_BUILD_TYPE STREQUAL "TestRuns") )
    message(WARNING "Please use -DCMAKE_BUILD_TYPE=[Debug | Release | RelWithDebInfo | MinSizeRel | TestRuns]")
endif()

set(build_result "Executable"
    CACHE STRING "Result of make command: Executable, SharedLib or StaticLib.")

if(NOT (build_result STREQUAL "Executable" OR build_result STREQUAL "SharedLib" OR build_result STREQUAL "StaticLib" OR build_result STREQUAL "UnitTests"))
    message(WARNING "Please use -Dbuild_result=[Executable | SharedLib | StaticLib | UnitTests]")
endif()

set(build_static "OFF"
    CACHE STRING "Build statically linked executable.")

set(profile_flags "OFF"
    CACHE STRING "Build using gprof flag (-pg).")

set(flto "OFF"
    CACHE STRING "Build with link-time optimization (-flto).")
set(singlesource "OFF"
        CACHE STRING "Build with a single source file to simulate link-time optimization. Warning: slow!")

if(WIN32)
    set(unixlike "OFF"
        CACHE STRING "Compiling on Linux or macOS.")
else()
    set(unixlike "ON"
        CACHE STRING "Compiling on Linux or macOS.")
endif()

set(soplex "OFF"
    CACHE STRING "Use SoPlex to check for rational infeasibility.")
set(soplex_src "${PROJECT_SOURCE_DIR}/soplex/src"
    CACHE STRING "SoPlex /src directory used to find the header files.")
set(soplex_build "${PROJECT_SOURCE_DIR}/soplex_build"
    CACHE STRING "SoPlex build directory used to find the compiled libraries and the header file soplex/config.h.")

set(coinutils "OFF"
    CACHE STRING "Coin OR Utils library to parse .mps and .lp files.")
set(zlib "OFF"
    CACHE STRING "Use ZLIB to compress proof certificates.")
set(mimalloc "OFF"
    CACHE STRING "Link with faster memory allocator mimalloc")

set(testruns_timeout 1 CACHE STRING "Timeout of scenario test runs.")

if(build_static)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

if(profile_flags)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
endif()

if(flto)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")
endif()

if(mimalloc)
    find_package(mimalloc 1.4 REQUIRED)
    target_link_libraries(ManyWorlds mimalloc-static)
endif()

set(GIT_BRANCH "unknown")
set(GIT_COMMIT_HASH "unknown")
# Get the current working branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the latest abbreviated commit hash of the working branch
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DGIT_COMMIT_HASH=${GIT_COMMIT_HASH}")
add_definitions("-DGIT_BRANCH=${GIT_BRANCH}")

if(unixlike)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-attributes")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W1")
endif()

set(test_files
    src/tests/algorithms.cpp
    src/tests/inferences.cpp
    src/tests/input.cpp
    src/tests/optimization.cpp
    src/tests/memory.cpp
)

set(source_files
    src/constraints/Constr.cpp
    src/constraints/ConstrExp.cpp
    src/constraints/ConstrSimple.cpp
    src/constraints/ConstrExpPools.cpp
    src/propagation/LpSolver.cpp
    src/Solver.cpp
    src/interface/IntConstraint.cpp
    src/interface/IntProg.cpp
    src/datastructures/SolverStructs.cpp
    src/Logger.cpp
    src/datastructures/IntSet.cpp
    src/parsing.cpp
    src/datastructures/Heuristic.cpp
    src/Stats.cpp
    src/Options.cpp
    src/auxiliary.cpp
    src/Optimization.cpp
    src/quit.cpp
    src/propagation/Propagator.cpp
    src/propagation/Equalities.cpp
    src/propagation/Implications.cpp

    src/used_licenses/licenses.cpp
    src/used_licenses/zib_apache.cpp
    src/used_licenses/roundingsat.cpp
    src/used_licenses/MIT.cpp
    src/used_licenses/boost.cpp
    src/used_licenses/EPL.cpp
    src/used_licenses/COPYING.cpp
)

set(header_files
    src/constraints/Constr.hpp
    src/constraints/ConstrExp.hpp
    src/constraints/ConstrSimple.hpp
    src/datastructures/SolverStructs.hpp
    src/constraints/ConstrExpPools.hpp
    src/propagation/LpSolver.hpp
    src/Solver.hpp
    src/interface/IntConstraint.hpp
    src/interface/IntProg.hpp
    src/Logger.hpp
    src/datastructures/IntMap.hpp
    src/datastructures/IntSet.hpp
    src/typedefs.hpp
    src/parsing.hpp
    src/datastructures/Heuristic.hpp
    src/Stats.hpp
    src/Options.hpp
    src/auxiliary.hpp
    src/quit.hpp
    src/Optimization.hpp
    src/propagation/Propagator.hpp
    src/propagation/Equalities.hpp
    src/propagation/Implications.hpp
    src/Global.hpp
    src/interface/Exact.hpp

    src/used_licenses/licenses.hpp
    src/used_licenses/zib_apache.hpp
    src/used_licenses/roundingsat.hpp
    src/used_licenses/MIT.hpp
    src/used_licenses/boost.hpp
    src/used_licenses/EPL.hpp
    src/used_licenses/COPYING.hpp
)

set(main_files
    src/main.cpp
    src/interface/Exact.cpp
    src/singlesource.cpp
)

set(files_to_format ${main_files} ${source_files} ${test_files} ${header_files})
if(singlesource)
    set(files_to_compile src/singlesource.cpp)
else()
    set(files_to_compile ${source_files})
endif()

if(build_result STREQUAL "Executable")
    add_executable(Exact ${files_to_compile} src/main.cpp)
    install(TARGETS Exact RUNTIME DESTINATION bin)
    add_custom_target(
            testruns
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
            COMMAND ./run_tests.sh ${testruns_timeout} "testruns" ${CMAKE_CURRENT_BINARY_DIR}/Exact
    )
    add_dependencies(testruns Exact)
endif()

if(build_result STREQUAL "SharedLib")
    add_library(Exact SHARED ${files_to_compile})
    set_target_properties(Exact PROPERTIES PUBLIC_HEADER src/interface/Exact.hpp)
endif()

if(build_result STREQUAL "StaticLib")
    add_library(Exact STATIC ${files_to_compile})
    set_target_properties(Exact PROPERTIES PUBLIC_HEADER src/interface/Exact.hpp)
    set(Boost_USE_STATIC_LIBS ON)
endif()

if(build_result STREQUAL "UnitTests")
    add_executable(Exact src/external/doctest/main.cpp ${files_to_compile} ${test_files})
else()
    target_compile_definitions(Exact PUBLIC DOCTEST_CONFIG_DISABLE=1)
endif()

target_include_directories(Exact PUBLIC ${PROJECT_SOURCE_DIR}/src/)

if(unixlike)
    target_compile_definitions(Exact PUBLIC UNIXLIKE=1)
endif()

if(Boost_MINOR_VERSION LESS 81)
    target_compile_definitions(Exact PUBLIC ANKERLMAPS=1)
endif()

if(soplex)
    target_include_directories(Exact SYSTEM PUBLIC ${soplex_src} ${soplex_build})
    if(build_static)
        target_link_libraries(Exact "${soplex_build}/lib/libsoplex.a")
    else()
        if(APPLE)
            target_link_libraries(Exact "${soplex_build}/lib/libsoplexshared.dylib")
        else()
            target_link_libraries(Exact "${soplex_build}/lib/libsoplexshared.so")
        endif()
    endif()
    target_compile_definitions(Exact PUBLIC WITHSOPLEX=1)
endif()

if(coinutils)
    find_package(CoinUtils REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(BZip2 REQUIRED)
    target_include_directories(Exact PUBLIC ${CoinUtils_INCLUDE_DIR})
    target_link_libraries(Exact CoinUtils z bz2)
    target_compile_definitions(Exact PUBLIC WITHCOINUTILS=1)
endif()

if(zlib)
    find_package(ZLIB REQUIRED)
    target_include_directories(Exact PUBLIC ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(Exact z)
    target_compile_definitions(Exact PUBLIC WITHZLIB=1)
endif()

target_include_directories(Exact PUBLIC ${Boost_INCLUDE_DIRS})

FIND_PROGRAM(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    add_custom_target(
        format
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${CLANG_FORMAT}
        -style=file
        -i
        ${files_to_format}
    )
    add_custom_target(
        format_check
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${CLANG_FORMAT}
        --dry-run
        --Werror
        -style=file
        ${files_to_format}
    )
endif()
