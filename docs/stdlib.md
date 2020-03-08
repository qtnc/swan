# Standard library
Here's a reference of the standard library included with the language.

The following classes, objects, methods, functions, form the core part of the language.

Note that a few of these modules and methods related to them can be disabled at compile time with several options. This is the case for Regex, Random, Deque/Dictionary/Heap/LinkedList/SortedSet, and Grid.
Unless so disabled, they are always available in the CLI as well as when embedding Swan into your own C++ application.

## Bool: is Object
A bool can take only two values: true or false.

- Constructor: Bool(any), taking the truth value of any
- Implicit construction when using literals *true* and *false*
- Comparison operators: ==, !=
- Unary operators: !

Methdos: 

- hashCode: return an hash value used for indexing unsorted sequences such as Map and Set
- toString: return 'true' or 'false'

## Class: is Object
An object of type Class represents a class, e.g. Object, Num, Bool, etc.
The is operator overload of class provide for instance check, i.e. `4 is Num` returns true.

Methods:

- name: name of the class, e.g. Num, Bool, etc.
- toString: return a string representation of this object. For classes, this is equivalent to its name.

## Deque: is Iterable
A deque, abbreviation for double-ended queue, is a collection of items. Deque has the particularity to allow both constant time random access and insertion/removal at beginning and end of sequence. It is generally good in these cases. When elements have to be inserted or removed in the middle, linked list is better.
Deque is often said as being a good compromise between list and linked list, where eachever drawbacks are balanced.

- Constructor: Deque(sequences...): create a list from one or more concatenated sequences
- Operators: `[], []=, +, in`
- Comparison operators: ==, !=

Like with list, items in a deque are indexed by their position, 0 being the first, 1 the second, etc. 
Negative indices count from the end, thus -1 indicates the last item, -2 the one before the last, etc.
Indexing with a range, i.e. `deque[2...5]` allow to return or overwrite a subsequence. The size of the deque is automatically adjusted when overwriting subsequences of different sizes.

Methods:

- static Deque.of(items...): create a deque from the given items
- add(...items): add one or more items at the end of the deque
- clear: clear the whole deque
- fill(start, end, value): fills the range between start and end by the value given
- fill(range, value): fills the range specified by the value given
- fill(value): fill the entire deque with the specified value
- insert(index, ...items): insert one or more items starting at the given position
- indexOf(needle, start=0): search for needle in the deque, return its position if found, -1 if not
- iterator: return an iterator to iterate through the elements of this deque in order
- lastIndexOf(needle, start=length): search for needle in the deque from the end, return its position if found, -1 if not
- length: return the number of element in the deque
- lower(needle, comparator=::<): return the index of the greatest element less or equal than needle by doing a binary search. This suppose that the elements are sorted aoccording to the comparator given. 
- pop: remove an item from the end of the deque and return it
- push(...items): add one or more items at the end of the deque
- remove(...items): remove one or more items
- removeAt(...indices): remove one or more items at specified indices. Indices can be numbers or ranges.
- removeIf(predicate): remove all items from the deque for which the predicate returned true
- resize(newLength, value=undefined): resize the deque to make it having the new length specified. If it is longer, fill the new elements with the given value.
- shift: remove and return the first item of the deque
- toString: return a string like "[1, 2, 3, 4, 5]"
- unshift(items...): insert one or more items at the beginning of the deque
- upper(needle, comparator=::<): return the index of the greatest element strictly less than needle by doing a binary search. This suppose that the elements are sorted according to the comparator given.

## Dictionary: is Mapping
A Dictionary is an associative container where keys are sorted.

Constructor: Dictionary([sorter=::<], mappings...), create a new dictionary from optional source mappings and optional sorter; sorter is any kind of callable taking two arguments and returning true if the first argument goes before the second.
- Operators: [], []=, +, in

Items in a dictionary are indexed by their key, i.e. `dictionary["somekey"]` returns or overwrites the value associated with "somekey".
If several values have been associated with the same key, the value returned when indexing is unspecified.

Methods:

