# Pente CPP

[![Build Status](https://github.com/underseeg/pente_cpp/actions/workflows/build.yml/badge.svg)](https://github.com/underseeg/pente_cpp/actions/workflows/build.yml)

This is a C++23 implementation of the game Pente. It supports two players via a command line interface.

This codebase is an exercise in writing Modern C++ and designing around algorithms rather than raw loops. The code in main is not intended to be clean, the subject of this exercise is the board and game logic in penteboard.h/cpp.

Readability & maintainability is favoured over pre-mature optimisation. This is a command line HCI after all; performance is not an issue.

The CI build validates on Windows and Ubuntu and does the following:

* enables common warnings and treats them as errors
* runs static analysis using cppcheck
* runs unit tests on the board and game logic
