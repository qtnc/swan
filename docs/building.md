# Building
The following libraries are needed to build the VM:

- A few of the **boost** header libraries, such as string algorithms
- [UTF8-CPP](http://utfcpp.sourceforge.net/)
- [Base64 encoding and decoding for C++](https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp)  (included)

To build the CLI, you will need additional libraries. They aren't needed if you don't want to build the CLI.

- (currently none)

You have the following compilation options to build or not certain parts of QScript.
To set them, use `#define`, or the `-D` command-line option of your compiler.

- NO_BUFFER: disable the Buffer type and related functions
- NO_MUTEX: disable thread locking; scripts shouldn't be run from multiple threads simultaneously if this option is set.
- NO_OPTIONAL_COLLECTIONS: disable Dictionary and LinkedList types as well as related functions
- NO_RANDOM: disable Random type and the rand() function
- NO_REGEX: disable regular expression support and String methods using regex. 
- USE_BOOST_REGEX: use boost::regex instead of std::regex for regular expression support. You will need to link with boost::regex.
- USE_COMPUTED_GOTO: use computed jump table to switch opcodes; may speed up or slow down the VM depending on the compiler.

