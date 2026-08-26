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

##### coverage support

option(ENABLE_COVERAGE "Enable coverage reporting (requires gcovr)" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Coverage enabled: adding coverage compile/link flags (scoped to pente_board and tests)")

    find_program(GCOVR_BIN NAMES gcovr REQUIRED)

    add_library(coverage_flags INTERFACE)
    target_compile_options(coverage_flags INTERFACE -O0 -g --coverage)
    target_link_options(coverage_flags INTERFACE --coverage)

    # Apply coverage only to the library that contains penteboard sources
    target_link_libraries(pente_board PRIVATE coverage_flags)

    # Ensure test executable is linked with coverage so coverage data is emitted
    # when tests run. Do not add compilation instrumentation to tests themselves.
    target_link_libraries(penteboard_tests PRIVATE coverage_flags)

    # runs ctest and then gcovr to generate a coverage report in HTML format
    add_custom_target(coverage ALL
        DEPENDS penteboard_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${CMAKE_COMMAND} -E make_directory coverage_html
        COMMAND ${GCOVR_BIN} -r ${CMAKE_SOURCE_DIR} --exclude ".*tests.*" --html --html-details -o coverage_html/index.html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests and generating coverage report (coverage_html/index.html)"
        VERBATIM
    )
endif()

##### cppcheck target

option(ENABLE_CPPCHECK "Run static analysis" OFF)

if(ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck REQUIRED)
    
    set(CPPCHECK_OPTIONS
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
        --enable=all
        --inconclusive
        --check-level=exhaustive
        --force
        -i${CMAKE_SOURCE_DIR}/tests #exclude tests from cppcheck
        --error-exitcode=1
        --suppress=missingIncludeSystem
        --suppress=checkersReport
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

##### doxygen target

option(ENABLE_DOXYGEN "Generate Doxygen API docs" OFF)

if(ENABLE_DOXYGEN)
    find_package(Doxygen REQUIRED COMPONENTS dot)

    set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(doxygen
        COMMAND $<TARGET_FILE:Doxygen::doxygen> ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()