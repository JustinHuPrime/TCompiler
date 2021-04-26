# Contributing to the T Compiler

<!-- Thanks for contributing, point to Code of Conduct -->

<!-- Roadmap and issues -->

## Infrastructure

The T compiler uses the latest version of `gcc` to compile the latest version of C, but also requires `doxygen`, `texlive`, `graphviz`, and `texlive-latex-extra` to build documentation and typeset the standard.

To build the compiler, run `make` (optionally with `-j` to parallelize). This builds a lightly-optimized release version, and runs the tests.

## Project Structure

The compiler follows the standard pass structure, modified to support context-sensitive parsing:

0. arguments are parsed and the list of files to process is constructed
1. the files are parsed into an AST without type information (see `src/main/parser/parser.c` for more details about how this is done)
2. the AST is traversed and type information is added
3. the AST is converted into an IR
4. the IR is converted into assembly
5. the resulting assembly is written out

### Folder Structure

- `src` contains the source code for the compiler and its tests
  - `main` contains the compiler
    - `ast` contains the definition of the abstract syntax tree, environments, symbol tables, and types
    - `lexer` contains the lexer - while this could be a subfolder of `parser`, it's usable on its own, so it remains separate
    - `parser` contains the parser and symbol table builder
    - `typechecker` contains the type checker
    - `util` contains various utility functions
      - `container` contains data structures
  - `test` contains tests and the test engine for the compiler
    - `tests` contains the actual tests themselves
    - `util` contains utilities for tests
- `standard` contains the language standard
- `testFiles` contains files used for tests

## Code Style

The T Langauge Compiler uses clang-format as the code style enforcer. Your code style must match exactly with what clang-format generates. As per Google's C++ code style, indentation is in spaces, and one level of indent is two spaces. Doxygen is used to generate documentation. All header file declarations should have Doxygen documentation. Implementation file only declarations (static functions, etc.) may omit Doxygen documentation, but this is discouraged except in cases of truly trivial functions.