- Dictionary.of([sorter], entries...): create a dictionary with an optional sorter and initial entries
- clear: remove all entries from the dictionary
- iterator: return an iterator to iterate through (key, value) pairs of the dictionary. The pairs are ordered by their key as defined by the sorter given at construction.
- length: return the number of entries present in the dictionary
- lower(key): returns the nearest key present in the dictionary that come after the key given, or return the argument given itself if it is present
- put(key, value): inserts the given key/value pair in the dictionary regardless of if the key already exists; this breaks the uniqueness rule of the keys, it can be useful sometimes but do it with caution
- remove(...keys): remove one or more keys from the dictionary
- toString: return a string like "{a: 1, b: 2, c: 3}"
- upper(key): returns the nearest key present in the dictionary that come after the key given

## Function: is Object
Class representing all function objects.

- Constructor: Function(code): returns a function, evaluating the string given as an expression. 
- Constructor: Function(n), with n<0: return the currently running function a the given stack level, i.e. Function(-1) returns the current function, Function(-2) the callee, Function(-3) the callee of the callee, etc.
- Operators: ()

Methods:

- bind(...args): create a function, which, when called, will call this function with one or more bound arguments. Example: if `g=f.bind(1,2)`, calling `g(3,4)` will be equivalent to calling `f(1,2,3,4)`. Functional programming afficionados may call this currifying.

## Fiber: is Iterable
A Fiber represent a paralel execution fiber, as known as coroutine.
You can iterate a fiber to fetch all yielded values in turn.

- Constructor: Fiber(func): construct a fiber running the given function. The function will actually be called when calling fiber.next for the first time.
- Constructor: Fiber(): without argument, this returns the current executing fiber

Operator () is the same as next method: it calls the fiber. 

- Arguments passed to operator()/next are passed as function arguments when called for the first time. 
- For subsequent calls, the argument passed to next is the value returned from the yield expression. 
- The value returned from next is the yielded or returned value from the fiber.


## Grid: is Iterable
A grid is a bidimensional structure. It is indexed with two indices x (representing columns) and y (representing rows).
Grid can be used as 2D map, or as matrix in the mathmatical sense. A few useful methods covering these two topics are provided, pathfinding as well as matrix multiplication ammong others.

- Constructor: Grid(width, height): construct a grid with the given size, initially filled with 0s.
- Implicit construction when using the construction syntax `[ ..., ...; ... ]`
- Operators: `[], []=, +, -, *, /, **`
- Unary operators: -
- Comparison oprators: ==, !=

Grids are indexed with their X and Y coordinates. i.e. `grid[0, 0]` denotes the first top-left cell.
Negative indices count from the end, i.e. `grid[-1, -1]` denotes the last bottom-right cell.
Indexing with ranges allow to  return or overwrite subgrids, i.e. `grid[1..-1, 1..-1]` returns a subgrid excluding the first and last row and the first and last column. 

Methods:

- draw(startX, startY, endX, endY, value): draw a line between the points (startX, startY) and (endX, endY), setting the touched cells with the value given
- fill(startX, startY, endX, endY, value): fills a rectangular area with a given value
- fill(xRange, yRange, value): fills a rectangular area with a given value
- floodFill(x, y, value): fills the grid with the specified value, using the flood fill algorithm starting at the given point
- hasDirectPath(startX, startY, endX, endY, traversaleTest): test if there exist a direct path between (startX, startY) and (endX, endY). Return a 3-tuple (result, impactX, impactY) telling if there is a direct path, and if not, where is the impact point. See below for a description of the traversalTest callback.
- iterator: return an iterator to iterate through the value in the grid. Values are ordered left to right, top to bottom.
- pathfind(startX, startY, endX, endY, traversalTest): try to find a path between (startX, startY) and (endX, endY) using A\* pathfinding algorithm. If a path is found, return a list of 2-tuple indicating the path to follow. Null is returned if no path is found.

**Pathfinding traversal test callback**  
The traversal test callback takes 6 arguments and must return a value indicating whether or not a given way is traversable (allow traveling through it).
Arguments: value, grid, newX, newY, previousX, previousY, where (newX, newY) is the point where the algorithm wants to go to, (previousX, previousY) is the point which it comes from, grid is a reference to the whole grid, and value is the same as `grid[newX,newY]`. For most usages, considering only the first argument alone can be sufficient.
Return value: false or any value <1 to denote an unpassable wall, true or any value >=1 to denote an open passage with its cost. Returning a value greater than 1 indicates a passage that can be difficult or dangerous, which would be better avoided if possible.

## Heap: is Iterable
Storing items in heap order allows to quickly fetch the element with the highest priority (the greatest one according to the sorter).
It is generally faster than a sorted set, if you only need to access the greatest element, and don't care about the order of other items after the first one.
Its downsides are that, removing elements other than the first greatest one may be slow, and elements aren't returned in their natural order when iterating.

