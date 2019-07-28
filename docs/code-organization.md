# Code organization
The code is organized as follows under src/ directory:

- cli/		Swan's command-line interface (CLI) including interactive REPL, compiler to bytecode and compiler to standalone executable
- cpprintf/		A tiny library to manage C-like printf syntax in C++
- exebin/: executable image for the compiler to standalone executable
- include/		Header files to include in your projects using Swan
- modules/		Built-in modules included in the CLI. You may use these modules in your own projects independtly of the CLI.
- swan/		The Swan VM and base library
  - lib/		Swan's base library
  - parser/		Swan parser and compiler
  - vm/		Swan virtual machine (VM)
