# The T Programming Language

T is a C-like, module based programming language designed to be a fresh start, incorporating the good features from C and C++.

This repo is the source code for the T Language Compiler, `tlc`.

## Invoking

`tlc` accepts up to 2^31 - 2 arguments, which can be either a file name or an option. File names are extension-sensitive. Depending on the options, the compiler may produce an error if it encouters a file it does not recogize, or it may ignore the files it does not recognize with a warning. A list of the options follows.

### Options

#### Architecture

* `--arch=x86`: sets the architecture to target to x86_64. Conflicts with `--arch=sep`.

* `--arch=sep`: sets the architecture to target to sep. Conflicts with `--arch=x86`.

#### Warnings

All warning options have three forms, a `-Werror` form, a `-W...` form, and a `-Wno-...` form. These forms instruct the compiler to either produce an error if this particular event is encountered (stopping compilation), produce a warning, or ignore the issue.

* `duplciate-file`: duplciated files given. Defaults to error. If ignored, later files have no effect.

* `duplicate-import`: duplicated imports in a module. Defaults to ignore. If ignored, later imports have no effect.

* `unrecognized-file`: unrecognized file extensions. Default to error.