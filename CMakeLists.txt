cmake_minimum_required(VERSION 3.16)

project(pente_app VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

##### pente code library

add_library(pente_board
    src/penteboard.cpp
)

target_include_directories(pente_board PUBLIC src)

if(MSVC)
    # warnings as errors, level 4 warnings
    target_compile_options(pente_board PRIVATE /WX)
    target_compile_options(pente_board PRIVATE /W4)
else()
    # warnings as errors, common warnings for GCC
    target_compile_options(pente_board PRIVATE -Werror)
    target_compile_options(pente_board PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wold-style-cast
        -Wuseless-cast
    )
endif()

##### pente application

add_executable(pente_app
    src/main.cpp
)

target_link_libraries(pente_app PRIVATE pente_board)

##### pente tests

enable_testing()

add_executable(penteboard_tests
    tests/penteboard_tests.cpp
)

target_link_libraries(penteboard_tests PRIVATE pente_board)

add_test(NAME penteboard_tests COMMAND penteboard_tests)

##### cppcheck target

option(ENABLE_CPPCHECK "Run static analysis" ON)

if(ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck REQUIRED)
    
    set(CPPCHECK_OPTIONS
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
        --enable=all
        --inconclusive
        --check-level=exhaustive
        --force
        --error-exitcode=1
        --suppress=missingIncludeSystem
        --suppress=missingInclude
        --suppress=checkersReport
        --suppress=unmatchedSuppression
        --inline-suppr
    )

    add_custom_target(cppcheck ALL
        COMMAND
            ${CPPCHECK_EXE}
            ${CPPCHECK_OPTIONS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis"
        VERBATIM
    )
endif()