# Swan

Swan is a small scripting language designed to be embedded into C++ applications such as games.
Heavily inspired from the well known JavaScript and Python, its friendly syntax should quickly look familiar to anyone with a little programming experience.

# Language syntax 
The [language syntax](docs/language-syntax.md) looks like a mix between Python, JavaScript and Ruby, with a few original changes and additions. Example below:

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
Vector::toString = def(this){ "Vector(%1, %2, %3)".format(this.x, this.y, this.z) }

# Calls our toString and prints "Vector(3, 4, 0)"
print(v3)
```

[>> Language syntax and basic constructs](docs/language-syntax.md)

[>> Standard library documentation](docs/stdlib.md)

# Features 
- Gentle syntax
- Loosly typed
- Class object-oriented
- Allow functional style with closures and higher order functions
- Everyting is an object, including numbers, closures, boolean values and null/undefined themselves
- Fibers better known as coroutines
- Operator overloading
- Extending or redefining classes at runtime (except built-in types)


Core built-in types: Bool, Fiber, Function, List, Map, null, Number, Set, String, Tuple, undefined

Other characteristics of this little programming language include:

- Single 64-bit floating point number type (C double), as in JavaScript
- Stack-based virtual machine
- Compact primary value representation thank to NaN tagging
- Mark and sweep stop-the-world garbage collector

# Safe embedding

The default standard library of the language don't provide any access to screen, keyboard, files, network or any other input/output device. 
The host C++ application is responsible for giving its own controlled API to the outside world if it wants to.
This makes Swan ideal for embedding into games or other applications wanting to provide user scripting capabilities.

Embedding has been carefully made to be as easy as it can be, example below:

```
// Our Point C++ class
class Point {
public:
double x, y;
Point (double x1, double y1): x(x1), y(y1) {}
double length () { return sqrt(x*x+y*y); }
Point operator+ (const Point& p) { return Point(x+p.x, y+p.y); }
};

// Register the class
fiber.registerClass<Point>("Point");

// Register the constructor
fiber.registerConstructor<Point, double, double>();

// Register the destructor. Quite useless here as our type is a POD, but still, it's better to declare it anyway.
fiber.registerDestructor();

// Register two properties x and y
fiber.registerProperty("x", PROPERTY(Point, x));
fiber.registerProperty("y", PROPERTY(Point, y));

// Register the length method
fiber.registerMethod("length", METHOD(Point, length));

// Register the + operator
fiber.registerMethod("+", METHOD(Point, operator+));
```


[>> More  details about embedding API](docs/embedding.md)

# Running Swan standalone
You can also write standalone programs in Swan  and run them with the CLI.
The CLI provides some common usage libraries such as file and console I/O access.

The CLI also comes with an interactive REPL that let you have a quick code trial session:

```
?>> 1+1
2
?>> s = 'I have 5 apples and 3 oranges'
I have 5 apples and 3 oranges
?>> s.replace(/\d+/, def m: m[0].toNumber+5)
I have 10 apples and 8 oranges
?>> with IO.open('test.txt', 'w') as file {
?.. file.write('It works!\r\n')
?.. }
?>> file = IO.open('test.txt', 'r')
?>> file.read
It works!
?>> file.close
```

[>> More about the CLI](docs/cli.md)

[>> Standard library provided by the CLI](docs/cli-stdlib.md)

# Performances
Globally, Swan appears to be a little faster than CPython, but slower than Wren, and a lot slower than Lua.

[>> More on performances](docs/performances.md)

# Building
I have compiled Swan with GCC 8.1 on windows 10 (MinGW), both in 32 and 64 bits.
Let me know if you have issues with other compilers.

[>> More on building](docs/building.md)

[>> Code organization](docs/code-organization.md)

# Why Swan ?

I were looking for a language to add scripting capabilities in a game.
After months and even years of trials with different available libraries, none exactly were providing everything I needed, and therefore I decided to start my own scripting language project.

- [Lua](http://lua.org/) is ultra small, hyper fast, quite powerful, but has sometimes complicated and unusual object-oriented code due to its prototype-based implementation, as well as totally counter-intuitive array indices starting at 1.
- Ruby is reasonably small, fast, has interesting ideas and philosophy, but has sometimes a weird syntax. I'm used to brace-like languages and I'm not a big fan of their block concept.
- JavaScript, especially since ES6 and ES2015, is really becoming a great language; but it is too bloated, too hard to build in a embeddable way, and too much linked to web development, even node.js. The base V8 engine from Google is just impossible to build because you need python 2, perl, and 10GB of useless tools I will never use. Google, please, I just want to do `make` or `g++ *.cpp` and then have fun, not spend days to download, install and configure things.
- [Python](http://python.org/) is simple, powerful and has a nice syntax, but has the problem to be almost unembeddable safely into a C++ application because system functions such as open are too deeply integrated in the core of the language. There exist smaller distributions but they are dedicated to devices for IoT (micropython), what isn't my case.
- [AngelScript](http://angelcode.com/) is a good project, in relatively modern C++, but really difficult to embed very well; and finally I need something more flexible than a statically-typed language.
- [Wren](https://github.com/wren-lang/wren) is really a nice language: quite small, simple, fast, ideal for embedding, nice syntax... but it leaks some important features like call reentrancy, saving/reloading bytecode, relatively curious way to register foreign interfaces, compilation in a single pass make it difficult to customize what I feel missing, and... we are in 2019, why most of the people making scripting languages are always implementing them in C ?

For the implementation as well as most of the syntax, I have been inspired and learned a lot from Wren. It's really a good project.
I must say a great thank you to them, because they explain very well what is under the hood of a scripting language. Many things stay an enigma when reading lua implementation, which is much more complex.
Wren's syntax is mostly inspired from Ruby. As a consequence, Swan has also taken a little of it, although I tried to pythonize and javascriptize it to my tastes. 

With the hope that you will ahve as much fun using it as I have making it !
