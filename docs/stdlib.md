# Standard library
Here's a very quick reference of the standard library included with the language.

The following classes, objects, methods, functions, form the core part of the language.

Note that a few of these modules and methods related to them can be disabled at compile time with several options. This is the case for Regex, Random, Dictionary/LinkedList.
Unless so disabled, they are always available in the CLI as well as when embedding QScript into your own C++ application.

## Bool: is Object
A bool can take only two values: true or false.

- Constructor: Bool(any), taking the truth value of any
- Implicit construction when using literals *true* and *false*
- Comparison operators: ==, !=
- Unary operators: !

Methdos: 

- hashCode
- toString

## Buffer: is Sequence
A Buffer is an immutable sequence of octets or 8-bit characters, i.e. a file or memory block, or a non-UTF-8 string.

- Constructor: Buffer(items...): where each item is a number between 0 and 255, create the buffer from the given sequence of bytes
- Constructor: Buffer(string, encoding="UTF-8"), constructing a buffer holding the data of the strig given, encoding characters in the given encoding.
- Operators: +

Methods:

- static Buffer.of(sequences...): create a buffer from one or more concatenated buffers
- endsWith(needle): return true if this buffer ends with the data present in needle
- findFirstOf(needle): return the position where needle is found within this buffer, or -1 if not found
- lastIndexOf(needle, start=length): return the position where needle is found within this buffer starting search from the end, or -1 if not found
- length: return the length of the buffer in bytes
- indexOf(needle, start=0): return the position where needle is found within this buffer, or -1 if not found
- startsWith(needle): return true if this buffer starts with the data present in needle
- toString

## Class: is Object
An object of type Class represents a class, e.g. Object, Num, Bool, etc.

Methods:

- is: the is operator overload of class provide for instance check, i.e. `4 is Num` returns true.
- name: name of the class, e.g. Num, Bool, etc.
- toString: this overload returns the same as name

## Dictionary: is Sequence
A Dictionary is an associative container where keys are sorted.

Constructor: Dictionary(sorter=::<, items...), where sorter is any kind of callable taking two arguments and returning true if the first argument goes before the second.
- Operators: [], []=, in

Methods:

- Dictionary.of(sorter, sequences...): create a dictionary from one or more source mappings
- clear: remove all entries from the dictionary
- length: return the number of entries present in the dictionary
- lower(key): returns the nearest key present in the dictionary that come after the key given, or return the argument given itself if it is present
- remove(...keys): remove one or more keys from the dictionary
- toString: return a string like "{a: 1, b: 2, c: 3}"
- upper(key): returns the nearest key present in the dictionary that come after the key given

## Function: is Object
Class representing all function objects.

No specific methods beside () operator.

## Fiber: is Sequence
A Fiber represent a paralel execution fiber, as known as generator or coroutine.
You can iterate a fiber to fetch all yielded values in turn.

NO specific methods beside () operator.

## LinkedList: is Sequence
A linked list is a collection of items connected together via linked nodes. IN principle, all items are of the same type, though this isn't enforeced.
When items are frequently added or removed at the beginning or at the end but never in the middle of the list, its performances are better than List. LinkedList fits well when used as a queue or stack.

- Constructor: LinkedList(items...): construct a linked list from a serie of given items
- Operators: [], []=, in

Methods:

- static LinkedList.of(sequences...): create a linked list from the concatenation of one or more sequences
- push(...items): push one or more items at the end of the list
- pop: pop an item from the end of the list and return it
- remove(...items): remove one or more items from the list
- removeIf(predicate): remove all items from the list for which the predicate returned true
- shift: pop an item from the beginning of the list and return it
- toString: return a string like "[1, 2, 3]"
- unshift(...items): push one or more items at the begining of the list