- Constructor: Heap([sorter=::>], ...sources): construct an heap from an optional sorter and initial sources
- Operators: none

Methods:

- static Heap.of([sorter=::>], ...items): create an heap from a serie of items and an optional sorter
- clear: remove all elements from the heap
- first: return the first item  of the heap (the greatest one according to the sort order defined by the sorter)
- iterator: return an iterator to iterate through the elements of the queue. Elements are yielded in an unspecified order, *not* in the order given by the sorter.
- length: return the number of items present in the queue
- pop: remove and return the first item (the greatest one according to the sort order defined by the sorter)
- push(...items): insert one or more items in the heap
- remove(...items): remove one or more items from the heap. Be careful, this operation may be slow (linear worst case)

## Iterable: is Object
The Iterable class is the base class for all subclasses holding a sequence of something that can be iterated through, such as String, List, Tuple, Map, etc.

Methods:

- all(predicate): return true if predicate(x) returned true for all x in the sequence. Return true for empty sequence.
- any(predicate): return true if predicate(x) returned true for at least one x in the sequence. Return false for empty sequence.
- bunches(n): return a sequence which returns the items of this iterable in bunches of n-tuples. For example, `[1..10].bunches(3)` would give `(1, 2, 3)`, then `(4, 5, 6)` and finally `(7, 8, 9)`. The last returned element is filled with undefined if necessary to always return a n-tuple.
- count(needle): return the number of elements in the sequence that are equal to needle.
- countIf(predicate): return the number of x in the sequence for which predicate(x) returned true
- dropWhile(predicate): return a sequence with all x in this Iterable, until predicate(x) returns true
- enumerate(n=0): return a sequence where each elements in the sequence are transformed into tuples `(n, this[0]), (n+1, this[1]), (n+2, this[2]), ..., (n+l, this[l])`
- filter(predicate): return a sequence containing only x for which predicate(x) returned true
- find(predicate)): return the first x for which predicate(x) returns true, or null if none returned true.
- first: return the first element of the sequence. Note that the sequence may don't have a predictable orders, e.g. Set
- flatten: return and iterator flattening nested structures. For example `[1, [2, 3], 4, [5, [6, [7, 8], 9]]` would become `[1, 2, 3, 4, 5, 6, 7, 8, 9]`.
- iterator: return an iterator to iterate through the elements of this iterable sequence
- join(separator=""): return a string by concatenating all elements from the sequence in turn, separating them with the given separator.
- last: return the last element of the sequence. Note that the sequence may don't have a predictable orders, e.g. Set
- limit(n): return a sequence limited to n elements, i.e. all elements after the nth are dropped
- map(mapper): return a new sequence with elements mapped from this Iterable
- max(comparator=max): return the greatest element of the sequence according to the comparator given. If comparator is omited, the global function max is taken.
- min(comparator=min): return the least element of the sequence according to the comparator given. If comparator is omited, the global function min is taken.
- none(predicate): return true if predicate(x) returned true for none of the x in the sequence. Return true for empty sequence.
- reduce(reducer, initial=null): iteratively reduce elements from this Iterable using the reducer given. Return null for empty sequence, initial if the sequence has a single element. For example, `[1, 2, 3, 4, 5].reduce(::+)` would yield 15, suming 1+2+3+4+5.
- skip(n): return a sequence with n first elements skipped
- skipWhile(predicate): return a sequence with all x in this Iterable, skipping initial elements as long as predicate(x) returns true

## LinkedList: is Iterable
A linked list is a collection of items connected together via linked nodes. IN principle, all items are of the same type, though this isn't enforeced.
When items are frequently added or removed at the beginning or at the end but never in the middle of the list, its performances are better than List. LinkedList fits well when used as a queue or stack.

Note: to prevent from possible slow operations, LinkedList doesn't provide `[]` and `[]=` indexing/subscripting  operators.

- Constructor: LinkedList(sequences...): create a linked list from the concatenation of one or more sequences
- Operators: +, in

Methods:

- static LinkedList.of(items...): construct a linked list from a serie of given items
- iterator: return an iterator to iterate through the elements of this linked list in order
- push(...items): push one or more items at the end of the list
- pop: pop an item from the end of the list and return it
- remove(...items): remove one or more items from the list
- removeIf(predicate): remove all items from the list for which the predicate returned true
- shift: pop an item from the beginning of the list and return it
- toString: return a string like "[1, 2, 3]"
- unshift(...items): push one or more items at the begining of the list

