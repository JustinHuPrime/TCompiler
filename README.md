# The T Programming Language version 0.2.0

![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/JustinHuPrime/TCompiler/unit-test.yml?branch=main)
![GitHub](https://img.shields.io/github/license/JustinHuPrime/TCompiler)
![GitHub release (latest SemVer including pre-releases)](https://img.shields.io/github/v/release/JustinHuPrime/TCompiler?include_prereleases)
<!-- Github issues and pr status goes here once we shift to that workflow -->

T is a C-like, module based programming language designed to be a fresh start, incorporating the good features from C and C++.

This repo is the source code for the T Language Compiler, `tlc`.

## Invoking

`tlc` accepts up to 2^31 - 2 arguments, which can be either a file name or an option. File names are extension-sensitive. Depending on the options, the compiler may produce an error if it encounters a file it does not recognize, or it may ignore the files it does not recognize with a warning. A list of the options follows.

### Options

Note that if a later option conflicts with an earlier option, the later option will apply to all files, even those before the earlier option. Additionally, all arguments after `--` are treated as files.

#### Informational Options

* `--help`, `-h`, `-?`: overrides all other options, displays usage information, and causes the compiler to immediately exit normally.

* `--version`: overrides all other options, except for `--help` and its abbreviations, displays version and copyright information, and causes the compiler to immediately exit normally.

#### Architecture

* `--arch=x86_64-linux`: sets the target architecture to x86_64 Linux (ELF, System V ABI, SSE2 required, `nasm` assembly syntax). Default.

<!-- #### Code Generation

* `-fPDC`: generate fixed-position code. Default.

* `-fPIE`: generate position independent code suitable for relocatable executable use.

* `-fPIC`: generate position independent code suitable for shared library or relocatable executable use. -->

#### Warnings

All warning options have three forms, a `-W...=error` form, a `-W...=warn` form, and a `-W...=ignore` form. These forms instruct the compiler to either produce an error if this particular event is encountered (stopping compilation), produce a warning, or ignore the issue. So, for example, `-Wfoo=error` makes `foo` into an error, `-Wfoo=warn` makes `foo` into a warning, and `-Wfoo=ignore` ignores `foo`.

<!-- * `const-return`: a function's declared return type is explicitly declared as constant. Defaults to warn. -->

<!-- * `duplicate-decl-specifier`: a data type has const or volatile applied to the same thing more than once. Defaults to warn. -->

* `duplicate-file`: duplicated files given. Defaults to error. If not an error, later files have no effect.

* `duplicate-import`: duplicated imports in a module. Defaults to ignore. If not an error, later imports have no effect.

<!-- * `reserved-id`: something is defined using an id starting with two underscores (a reserved id). Defaults to error. -->

<!-- * `unreachable`: unreachable statements detected. Defaults to warn. -->

* `unrecognized-file`: unrecognized file extensions. Defaults to error. If not an error, unrecognized files are skipped.

#### Debug Options

The option `--debug-dump` can be set to one of the following values:

* `--debug-dump=none`: default, turns off the data dump

* `--debug-dump=lex`: dumps the results of the lex phase

* `--debug-dump=parse`: dumps the results of the parse phase

* `--debug-dump=translation`: dumps the results of the translate to IR phase

* `--debug-dump=blocked-optimization`: dumps the results of the blocked IR optimization phase

* `--debug-dump=trace-scheduling`: dumps the results of the trace scheduling phase

* `--debug-dump=scheduled-optimization`: dumps the results of the scheduled IR optimization phase

Additionally, the following options can be used:

* `--debug-validate-ir`: validates IR after any step that changes it

* `--no-debug-validate-ir`: default, turns off IR validation

### Limits

The T compiler will memory map all referenced files. As such, the system must have enough address space to handle the memory mappings.
