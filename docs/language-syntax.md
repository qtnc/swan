# Language syntax
Here is a description of the language's syntax.

## Reserved keywords
Some people take the number of keywords to grade the complexity of a language.

35 actual keywords currently in use (synonyms aren't counted):

* and (as a synonym for `&&`)
* as
* async
* await
* break
* case
* class
* const
* continue
* def (as a synonym for function)
* default
* else
* export
* false
* for
* function
* global
* if
* import
* in
* is
* let
* not (as a synonym for `!`)
* null
* or (as a synonym for `||`)
* repeat
* return
* static
* super
* switch
* throw
* true
* try
* undefined
* var (as a synonym for let)
* with
* while
* yield

## Literal values and basic types
### Numbers
Numbers are all 64-bit floating point values (C double). Valid number forms include:

* Regular double: 3.14
* Hexadecimal: 0xFFFD
* Octal: 0o777 or 0777
* Binary: 0b111

Usual operators are defined on numbers:

- Addition, subtraction, multiplication, division, modulus: `+, -, *, /, %`
- Exponentiation: `**`
- Integer division: `\\`
- Bitwise operators: `|, &, ^, <<, >>`

### Boolean values
The Bool type only admit two literal values: true and false.

### Strings
Strings can be quoted using double quotes `"`, single quotes `'` or backtick `\``.

The following escape sequencses are recognized:

- \r, \n: cariage return, line feed
- \t: tabulation
- \e, escape
- \b, \f, \0: backspace, form feed, null character
- \xHH: ASCII character, with HH two hex digits
- \uHHHH: unicode character, with HHHH four hex digits
- \UHHHHHHHH: extended unicode character,  with HHHHHHHH height hex digits

### Regular expressions
Regular expressions can be literaly written as /pattern/options.

Depending on the compilation options, more or less patterns and options are recognized.
General syntax follows perl or PCRE. Options includes:

- i: ignore case
- y: sticky/continous flag

### Ranges
A Range is a simple way to express a range of values with a starting and ending value. It is especially useful in for loops.

- 0..10: Closed range from 0 to 9, or 0 to 10 exclusive, i.e. 10 isn't in the range
- 1...10: Inclusive range from 1 to 10, i.e. 10 is in the range
- Range(start, end, step, inclusive): complete constructor. For example Range(1, 10, 2) will generate 1, 3, 5, 7, 9.

### Null and undefined
Null is different than undefined. As in JavaScript, null designates the absence of a value, while undefined indicates something that isn't, or not yet defined.
Undefined is what is returned when indexing a list out of its bounds, when requesting a non-exist key in a map, and it also serves to signal the end of an iterable sequence of values.
Null is never assigned or returned by anything in the Swan standard library; its use is entirely up to you.

## Operators and their priority
Here are all available operators, ordered by priority from highest to lowest:

* Call operator: `(...)`
* Lookup operators: `.`, `.?` and `::`
* Subscript operator: `[...]`, multiple indices are supported
* Postfix operators: currently none, reserved for future uses
* Prefix operators: `!, +, -, ~`
* Exponentiation operators: `**` and `@`
* Multiplication, division and modulus: `*, /, %`
* Addition and subtraction: `+, -`
* Bitwise operators: `|, &, ^, <<, >>`
* Range operators: `.., ...`
* Comparison operators: `==, !=, <, >, <=, >=, in, is`
* Logical short-circuiting operators: `&&, ||, ??`, resp. logical and, logical or, null coalescing; `and` and `or` can be used as alternatives to `&&` and `||` with the same priority
* Conditional ternary operator: `?:`
* Assignment operators: `=, +=, -=, *=, /=, %=, **=, @=, &=, |=, ^=, <<=, >>=, &&=, ||=, ??=`

Arithmetic, bitwise, comparison, range and prefix operators can be overloaded.
Other cannot. Compound assignment operators are always compiled down to operation and then assignment, i.e. `a+=b`  is always compiled as it has been written `a=a+b`.

## Control flow
Here are the available control flows in Swan:

- if *condition* *statement*
- if *condition* *statement* else *statement*
- repeat *statement* while *condition*
- while *condition* *statement*
- for *loop variable* in *iterable* *statement*
- for *loop variables*; *condition*; *tail* *statement*

Where:

- *Condition* is an expression evaluated for truth. Like in Ruby, null is considered false, any other value different than false itself is considered true (including number 0 and empty string)
- *Statement* is a single statement or a block of several statements enclosed in `{ ... }`

There's nothing special for *if*, *else* and *while*, they behave as in other programming languages.
The *repeat-while* loop is executed at least once. The *while* loop may not be executed at all if the condition is false to begin with.

The first form of *for* (also called foreach) loops over a sequence. 
It is like Python's *for-in*, JavaScript's *for-of*, Java's and C++'s enhanced for loops.

- *Loop variable* is the name of a variable that will be used to store each item produced by the iterator in turn, optionally preceded by `var`or `let` (like with JavaScript).
- *Iterable* is an expression that can be iterated (see iteration protocol below)

The second form of *for* is similar to the traditional C-like for loop:

- *loop variables* is a declaration of one or more variables that will be used in the loop statement. The instruction necessarily creates one or more new variables.
- *tail* is executed just before the *condition* after the first iteration; it's the typical place for variable increment.
- In the contrary to traditional C, none of the three parts may be omited

Note that, like Python and in contrary to C, C++, Java, JavaScript and many other C-like languages, parentesis are optional around the condition.
As in Python, a colon `:` can optionally separate the condition from a single statement for readability.

Swan provide also the popular *switch* control statement:

```
switch (testExpression) {
case value1: ...
case value2: ...
case value3: ...
default: ...
}
```

The test expression and the different case values are tested with the `==`operator by default. You can use another comparison function by specifying it as follows.
This can be useful to be more strict (by using `is` instead of `==`, or to achieve a different effect):

```
# Example 1 using < and > in comparisons
switch x {
case <0: print('x is negative'); break
case <10: print('x is small'); break
case >100: print('x is big'); break
}

# Example 2 using is to test type
switch value {
case is Num: print('you gave a number'); break
case is String: print('You gave a string'); break
default: print('Please give a number or a string'); break
}
```

As in C/C++, Java and all other programming languages having the switch statement, the control jumps to the first matched value, and then the execution continues across all the following cases; this behavior is commonly known as *fallthrough*.
To prevent fallthrough, use the break keyword.
But in the contrary of C/C++, Java and others, you can compare anything you want, you aren't restricted to numbers or strings.

Switch can also be used as an expression:

```
let n = 3
let r = switch n {
case 0: -1
case 1: 2
case 2: 4
case 3: 6
default: 12
}
```

When switch is used as an expression, the different cases must be single expressions as well (it is forbidden to have a block of statements). There is obviously no fallthrough once a result is selected.

Switch provide no performance bonus compared to a chain of if/else.

## Iteration protocol
A for loop written as:

```
for item in iterable {
statements...
}
```

Is more or less equivalent to the below code, except that `iterator` aren't effectively accessible.

```
let iterator = iterable.iterator
while true {
let item = iterator.next
if item is undefined: break
statements...
}
```

Thus, the end of the loop is simply signaled by returning *undefined* from the next method.

## Collections: lists, tuples, maps, sets
### Lists and tuples
List items are enclosed in brackets `[...]`. Items can be of etherogenous types (including list temselves).  
Example: `[1, 2, 3.14, true, false, null, "String"]`.

Tuple items are enclosed in parentesis `(...)`. Items can be of etherogenous types (including tuples temselves).   
Example: `(1, 2, 3.14, true, false, null, "String")`.

The difference between lists and tuples is that tuples are immutable. 
Tuples are normally used to hold etherogenous elements and where the position is usually significant.
Lists are better used when all elements are of the same type and where actual position in the list has no real significance.
As in python, due to syntax ambiguities, an additional comma must be present at the end of a single-item tuple, i.e. `(1,)`.

### Maps and sets
Maps key/value pairs are enclosed in braces `{...}`. Keys must be hashable, i.e. implement the method hashCode. This is the case for Num, String and Tuples.   
Example: `{"one": "un", "two": "deux", "three": "trois", "four": "quatre"}`

As in JavaScript, `{one: 1}` is the same as `{"one": 1}`. In other words, quotes around the key are optional as long as it is a valid name.

IF you don't specify the value explicitely, the key is also used as the value. Thus, `{1}` is equivalent to `{1: 1}` and `{one}` is the same as `{"one": one}`.

Use `{[one]: 1}` if you want the key to be the value of variable `one` (computed key). You may also write `{[1+1]: 2}` for {2: 2}`.

Maps are more or less compatible with JSON.
However, in contrary to JavaScript, map entries cannot be accessed using dot notation `map.key`. Use `map["key"]` instead. Dot notation is exclusively used to call methods on objects.

Sets are enclosed in angle brackets `<...>`. As with maps, items of a set must be hashable types such as Num, String or Tuple.
Example: `<1, 2, 3, 4, 5>`

Elements in a set are guaranteed to be unique. Sets support operators fors `&, |, -, ^`  for resp. intersection, union, difference and symetric difference.

You can create your own hashable types by implementing the *hashCode* method, which must return a number.
Similarly to Java, when you implement *hashCode*, you should also implement the `==` operator, as well as make sure that the hashCode remains immutable after the object has been created, otherwise you may not find back your item in a map or set.

### Grids a.k.a matrices
If Swan has been compiled with grid/matrix support, an additional Grid type is available.
It represents a 2D table, which gamers often call grid or 2D map, and which mathematicians call matrix. Each cell contains a value and is usually designated by its column (X) and row (Y).

The syntax for writing literal grids is as following:

```
[
  1, 2, 3;
  4, 5, 6;
  7, 8, 9
]
```

Which can also be written in a one-line shortened form:  
`[1, 2, 3; 4, 5, 6; 7, 8, 9]`

## Functions and closures
Functions can capture variables to form closures.
The `def` keyword is a shorter synonym for `function`.

```
# Declaring a function
function half (x) {
return x/2
}
var five = half(10) # five is 5

# Declaring a function like a lambda
var double = def(x): return x*2
var twenty = double(10) # twenty is 20

# Declaring a function as a lambda with implicit `this` argument
var triple = def>{ return this*3 }
var thirty = triple(10) # thirty is 30
```

- By using `>`, you get an implicit `this` as first parameter of the function. You may also use `*` as a shortcut for fiber (see below) or `&` for async.
- Parentesis are optional if the lambda takes no explicit parameter or just a single one.
- If the last instruction of the function is an expression, it is taken as the return value

IF the last parameter is prefixed or suffixed by `...`, the excess parameters passed to the function are packed into a tuple. This enables variadict functions:

```
function printItems (...items) {
for item in items: print(item)
}

printItems("one", "two", "three")
printItems(1, 2, 3, 4, 5)
```

The tuple can be empty (if less parameters are passed to the function), but never null.

Lambdas can be expressed using the arrow syntax, as popularized by Java and JavaScript as well:

```
# The following expression are all equal
x => x+1
x -> x+1
def(x): x+1
def(x){ return x+1 }
function (x) { return x+1 }

# The following expression are all equal
(a,b) => (b,a)
(a,b) -> (b,a)
def(a,b): (b,a)
def(a,b){ return (b,a) }
function (a, b) { return (b, a) }

# However, beware that the following three are **NOT** equal
# See further below in chapter destructuring for more info
(a,b)=>a+b # is equivalent to def(a,b): a+b
[a,b]=>a+b # is equivalent to def([a,b]): a+b
{a,b}=>a+b # is equivalent to def({a,b}): a+b
```

## Classes
As in all object-oriented programming languages, we can create classes and instantiate objects. The syntax is better explained with an example:

```
class Vector is Object {
# Constructor
constructor (x, y, z) {
_x=x # implicitly declares a field named x
_y = y # We know that they are field because of the _
_z = z
}

# Define getters for the three fields
# Fields are never accessible from outside if the class provide no getters
x { _x } 
y { _y }
z { _z }

# Define setters for the three fields
# If the class provides a getter but no setter, this makes a read-only property
x= (x) { _x=x }
y= (y) { _y=y }
z= (z) { _z=z }

# Define a regular method
# Since it doesn't take any parameter, it can equally be used as a property
# i.e. myvector.length and myvector.length() are equivalent
length { return sqrt(_x**2 + _y**2 + _z**2) }

# Operator overloading
+ (other) { return Vector(x+other.x, y+other.y, z+other.z) }
}

# Create two new vectors
var first = Vector(1, 2, 3)
var second = Vector(2, 2, -3)
var third = first + second  # Calls the overloaded +

print(third.x) # 3
print(third.y) # 4
print(third.z) # 0
print(third.length) # 5
```

- A  class can inherit from a single parent superclass. Everything is at least an Object If no other parent is explicitely specified.
- A name starting with `_` denotes an instance field. They are implicitely declared at their first use.
- A name starting with `__` denotes a static field. They are implicitely declared at their first use.
- A field cannot be accessed from outside its class. You cannot access a field from another object as well. IN fact, `object._field` is simply invalid. This provides built-in encapsulation.
- You can declare static methods by preceding its name by the static keyword.
- Each method take an implicit `this` first parameter.
- The constructor is simply called `constructor`. You can also define a static constructor, which will be called immediately after the class is defined. 

Additionally to arithmetic, bitwise and comparison operators, you can also override prefix operators as well as call and indexing/subscript:

- *`()`* for call. Overriding call operator makes an object calalble like a function, i.e. `object(param1, param2, ...)`. 
- *`[]`* for indexing. Overriding indexing operator makes an object subscriptable like a list or map. It is possible to have more than one parameter to index multidimensional structures.
- *`[]=`* for index assignment. If you only override `[]`, but not `[]=`, indexing is read-only, i.e. you can write `object[x]` but not `object[x]=value`. Subscript setters take one additional parameter than subscript getters, the last parameter is the assigned value.
- *unm* for unary minus and *unp* for unary plus

## Method references and bound methos

```
# We can take a function corresponding to an operator. 
# This is called generic method or method symbol
# ::+ is a kind of shortcut for def(a,b): a+b
let plus = ::+
print(plus(7, 9)) # 16
print(plus('ab', 'cd')) # abcd

# We can take a method from a class and use it as a standalone function:
# Num::+ is a kind of shortcut for def(a,b): a+b
var plus = Num::+, multiply = Num::*
print(plus(4, 3)) #7
print(multiply(3, 4)) #12

# We can also bind an object to a function:
# 3::* is a kind of shortcut for def(x): 3*x
var triple = 3::*
print(triple(9)) #27

# We can even assign a new method to a class already constructed
# The following is totally useless, but it's fun
Num::* = Num::/
print(4*5) #0.8 (!)
```

The difference between `::+` and `Num::+` is that, in the second case, the particular implementation of the `+` method for the class `Num` is taken. In the first case, the implementation taken depends on the first parameter, i.e. `::+(1,2)` will take `Num::+` while `::+('a', 'b')` will take `String::+`.


## Syntax suggars for object-oriented programming
There are some additional syntax suggars as shown below:

The constructor of the above class Vector:

```
constructor (x, y, z) {
_x=x 
_y = y 
_z = z
}
```

Can be shortened as:

```
constructor (_x, _y, _z);
```

By the same principle, the setter:

```
x { _x=x }
```

Can be simplified as:

```
x(_x);
```


There is even a better shortcut to define simple accessors. The `x`accessor for example:

```
x { _x }
x= (value) { _x=value }
```

Can be shortened into a simple `var x` inside the class:

`var x`

## Fibers
Fibers are better known under the name coroutine:

```
let fibonacci = Fiber(def{
let a=1, b=0
while true {
let tmp = a
a+=b
b=tmp
yield a
}
})

print("The first 10 numbers of the fibonacci serie are: ")
for i in 1...10 {
print(fibonacci)
}
```

By following the `def` keyword or the name of the function by `*`, a Fiber is created.
`def*(x){...}` is a shortcut for `Fiber(def(x){...})`.

## Comprehension syntax
You can create simple generators using the by comprhension syntax, similarely with what you can do in python.
The following also works to construct lists, tuples, sets and maps.

```
var multiplesOf5  = x for x in 1...100 if x%5==0 # Generates 5, 10, 15, 20, 25, 30, 35, 40, ..., 100
var squares = x**2 for x in 1...20 # generates 1, 4, 9, 16, 25, 36, ..., 400
var listOfSquares = [x**2 for x in 1...20] # Store the sequence in a list
var couples = [(x,y) for x in 1...5 for y in 1...5 if x<y] # Nested comprehention; this will generate [(1, 2), (1, 3), (1, 4), (1, 5), (2, 3), (2, 4), (2, 5), (3, 4), (3, 5), (4, 5)]
```

The general syntax is: *expression* for *loop variable* in *iterable* if *filter*, where:

- *Expression* is the value of the successive items to be generated; this expression normally uses the loop variable
- *Loop variable* is the name of the variable which will contain the successive values yielded by the *iterable*
- *Iterable* is an expression taht can be iterated over, such as a range, a list, a tuple, or a fiber
- *Filter* is an optional filter condition; if the result of the condition is true for a given item, it will be generated in the final sequence; if not it will be skipped.
- Multiple for loops can be nested as shown in the example above.

A comprehension expression is compiled down to a code similar to the following:

```
def*{
for loopVariable in iterable {
if condition: yield expression
}
}
```

Therefore, a comprehention expression returns a Fiber.

In fact, Swan supports extended/advanced comprehension. The complete syntax of an expression by comprehension is:
*expression* for *loop variable* in *iterable* *nested elements*, where nested elements can be zero, one, or more of:

- nested for loop
- `if condition`: a filter, the item is yielded only if the condition is met
- `while condition` or `break if condition`: the loop is stopped as soon as the condition become false
- `continue if condition`: similar to the simple filter, but resume to the next iteration instead of simply skipping the item
- `with name = expression`: see the with expression further below
- traditional for loops can also be used

```
let l = List(1..5, 5...1) # [1, 2, 3, 4, 5, 4, 3, 2, 1]
let l2 = [x**2 for x in l while x<=3] # [1, 4, 9] (stops as soon as x>3)
let l2 = [x**2 for x in l break if x>3] # [1, 4, 9] (another way to say the same thing)
let l3 = [x for x, count=0, it=l.iterator; (x=it.next) && count<6; count+=1 if count>=2] # [3, 4, 5, 4] (only yield from the 2nd to the 6th element)

# Assuming filenames is a list of file names, this will gather all the lines from all the files, opening and closing them properly. This example requires the CLI.
let lines = [line for filename in filenames with file = FileReader(filename) for line in file]
```

## Decorators
Here's another popular construction inspired by python:

```
function logged (value) {
print('' + value + ' has been decorated')
return def(...args) {
print('Calling ' + value + ' with arguments ' + args)
let returnValue = value(...args)
print('Called ' + value + ' with arguments ' + args + ' returned ' + returnValue)
return returnValue
}
}



# This will print:
# Function@... has been decorated
@logged function test (a, b) {
print('Function called with ' + a + ' and ' + b)
return a+b
}

# The following call will print:
# Calling Function@.... with arguments (12, 34)
# Function called with 12 and 34
# Called Function@.... with arguments (12, 34) returned 46
test(12, 34)
```

The following elements can be decorated:

- Function declarations (as above)
- Class declarations
- Variable declarations (the decoration applies once for each variables declared)
- Function/method parameters
- Lambda expressions

If there are multiple decorators specified for the same element, they are processed in their reverse order, i.e. the original element is passed to the last decorator, then the transformed element to the decorator before the last, etc. up to the first which makes the final transformation.

## Destructuring
Like in JavaScript ES6/ES2015, you can make destructuring assignments with any indexable collection such as tuples, lists and maps. It works as follows:

```
let map = {a: 1, b: 2, c: 3}
let list = [1, 2, 3, 4, 5]

# Creates a=1, b=2 and c=3
# Equivalent to: let a=map['a'], b=map['b'], c=map['c']
let {a, b, c} = map

# You may set default values in case the extracted map has no corresponding value
# This will create a=1, b=2, c=3 and d=true
let {a=false, b=true, c=false, d=true} = map

# The same is possible with indexable sequences such as tuples or lists
# This creates a=1, b=2, c=3, d=4 and e=5
# Equivalent to: let a=list[0], b=list[1], c=list[2], d=list[3], e=list[4]
let [a, b, c, d, e] = list

# You can use destructuring in function parameters
let f = def({one=1, two=2, three=3}) {
return one+two+three
}
f(three: 333, two: 222, one: 111) # 666

# You can also use destructuring in for loops and with comprehension syntax
# The following works because a map yield (key, value) tuples when iterated
let m1 = {one: 1, two: 2, three: 3}
for [key, value] in m1: print(key+'='+value) 

# This will produce { oneone: 1, twotwo: 4, threethree: 9 }
let m2 = Map( (k+k, v*v) for [k, v] in m1) 

# You can use destructuring with objects, too
class Student {
let name, grade
constructor (_name, _grade);
}
let classroom = [
Student('Alex', 57),
Student('Jane', 81),
Student('Dylan', 69)
]
for {::name, ::grade} in classroom {
print(format("%1 got a grade of %2 out of 100", name, grade))
}

# Destructuring can also be nested
let results = [
{ name: 'John', scores: [6, 3, 5] },
{ name: 'Jack', scores: [4, 2, 1] },
{ name: 'Joe', scores: [1, 6, 0] }
]
for {name, scores: [first, second, third]} in results {
print(name, first, second, third)
}
```

Difference with JavaScript: in JavaScript, you can use destructuring with any *iterable* element; in Swan, you can use destructuring only with any *indexable* element.
This means that the following is valid JavaScript, but invalid Swan:

```
let l = [1, 2, 3]
let [a, b, c] = l.map(function(x){ return x**2; });
```

## Error handling: try, catch, finally and with
The familiar *try, catch, finally* construct is available, as well as the *throw* keyword.

```
try {
throw "Something bad happened !"
} catch e {
print("An error occurred: " +e)
}
finally {
print("This is always executed, whether or not something bad effectively happened")
}
```

The *with* construct is also available as a shortcut for the common pattern of a resource that nees to be closed whatever happens:

```
with variable = expression {
statements...
}
```

Which is more or less equivalent to the following code:

```
let variable = expression
try {
statements...
}
finally {
variable.close
}
```

Except that the *variable* is no longer available after the closing brace. An optional catch block is allowed, where the variable is still available.


## Import and export
The general syntax of import is: import *destination* in *source*.

- *source* is the place from where to import. It's typically a constant string indicating a file name to load.
- *destination* is typically a destructuring expression to take what  you are interested in from the imported mapping.

For exemple, by writing:
`import {src: dest} in 'file.swan'`
would load the file file.swan, search for an export named src, and put the imported symbol into the dest variable. After this statement, you can use dest as a regular local variable.
A matching export written in file.swan could be èxport something as src`.

Simplifications used in map destructuring can of course be used in import, too. If the source name is the same as the destination name, `import {sth: sth} in 'file'` can be written `import {sth} in 'file'`. 
This is the most common case, since usually the name of the imported element is kept unchanged.

You can use `import *` to bind all exported map keys into local variables, but you can do it only if the source is a constant string, i.e. no `import *` from a dynamic source.
Import can also be used as an expression. In this case a map is returned. This can be useful for dynamic imports, i.e. `var file = 'some.swan', map = import file`. IN this case `import *` isn't permitted either.

To be able to import symbols from another source, they have to be explicitly exported first. The export keyword can be used in several ways:

- export *expression* as *exportName*
- export class *ClassName* { ... }
- export function *functionName* (arguments) { ... }
- export var *variableName* = *expression*

With the first syntax, the given expression is stored in the exporting symbols with the given name.
The second, third and fourth syntaxes respectively declare a class, function or serie of local variables. They are both stored in exporting symbols and normally usable in the current file.
When using the export keyword, a local variable *exports* is automatically created to hold a map of exported symbols. 

## Miscellaneous syntax suggars
### Implicit map 
When the last parameter of a function is a map, you can omit the encosing braces when calling it:

`func(1, 2, 3, x: 4, y: 5, z: 6)`  
is the same as
`func(1, 2, 3, {a: 4, b: 5, c: 6})`

This may be used to simulates named function parameters to some extent, in combination with destructuring.
`func(x: x)` can be further shortened as `func(x:)`.

### Implicit multiplication
In mathematics, the multiplication sign can be omited in certain cases. In Swan too !
It is possible between a number and an expression starting with a name or a parent. For example:

```
let x = 15, s = 'la'
print(3x) #45
print(7(x+1)) #42
print(7x+1) #36
print(3x**2) #775
print(3(x**2)%36) #27
print(3s) # 'lalala'
```

### Unpack operator
The unpack operator, as known as rest and spread operators in JavaScript ES6/ES2015, can be used to expand an iterable sequence into its elements, or to pack the last elements of an indexable sequence.
This special operator is also known as stared expression in python.

```
let l1 = [1, 2, 3], l2 = [4, 5, 6]
let l3 = [0, ...l1, ...l2, 7]
print(l3) # [0, 1, 2, 3, 4, 5, 6, 7]

let [a, b, ...c] = l3
print(a) #0
print(b) #1
print(c) #[2, 3, 4, 5, 6, 7]

let m1 = {a: 1, b: 2}, m2 = {c: 3, d: 4}
let m3 = {...m1, e: 5, ...m2}
print(m3) #{a: 1, b: 2, c: 3, d: 4, e: 5}

let {a, b, ...c} = m3
print(a) #1
print(b) #2
print(c) #{c: 3, d: 4, e: 5}
```

## Comments
Let's finish with the most usless thing, comments.
As in many programming languages, there exist *short comments* going up to the end of the line, and *long comments* that can span multiple lines.

- Short comments start with `#` and go up to the end of the line
- Long comments start with `#a` and end with `a#`, where `a` can be any non-space and non-alphanumeric symbol. An opening paren, brace or bracket naturally match with the corresponding closing symbol.

Examples:

```
# This is a single line comment

#* This is a long comment
    that can span multiple lines *#

#{ When using paren, brackets or braces,
    the comment must be closed with the adequate closing symbol
    rather than the same symbol as the one used for opening }#

#% Note that long comments may be nested
#% several times with the same symbol
%# this is still part of the comment
#[ this also, but stay careful
%# because this, no longer ! ]#
```

C-style comments `//` and `/*` are also recognized.