## List: is Iterable
A list is a collection of items, in principe all of the same type (even if it isn't enforced). They are generally good, except when items are frequently added or removed in the beginning or in the middle of the list, in which case LinkedList is better.

- Constructor: List(sequences...): create a list from one or more concatenated sequences
- Implicit construction when using [...] notation
- Operators: `[], []=, +, *, in`
- Comparison operators: ==, !=

Items in a list are indexed by their position, 0 being the first, 1 the second, etc. 
Negative indices count from the end, thus -1 indicates the last item, -2 the one before the last, etc.
Indexing with a range, i.e. `list[2...5]` allow to return or overwrite a sublist. The size of the list is automatically adjusted when overwriting sublists of different sizes.

Methods:

- static List.of(items...): create a list from the given items
- add(...items): add one or more items at the end of the list
- clear: clear the whole list
- draw([random=rand], count=1): randomly draw the specified number of elements from the list. Drawn elements aren't removed from the source list. AVailability depends on the Random class being available.
- draw([random=rand], weights): randomly draw an element from the list, selecting the element with probabilities weights (See Random for more info). Drawn element isn't removed from the list. AVailability depends on the Random class being available.
- fill(start, end, value): fills the range between start and end by the value given
- fill(range, value): fills the range specified by the value given
- fill(value): fill the entire list with the specified value
- insert(index, ...items): insert one or more items starting at the given position
- indexOf(needle, start=0): search for needle in the list, return its position if found, -1 if not
- iterator: return an iterator to iterate through the elements of this list in order
- lastIndexOf(needle, start=length): search for needle in the list from the end, return its position if found, -1 if not
- length: return the number of element in the list
- lower(needle, comparator=::<): return the index of the greatest element less or equal than needle by doing a binary search. This suppose that the elements are sorted aoccording to the comparator given. 
- pop: remove an item from the end of the list and return it
- push(...items): add one or more items at the end of the list
- remove(...items): remove one or more items
- removeAt(...indices): remove one or more items at specified indices. Indices can be numbers or ranges.
- removeIf(predicate): remove all items from the list for which the predicate returned true
- resize(newLength, value=undefined): resize the list to make it having the new length specified. If it is longer, fill the new elements with the given value.
- reserve(capacity): prepare the collection to contain at least the given number of elements by allocating memory in advance.
- reverse: reverse the elements in the list, so that the first becomes the last one and vice-versa.
- rotate(distance): shift the items in the list; depending on distance, first elements become the last ones or last become the first ones.
- shuffle(random=rand): randomly shuffles the elements in the list. AVailability depends on the Random class being available.
- slice(start, end): return a sublist containing elements from start inclusive to end exclusive. Equivalent to `list[start..end]`.
- sort(comparator=::<): sort the elements in the list
- splice(start, end, ...newItems): erase the elements from start inclusive to end exclusive, and then insert newItems at their place. Equivalent to `list[start..end] = newItems`.
- toString: return a string like "[1, 2, 3, 4, 5]"
- upper(needle, comparator=::<): return the index of the greatest element strictly less than needle by doing a binary search. This suppose that the elements are sorted according to the comparator given.

## Map: is Mapping
A Map is an associative container where key/value pairs are held with no particular order. If keys need to be ordered, Dictionary must be used.
IN order to be held in a Map, keys must all be hashable, i.e. implement the hashCode method. This is the case for Number, String, Bool and Tuple.

- Constructor: Map(sequences...): construct a Map from one or more source mappings
- Implicit construction when using {...} notation
- Operators: [], []=, +, in

Items in a map are indexed by their key, i.e. `map["somekey"]` returns or overwrites the value associated with "somekey".

Methods: 

- static Map.of(entries...): construct a map from a serie of entries. Each entry must be an iterable with two elements, for example a 2-tuple.
- clear: remove all items from the map
- iterator: return an iterator to iterate through (key, value) pairs of the map. The entries are iterated in an unspecified order.
- length: return the number of key/value pairs present in the map
- remove(...keys): remove one or more keys from the map
- reserve(capacity): prepare the collection to contain at least the given number of elements by allocating memory in advance.
- toString: return a string like "{a: 1, b: 2, c: 3, d: 4}"

## Mapping: is Iterable
The Mapping class represent the parent of all mappable types, i.e. structures that hold a mapping of key/value pairs. Map and Dictionary are Mappable.
Operators `|, &, -, ^` are defined and work similarely as manipulating a set of keys.

- flipped: return a map where keys become values and values become keys.
- get(key, func): return this[key] if it is present in the map; otherwise, compute func(key) and store it in the map before returning it.
- keys: return an iterable sequence enumerating all existing keys
- set(key, value, merger): assign `this[key]=value` if it isn't yet present in the map; otherwise, assign `this[key]=merger(oldValue, value)`; in any case, return the value previously associated to the key.
- values: return an iterable sequence enumerating all values

## Null: is Object
The Null class has only one instance, null itself.

No specific method

## Num: is Object
Class of all numbers.

- Constructor: Num(string, base=10): where base can be between 2 and 36 inclusive
- Implicit construction when using number literals such as 123, 3.14, -49, 0xFF, 0b111
- Operators: `+, -, *, /, %, **, \, &, |, ^, <<, >>`
- Comparison operators: >`<, <=, ==, >=, >, !=, <=>`
- Unary operators: ~, +, -

Methods:

- format(precision=2, decimalSeparator=".", groupSeparator="", padding=0, groupLength=3): format the number into a string with the specified parameters: precision is the number of digits after the decimal separator. Example: `12345.6789.format(2, ",", "'")` results in `12'345.68`. Giving a precision <0 requests for exponential notation. IF padding!=0, the appropriate number of 0s are prepended to make a string of the given length.
- frac: return the fractional part of the number, e.g. 1.23 and -67.89 resp. return  0.23 and -0.89
- hashCode: return an hash value used for indexing unsorted sequences such as Map and Set
- int: return the integer part of the number, e.g. 1.23 and -67.89 resp. return 1 and -67.
- sign: return the sign of the number, 1 for positive, -1 for negative or 0 for 0 (a.k.a signum)
- toString(base=10): return a string representation of the number in the given numeral base, where base can be between 2 and 36 inclusive. If base is given, only the integral part is taken.

## Object
Object is the base class for all objects.

- Comparison operators: `==, !=, is`
- Unary operators: !, ?

Methods: 

- class: return the type of the object as Class object
- toString: return a string representation of this object. If there is no more specific overload, the default toString of all objects return the type and the memory location where the object is, e.g. "Object@0xFFFD000012345678"

## Range: is Iterable
A range, as its name says, denotes a range of numbers.
Each range has a *start*, *end* and *step*. Ranges can be viewed as collections that efficiently contain any value `start + step*N` for any integral N N as long as the result is between *start* and *end*.

- Constructor: Range(start, end, step=1, endInclusive=false)
- Constructor: Range(end, step=1, endInclusive=false), where start=0
- Implicit construction when using N..M or N...M  notations, where N..M is equivalent to Range(N, M, 1, false) and N...M to Range(N, M, 1, true).
- Operators: [], in

Indexing a range works similarly as if it were a list. Thus `range[0]` returns start, `range[n]` returns `start+n*step` if `<end`.

Methods:

- iterator: return an iterator to go through the values of the range in order

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
The random generator used is the standard C++11 MT19937. It's a good and quite fast random number generator, but too weak for cryptography.

- rand(), without parameters: generate a number between 0 and 1
- rand(n), for n<=1: generate a number between 0 and 1 and return true if the generated number is <n.
- rand(n), for n>1: generates an integer between 0 and n exclusive.
- rand(min, max): generates a number between min and max inclusive
- rand(sequence), for any sequence of numbers: generate an integer between 0 and sequence.length -1 with weighted probabilities. For example, `rand([1, 2, 3])` will generate 0 with a probability of 1/6, 1 with probability 2/6 and 2 with probability 3/6. To draw an element randomly from a sequence, see List.draw.

Other methods:

- Constructor: Random([seed]): construct a pseudo-random number generator with a given seed number, or use any system-dependent method of obtaining seed if seed is omited
- normal(mu=0, sigma=1): generates a number according to normal/gaussian distribution with mean mu and deviation sigma. The generated number has ~65% chance to be between mu-sigma and mu+sigma, ~90% between mu -2sigma and mu +2sigma, and ~96% between mu -3sigma and mu +3sigma. There is no bounds, so a number as big as mu + 100sigma may be generated, though with an extremely low probability.
- reset(seed): reset this random number generator to the seed provided

## Set: is Iterable
A Set is a collection of items, in principle all of the same type (although nothing is enforced), where order has no importance and where any item may only be present once.
Another characteristic of sets beside the uniqueness of held objects is their ability to make set opations: union, intersection, difference and symetric difference

- Constructor: Set(...sequences): construct a set from one or more concatenated sequences
- Implicitly constructed when using `<...>` notation
- Operators: `-, &, |, ^, in`
- Comparison operators: ==, !=

Methods: 

- static Set.of(items...): construct a set from individual elements
- add(...items): add one or more items
- clear: empty the whole set
- iterator: return an iterator to go through the elements of this set. The iteration is in an unspecified order.
- length: return the number of elements present in the set
- remove(...items): remove one or more items from the set
- reserve(capacity): prepare the collection to contain at least the given number of elements by allocating memory in advance.
- toString: return a string like "<1, 2, 3, 4, 5>"

## SortedSet: is Iterable
A sorted Set is a collection of items, in principle all of the same type (although nothing is enforced), where items are always kept in order.
Normally, as with sets, items in a sorted set may only be present once, but the rule may be circumvented (do it with caution).
Another characteristic of sets beside the uniqueness of held objects is their ability to make set opations: union, intersection, difference and symetric difference

- Constructor: SortedSet([sorter=::<], ...items): construct a sorted set from one or more concatenated sequences, sorting the items by the sorter given
- Operators: `-, &, |, ^, in`
- Comparison operators: ==, !=

Methods: 

- static SortedSet.of([sorter=::<], items...): construct a set from individual elements and sort them using the sorter given
- add(...items, allowDuplicate=false): add one or more items. If allowDuplicate=true, then items are addded to the set regardless of they were already present; this breaks the uniqueness rule, it can be useful sometimes but do it with caution.
- clear: empty the whole set
- first: return the first (smallest) element of the set
- iterator: return an iterator to iterate through the elements of this sorted set. Elements are iterated in their order as defined by the sorter specified at construction.
- lower(key): returns the nearest element present in the set that come after the key given, or return the argument given itself if it is present
- last: return the last (largest) element of the set
- length: return the number of elements present in the set
- pop: remove and return the last (largest) element of the set
- remove(...items): remove one or more items from the set
- shift: remove and return the first (smallest) element of the set
- toString: return a string like "<1, 2, 3, 4, 5>"
- upper(key): returns the nearest element present in the set that come after the key given

## String: is Iterable
A String holds an immutable sequence of UTF-8 characters.

- Constructor: String(bufffer, encoding), will convert the data in the given buffer into a string, decoding characters of the specified encoding.
- Constructor: String(any), any will be converted to string using toString method.
- Operators: [], +, in
- Comparison operators: >`<, <=, ==, >=, >, !=` via `<=>`

Methods: 

- static String.of(...sequences): construct a string by concatenating one or more other strings or objects
- codePointAt(index): return the code point at given character position 0..0x1FFFFF
- endsWith(needle): return true if needle is found at the end of the string
- fill(pattern, length, side=1): return a new string where this string is filled/padded up to the specified length using the given pattern; side tells where to place the padding pattern: 1=left, 2=right, 3=both/center.
- findAll(regex, group=0): find all matches of the regular expression against this string. For each match, take the group number given as result, or return a list of RegexMatchResult objects if group=true.
- findFirstOf(needles, start=0): search for the first occurence of one of the characters inside needle; return -1 if nothing is found.
- format(...items): take this string as a format string and format accordingly; see the format global function for more info.
- hashCode: return an hash value used for indexing unsorted sequences such as Map and Set
- indexOf(needle, start=0): search for needle in the string, returning the position where it has been found, or -1 if not found.
- iterator: return an iterator to go through each character of the string
- lastIndexOf(needle, start=length): search for needle in the string from its end, returning the position where it has been found, or -1 if not found.
- length: return the length of the string, its number of characters / code points.
- lower: transform the string to lowercase
- replace(needle, replacement)): search for occurences of needle and replace them with replacement. Needle can be a String or a Regex. If needle is a Regex, replacement can be a String or a callback function, which will be called with a RegexMatchResult object for each match found, the return value of that callback is taken as final replacement in the string for the match.
- search(regex, start=0, returnFullMatchResult=false): search for a match of the Regex against the string. Return a position or -1 if returnFullMatchResult=false, a RegexMatchResult object or null if returnFullMatchResult=true.
- split(separator): split the string into a sequence of elements using a given separator. The separator can be a string or a regex.
- startsWith(needle): return true if needle is found at the start of the string
- toNumber(base=10): convert the string to a number, written in the given numeral base; base can be between 2 and 36.
- toString: return itself
- upper: transform the string to uppercase

