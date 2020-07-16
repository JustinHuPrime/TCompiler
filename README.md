# The T Programming Language version 0.2.0

![GitHub](https://img.shields.io/github/license/JustinHuPrime/TCompiler.svg)

T is a C-like, module based programming language designed to be a fresh start, incorporating the good features from C and C++.

This repo is the source code for the T Language Compiler, `tlc`.

## Invoking

`tlc` accepts up to 2^31 - 2 arguments, which can be either a file name or an option. File names are extension-sensitive. Depending on the options, the compiler may produce an error if it encouters a file it does not recogize, or it may ignore the files it does not recognize with a warning. A list of the options follows.

### Options

Note that if a later option conflicts with an earlier option, the later option will apply to all files, even those before the eariler option. Additionally, all arguments after `--` are treated as files.

#### Informational Options

* `--help`, `-h`, `-?`: overrides all other options, displays usage information, and causes the compiler to immediately exit normally.

* `--version`: overrides all other options, except for `--help` and its abbreviations, displays version and copyright information, and causes the compiler to immediately exit normally.

<!-- #### Architecture

* `--arch=x86_64-linux`: sets the target architecture to x86_64 on Linux (ELF w/ System V ABI). Default.

#### Code Generation

* `-fPDC`: generate fixed-position code. Default.

* `-fPIE`: generate position independent code suitable for relocatable executable use.

* `-fPIC`: generate position independent code suitable for shared library or relocatable executable use. -->

#### Warnings

All warning options have three forms, a `-W...=error` form, a `-W...=warn` form, and a `-W...=ignore` form. These forms instruct the compiler to either produce an error if this particular event is encountered (stopping compilation), produce a warning, or ignore the issue. So, for example, `-Wfoo=error` makes `foo` into an error, `-Wfoo=warn` makes `foo` into a warning, and `-Wfoo=ignore` ignores `foo`.

<!-- * `const-return`: a function's declared return type is explicitly declared as constant. Defaults to warn. -->

<!-- * `duplicate-decl-specifier`: a data type has const or volatile applied to the same thing more than once. Defaults to warn. -->

* `duplciate-file`: duplciated files given. Defaults to error. If not an error, later files have no effect.

<!-- * `duplicate-import`: duplicated imports in a module. Defaults to ignore. If not an error, later imports have no effect. -->

<!-- * `overload-ambiguity`: an overload set whose members may be ambiguous when called using default arguments. Defaults to error. Functions whose arguments are always ambiguous (i.e. takes the same arguments when including defaulted arguments) always results in an error. -->

<!-- * `reseved-id`: something is defined using an id starting with two underscores (a reserved id). Defaults to error. -->

<!-- * `void-return`: returning void in a non-void function. Defaults to error. May not be an error if inline assembly is used to setup the return value. -->

<!-- * `unreachable`: unreachable statements detected. Defaults to warn. -->

* `unrecognized-file`: unrecognized file extensions. Defaults to error. If not an error, unrecognized files are skipped.

#### Debug Options

The option `--debug-dump` can be set to 3 values:

* `--debug-dump=none`: default, turns off the data dump

* `--debug-dump=lex`: dumps the results of the 'lex' phase

<!-- * `--debug-dump=parse-structure`: dumps the results of the parse phase, as a constructor-style tree -->

<!-- * `--debug-dump=parse-pretty`: dumps the results of the parse phase, as a pretty-printed T program -->

<!-- * `--debug-dump=ir`: dumps the results of the translate to IR phase -->

<!-- * `--debug-dump=asm-1`: dumps the results of phase one assembly translation (note that phases are architecture specific)

  * In `x86_64`, phase one is initial instruction selection. -->

<!-- * `--debug-dump=asm-2`: dumps the results of phase two assembly translation (note that phases are architecture specific)

  * In `x86_64`, phase two is register allocation. -->

### Limits

The T compiler will memory map all referenced files. As such, the system must have enough address space to handle the memory mappings.

## Contributing

The T Langauge Compiler uses clang-format as the code style enforcer. Your code style must match exactly with what clang-format generates. As per Google's C++ code style, indentation is in spaces, and one level of indent is two spaces. Doxygen is used to generate documentation. All header file declarations should have Doxygen documentation. Implementation file only declarations (static functions, etc.) may omit Doxygen documentation, but this is discouraged except in cases of truly trivial functions.
