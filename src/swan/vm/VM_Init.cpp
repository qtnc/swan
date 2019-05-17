#include "VM.hpp"
#include "../../include/cpprintf.hpp"
#include<boost/algorithm/string.hpp>
#include<fstream>
#include<iostream>
using namespace std;

#include "../lib/builtin-code.h"

void initPlatformEncodings ();

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

static string defaultFileLoader (const string& filename) {
ifstream in(filename, ios::binary);
if (!in) throw std::runtime_error(format("Couldn't open file: %s", filename));
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
rootFiber(nullptr),
firstGCObject(nullptr),
gcMemUsage(0),
gcTreshhold(65536),
gcTreshholdFactor(200),
gcLock(false),
pathResolver(defaultPathResolver),
fileLoader(defaultFileLoader),
messageReceiver(defaultMessageReceiver),
importHook(defaultImportHook)
{
GCLocker gcLocker(*this);
objectClass = QClass::create(*this, nullptr, nullptr, "Object", 0, 0);
classClass = QClass::create(*this, nullptr, objectClass, "Class", 0, -1);

auto boolMetaClass = QClass::create(*this, classClass, classClass, "BoolMetaClass", 0, -1);
auto classMetaClass = QClass::create(*this, classClass, classClass, "ClassMetaClass", 0, -1);
auto fiberMetaClass = QClass::create(*this, classClass, classClass, "FiberMetaClass", 0, -1);
auto functionMetaClass = QClass::create(*this, classClass, classClass, "FunctionMetaClass", 0, -1);
auto iterableMetaClass = QClass::create(*this, classClass, classClass, "IterableMetaClass", 0, -1);
auto iteratorMetaClass = QClass::create(*this, classClass, classClass, "IteratorMetaClass", 0, -1);
auto listMetaClass = QClass::create(*this, classClass, classClass, "ListMetaClass", 0, -1);
auto mapMetaClass = QClass::create(*this, classClass, classClass, "MapMetaClass", 0, -1);
auto mappingMetaClass = QClass::create(*this, classClass, classClass, "MappingMetaClass", 0, -1);
auto numMetaClass = QClass::create(*this, classClass, classClass, "NumMetaClass", 0, -1);
auto objectMetaClass = QClass::create(*this, classClass, classClass, "ObjectMetaClass", 0, -1);
auto rangeMetaClass = QClass::create(*this, classClass, classClass, "RangeMetaClass", 0, -1);
auto setMetaClass = QClass::create(*this, classClass, classClass, "SetMetaClass", 0, -1);
auto stringMetaClass = QClass::create(*this, classClass, classClass, "StringMetaClass", 0, -1);
auto systemMetaClass = QClass::create(*this, classClass, classClass, "SystemMetaClass", 0, -1);
auto tupleMetaClass = QClass::create(*this, classClass, classClass, "TupleMetaClass", 0, -1);

boolClass = QClass::create(*this, boolMetaClass, objectClass, "Bool", 0, -1);
functionClass = QClass::create(*this, functionMetaClass, objectClass, "Function", 0, -1);
iterableClass = QClass::create(*this, iterableMetaClass, objectClass, "Iterable", 0, 0);
iteratorClass = QClass::create(*this, iteratorMetaClass, objectClass, "Iterator", 0, 0);
fiberClass = QClass::create(*this, fiberMetaClass, iterableClass, "Fiber", 0, -1);
listClass = QClass::create(*this, listMetaClass, iterableClass, "List", 0, -1);
mappingClass = QClass::create(*this, mappingMetaClass, iterableClass, "Mapping", 0, 0);
mapClass = QClass::create(*this, mapMetaClass, mappingClass, "Map", 0, -1);
nullClass = QClass::create(*this, classClass, objectClass, "Null", 0, -1);
numClass = QClass::create(*this, numMetaClass, objectClass, "Num", 0, -1);
rangeClass = QClass::create(*this, rangeMetaClass, iterableClass, "Range", 0, -1);
setClass = QClass::create(*this, setMetaClass, iterableClass, "Set", 0, -1);
stringClass = QClass::create(*this, stringMetaClass, iterableClass, "String", 0, -1);
systemClass = QClass::create(*this, systemMetaClass, objectClass, "System", 0, -1);
tupleClass = QClass::create(*this, tupleMetaClass, iterableClass, "Tuple", 0, -1);

listIteratorClass = QClass::create(*this, classClass, iteratorClass, "ListIterator", 0, -1);
mapIteratorClass = QClass::create(*this, classClass, iteratorClass, "MapIterator", 0, -1);
rangeIteratorClass = QClass::create(*this, classClass, iteratorClass, "RangeIterator", 0, -1);
setIteratorClass = QClass::create(*this, classClass, iteratorClass, "SetIterator", 0, -1);
stringIteratorClass = QClass::create(*this, classClass, iteratorClass, "StringIterator", 0, -1);
tupleIteratorClass = QClass::create(*this, classClass, iteratorClass, "TupleIterator", 0, -1);

#ifndef NO_BUFFER
auto bufferMetaClass = QClass::create(*this, classClass, classClass, "BufferMetaClass", 0, -1);
bufferClass = QClass::create(*this, bufferMetaClass, iterableClass, "Buffer", 0, -1);
bufferIteratorClass = QClass::create(*this, classClass, iteratorClass, "BufferIterator", 0, -1);
#endif
#ifndef NO_REGEX
auto regexMetaClass = QClass::create(*this, classClass, classClass, "RegexMetaClass", 0, -1);
regexClass = QClass::create(*this, regexMetaClass, objectClass, "Regex", 0, -1);
regexMatchResultClass = QClass::create(*this, classClass, objectClass, "RegexMatchResult", 0, -1);
regexIteratorClass = QClass::create(*this, classClass, iteratorClass, "RegexIterator", 0, -1);
regexTokenIteratorClass = QClass::create(*this, classClass, iteratorClass, "RegexTokenIterator", 0, -1);
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
auto dictionaryMetaClass = QClass::create(*this, classClass, classClass, "DictionaryMetaClass", 0, -1);
dictionaryClass = QClass::create(*this, dictionaryMetaClass, mappingClass, "Dictionary", 0, -1);
dictionaryIteratorClass = QClass::create(*this, classClass, iteratorClass, "DictionaryIterator", 0, -1);
auto linkedListMetaClass = QClass::create(*this, classClass, classClass, "LinkedListMetaClass", 0, -1);
linkedListClass = QClass::create(*this, linkedListMetaClass, iterableClass, "LinkedList", 0, -1);
linkedListIteratorClass = QClass::create(*this, classClass, iteratorClass, "LinkedListIterator", 0, -1);
auto heapMetaClass = QClass::create(*this, classClass, classClass, "HeapMetaClass", 0, -1);
heapClass = QClass::create(*this, heapMetaClass, iterableClass, "Heap", 0, -1);
heapIteratorClass = QClass::create(*this, classClass, iteratorClass, "HeapIterator", 0, -1);
auto sortedSetMetaClass = QClass::create(*this, classClass, classClass, "SortedSetMetaClass", 0, -1);
sortedSetClass = QClass::create(*this, sortedSetMetaClass, iterableClass, "SortedSet", 0, -1);
sortedSetIteratorClass = QClass::create(*this, classClass, iteratorClass, "SortedSetIterator", 0, -1);
#endif
#ifndef NO_RANDOM
auto randomMetaClass = QClass::create(*this, classClass, classClass, "RandomMetaClass", 0, -1);
randomClass = QClass::create(*this, randomMetaClass, iterableClass, "Random", 0, -1);
#endif
#ifndef NO_GRID
auto gridMetaClass = QClass::create(*this, classClass, classClass, "GridMetaClass", 0, -1);
gridClass = QClass::create(*this, gridMetaClass, iterableClass, "Grid", 0, -1);
gridIteratorClass = QClass::create(*this, classClass, iteratorClass, "GridIterator", 0, -1);
#endif

objectClass->type = objectMetaClass;
classClass->type = classMetaClass;
init();
}

QVM::~QVM () {
globalVariables.clear();
globalSymbols.clear();
methodSymbols.clear();
imports.clear();
stringCache.clear();
foreignClassIds.clear();
keptHandles.clear();
QObject* obj = firstGCObject;
while(obj){
QObject* next = reinterpret_cast<QObject*>(reinterpret_cast<uintptr_t>(obj->next) &~3);
delete obj;
obj=next;
}
delete classClass;
delete objectClass;
}

void QVM::init () {
LOCK_SCOPE(gil)
initPlatformEncodings();

initBaseTypes();
initNumberType();
initIterableType();
initStringType();
initListType();
initMapType();
initTupleType();
initSetType();
initRangeType();

#ifndef NO_BUFFER
initBufferType();
#endif

#ifndef NO_OPTIONAL_COLLECTIONS
initLinkedListType();
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

activeFiber = rootFiber = construct<QFiber>(*this);
auto& f = *rootFiber;
f.loadString(string(BUILTIN_CODE, sizeof(BUILTIN_CODE)), "<builtIn>");
f.call(0);
f.pop();

//println("sizeof(QObject)=%d, sizeof(QFiber)=%d, sizeof(QInstance)=%d, sizeof(QClass)=%d, sizeof(QVM)=%d", sizeof(QObject), sizeof(QFiber), sizeof(QInstance), sizeof(QClass), sizeof(QVM));
//println("End of init, mem usage = %d", gcMemUsage);
}

