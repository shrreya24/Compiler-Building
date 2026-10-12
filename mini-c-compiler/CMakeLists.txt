cmake_minimum_required(VERSION 3.12)
project(MiniCCompiler VERSION 1.0.0 LANGUAGES CXX)

# Enforce C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler Warnings
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core Compiler Library Sources
set(COMPILER_CORE_SOURCES
    lexer.cpp
    parser.cpp
    semantic.cpp
    ir.cpp
)

# 1. Main Compiler Executable
add_executable(compiler
    main.cpp
    ${COMPILER_CORE_SOURCES}
)

# 2. Automated Test Runner Executable
add_executable(test_runner
    tests/test_runner.cpp
    ${COMPILER_CORE_SOURCES}
)

# Enable CTest integration
enable_testing()
add_test(
    NAME CompilerTestSuite
    COMMAND test_runner ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
