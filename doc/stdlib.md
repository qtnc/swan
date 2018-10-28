# Standard library
Here's a very quick reference of the standard library included with the language.

## Bool: is Object
- hashCode
- toString

## Class: is Object
An object of type Class represents a class, e.g. Object, Num, Bool, etc.

- is: the is operator overload of class provide for instance check, i.e. `4 is Num` returns true.
- name: name of the class, e.g. Num, Bool, etc.
- toString: this overload returns the same as name

## Function: is Object
Class representing all function objects.

## Fiber: is Sequence
tbc

## List: is Sequence
- add(items...)
- clear
- insert(index, items...)
- indexOf(needle, start=0)
- lastIndexOf(needle, start=length)
- length
- pop
- push(items...)
- remove(items...)
- removeAt(indices...)
- removeIf(predicate)
- reverse
- rotate(distance)
- sort(comparator=::<)
- toString

## Map: is Sequence
- clear
- length
- remove(items...)
- toString

## Null: is Object
The Null class has only one instance, null itself.

- toString: returns the string `null`.

## Num: is Object
Class of all numbers.

- hashCode: return the hash code of the object, especially used as hashing key for maps, sets and similar structures.
- toString(base=10)

TBC

## Object
Object is the base class for all objects.

- is: the is operator of Object is the same as the operator ==
- toString: return a string representation of the object. For objects, this means a string in the form `<type>@0x<address>`, e.g. `Object@0xFFFD0000000012345678`.
- type: return the class object  indicating the type of the object
- == and !=: base comparison operators; an object is only equal to another if it exactly points to the same object in memory
- !: not operator; for an object, always returns false, since an object is never considered falsy

## Range: is Sequence
tbc

## Regex: is Object
- test(str)

## Sequence: is Object
The sequence class is the base class for all subclasses hloding a sequence of something, such as String, List, Tuple, Map, etc.
For more info about iterator, iterate and iteratorValue methods, see iteration protocol.

- iterator: return an object that can be iterated via iterate and iteratorValue methods. The default for sequences is to return itself.
- iterate(key): given the current key, return the key of the next item. Passing key=null yield the first key; returns null if the key passed was the last one.
- iteratorValue(key): return the value corresponding to the iteration key passed

- join(separator="")

TBC

## Set: is Sequence
- add(items...)
- clear
- length
- toString

## String: is Sequence
- byteLength
- codePointAt(index)
- endsWith(needle)
- findAll(regex, returnedGroup=0, returnFullMatchResult=false)
- findFirstOf(needles, start=0)
- hashCode: return the hash code of the object, especially used as hashing key for maps, sets and similar structures.
- indexOf(needle, start=0)
- lastIndexOf(needle, start=length)
- length
- lower
- replace(regex, replacement)
- search(regex, start=0, returnFullMatchResult=false)
- split(regex)
- startsWith(needle)
- toNum(base=10)
- toString
- upper

## Tuple: is Sequence
- hashCode
- length
- toString