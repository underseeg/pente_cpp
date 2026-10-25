# Pente CPP

[![CI Build Status](https://github.com/underseeg/pente_cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/underseeg/pente_cpp/actions/workflows/ci.yml)

This is a C++23 implementation of the game Pente. It supports two players via a command line interface.

This codebase is an exercise in writing Modern C++ and designing around algorithms rather than raw loops. The board and game rules live in `penteboard.h/cpp` and `pentegame.h/cpp`; the command-line program handles move entry and display. The code in main is not intended to be clean.

Readability & maintainability is favoured over pre-mature optimisation. This is a command line HCI after all; performance is not an issue.

For the sake of practicing good practice, there is a CI build (although I did not force PRs to main just for myself). It does the following:

* On push and PR
  * Runs the build on Windows and Ubuntu
  * Enables common warnings and treats them as errors
  * Runs static analysis using cppcheck
  * Runs unit tests (with coverage data, at 100% branch coverage) on the board and game logic
  * Builds Doxygen documentation
* On push to main
  * Publishes Doxygen documentation to GitHub Pages
  * Publishes a coverage report to GitHub Pages

Documentation & coverage data (again, just for demonstrating good practice): <https://underseeg.github.io/pente_cpp/>