## List: is Sequence
A list is a collection of items, in principe all of the same type (even if it isn't enforced). They are generally good, except when items are frequently added or removed in the beginning or in the middle of the list, in which case LinkedList is better.

- Constructor: List(items...): create a list from the given items
- Implicit construction when using [...] notation
- Operators: [], []=, in

Methods:

- static List.of(sequences...): create a list from one or more concatenated sequences
- add(...items): add one or more items at the end of the list
- clear: clear the whole list
- draw([random=rand], count=1): randomly draw the specified number of elements from the list. Drawn elements aren't removed from the source list.
- draw([random=rand], weights): randomly draw an element from the list, selecting the element with probabilities weights (See Random for more info). Drawn element isn't removed from the list.
- insert(index, ...items): insert one or more items starting at the given position
- indexOf(needle, start=0): search for needle in the list, return its position if found, -1 if not
- lastIndexOf(needle, start=length): search for needle in the list from the end, return its position if found, -1 if not
- length: return the number of element in the list
- pop: remove an item from the end of the list and return it
- push(...items): add one or more items at the end of the list
- remove(...items): remove one or more items
- removeAt(...indices): remove one or more items at specified indices. Indices can be numbers or ranges.
- removeIf(predicate): remove all items from the list for which the predicate returned true
- reverse: reverse the elements in the list, so that the first becomes the last one and vice-versa.
- rotate(distance): shift the items in the list; depending on distance, first elements become the last ones or last become the first ones.
- shuffle(random=rand): randomly shuffles the elements in the list
- sort(comparator=::<): sort the elements in the list
- toString: return a string like "[1, 2, 3, 4, 5]"

## Map: is Sequence
A Map is an associative container where key/value pairs are held with no particular order. If keys need to be ordered, Dictionary must be used.
IN order to be held in a Map, keys must all be hashable, i.e. implement the hashCode method. This is the case for Num, String and Tuple.

- Constructor: Map(items...): where items can be a Dictionary, another Map, or any sequence of key/value pair tuples.
- Implicit construction when using {...} notation
- Operators: [], []=, in

Methods: 

- static Map.of(sequences...): construct a Map from one or more source mappings
- clear: remove all items from the map
- flipped: return a map where keys become values and values become keys.
- getOrCompute(key, func): return this[key] if it is present in the map; otherwise, compute func(key) and store it in the map before returning it.
- keys: return a sequence enumerating all existing keys
- length: return the number of key/value pairs present in the map
- remove(...keys): remove one or more keys from the map
- toString: return a string like "{a: 1, b: 2, c: 3, d: 4}"
- values: return a sequence enumerating all values

## Null: is Object
The Null class has only one instance, null itself.

No specific method

## Number: is Object
Class of all numbers.

- Constructor: Num(string, base=10): where base can be between 2 and 36 inclusive
- Implicit construction when using number literals such as 123, 3.14, -49, 0xFF, 0b111
- Operators: `+, -, *, /, %, **, \, &, |, ^, <<, >>`
- Comparison operators: >`<, <=, ==, >=, >, !=`
- Unary operators: ~, +, -

Methods:

- format(precision=2, decimalSeparator=".", groupSeparator="", padding=0, groupLength=3): format the number into a string with the specified parameters: precision is the number of digits after the decimal separator. Example: `12345.6789.format(2, ",", "'")` results in `12'345.68`. Giving a precision <0 requests for exponential notation. IF padding!=0, the appropriate number of 0s are prepended to make a string of the given length.
- frac: return the fractional part of the number, e.g. 1.23 and -67.89 resp. return  0.23 and -0.89
- hashCode
- int: return the integer part of the number, e.g. 1.23 and -67.89 resp. return 1 and -67.
- sign: return the sign of the number, 1 for positive, -1 for negative or 0 for 0 (a.k.a signum)
- toString(base=10): where base can be between 2 and 36 inclusive

Math functions: can be indifferently called as Num methods or as traditional gobal functions, e.g. `(-10).abs` or `abs(-10)`.

- abs(n): absolute value
- acos(n), asin(n), atan(n), cos(n), sin(n), tan(n): trigonometric functions
- acosh(n), asinh(n), atanh(n), cosh(n), sinh(n), tanh(n): hyperbolic trigonometric functions
- cbrt(n), sqrt(n): cubic and square root
- ceil(n), floor(n), round(n), trunc(n): rounding functions
- exp(n), log(n, [base]): exponential and logarithm

## Object
Object is the base class for all objects.

- Comparison operators: `==, !=, is`
- Unary operators: !

Methods: 

- toString: if there is no more specific overload, the default toString of all objects return the type and the memory location where the object is, e.g. "Object@0xFFFD000012345678"
- type: return the type of the object as Class object

## Range: is Sequence
A range, as its name says, denotes a range of numbers.
Each range has a *start*, *end* and *step*. Ranges can be viewed as collections that efficiently contain any value `start + step*N` for any integral N N as long as the result is between *start* and *end*.

- Constructor: Range(start, end, step=1, endInclusive=false)
- Constructor: Range(end, step=1, endInclusive=false), where start=0
- Implicit construction when using N..M or N...M  notations, where N..M is equivalent to Range(N, M, 1, false) and N...M to Range(N, M, 1, true).
- Operators: [], in

No specific methods

## Regex: is Object
A Regex object holds a compiled regular expression. 
- Constructor: Regex(pattern, options=""), where pattern is a regular expression in ECMAScript syntax.
- Implicit construction when using the /pattern/options notation

Methods: 

- length: return the number of capturing parens
- test(string): return true if the string completely matches the regular expression

Regular expression options:

- c: use locale collations if possible
- f: find/replace only the first occurence
- i: ignore case
- y: sticky flag; further matches must start when the last one stopped (a.k.a. continuous mode)
- E: ignore empty matches
- M: no multiline; `^`and `$` only match at the beginning/end 
- S: no dot all; `.` don't match newlines
- s: dot all: `.`matches newlines
- x: extended mode; allow spaces in the regex expression
- z: extended replacement format: allow certain special syntaxes in replacement strings

Depending if compilation has been done with boost::regex or std::regex, some syntax constructs and options may, or may not be available.
For example, look-behind assertions `(?<=...)` are only available with boost.
Options c, s, x, z, E, M, S are also only available with boost.

## RegexMatchResult
RegexMatchResult represent a match result of a regular expression match or search. This object can be returned by String.search or in the callback of String.replace.

- Operators: []

Methods:

- end(group=0): return the position where the match of the nth group ends
- length(group=0): return the length of the nth matched group
- start(group=0): return the position where the match of the nth group starts

## Random: is Object
A random object holds the state of a pseudo-random number generator.

Operator() is used to generate a random number out of the generator.
A default global Random instance is created with the name *rand*.

- rand(), without parameters: generate a number between 0 and 1
- rand(n), for n<=1: generate a number between 0 and 1 and return true if the generated number is <n.
- rand(n), for n>1: generates an integer between 0 and n exclusive.
- rand(min, max): generates a number between min and max inclusive
- rand(sequence), for any sequence of numbers: generate an integer between 0 and sequence.length with weighted probabilities. For example, `rand([1, 2, 3])` will generate 0 with a probability of 1/6, 1 with probability 2/6 and 2 with probability 3/6. To draw an element randomly from a sequence, see List.draw.

Other methods:

- Constructor: Random([seed]): construct a pseudo-random number generator with a given seed number, or use any system-dependent method of obtaining seed if seed is omited
- normal(mu=0, sigma=1): generates a number according to normal/gaussian distribution with mean mu and deviation sigma. The generated number has ~65% chance to be between mu-sigma and mu+sigma, ~90% between mu -2sigma and mu +2sigma, and ~96% between mu -3sigma and mu +3sigma. There is no bounds, so a number as big as mu + 100sigma may be generated, though with an extremely low probability.

## Sequence: is Object
The sequence class is the base class for all subclasses holding a sequence of something, such as String, List, Tuple, Map, etc.
For more info about iterator, iterate and iteratorValue methods, see iteration protocol.

- iterator: return an object that can be iterated via iterate and iteratorValue methods. The default for sequences is to return itself.
- iterate(key): given the current key, return the key of the next item. Passing key=null yield the first key; returns null if the key passed was the last one.
- iteratorValue(key): return the value corresponding to the iteration key passed

Other methods:

- all(predicate): return true if predicate(x) returned true for all x in the sequence. Return true for empty sequence.
- any(predicate): return true if predicate(x) returned true for at least one x in the sequence. Return false for empty sequence.
- count(needle): return the number of elements in the sequence that are equal to needle.
- countIf(predicate): return the number of x in the sequence for which predicate(x) returned true
- dropWhile(predicate): return a sequence with all x in this sequence, until predicate(x) returns true
- enumerate(n=0): return a sequence where each elements in the sequence are transformed into tuples `(n, this[0]), (n+1, this[1]), (n+2, this[2]), ..., (n+l, this[l])`
- filter(predicate): return a sequence containing only x for which predicate(x) returned true
- find(predicate)): return the first x for which predicate(x) returns true, or null if none returned true.
- first: return the first element of the sequence. Note that the sequence may don't have a predictable orders, e.g. Set
- join(separator=""): return a string by concatenating all elements from the sequence in turn, separating them with the given separator.
- last: return the last element of the sequence. Note that the sequence may don't have a predictable orders, e.g. Set
- limit(n): return a sequence limited to n elements, i.e. all elements after the nth are dropped
- map(mapper): return a new sequence with elements mapped from this sequence
- max(comparator): return the greatest element of the sequence according to the comparator given. 
- min(comparator): return the least element of the sequence according to the comparator given. 
- none(predicate): return true if predicate(x) returned true for none of the x in the sequence. Return true for empty sequence.
- reduce(reducer, initial=null): iteratively reduce elements from this sequence using the reducer given. Return null for empty sequence, initial if the sequence has a single element.
- skip(n): return a sequence with n first elements skipped
- skipWhile(predicate): return a sequence with all x in this sequence, skipping initial elements as long as predicate(x) returns true
- toList: return a list containing the elements of this sequence
- toMap: return a map containing the elements of this sequence
- toSet: return a set containing the elements of this sequence
- toTuple: return a tuple containing the elements of this sequence
- zipWith(sequences...): produce a sequence of tuples containing paired elements. For example, `(1, 2, 3).zipWith(('a', 'b', 'c'))` whould produce `(1, 'a'), (2, 'b'), (3, 'c')`

