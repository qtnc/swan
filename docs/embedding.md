# Enbedding

## Initialisation
First of all, you need to create the VM, as well as the fiber where Swan code will be executed.

```cpp
Swan::VM* vm = Swan::VM::create();
Swan::Fiber& fiber = vm->getActiveFiber();
```

Fibers can only be manipulated through references, as their construction and destruction are managed by the VM.
When you have finished using the VM, call `release()`; don't use delete.

## Fiber and stack
A *fiber* is a thread of execution.
It holds a stack of values, which are used when running Swan code: functions or methods to call, objects, parameters and returns. The stack  grows and shrinks as code is executed.
If you have already embedded lua or another similar scripting language, the following should look pretty familiar. 

Values on the stack are accessed by their index. Positive indices from 0 upwards indicate the nth element from the current base, while negative indices count from the top downwards.
Thus, index 0 indicates the stack base, while -1 indicates the last element at the top. 
The element at the stack base is a little special: slot 0 is used for receiver object of methods (the object usually known as *this* or *self*), as well as for return values.

Calling C++ from Swan or Swan from C++ involves modifying the stack. Fibers have three important families of methods to manipulate the stack:

- getXXX, such as getBool, getNum, getString, etc. fetches the value present at the given stack index
- setXXX, such as setBool, setNum, setString, etc. sets the given stack index to the specified value. The value previously present at that place is erased.
- pushXXX, such as pushBool, pushNum, pushString, etc. pushes the given value at the top of the stack, making it grow by one element.

To complete the general list, *pop* removes the top value of the stack, making it shrink by one unit.

## Calling Swan from C++
### Loading code
There are two basic methods to load Swan code:

- *`loadString(const std::string& source, const std::string& name)`*: this loads the source code contained in the source string. The name is used for debugging, in case a compilation or runtime error happens.
- *`loadFile(const std::string& filename)`*: loads the source from the specified filename.

Both methods push the fonction(s) that have just been loaded on top of the stack without executing, as if *pushNativeFunction* had been called. In case of errors, an exception is thrown.
The number of functions pushed is returned by both methods. Several functions may be pushed in case you are loading compiled bytecode.

Note that you can override the way files are loaded by setting a custom file loader (see *setFileLoader* and *setPathResolver* methods).
BY default, it searches for regular files in the current directory, using `/` or `\` as directory separators.

### Importing code
For more advanced uses, *`import(originatingName, requestedName)`* is recommanded rather than `loadFile` or `loadString`.
Calling `import` in C++ has the same effect as the Swan import instruction.

`import()` has the advantage to both load and run the requested file, as well as correctly handling the case of bytecode pushing several functions on the stack at once.
In fact, in that case, all functions have to be executed in turn from the stack top downwards and intermediate functions returns must be registered in the import list (see the `storeImport` method).
The simplest `loadFile()` and `loadString()` don't handle that.

### Calling a function
To call a function, follow these steps:

- Push the function to be called on the stack
- Push all the arguments, using the methods pushNum/pushString/pushBool/etc.
- call the method *`call(int nArgs)`* on the Swan::Fiber, specifying the number of arguments to pass to the function.

When the fonction is done executing, its return value is at the top of the stack (at index -1). Don't forget to pop this return value when you are done using it.

Functions loaded by *loadSource* or *loadFile* don't take any argument. Thus, running a given file is done as follows:

```cpp
fiber.loadFile("some-script.swan");
fiber.call(0);
fiber.pop();
```

### Calling a method
To call a method, follow these steps:

- Push the receiver this object on the stack
- Push all method arguments, using pushNum/pushString/pushBool/etc.
- Call the method *`callMethod(const std::string& name, int nArgs)`* on the Swan::Fiber, specifying the name of the method to be called and the number of arguments passed. The receiver this object counts for one argument, so nArgs must be greater than 0.

As for functions, the return value after the call is at the top of the stack (at index -1).

## Call C++ from Swan
Functions and objects that need to be called from Swan have first to be registered in the Swan VM.

Functions or methods called from Swan have the prototype *`void (Swan::Fiber&)`*. Functions of that particular prototype are next called *native functions* or *wrapper functions*.
Normally a wrapper does the following:

- Fetch the arguments passed in the stack, using getNum/getString/getBool/etc. For a method, the receiver this object is always at stack index 0. Use *getArgCount* to know how many arguments you have.
- Do the real work
- Return a value to Swan using setNum/setString/setBool/etc. The return value of a function is normally always in stack index 0.

Note that if the return step is omited, slot 0 stays unchanged, and thus the first argument or the *this* is returned by default.

To register a function, it goes as follows:

- Push your native function with the prototype `void (Swan::Fiber&)` on the stack by calling *pushNativeFunction*
- Call *storeGlobal* to set it as a global function, or *storeMethod*. For a method The class object must be just below the pushed function (at index -2).

You can alternatively use *registerFunction* and *registerMethod* which do both directly. *registerProperty* takes two functions is intented to register a getter/setter.

As an exemple, here's the registration of a *add* Swan function, taking two numbers and returning their sum:

```cpp
void add (Swan::Fiber& fiber) {
double first = fiber.getNum(0); // get first number 
double second = fiber.getNum(1); // get second number 
fiber.setNum(0, first+second); // Stor the sum in slot 0, which is the return value
}