## Tuple: is Iterable
A tuple is a sequence of items, generally of etherogeneous types, as opposed to lists where all elements are supposed to be of the same type.
The order of the elements in a tuple are also often significant, i.e. `(1, 2)` means something else than `(2, 1)`, while it is often unsignificant for lists.
The other big difference with lists is that tuples are immutable and override the hashCode method. They can thus be used as keys in unsorted sequences like Map and Set.

- Constructor: Tuple(...sequences): construct a tuple out of one or more source sequences
- Implicitly constructed when using (...,) notation
- Operators: `[], +, *, in`
- Comparison operators: `<, <=, ==, >=, >, !=` via `<=>`

Items in a tuple are indexed by their position, 0 being the first, 1 the second, etc. 
Negative indices count from the end, thus -1 indicates the last item, -2 the one before the last, etc.
Indexing with a range, i.e. `tuple[2...5]` allow to return a subtuple.

Methods:
- static of(...items): construct a tuple from one or more individual items
- hashCode: return an hash value used for indexing unsorted sequences such as Map and Set
- iterator: return an iterator going through the elements of this tuple in order
- length: return the number of elements in this tuple
- slice(start, end): return a subtuple containing elements from start inclusive to end exclusive. Equivalent to `tuple[start..end]`.
- toString: return a string like "(1, 2, 3, 4, 5)"

