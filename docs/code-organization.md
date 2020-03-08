# Code organization

1. If you want to embed Swan in your C++ application as a scripting language, you only need to build Swan VM and base library (under swan/ directory).
2. If you want to make little standalone scripts, or play yourself with the language outside of any C++ host, you also need to build the CLI (cli/ and modules/ directories)
3. If you want to create full Swan programs and distribute them to people who don't have Swan, you need again to add the executable image (exebin/ directory)

The code is organized as follows under src/ directory:

- cli/		Swan's command-line interface (CLI) including interactive REPL, compiler to bytecode and compiler to standalone executable. (2)
- cpprintf/		A tiny library to manage C-like printf syntax in C++. (1)
- exebin/: executable image for the compiler to standalone executable. (3)
- extmodules/: External modules compiled into DLL/SO, imported into Swan CLI using import mechanism. All of the modules under this directory are completely optional. You may use these modules in your own projects independtly of the CLI. (2)
- include/		Header files to include in your projects using Swan. (1)
- modules/		Built-in modules included in the CLI. You may use these modules in your own projects independtly of the CLI. (2)
- swan/		The Swan VM and base library. (1)
  - lib/		Swan's base library, containing implementations of methods of base types (String, List, etc). (1)
  - parser/		Swan parser and compiler. (1)
  - vm/		Swan virtual machine (VM) including garbage collector (GC). (1)
