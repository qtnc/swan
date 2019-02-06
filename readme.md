# Swan

Swan is a small scripting language designed to be embedded into C++ applications such as games.

# Language syntax 
```
# Define a vector class
class Vector {
# Define the constructor
constructor (x, y, z) {
# Fill the instance fields with the passed parameters
_x=x 
_y=y
_z=z
}

  # Define a property x (a getter and a setter)
  x { _x } 
  x= (value) { _x=value }

  # Similarly define an y and a z properties
  y { _y }
  y= (value) { _y=value }
  z { _z }
  z= (value) { _z=value }

  # Define a length method
  length { sqrt(_x**2 + _y**2 + _z**2) }

  # Overload the + operator
  + (other) { Vector(_x+other.x, _y+other.y, _z+other.z) }
}

# Create two vectors
let v1 = Vector(1, 2, 3)
let v2 = Vector(2, 2, -3)

# Use the overloaded +
let v3 = v1 + v2

# Call the length method and pritns 5
print(v3.length)

# WE forgot a toString method in our Vector class ! No problem, let's add it
Vector::toString = $(this){ "Vector(%1, %2, %3)".format(this.x, this.y, this.z) }

# Prints: the length of Vector(7, 8, 9) is 13.93
let coords = [7, 8, 9]
let v4 = Vector(...coords)
print("The length of " + v4 + " is " + v4.length) 
```

[>> Language syntax and basic constructs](docs/language-syntax.md)

[>> Standard library documentation](docs/stdlib.md)

# Features 
- Loosly typed
- Class object-oriented
- Allow functional style
- Everyting is an object, including numbers, closures, boolean values and null itself
- Fibers better known as generators or coroutines
- Operator overloading
- Extending or redefining classes at runtime (except built-in types)

Core built-in types: Bool, Fiber, Function, List, Map, Null, Number, Set, String, Tuple

In fact, I tried to take the best of JavaScript, Python and Ruby altogether.
This should make the language usable right out of the box for most programmers.

Other characteristics of my little programming language include:

- Single 64-bit floating point number type (C double), as in JavaScript
- Stack-based virtual machine
- Compact primary value representation thank to NaN tagging
- Mark and sweep garbage collector, running on demand only

# Safe embedding

The default standard library of the language don't provide any access to screen, keyboard, files, network or any other input/output device. 
The host C++ application is responsible for giving its own controlled API to the outside world if it wants to.
This makes Swan ideal for embedding into games or other applications wanting to provide user scripting capabilities.

Embedding has been carefully made to be as easy as it can be.

[>> More  details about embedding API](docs/embedding.md)

# Running Swan standalone
You can also write standalone programs in Swan  and run them with the CLI.
The CLI provides some common usage libraries such as file and console I/O access.

[>> More about the CLI](docs/cli.md)

[>> Standard library provided by the CLI](docs/cli-stdlib.md)

# Performances
Swan seem to be about 15-17% faster than python 3.6 and 48% slower than lua 5.1.4.
This isn't so bad. This places QScript #19 out of 50 in [scriptorium](https://github.com/r-lyeh-archived/scriptorium).
Be careful though, benchmarks never represent the reality.

[>> More on performances](docs/performances.md)

# Building
I have compiled Swan with GCC 8.1 on windows 10 (MinGW), both in 32 and 64 bits.
Let me know if you have issues with other compilers.

[>> More on building](docs/building.md)

# Why QSwan ?

I were looking for a language to add scripting capabilities in a game.
After months and even years of trials with different available libraries, none exactly were providing everything I needed, and therefore I decided to start my own scripting language project.

- [Lua](http://lua.org/) is ultra small, hyper fast, quite powerful, but has sometimes complicated and unusual object-oriented code due to its prototype-based implementation, as well as totally useless array indices starting at 1.
- Ruby is reasonably small, fast, has interesting ideas and philosophy, but has sometimes a weird syntax. I'm used to brace-like languages.
- JavaScript, especially since ES6 and ES2015, is really becoming a great language; but it is too bloated, too hard to build in a embeddable way, and too much linked to web development, even node.js. The base V8 engine from Google is just impossible to build because you need python 2, perl, and 10GB of useless tools I will never use. Google, please, I just want to do `g++ *.cpp` and then have fun, not pass days to download, install and configure things.
- [Python](http://python.org/) is simple, powerful and has a nice syntax, but has the problem to be almost unembeddable safely into a C++ application because system functions such as open are too deeply integrated in the core of the language. There exist smaller distributions but they are dedicated to devices for IoT, what isn't my case.
- [AngelScript](http://angelcode.com/) is a good project, in relatively modern C++, but really difficult to embed very well; and finally I need something more flexible than a statically-typed language.
- [Wren](https://github.com/wren-lang/wren) is really a nice language: quite small, simple, fast, ideal for embedding, nice syntax... but it leaks some important features like call reentrancy, compilation in a single pass make it difficult to customize what I feel missing, and... we are in 2019, why most of the people making scripting languages are always implementing them in C ?

For the implementation as well as most of the syntax, I have been inspired and learned a lot from Wren. It's really a good project.
I must say a great thank you to them, because they explain very well what is under the hoo of a scripting language, because many things stay an enigma when reading lua implementation.