## Global functions
- format(fmt, ...items): create a formatted string from a format string and parameters. See further below for more info.
- format(fmt, map): create a string where expression $([a-z]+) are replaced by the corresponding value in the map.
- gcd(...values), lcm(...values): compute the GCD (greatest common divisor) or LCM (least common multiple) of the values given. Return 1 if called without any argument.
- max(...items), min(...items): return the least or greatest of the given items
- reversed(iterable): return a list with the elements of iterable appearing in reverse order
- sorted(iterable, comparator=::<): return a list with elements of iterable sorted according to the comparator given
- zip(sequences..., grouping=Tuple): take two or more sequences and map them together using the grouping function provided. For example, `zip([1,2,3], [4,5,6])` would yield a sequence `[(1,4), (2,5), (3,6)]`. Giving another grouping function allows to produce something else than just tuples; for example `zip([1,2,3], [4,5,6], ::+)` would yield `[5, 7, 9]` (1+4, 2+5, 3+6). Stops with the shortest of the sequences given.

## Math functions
Math functions can be indifferently used as methods of the Number class, i.e. `10.abs` like in ruby, or as traditional global functions, i.e. `abs(10)`.
By default, both are allowed, but depending on the configuration, only one of the two alternatives may be  given.

- abs(n): absolute value
- acos(n), asin(n), atan(n), cos(n), sin(n), tan(n): trigonometric functions
- acosh(n), asinh(n), atanh(n), cosh(n), sinh(n), tanh(n): hyperbolic trigonometric functions
- cbrt(n), sqrt(n): cubic and square root
- ceil(n), floor(n), round(n), trunc(n): rounding functions
- exp(n), log(n, [base]): exponential and logarithm

