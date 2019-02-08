# The command-line interface (CLI)
The CLI can be used to run scripts. It provides a set of common usage libraries such as file access and console input/output.
It also features an interative mode (REPL) that let you quickly experiment live.

[>> Libraries provided with the CLI](cli-stdlib.md)

## CLI command-line options
Synopsis: swan [-c] [-e code] [-i] [-o outputFile] [scriptFile]

compile and Run the specified scriptFile. If no script is specified, run the interactive REPL.

Options:

- `-c`: compile a script without running it.
- `-h, --help` or `-?`: print CLI help and exit
- `-i`: run CLI interactive mode a.k.a REPL
- `-e`: evaluate the code given directly on the command-line
- `-o`: outuput file for byte-code compilation.


## The interactive REPL
The interactive REPL allow you to quickly experiment with the language and the libraries provided.

Type any code on the `?>>`prompt. The result of execution is printed if not null.
You may write instructions or blocks that take multiple lines; in this case, the prompt becomes `...` to indicate that it waits for more input.
Type `quit` or `exit` to close the REPL.
