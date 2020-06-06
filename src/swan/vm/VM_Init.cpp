#include "VM.hpp"
#include "OpCodeInfo.hpp"
#include "../../include/cpprintf.hpp"
#include<boost/algorithm/string.hpp>
#include<fstream>
#include<iostream>
using namespace std;

#include "../lib/builtin-code.h"

void purgeMem ();

static void defaultMessageReceiver (const Swan::CompilationMessage& m) {
const char* kinds[] = { "ERROR", "WARNING", "INFO" };
println(std::cerr, "%s: %s:%d:%d near '%s': %s", kinds[m.kind], m.file, m.line, m.column, m.token, m.message);
}

static string defaultPathResolver (const string& startingPath, const string& pathToResolve) {
vector<string> cur, toResolve;
boost::split(cur, startingPath, boost::is_any_of("/\\"), boost::token_compress_off);
boost::split(toResolve, pathToResolve, boost::is_any_of("/\\"), boost::token_compress_off);
if (!cur.empty()) cur.pop_back(); // drop file name
toResolve.erase(remove_if(toResolve.begin(), toResolve.end(), [](const string& s){ return s=="."; }), toResolve.end());
{ 
auto it = toResolve.begin();
auto initial = find_if_not(toResolve.begin(), toResolve.end(), [](const string& s){ return s==".."; }) -toResolve.begin();
while((it = find(toResolve.begin()+initial, toResolve.end(), ".."))!=toResolve.end()) {
toResolve.erase(--it);
toResolve.erase(it);
}}
if (!toResolve.empty() && toResolve[0]=="") cur.clear();
else while (!toResolve.empty() && !cur.empty() && toResolve[0]=="..") {
toResolve.erase(toResolve.begin());
cur.pop_back();
}
cur.insert(cur.end(), toResolve.begin(), toResolve.end());
return boost::join(cur, "/");
}

static inline bool open (std::ifstream& in, const std::string& filename, std::ios::openmode mode) {
in.open(filename, mode);
return static_cast<bool>(in);
}

static string defaultFileLoader (const string& filename) {
ifstream in;
if (!open(in, filename+".sb", ios::binary) && !open(in, filename+".swan", ios::binary) && !open(in, filename, ios::binary)) error<ios_base::failure>("Couldn't open %s", filename);
ostringstream out(ios::binary);
out << in.rdbuf();
string s = out.str();
return s;
}

static bool defaultImportHook (Swan::Fiber& f, const std::string& fn, Swan::VM::ImportHookState state, int n) {
return false;
}


QVM::QVM ():
activeFiber(nullptr),
firstGCObject(nullptr),
gcMemUsage(0),
gcTreshhold(1<<18),
gcTreshholdFactor(200),
gcLock(false),
pathResolver(defaultPathResolver),
fileLoader(defaultFileLoader),
messageReceiver(defaultMessageReceiver),
importHook(defaultImportHook)
{
GCLocker gcLocker(*this);
objectClass = QClass::create(*this, nullptr, nullptr, "Object", 0, 0);
classClass = QClass::createFinal(*this, nullptr, objectClass, "Class");
addToGC(classClass);
addToGC(objectClass);

auto functionMetaClass = QClass::createFinal(*this, classClass, classClass, "FunctionMetaClass");
auto closureMetaClass = QClass::createFinal(*this, classClass, classClass, "ClosureMetaClass");
auto stdFunctionMetaClass = QClass::createFinal(*this, classClass, classClass, "StdFunctionMetaClass");
auto boundFunctionMetaClass = QClass::createFinal(*this, classClass, classClass, "BoundFunctionMetaClass");

auto boolMetaClass = QClass::createFinal(*this, classClass, classClass, "BoolMetaClass");
auto classMetaClass = QClass::createFinal(*this, classClass, classClass, "ClassMetaClass");
auto fiberMetaClass = QClass::createFinal(*this, classClass, classClass, "FiberMetaClass");
auto iterableMetaClass = QClass::createFinal(*this, classClass, classClass, "IterableMetaClass");
auto iteratorMetaClass = QClass::createFinal(*this, classClass, classClass, "IteratorMetaClass");
auto listMetaClass = QClass::createFinal(*this, classClass, classClass, "ListMetaClass");
auto mapMetaClass = QClass::createFinal(*this, classClass, classClass, "MapMetaClass");
auto mappingMetaClass = QClass::createFinal(*this, classClass, classClass, "MappingMetaClass");
auto numMetaClass = QClass::createFinal(*this, classClass, classClass, "NumMetaClass");
auto objectMetaClass = QClass::createFinal(*this, classClass, classClass, "ObjectMetaClass");
auto rangeMetaClass = QClass::createFinal(*this, classClass, classClass, "RangeMetaClass");
auto setMetaClass = QClass::createFinal(*this, classClass, classClass, "SetMetaClass");
auto stringMetaClass = QClass::createFinal(*this, classClass, classClass, "StringMetaClass");
auto tupleMetaClass = QClass::createFinal(*this, classClass, classClass, "TupleMetaClass");

classClass->type = classMetaClass;

functionClass = QClass::createFinal(*this, functionMetaClass, objectClass, "Function");
closureClass = QClass::createFinal(*this, closureMetaClass, functionClass, "Closure");
boundFunctionClass = QClass::createFinal(*this, boundFunctionMetaClass, functionClass, "BoundFunction");
stdFunctionClass = QClass::createFinal(*this, stdFunctionMetaClass, functionClass, "StdFunction");

boolClass = QClass::createFinal(*this, boolMetaClass, objectClass, "Bool");
iterableClass = QClass::create(*this, iterableMetaClass, objectClass, "Iterable", 0, 0);
iteratorClass = QClass::create(*this, iteratorMetaClass, objectClass, "Iterator", 0, 0);
fiberClass = QClass::createFinal(*this, fiberMetaClass, iterableClass, "Fiber");
listClass = QClass::createFinal(*this, listMetaClass, iterableClass, "List");
mappingClass = QClass::create(*this, mappingMetaClass, iterableClass, "Mapping", 0, 0);
mapClass = QClass::createFinal(*this, mapMetaClass, mappingClass, "Map");
nullClass = QClass::createFinal(*this, classClass, objectClass, "null");
numClass = QClass::createFinal(*this, numMetaClass, objectClass, "Num");
objectClass->type = objectMetaClass;
rangeClass = QClass::createFinal(*this, rangeMetaClass, iterableClass, "Range");
setClass = QClass::createFinal(*this, setMetaClass, iterableClass, "Set");
stringClass = QClass::createFinal(*this, stringMetaClass, iterableClass, "String");
tupleClass = QClass::createFinal(*this, tupleMetaClass, iterableClass, "Tuple");
undefinedClass = QClass::createFinal(*this, classClass, objectClass, "undefined");

listIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "ListIterator");
mapIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "MapIterator");
rangeIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "RangeIterator");
setIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "SetIterator");
stringIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "StringIterator");
tupleIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "TupleIterator");