## Format(fmt, ...) function
The format string of the format(fmt, ...) function allow different syntaxes:

- %1, %2, %3, ... or $1, $2, $3, ... or {1}, {2}, {3}, etc. take the nth argument, the first argument is number 1, not 0.
- %key or $key or {key} looks up *key* in a map passed in last parameter of format
- {n:fmt} or {key:fmt} formats the value according to the *fmt* format specified.

The format *fmt* in {n:fmt} or {key:fmt} should be composed of zero, one or more format specifiers followed by a type indicator. The type indicator is the final character juste before the ending '}'.
Only the {...} notation allow formatting options, % and $ don't.
This fmt is obvious inspired from C printf family. 

The type indicator may be:

- d, i, u, l: a decimal integer
- x, x: an hexadecimal integer
- o: an octal integer
- g: a number with fixed precision
- f: a number with a fixed number of decimals
- e, E: a number in exponential notation
- s: a string

Depending on the type indicator, one or more of these format specifiers can be also specified. They can appear in any order as long as they are before the final type indicator:

- '.n', where n is an integer: precision
- '$n', where n is an integer: width
- '0': pad with zeroes instead of spaces
- '+': always show the sign
- '#': use alternate notation; '0x' prefix for hexadecimal integers, or always show decimal part for numbers

