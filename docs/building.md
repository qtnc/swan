# Building

##Dependencies
The following libraries are needed to build the VM:

- [Boost](http://boost.org/): core, string_algo, iostreams (all are header-only)
- [UTF8-CPP](http://utfcpp.sourceforge.net/)
- [Base64 encoding and decoding for C++](https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp)  (included)

The following are optional. Their use are enabled or disabled depending on compilation options.

- [Boost](http://boost.org/): regex

To build the CLI, you will need additional libraries. They aren't needed if you don't want to build the CLI.

- (currently none)

## Makefile options
You have the following compilation options to build or not certain parts of Swan.


You can set the following variables when invoking make, `make mode=release` for example.

- mode: set it to *debug* or *release* to produce a debug or release build respectively. mode=debug by default.
- regex: set it to *boost*, *std* or *none* to respectively use boost::regex or std::regex as regular expression library, or none to disable regular expressions. By default, regex=std.
- options: a space-separated list of flags that you can set, see below for a list of possible flags. These flags are passed to the compiler using the -D switch.

Here are possible flags for the *options* variable. By default, none of them are set:

**Swan VM options**

- NO_BUFFER: disable the Buffer type and related functions. This also disables charset management and JSON encoding/decoding. Boost::iostreams is no longer necessary.
- NO_GRID: disable the Grid type and related functions. The grid syntax `| value, value, value, ... |` won't be parsed.
- NO_MUTEX: disable thread locking; scripts shouldn't be run from multiple threads simultaneously if this option is set.
- NO_OPTIONAL_COLLECTIONS: disable Dictionary and LinkedList types as well as related functions
- NO_RANDOM: disable Random type and the rand() function
- NO_REFLECT: disable reflection/meta-class global functions: createClass, load/storeGlobal, load/storeField, load/store(Static)Method
- NO_REGEX: disable regular expression support and String methods using regex. Literal regex syntax `/pattern/options` won't be parsed. This flag is automatically set if regex=none.
- USE_BOOST_REGEX: use boost::regex instead of std::regex for regular expression support. You will need to link with boost::regex. Automatically set if regex=boost.
- USE_COMPUTED_GOTO: use computed jump table to switch opcodes; may speed up or slow down the VM depending on the compiler.


**CLI options**

(currently none)
