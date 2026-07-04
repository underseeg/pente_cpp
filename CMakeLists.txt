cmake_minimum_required(VERSION 3.16)

project(pente_app VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(pente_board
    src/penteboard.cpp
)

target_include_directories(pente_board PUBLIC src)

add_executable(pente_app
    src/main.cpp
)

target_link_libraries(pente_app PRIVATE pente_board)

enable_testing()

add_executable(penteboard_tests
    tests/penteboard_tests.cpp
)

target_link_libraries(penteboard_tests PRIVATE pente_board)

add_test(NAME penteboard_tests COMMAND penteboard_tests)
