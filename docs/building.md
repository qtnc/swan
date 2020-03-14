# Building

##Dependencies
The following libraries are needed to build the VM:

- [Boost](http://boost.org/): core, string_algo, heap, container  (all are header-only)
- [UTF8-CPP](http://utfcpp.sourceforge.net/)

The following are optional. Their use are enabled or disabled depending on compilation options.

- [Boost](http://boost.org/): regex
- [Base64 encoding and decoding for C++](https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp)  (included)

To build the CLI, you will need additional libraries. They aren't needed if you don't want to build the CLI.

- [Boost](http://boost.org/): filesystem

## Makefile options
You have the following compilation options to build or not certain parts of Swan.


You can set the following variables when invoking make, `make mode=release` for example.

- mode: set it to *debug* or *release* to produce a debug or release build respectively. mode=debug by default.
- regex: set it to *boost*, *std* or *none* to respectively use boost::regex or std::regex as regular expression library, or none to disable regular expressions. By default, regex=std.
- options: a space-separated list of flags that you can set, see below for a list of possible flags. These flags are passed to the compiler using the -D switch.

Here are possible flags for the *options* variable. By default, none of them are set:

**Swan VM options**

- DEBUG: include additional debugging code. Automatically set if mode=debug.
- DEBUG_GC: add GC-specific debugging code and stresses the GC to be triggered at every memory allocation.
- NO_GRID: disable the Grid type and all related functions. The grid syntax won't be parsed. Implies NO_GRID_MATRIX and NO_GRID_PATHFIND.
- NO_GRID_MATRIX: disable all grid methods related to matrices
- NO_GRID_PATHFIND: disable all grid methods related to pathfinding
- NO_OPTIONAL_COLLECTIONS: disable Dictionary, LinkedList and SortedSet types as well as related functions
- NO_RANDOM: disable Random type and the rand() function
- NO_REFLECT: disable reflection/meta-class global functions: createClass, load/storeGlobal, load/storeField, load/store(Static)Method
- NO_REGEX: disable regular expression support and String methods using regex. Literal regex syntax `/pattern/options` won't be parsed. This flag is automatically set if regex=none.
- NO_THREAD_SUPPORT: disable thread locking; scripts shouldn't be run from multiple threads simultaneously if this option is set. May improve performences if Swan is run by only one thread.
- USE_BOOST_REGEX: use boost::regex instead of std::regex for regular expression support. You will need to link with boost::regex. Automatically set if regex=boost.
- USE_COMPUTED_GOTO: use computed jump table to switch opcodes; may speed up or slow down the VM depending on the compiler.


**CLI options**

(currently none)