Given a number 123.45, here are some examples:

- '.3g' => 123
- '.4g' => 123.5
- '.1f' => 123.5
- '.2f' => 123.45
- '.3f' => 123.450
- '.4f' => 123.4500
- 'e' => 1.2345e+02
- 'd' => 123
- 'x' => 7b
- 'X' => 7B
- '0$5X' => 0007B
- '0$5#x' => 0x7b
- '+d' => +123

## Reflection/metaclass functions
This family of global functions is used to dynamically create classes, or access fields and global variables with their names. They may be disabled by configuration.

- createClass(name, [parents...], fieldCount=0, staticFieldCount=0): create a new class with the given name, parents, number of fields and static fields. If no parents are given, automatically inherit at least from Object. 
- loadField(object, index): access the nth field of the given object
- storeField(object, index, newValue): set the nth field of the object to a new value
- loadGlobal(name): access the global variable with the given name
- storeGlobal(name, value): set a global variable to a new value
- loadMethod(object, name): access the named method of the object. Equivalent to `object::name` with a dynamic name.
storeMethod(class, name, method): Store the given method to the class under its name. Equivalent to `class::name = method` where name would be dynamic.
storeStaticMethod(class, name, method): Store a static method of the class under its name. Equivalent to `class.type::name = method` where name would be dynamic.

## Collection operations complexity cheatsheet

Collection | In | Top | Get | Back | Unshift | Insert | Push | Shift | Remove | Pop | Ordered | Sorted | Multi
-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----
List | O(n) | O(1) | O(1) | O(1) | O(n) | O(n) | O(1) | O(n) | O(n) | O(1) | Yes | No | Yes
Tuple | O(n) | O(1) | O(1) | O(1) | n/a | n/a | n/a | n/a | n/a | n/a | Yes | No | Yes
LinkedList | O(n) | O(1) | O(n/2) | O(1) | O(1) | O(1) | O(1) | O(1) | O(1) | O(1) | Yes | No | Yes
Deque | O(n) | O(1) | O(1) | O(1) | O(1) | O(n/2) | O(1) | O(1) | O(n/2) | O(1) | Yes | No | Yes
Set | O(1) | n/a | n/a | n/a | O(1) | O(1) | O(1) | O(1) | O(1) | O(1) |  No | No | No
SortedSet | O(log(n)) | O(log(n)) | O(log(n)) | O(log(n)) | O(log(n)) | O(log(n)) | O(log(n)) | O(log(n)) | O(log(n)) | O(log(n)) | Yes | Yes | No
Heap | O(n) | O(1) | O(n) | O(n) | O(log(n)) | O(log(n)) | O(log(n)) | O(n log(n)) | O(n log(n)) | O(n log(n)) | No | Yes | Yes

- In: containment check, i.e. check if an element is present in the sequence
- Top: retriev the first element of the sequence
- Get. retriev a random element from the sequence
- Back: retriev the last element of the sequence
- Unshift: insert a new element at the beginning of the sequence
- Insert: insert an element at a random position in the sequence
- Push: insert an element at the end of the sequence
- Shift: remove the first element of the sequence
- Remove: remove a random element in the middle of the sequence
- Pop: remove an element at the end of the sequence
- Ordered: whether or not the elements are returned in some defined order when iterating through the sequence
- Sorted: whether or not the elements are sorted in the sequence
- Multi: whether or not it is possible to insert several times the same element in the sequence

## Mapping operations complexity cheatsheet

Collection | Lookup | Put | Remove | Ordered | Sorted | Multi
-----|-----|-----|-----|-----|-----|------
Map | O(1) | O(1) | O(1) | No | No | No
Dictionary | O(log(n)) |O(log(n)) |O(log(n)) | Yes | yes | Yes

- Lookup: retrieving a mapping given its key
- Put: insert a new mapping
- Remove: remove a mapping given a key
- Ordered: whether or not the elements are returned in some defined order when iterating through the sequence
- Sorted: whether or not the elements are sorted in the sequence
- Multi: whether or not it is possible to insert several times a mapping with the same key in the sequence
