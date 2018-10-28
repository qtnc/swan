# Language syntax
Here is a description of the language's syntax.

## Reserved keywords
Some people take the number of keywords to grade the complexity of a language.

28 actual keywords currently in use (synonyms aren't counted):

* and (as a synonym for `&&`)
* as
* break
* class
* const
* continue
* else
* export
* false
* for
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
* throw
* true
* try
* var (as a synonym for let)
* with
* while
* yield

5 keywords currently not used but reserved for potential future use:

* async
* await
* case
* default
* switch

## Literal values
* Boolean values: true, false
* Null value: null
* Number: 10, -69, 3.14, 0xFFFD, 0777, 0b111
* String: `"Hello"`, `'world'`
* List: `[1, 2, 3, 4, 5]`, `[]`
* Map: `{one: 1, two: 2, three: 3 }`, `{}`
* Tuple: `(1, 2, 3, 4, 5)`, `()`, `(single,)`
* Range: `0..10` (end exclusive), `1...10` (end inclusive)
* Set: <1, 2, 3, 4, 5>, <1>, <>
- Regular expression: /pattern/options

Numbers are all 64-bit floating point values (C double). Valid number forms include:

* Regular double: 3.14
* Hexadecimal: 0xFFFD
* Octal: 0o777 or 0777
* Binary: 0b111

Strings can be quoted using double quotes `"`, single quotes `'` or backtick `\``. 

@TODO: implement regular expression literal with the usual syntax `/pattern/options`

## Operators and their priority
Here are all available operators, ordered by priority from highest to lowest:

* Call operator: `(...)`
* Lookup operators: `.` and `::`
* Subscript operator: `[...]`, multiple indices are supported
* Postfix operators: currently none, reserved for future uses
* Prefix operators: `!, +, -, ~`
* Exponentiation operator: `**`
* Multiplication, division and modulus: `*, /, %`
* Addition and subtraction: `+, -`
* Bitwise operators: `|, &, ^, <<, >>`
* Range operators: `.., ...`
* Comparison operators: `==, !=, <, >, <=, >=, in, is`
* Logical short-circuiting operators: `&&, ||, ??`, resp. logical and, logical or, null coalescing; `and` and `or` can be used as alternatives to `&&` and `||` with the same priority
* Conditional ternary operator: `?:`
* Assignment operators: `=, +=, -=, *=, /=, %=, **=, &=, |=, ^=, <<=, >>=, &&=, ||=, ??=`

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

## Iteration protocol
A for loop written as:

```
for item in iterable {
statements...
}
```

Is more or less equivalent to the below code, except that `key` and `iterator` aren't effectively accessible.

```
let iterator = iterable.iterator
let key = null
while true {
key = iterator.iterate(key)
if !key: break
let item = iterator.iteratorValue(key)
statements...
}
```

## Lists, tuples and maps
List items are enclosed in brackets `[...]`. Items can be of etherogenous types (including list temselves).
Example: `[1, 2, 3.14, true, false, null, "String"]`.

Tuple items are enclosed in parentesis `(...)`. Items can be of etherogenous types (including tuples temselves).
Example: `(1, 2, 3.14, true, false, null, "String")`.
The difference between lists and tuples is that tuples are immutable.

Maps key/value pairs are enclosed in braces `{...}`. Key and values can be of any type, but you are strongly advised to use immutable keys, i.e. String or Number.

As in JavaScript, `{one: 1}` is the same as `{"one": 1}`. In other words, quotes around the key are optional as long as it is a valid name.

IF you don't specify the value explicitely, the key is also used as the value. Thus, `{1}` is equivalent to `{1: 1}` and `{one}` is the same as `{"one": one}`.

Use `{[one]: 1}` if you want the key to be the value of variable `one` (computed key). You may also write `{[1+1]: 2}` for {2: 2}`.

Maps are more or less compatible with JSON.

However, in contrary to JavaScript, map entries cannot be accessed using dot notation `map.key`. Use `map["key"]` instead.
Dot notation is exclusively used to call methods on objects.

## Functions and closures
Functions are always declared as lambdas, and they can capture variables to form closures.

```
var double = $(x): return x*2
var twenty = double(10) # twenty is 20

var triple = $${ return this*3 }
var thirty = triple(10) # thirty is 30
```

* By using `$$` instead of `$`, you get an implicit `this` as first parameter of the function.
* Parentesis are optional if the lambda takes no explicit parameter or just a single one.
* If the last instruction of the function is an expression, it is taken as the return value

IF the last parameter is suffixed by `...`, the excess parameters passed to the function are packed into a tuple. This enables variadict functions:

```
var printItems = $(items...) {
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

``````
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

## Bonus syntax suggars for object-oriented programming
There are some additional bonus syntax suggars as shown below:

The constructor of the above class Vector:

`` 
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

The *with* construct is also available as a shortcut for a common pattern:

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