fiber.pushNativeFunction(add);
fiber.storeGlobal("add");
```

## Simplified registration interface
IF you have a lot of C++ objects, methods and functions to register, writing wrapper functions quickly become thedious and error prone.
The binding helper SwanBinding.hpp give a way to register them far more easily thank to templates. For most common cases you don't need to manually write wrappers yourself.

To register a new class, use *`registerClass<T>(const std::string& name, int nParents=0)`*, with *T* your C++ class, *name* its name in Swan, and *nParents* the number of parent classes.
Only the fields of the first parent are inherited, other additional parents are only mixins. If nParents=0, then the class automatically inherits from Object. The parents must be present on the stack.

To register a constructor, use *`registerConstructor<T, A...>()`*, where *T* is your C++ class, and *A...* constructor arguments. 
In fact, this registers a *constructor* method passing the right parameters.

To register a destructor, use *`registerDestructor<T>()`*, where *T* is your C++ class. You are recommanded to do it even if the class has no explicit destructor.
The destructor will be called upon garbage collection of the object.

Registration of functions, methods and properties are greatly simplified by the use of macros.
The following macros convert C++ fonctions into valid Swan functions with the prototype `void (Swan::Fiber&)`, i.e. it does the wrapping and unwrapping of arguments and return value automatically.

- *`METHOD(TYPE, NAME)`* takes a method *NAME* of the class *TYPE*. The given method can't have multiple overloads, otherwise the C++ compiler doesn't know which one to take.
- *`FUNCTION(NAME)`* takes a fonction with the name *NAME*. There can't be multipel overloads of the fonction with the same name, otherwise the C++ compiler doesn't know which one to take.
- *`PROPERTY(TYPE, NAME)`* takes a property *NAME* in the class *TYPE*, i.e. a member field. This generates a getter and a setter. For the setter to work, the given member field must obviously not be const.
- *`GETTER(TYPE, NAME)`* and *`SETTER(TYPE, NAME)`* generates only a getter or only a setter for the given member field. Useful if you wish to write a read-only or write-only property.
- *`STATIC_METHOD(NAME)`* is intended to be used when registering static methods with *registerStaticMethod*. In that particular case, stack index 0 contains a reference to the class object, which has to be ignored when retrieving method arguments in C++.

Example with a simple point c++ class:

```
// Our Point class
struct Point {
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

// We have finished registration, pop the Point class object
fiber.pop(); 
```

The IO and Date modules of the CLI give more elaborated examples of C++ API made available to scripts.

## Saving bytecode
Swan give you the ability to save compiled code (a.k.a. bytecode) to disk for a later reuse without the need to recompile again from source.
There are two functions for doing that: a low-level, and a higher level one.

- *`dumpBytecode`*dumps the function on top of the stack only
- `*importAndDumpBytecode`* imports a file, executes its top-level code (code which isn't inside any function), and then dumps it.

The advantage of `importAndDumpBytecode` is that it dumps everything needed for a  later run, e.g. if you import and dump A but A imports B and C, B and C are also dumped. That's why a dumped bytecode file can contain several functions.
The generated file should be imported by calling the C++ `fiber.import` or using import in Swan.

## Multithreading
Unless Swan has been compiled with the NO_THREAD_SUPPORT option, the whole VM is protected by a single so called global interpreter lock (GIL).
Hance, no two fibers are effectively run concurrently. 

I tried to allow it, but it was too complicated. Problems essentially come from the garbage collector, which anyway needs to lock all fibers in all threads to do its job correctly.
Perhaps it will be for another day. If you want to contribute in making a concurrent GC, the door is open.

You can still use the same VM across multiple threads, as long as you take care of releasing the GIL and acquiring it again when you are doing I/O or other operations that take time without interaction with Swan.
The VM has two methods `lock`and `unlock`, but it's better to use the RAII guard `ScopeUnlocker` class.

## VM settings and language options
When embedding, you can set a few VM parameters as well as options that will change how scripts are parsed, compiled or run.

SEt language options with the *VM::setOption* method. The following options are available (from Option enum) :

- VAR_DECL_MODE: define how variable declarations are threated; can be one of three possible modes:
    - VAR_STRICT: when an unknown variable is referenced in the code, compilation is stopped and throws an error. Variables must always be declared with *let* or *const*. This is the recommanded option.
    - VAR_IMPLICIT: when an unknown variable is referenced in the code, it is implicitly declared as a new local variable, as if *let* or *const* had been used explicitly. That's seem cool, but may lead to surprises when using several times the same variable names in nested scopes; it's better avoided.
    - VAR_IMPLICIT_GLOBAL: same as VAR_IMPLICIT, except that new variable is implicitly declared global. This is useful for the CLI, but strongly discouraged for usual codes.

Set VM parameters by using the appropriate method. The following settings can be tweaked:

- *Path resolver*: the path resolver resolves a relative path to an absolute one. By default, usual path syntax is used, with indifferent `/` or `\` separators, `..` for parent directory and `.` for current directory. An exception should be thrown when encountering an invalid path.
- *file loader: the file loader takes an absolute path and loads the corresponding source file. By default, it loads files from the current directory. You can override this to load sources from custom or non file-based storage, to prevent unwanted file access, or to disable imports completely. An exception should be thrown when trying to load an unexisting file.
- *compilation message receiver*: the compilation message receiver collects messages generated during the compilation (error messages). By default, messages are printed to standard output.
- *import hook*: you can have greater control on imports with the hook. 
For each import made in C++ or in Swan, it is called at different steps: immediately at request (IMPORT_REQUEST), before trying to look in a file (BEFORE_IMPORT), once the file has been loaded but not yet run (BEFORE_RUN), or after it has been run (AFTER_RUN). 
On the two first steps, you have the option to return false and let the normal process going, or push something on the stack and return true. This gives you the possibility to load C++ libraries on demand from Swan. For example you can initialize a network library and push a Socket class on the stac, but do it only when `import "Socket"` is found in the code.
On the two last steps, you may examine what has been loaded on top of the stack and replace it with something else; the return value is ignored.
