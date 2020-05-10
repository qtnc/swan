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
classClass = QClass::createNonInheritable(*this, nullptr, objectClass, "Class");
addToGC(classClass);
addToGC(objectClass);

auto functionMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "FunctionMetaClass");
auto closureMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "ClosureMetaClass");
auto stdFunctionMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "StdFunctionMetaClass");
auto boundFunctionMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "BoundFunctionMetaClass");

auto boolMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "BoolMetaClass");
auto classMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "ClassMetaClass");
auto fiberMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "FiberMetaClass");
auto iterableMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "IterableMetaClass");
auto iteratorMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "IteratorMetaClass");
auto listMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "ListMetaClass");
auto mapMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "MapMetaClass");
auto mappingMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "MappingMetaClass");
auto numMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "NumMetaClass");
auto objectMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "ObjectMetaClass");
auto rangeMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "RangeMetaClass");
auto setMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "SetMetaClass");
auto stringMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "StringMetaClass");
auto tupleMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "TupleMetaClass");

classClass->type = classMetaClass;

functionClass = QClass::createNonInheritable(*this, functionMetaClass, objectClass, "Function");
closureClass = QClass::createNonInheritable(*this, closureMetaClass, functionClass, "Closure");
boundFunctionClass = QClass::createNonInheritable(*this, boundFunctionMetaClass, functionClass, "BoundFunction");
stdFunctionClass = QClass::createNonInheritable(*this, stdFunctionMetaClass, functionClass, "StdFunction");

boolClass = QClass::createNonInheritable(*this, boolMetaClass, objectClass, "Bool");
iterableClass = QClass::create(*this, iterableMetaClass, objectClass, "Iterable", 0, 0);
iteratorClass = QClass::create(*this, iteratorMetaClass, objectClass, "Iterator", 0, 0);
fiberClass = QClass::createNonInheritable(*this, fiberMetaClass, iterableClass, "Fiber");
listClass = QClass::createNonInheritable(*this, listMetaClass, iterableClass, "List");
mappingClass = QClass::create(*this, mappingMetaClass, iterableClass, "Mapping", 0, 0);
mapClass = QClass::createNonInheritable(*this, mapMetaClass, mappingClass, "Map");
nullClass = QClass::createNonInheritable(*this, classClass, objectClass, "null");
numClass = QClass::createNonInheritable(*this, numMetaClass, objectClass, "Num");
objectClass->type = objectMetaClass;
rangeClass = QClass::createNonInheritable(*this, rangeMetaClass, iterableClass, "Range");
setClass = QClass::createNonInheritable(*this, setMetaClass, iterableClass, "Set");
stringClass = QClass::createNonInheritable(*this, stringMetaClass, iterableClass, "String");
tupleClass = QClass::createNonInheritable(*this, tupleMetaClass, iterableClass, "Tuple");
undefinedClass = QClass::createNonInheritable(*this, classClass, objectClass, "undefined");

listIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "ListIterator");
mapIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "MapIterator");
rangeIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "RangeIterator");
setIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "SetIterator");
stringIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "StringIterator");
tupleIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "TupleIterator");

#ifndef NO_REGEX
auto regexMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "RegexMetaClass");
regexClass = QClass::createNonInheritable(*this, regexMetaClass, objectClass, "Regex");
regexMatchResultClass = QClass::createNonInheritable(*this, classClass, objectClass, "RegexMatchResult");
regexIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "RegexIterator");
regexTokenIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "RegexTokenIterator");
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
auto dequeMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "DequeMetaClass");
dequeClass = QClass::createNonInheritable(*this, dequeMetaClass, iterableClass, "Deque");
dequeIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "DequeIterator");
auto dictionaryMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "DictionaryMetaClass");
dictionaryClass = QClass::createNonInheritable(*this, dictionaryMetaClass, mappingClass, "Dictionary");
dictionaryIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "DictionaryIterator");
auto linkedListMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "LinkedListMetaClass");
linkedListClass = QClass::createNonInheritable(*this, linkedListMetaClass, iterableClass, "LinkedList");
linkedListIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "LinkedListIterator");
auto heapMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "HeapMetaClass");
heapClass = QClass::createNonInheritable(*this, heapMetaClass, iterableClass, "Heap");
heapIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "HeapIterator");
auto sortedSetMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "SortedSetMetaClass");
sortedSetClass = QClass::createNonInheritable(*this, sortedSetMetaClass, iterableClass, "SortedSet");
sortedSetIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "SortedSetIterator");
#endif
#ifndef NO_RANDOM
auto randomMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "RandomMetaClass");
randomClass = QClass::createNonInheritable(*this, randomMetaClass, iterableClass, "Random");
#endif
#ifndef NO_GRID
auto gridMetaClass = QClass::createNonInheritable(*this, classClass, classClass, "GridMetaClass");
gridClass = QClass::createNonInheritable(*this, gridMetaClass, iterableClass, "Grid");
gridIteratorClass = QClass::createNonInheritable(*this, classClass, iteratorClass, "GridIterator");
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