#ifndef NO_REGEX
auto regexMetaClass = QClass::createFinal(*this, classClass, classClass, "RegexMetaClass");
regexClass = QClass::createFinal(*this, regexMetaClass, objectClass, "Regex");
regexMatchResultClass = QClass::createFinal(*this, classClass, objectClass, "RegexMatchResult");
regexIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "RegexIterator");
regexTokenIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "RegexTokenIterator");
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
auto dequeMetaClass = QClass::createFinal(*this, classClass, classClass, "DequeMetaClass");
dequeClass = QClass::createFinal(*this, dequeMetaClass, iterableClass, "Deque");
dequeIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "DequeIterator");
auto dictionaryMetaClass = QClass::createFinal(*this, classClass, classClass, "DictionaryMetaClass");
dictionaryClass = QClass::createFinal(*this, dictionaryMetaClass, mappingClass, "Dictionary");
dictionaryIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "DictionaryIterator");
auto linkedListMetaClass = QClass::createFinal(*this, classClass, classClass, "LinkedListMetaClass");
linkedListClass = QClass::createFinal(*this, linkedListMetaClass, iterableClass, "LinkedList");
linkedListIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "LinkedListIterator");
auto heapMetaClass = QClass::createFinal(*this, classClass, classClass, "HeapMetaClass");
heapClass = QClass::createFinal(*this, heapMetaClass, iterableClass, "Heap");
heapIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "HeapIterator");
auto sortedSetMetaClass = QClass::createFinal(*this, classClass, classClass, "SortedSetMetaClass");
sortedSetClass = QClass::createFinal(*this, sortedSetMetaClass, iterableClass, "SortedSet");
sortedSetIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "SortedSetIterator");
#endif
#ifndef NO_RANDOM
auto randomMetaClass = QClass::createFinal(*this, classClass, classClass, "RandomMetaClass");
randomClass = QClass::createFinal(*this, randomMetaClass, iterableClass, "Random");
#endif
#ifndef NO_GRID
auto gridMetaClass = QClass::createFinal(*this, classClass, classClass, "GridMetaClass");
gridClass = QClass::createFinal(*this, gridMetaClass, iterableClass, "Grid");
gridIteratorClass = QClass::createFinal(*this, classClass, iteratorClass, "GridIterator");
#endif

init();
}

QVM::~QVM () {
fibers.clear();
imports.clear();
keptHandles.clear();
globalVariables.clear();
stringCache.clear();
foreignClassIds.clear();
garbageCollect();
for (QObject* ptr=firstGCObject; ptr; ) {
auto gci = QV(ptr).getClass(*this).gcInfo;
size_t size = gci->gcMemSize(ptr);
auto next = ptr->gcNext();
gci->gcDestroy(ptr);
deallocate(ptr, size);
ptr=next;
}
globalSymbols.clear();
methodSymbols.clear();
purgeMem();
unlock();
}

void QVM::init () {
initBaseTypes();
initNumberType();
initIterableType();
initStringType();
initListType();
initMapType();
initTupleType();
initSetType();
initRangeType();
#ifndef NO_OPTIONAL_COLLECTIONS
initLinkedListType();
initDequeType();
initDictionaryType();
initSortedSetType();
initHeapType();
#endif

#ifndef NO_REGEX
initRegexTypes();
#endif

#ifndef NO_RANDOM
initRandomType();
#endif

#ifndef NO_GRID
initGridType();
#endif

initGlobals();
initMathFunctions();

auto& f = createFiber();
f.loadString(string(BUILTIN_CODE, sizeof(BUILTIN_CODE)), "<builtIn>");
f.call(0);
f.pop();

#ifdef DEBUG
println("sizeof(QObject)=%d, sizeof(QFiber)=%d, sizeof(QInstance)=%d, sizeof(QClass)=%d, sizeof(QFunction)=%d, sizeof(QVM)=%d", sizeof(QObject), sizeof(QFiber), sizeof(QInstance), sizeof(QClass), sizeof(QFunction), sizeof(QVM));
println("OP_END=%d", OP_END);
println("End of init, mem usage = %d", gcMemUsage);
#endif
}

inline void QVM::addToGC (QObject* obj) {
#ifdef DEBUG_GC
if (!gcLock) garbageCollect();
#endif
if (gcMemUsage >=gcTreshhold && !gcLock) garbageCollect();
obj->gcNext(firstGCObject);
firstGCObject  = obj;
}

