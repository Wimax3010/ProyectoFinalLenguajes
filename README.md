# Rule-Based Language Interpreter and Static Analyzer

This project implements a custom production-grade compiler architecture to define, process, and execute a formal declarative rule-based language. The engine parses logical rules under an execution pipeline, builds an Abstract Syntax Tree (AST), runs iterative fixed-point evaluations over custom environments, and implements structural and dynamic static analysis routines.

## Team Members
* Isabella Benjarano
* Jerónimo Gutiérrez


## Environment Specifications
* **Operating System:** Compatible with Linux (Ubuntu 22.04 LTS or superior), macOS Sonoma, and Windows 11.
* **Language Environment:** C++17 Standard Compliant Profile.
* **Compiler Framework:** GCC `g++` (v11.0+) or Clang (v15.0+).
* **Build System:** Manual Terminal Compilation / Native Makefiles.

## Project Architecture
The system enforces clean separation of concerns separated into the following modular stages:
1. **Lexer:** Scans raw character feeds into explicit grammar tokens, managing case-sensitive keywords and validated regex expressions.
2. **Parser:** An optimized LL(1) manual recursive descent parser that transforms expressions into balanced logical nodes without external library dependencies.
3. **AST Layer:** Hierarchical structured representations of programs, constraints, and operational atomic conditions.
4. **Interpreter Engine:** Implements a fixed-point evaluation strategy, applying operational rule dependencies iteratively until environment state convergence is reached.
5. **Static Analysis Module:** Evaluates systems for structural Redundancies, multi-source Action Conflicts, and Potentially Inactive Rules.
