# Code organization
The code is organized as follows under src/ directory:

- cli/		Swan's command-line interface (CLI) including interactive REPL
- cpprintf/		A tiny library to manage C-like printf syntax in C++
- include/		Header files to include in your projects using Swan
- modules/		Built-in modules included in the CLI. You may use these modules in your own projects independtly of the CLI.
- swan/		The Swan VM and base library
  - lib/		Swan's base library
  - misc/		Miscellaneous stuff needed by the VM
  - parser/		Swan parser and compiler
  - vm/		Swan virtual machine (VM)