## Set: is Sequence
A Set is a collection of items, in principle all of the same type (although nothing is enforced), where order has no importance and where any item may only be present once.
Another characteristic of sets beside the uniqueness of held objects is their ability to make set opations: union, intersection, difference and symetric difference

- Constructor: Set(...items): construct a set from individual elements
- Implicitly constructed when using `<...>` notation
- Operators: `-, &, |, ^, in`

Methods: 

- static Set.of(sequences...): construct a set from one or more concatenated sequences
- add(...items): add one or more items
- clear: empty the whole set
- length: return the number of elements present in the set
- remove(...items): remove one or more items from the set
- toString: return a string like "<1, 2, 3, 4, 5>"

## String: is Sequence
A String holds an immutable sequence of UTF-8 characters.

- Constructor: String(bufffer, encoding), will convert the data in the given buffer into a string, decoding characters of the specified encoding.
- Constructor: String(any), any will be converted to string using toString method.
- Operators: [], +, in

Methods: 

- static String.of(...sequences): construct a string by concatenating one or more other strings or objects
- codePointAt(index): return the code point at given character position 0..0x1FFFFF
- endsWith(needle): return true if needle is found at the end of the string
- findAll(regex, group=0): find all matches of the regular expression against this string. For each match, take the group number given as result, or return a list of RegexMatchResult objects if group=true.
- findFirstOf(needles, start=0): search for the first occurence of one of the characters inside needle; return -1 if nothing is found.
- format(...items): take this string as a format string and format accordingly; see the format global function for more info.
- hashCode: return the hash code of the object, especially used as hashing key for maps, sets and similar structures.
- indexOf(needle, start=0): search for needle in the string, returning the position where it has been found, or -1 if not found.
- lastIndexOf(needle, start=length): search for needle in the string from its end, returning the position where it has been found, or -1 if not found.
- length: return the length of the string, its number of characters / code points.
- lower: transform the string to lowercase
- replace(needle, replacement)): search for occurences of needle and replace them with replacement. Needle can be a String or a Regex. If needle is a Regex, replacement can be a String or a callback function, which will be called with a RegexMatchResult object for each match found, the return value of that callback is taken as final replacement in the string for the match.
- search(regex, start=0, returnFullMatchResult=false): search for a match of the Regex against the string. Return a position or -1 if returnFullMatchResult=false, a RegexMatchResult object or null if returnFullMatchResult=true.
- split(separator): split the string into a sequence of elements using a given separator. The separator can be a string or a regex.
- startsWith(needle): return true if needle is found at the start of the string
- toNumber(base=10): convert the string to a number; base can be between 2 and 36.
- toString: return itself
- upper: transform the string to uppercase

