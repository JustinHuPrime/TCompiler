# The T Programming Language

T is a C-like, module based programming language designed to be a fresh start, incorporating the good features from C and C++.

This repo is the source code for the T Language Compiler, `tlc`.

## Invoking

`tlc` accepts up to 2^31 - 2 arguments, which can be either a file name or an option. File names are extension-sensitive. Depending on the options, the compiler may produce an error if it encouters a file it does not recogize, or it may ignore the files it does not recognize with a warning. A list of the options follows.

### Options

Note that if a later option conflicts with an earlier option, the later option will apply to all files, even those before the eariler option.

#### Architecture

* `--arch=x86`: sets the architecture to target to x86_64. Default.

* `--arch=sep`: sets the architecture to target to sep.

#### Warnings

All warning options have three forms, a `-W...=error` form, a `-W...=warn` form, and a `-W...=ignore` form. These forms instruct the compiler to either produce an error if this particular event is encountered (stopping compilation), produce a warning, or ignore the issue. So, for example, `-Wfoo=error` makes `foo` into an error, `-Wfoo=warn` makes `foo` into a warning, and `-Wfoo=ignore` ignores `foo`.

* `duplciate-file`: duplciated files given. Defaults to error. If not an error, later files have no effect.

* `duplicate-import`: duplicated imports in a module. Defaults to ignore. If not an error, later imports have no effect.

* `unrecognized-file`: unrecognized file extensions. Default to error. If not an error, unrecognized files are skipped.

#### Debug Options

The option `--debug-dump` can be set to 3 values:

* `--debug-dump=none`: default, turns off the data dump

* `--debug-dump=lex`: dumps the results of the 'lex' phase

* `--debug-dump=parse`: dumps the results of the parse phase

## Limits

Since the T compiler must read the 'module' line from every file, it keeps all specified declaration files open. Currently, this means that the compiler is limited by the number of open files permitted by your operating system.
