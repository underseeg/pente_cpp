# Pente CPP

[![CI Build Status](https://github.com/underseeg/pente_cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/underseeg/pente_cpp/actions/workflows/ci.yml)

This is a C++23 implementation of the game Pente. It supports two players via a command line interface.

This codebase is an exercise in writing Modern C++ and designing around algorithms rather than raw loops. The code in main is not intended to be clean, the subject of this exercise is the board and game logic in penteboard.h/cpp.

Readability & maintainability is favoured over pre-mature optimisation. This is a command line HCI after all; performance is not an issue.

For the sake of practicing good practice, there is a CI build. It does the following:

* On push and PR
  * Runs the build on Windows and Ubuntu
  * Enables common warnings and treats them as errors
  * Runs static analysis using cppcheck
  * Runs unit tests (with coverage data) on the board and game logic
  * Builds Doxygen documentation
* On push to main
  * Publishes Doxygen documentation to GitHub Pages
  * Publishes a coverage report to GitHub Pages

Documentation & coverage data (again, just for demonstrating good practice): <https://underseeg.github.io/pente_cpp/>