## Tuple: is Sequence
A tuple is a sequence of items, generally of etherogeneous types, as opposed to lists where all elements are supposed to be of the same type.
The order of the elements in a tuple are also often significant, i.e. `(1, 2)` means something else than `(2, 1)`, while it is often unsignificant for lists.
The other big difference with lists is that tuples are immutable.

- Constructor: Tuple(...items): construct a tuple from one or more individual items
- Implicitly constructed when using (...,) notation
- Operators: []

Methods:

- hashCode
- length: return the number of elements in this tuple
- toString: return a string like "(1, 2, 3, 4, 5)"

## System class
The System class isn't instantiable. It provies general System information and features.

Static methods:

- gc: run the garbage collector

## Global functions
- format(fmt, ...items): create a string where %1, %2, %3, etc. are replaced by the 1st, 2nd, 3rd, etc. parameters after fmt.
- format(fmt, map): create a string where expression $([a-z]+) are replaced by the corresponding value in the map.
- gcd(...values), lcm(...values): compute the GCD (greatest common divisor) or LCM (least common multiple) of the values given. Return 1 if called without any argument.
- max(...items), min(...items): return the least or greatest of the given items

Additionally, math functions (sin, cos, log, etc.) as well as rounding functions (floor, round, ceil, etc.) may or not be available as global functions. See Number class for a list of math functions.

