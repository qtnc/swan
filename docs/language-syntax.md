# Language syntax
Here is a description of the language's syntax.

## Reserved keywords
Some people take the number of keywords to grade the complexity of a language.

32 actual keywords currently in use (synonyms aren't counted):

* and (as a synonym for `&&`)
* as
* break
* case
* class
* const
* continue
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
* var (as a synonym for let)
* with
* while
* yield

2 keywords currently not used but reserved for potential future use:

* async
* await

## Literal values
* Boolean values: true, false
* Null value: null
* Num: 10, -69, 3.14, 0xFFFD, 0777, 0b111
* String: `"Hello"`, `'world'`
* List: `[1, 2, 3, 4, 5]`, `[]`
* Map: `{one: 1, two: 2, three: 3 }`, `{}`
* Tuple: `(1, 2, 3, 4, 5)`, `()`, `(single,)`
* Range: `0..10` (end exclusive), `1...10` (end inclusive)
* Set: `<1, 2, 3, 4, 5>, <1>, <>`
- Regular expression: /pattern/options

Numbers are all 64-bit floating point values (C double). Valid number forms include:

* Regular double: 3.14
* Hexadecimal: 0xFFFD
* Octal: 0o777 or 0777
* Binary: 0b111

Strings can be quoted using double quotes `"`, single quotes `'` or backtick `\``.

Regular expressions can be literaly written as /pattern/options.

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
Here are the common available control flows:

* if *condition* *statement*
* if *condition* *statement* else *statement*
* for *loop variable* in *iterable* *statement*
* repeat *statement* while *condition*
* while *condition* *statement*

Where:

* *Condition* is an expression evaluated for truth. Like in Ruby, null is considered false, any other value different than false itself is considered true (including number 0 and empty string)
* *Statement* is a single statement or a block of several statements enclosed in `{ ... }`
* *Loop variable* is the name of a variable that will be used to store each item produced by the iterator in turn, optionally preceded by `var`or `let` (like with JavaScript).
* *Iterable* is an expression that can be iterated (see iteration protocol below)

Note that, like Python and in contrary to C, C++, Java, JavaScript and many other C-like languages, parentesis are optional around the condition.
As in Python, a colon `:` can optionally separate the condition from a single statement for readability.

Swan provide also the popular *switch* control statement:

```
switch (testExpression) {
case value1: ...
case value2: ...
case value3: ...
else: ...
}
```

The test expression and the different case values are tested with the `==`operator by default. You can use another comparison function by specifying it: `switch testExpression with comparator { ... }`.
This can be useful to be more strict (by using `is` instead of `==`, or to achieve a different effect).

As in C/C++, Java and all other programming languages having the switch statement, the control jumps to the first matched value, and then the execution continues across all the following cases.
To prevent the execution of following cases, use the break keyword.
But in the contrary of C/C++, Java and others, you can compare anything you want, you aren't restricted to numbers or strings.

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
while iterator.hasNext {
let item = iterator.next
statements...
}
```

The iterator protocol has been inspired by Java.

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

A special syntax for writing literal grids exist, in two forms:

```
| 1, 2, 3 |
| 4, 5, 6 |
| 7, 8, 9 |
```

Which can also be written in its shortened form:  
`| 1, 2, 3; 4, 5, 6; 7, 8, 9 |`

You may mix the two forms, though it's quite unusual.

## Functions and closures
Functions can capture variables to form closures.
The `$` sign is a shorter synonym for the keyword `function`.

```
# Declaring a function
function half (x) {
return x/2
}
var five = half(10) # five is 5

# Declaring a function like a lambda
var double = $(x): return x*2
var twenty = double(10) # twenty is 20

# Declaring a function as a lambda with implicit `this` argument
var triple = $${ return this*3 }
var thirty = triple(10) # thirty is 30
```

* By using `$$` instead of `$`, you get an implicit `this` as first parameter of the function.
* Parentesis are optional if the lambda takes no explicit parameter or just a single one.
* If the last instruction of the function is an expression, it is taken as the return value

IF the last parameter is prefixed or suffixed by `...`, the excess parameters passed to the function are packed into a tuple. This enables variadict functions:

```
function printItems (...items) {
for item in items: print(item)
}

printItems("one", "two", "three")
printItems(1, 2, 3, 4, 5)
```

The tuple can be empty (if less parameters are passed to the function), but never null.

## Classes
As in all object-oriented programming languages, we can create classes and instantiate objects. The syntax is better explained with an example:

```
class Vector {
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

* A name starting with `_` denotes an instance field. They are implicitely declared at their first use.
* A name starting with `__` denotes a static field.
* A field cannot be accessed from outside its class. You cannot access a field from another object as well. IN fact, `object._field` is simply invalid.
* You can declare static methods by preceding its name by the static keyword.

Additionally to arithmetic, bitwise and comparison operators, you can also override prefix operators as well as call and indexing/subscript:

* *`()`* for call. Overriding call operator makes an object calalble like a function, i.e. `object(param1, param2, ...)`. 
* *`[]`* for indexing. Overriding indexing operator makes an object subscriptable like a list or map. It is possible to have more than one parameter to index multidimensional structures.
* *`[]=`* for index assignment. If you only override `[]`, but not `[]=`, indexing is read-only, i.e. you can write `object[x]` but not `object[x]=value`. Subscript setters take one additional parameter than subscript getters, the last parameter is the assigned value.
* *unm* for unary minus and *unp* for unary plus

## class inheritance and mixins
A  class can inherit from a single parent superclass.
Additional superclasses can be specified at declaration, but only their methods are copied, not their fields. This can be useful to create mixins.
If no superclass is explicitely specified, every class inherits by default from Object.

```
# A mixin class
# Every object that mixin this class has only to define a method < and a method ==
# and automatically inherits all other comparison operators
class Comparable {
>(x): x<this
>=(x): !(this<x)
<=(x): !(x<this)
!=(x): !(this==x)
}

class Vector: Object, Comparable {
# ... code from above ...
# Compare vectors by their length
< (other) { length < other.length }
== (other) { length == other.length }
}

var v1 = Vector(1, 2, 3)
var v2 = Vector(4, 5, 6)
# Call the > from Comparable, which will then call back the < from Vector
print(v2>v1) # true
```

## Method references and bound methos


```
# We can take a method from a class and use it as a standalone function:
# Num::+ is a kind of shortcut for $(a,b): a+b
var plus = Num::+, multiply = Num::*
print(plus(4, 3)) #7
print(multiply(3, 4)) #12

# We can also bind an object to a function:
# 3::* is a kind of shortcut for $(x): 3*x
var triple = 3::*
print(triple(9)) #27

# We can even assign a new method to a class already constructed
# The following is totally useless, but it's fun
Num::* = Num::/
print(4*5) #0.8 (!)
```


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
Fibers are better known under the names generator or coroutine:

```
let fibonacci = Fiber(${
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
- *Iterable* is an expression taht can be iterated over, such as a range, a list, a tuple, or a generator
- *Filter* is an optional filter condition; if the result of the condition is true for a given item, it will be generated in the final sequence; if not it will be skipped.
- Multiple for loops can be nested as shown in the example above.

A comprehension expression is compiled down to a code similar to the following:

```
$*{
for loopVariable in iterable {
if condition: yield expression
}
}
```

## Decorators
Here's another popular construction inspired by python:

```
function logged (value) {
print('' + value + ' has been decorated')
return $(...args) {
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
with expression as variable {
statements...
}
```

Which is equivalent to the following code:

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
The general syntax of import is: import *source* for *sourceName* as *destinationName*, ...

- *source* is the place from where to import. It's typically a constant string indicating a file name to load.
- *sourceName* is the name to import from in the source mapping
- *destinationName* is the name of the variable to which the imported symbol will be affected to

For exemple, by writing:
`import 'file.swan' for src as dest`
would load the file file.swan, search for an export named src, and put the imported symbol into the dest variable. After this statement, you can use dest as a regular local variable.
A matching export written in file.swan could be èxport something as src`.

Multiple symbols can naturally be imported at once from the same source with a comma-separated list: `import source for A as first, B as second, C as third, ...`.
Of course, the as clause can be omited, in which case the destination and source name are identical.
Import can also be used as an expression. In this case, `for` and `as` can't be used and a map is returned. This can be useful for dynamic imports, i.e. `var file = 'some.swan', map = import file`.

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

This may be used to simulates named function parameters to some extent, in combination with destructuring parameters (see below).

### Destructuring variable declaration
You can declare new variables and destructure them from a map or any numerically indexable sequence, as follows.
Note that, unlike JavaScript, *recursive destructuration* is impossible; in Swan it is kept simple; anyway, if recursion were possible, it would quickly become difficult to read, so it is anyway better avoided.

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

# You can use destructuration in function parameters
let f = $({one=1, two=2, three=3}) {
return one+two+three
}
f(three: 333, two: 222, one: 111) # 666

# You can also use destructuration in for loops and with comprehension syntax
let m1 = {one: 1, two: 2, three: 3}
for (key, value) in m1: print(key+'='+value) # It works since a map yield (key, value) tuples when iterated
let m2 = Map.of( (k+k, v*v) for (k, v) in m1) # This will produce { oneone: 1, twotwo: 4, threethree: 9 }
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
