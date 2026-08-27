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

option(ENABLE_TESTS "Enable unit tests" ON)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(ENABLE_TESTS OR ENABLE_COVERAGE)
    enable_testing()

    find_package(GTest CONFIG REQUIRED)

    add_executable(penteboard_tests
        tests/penteboard_tests.cpp
    )

    target_link_libraries(penteboard_tests PRIVATE pente_board GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(penteboard_tests)
endif()

##### coverage support

if(ENABLE_COVERAGE)
    message(STATUS "Coverage enabled: adding coverage compile/link flags (scoped to pente_board and tests)")

    find_program(GCOVR_BIN NAMES gcovr REQUIRED)

    # Allow overriding the gcov executable name from CI command line
    set(GCOV_EXECUTABLE "gcov" CACHE STRING "gcov executable name (set via CI)")

    # Apply coverage to the library. Also to tests else not all coverage is collected.
    # We exclude the test files when generating the report.
    target_compile_options(pente_board PRIVATE -O0 -g --coverage)
    target_compile_options(penteboard_tests PRIVATE -O0 -g --coverage)

    # Ensure test executable is linked with coverage
    target_link_options(penteboard_tests PRIVATE --coverage)

    # Runs ctest and then gcovr to generate a coverage report in HTML format
    add_custom_target(coverage ALL
        DEPENDS penteboard_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage_html
        COMMAND ${GCOVR_BIN} --gcov-executable ${GCOV_EXECUTABLE} -r ${CMAKE_SOURCE_DIR} --exclude-throw-branches --exclude-unreachable-branches --exclude ".*tests.*" --html --html-details -o ${CMAKE_BINARY_DIR}/coverage_html/index.html
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