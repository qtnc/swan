# QScript

QScript is a small scripting language designed to be embedded into C++ applications such as games.

At the moment, it is still highly experimental though. I wouldn't recommand usig it yet.

# Language syntax and features 

- Loosly typed
- Class object-oriented
- Allow functional style
- Everyting is an object, including numbers, closures, boolean values and null itself
- Fibers better known as generators or coroutines
- Operator overloading
- Extending or redefining classes at runtime (except built-in types)
- Basic types: Bool, Fiber, Function, List, Map, Null, Num, Object, Tuple, Range, Regex, String

In fact, I tried to take the best of JavaScript, Python and Ruby altogether.
This should make the language usable right out of the box for most programmers.

[>> Language syntax and basic constructs](docs/language-syntax.md)

*TODO* [>> Standard library documentation](docs/stdlib.md)

Other characteristics of my little programming language include:

- Single 64-bit floating point number type (C double), as in JavaScript
- Stack-based virtual machine
- Compact primary value representation thank to NaN tagging
- Mark and sweep garbage collector, running on demand only

# Safe embedding

The default standard library of the language don't provide any access to screen, keyboard, files, network or any other input/output device. 
The host C++ application is responsible for giving its own controlled API to the outside world if it wants to.
This makes QScript ideal for embedding into games or other applications wanting to provide user scripting capabilities.

[>> More  details about embedding API](docs/embedding.md)

# Running QScript standalone
You can also write standalone programs in QScript and run them with the CLI.

*TODO*  [>> More about the CLI](docs/cli.md)

*TODO*  [>> Standard library provided by the CLI](docs/cli-stdlib.md)

# Performances
I took the recursive fibonacci test at  [scriptorium](https://github.com/r-lyeh-archived/scriptorium) as a reference for benchmarking.

According to my tests on my own machine: lua 5.1.4 = 1.689sec vs. QScript = 2.511sec, 
I obtain 67 points, what places QScript in rank #17 out of 50. This isn't a bad score.
By comparison, CPython 3.6 is slower with 2.880s, which makes 59 points.

Be careful though, benchmarks never represent the reality; don't expect these numbers to stay true in the real life.

# Building
The following libraries are needed to build the VM:

- A few of the **boost** header libraries, such as string algorithms
- Boost::regex
- [UTF8-CPP](http://utfcpp.sourceforge.net/)
- [Base64 encoding and decoding for C++](https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp)  (included)

To build the CLI, you will need additional libraries. They aren't needed if you don't want to build the CLI.

- (currently none)

You have the following compilation options to build or not certain parts of QScript.
To set them, use `#define`, or the `-D` command-line option of your compiler.

- NO_BUFFER: disable the Buffer type and related functions
- NO_OPTIONAL_COLLECTIONS: disable Dictionary and LinkedList types as well as related functions
- NO_RANDOM: disable Random type and the rand() function
- NO_REGEX: disable regular expression support and String methods using regex. You no longer need boost::regex; use this option if you don't have it.
- USE_COMPUTED_GOTO: use computed jump table to switch opcodes; may speed up or slow down the VM depending on the compiler.

# Why QScript ?

- Lua has sometimes unusual prototype-based object orientation and totally useless array indices starting at 1.
- Ruby has interesting ideas, but has sometimes a weird syntax
- JavaScript, especially since ES6 and ES2015, is really becoming a great language; but it is too bloated, too hard to build in a embeddable way, and too much linked to web development (even node.js).
- Python is simple, powerful and has a nice syntax, but has the problem to be almost unembeddable safely into a C++ application because system functions such as open are too deeply integrated in the core of the language

So I tried to take all the best in these languages to make something as familiar and as nice to use as all of them.
I hope that you will enjoy it as I do.
