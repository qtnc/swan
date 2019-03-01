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
if (!in) QFiber::curFiber ->runtimeError("Couldn't open file: %s", filename);
ostringstream out(ios::binary);
out << in.rdbuf();
string s = out.str();
return s;
}


QVM::QVM ():
firstGCObject(nullptr),
gcAliveCount(0),
gcTreshhold(140),
gcTreshholdFactor(200),
pathResolver(defaultPathResolver),
fileLoader(defaultFileLoader),
messageReceiver(defaultMessageReceiver)
{
objectClass = QClass::create(*this, nullptr, nullptr, "Object", 0, 0);
classClass = QClass::create(*this, nullptr, objectClass, "Class", 0, -1);
fiberMetaClass = QClass::create(*this, classClass, classClass, "FiberMetaClass", 0, -1);
listMetaClass = QClass::create(*this, classClass, classClass, "ListMetaClass", 0, -1);
mapMetaClass = QClass::create(*this, classClass, classClass, "MapMetaClass", 0, -1);
numMetaClass = QClass::create(*this, classClass, classClass, "NumberMetaClass", 0, -1);
rangeMetaClass = QClass::create(*this, classClass, classClass, "RangeMetaClass", 0, -1);
setMetaClass = QClass::create(*this, classClass, classClass, "SetMetaClass", 0, -1);
stringMetaClass = QClass::create(*this, classClass, classClass, "StringMetaClass", 0, -1);
systemMetaClass = QClass::create(*this, classClass, classClass, "SystemMetaClass", 0, -1);
tupleMetaClass = QClass::create(*this, classClass, classClass, "TupleMetaClass", 0, -1);
boolClass = QClass::create(*this, classClass, objectClass, "Bool", 0, -1);
functionClass = QClass::create(*this, classClass, objectClass, "Function", 0, -1);
sequenceClass = QClass::create(*this, classClass, objectClass, "Sequence", 0, 0);
fiberClass = QClass::create(*this, fiberMetaClass, sequenceClass, "Fiber", 0, -1);
listClass = QClass::create(*this, listMetaClass, sequenceClass, "List", 0, -1);
mapClass = QClass::create(*this, mapMetaClass, sequenceClass, "Map", 0, -1);
nullClass = QClass::create(*this, classClass, objectClass, "Null", 0, -1);
numClass = QClass::create(*this, numMetaClass, objectClass, "Number", 0, -1);
rangeClass = QClass::create(*this, rangeMetaClass, sequenceClass, "Range", 0, -1);
setClass = QClass::create(*this, setMetaClass, sequenceClass, "Set", 0, -1);
stringClass = QClass::create(*this, stringMetaClass, sequenceClass, "String", 0, -1);
systemClass = QClass::create(*this, systemMetaClass, objectClass, "System", 0, -1);
tupleClass = QClass::create(*this, tupleMetaClass, sequenceClass, "Tuple", 0, -1);
#ifndef NO_BUFFER
bufferMetaClass = QClass::create(*this, classClass, classClass, "BufferMetaClass", 0, -1);
bufferClass = QClass::create(*this, bufferMetaClass, sequenceClass, "Buffer", 0, -1);
#endif
#ifndef NO_REGEX
regexMetaClass = QClass::create(*this, classClass, classClass, "RegexMetaClass", 0, -1);
regexClass = QClass::create(*this, regexMetaClass, objectClass, "Regex", 0, -1);
regexMatchResultClass = QClass::create(*this, classClass, objectClass, "RegexMatchResult", 0, -1);
regexIteratorClass = QClass::create(*this, classClass, sequenceClass, "RegexIterator", 0, -1);
regexTokenIteratorClass = QClass::create(*this, classClass, sequenceClass, "RegexTokenIterator", 0, -1);
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
dictionaryMetaClass = QClass::create(*this, classClass, classClass, "DictionaryMetaClass", 0, -1);
dictionaryClass = QClass::create(*this, dictionaryMetaClass, sequenceClass, "Dictionary", 0, -1);
linkedListMetaClass = QClass::create(*this, classClass, classClass, "LinkedListMetaClass", 0, -1);
linkedListClass = QClass::create(*this, linkedListMetaClass, sequenceClass, "LinkedList", 0, -1);
priorityQueueMetaClass = QClass::create(*this, classClass, classClass, "PriorityQueueMetaClass", 0, -1);
priorityQueueClass = QClass::create(*this, priorityQueueMetaClass, sequenceClass, "PriorityQueue", 0, -1);
sortedSetMetaClass = QClass::create(*this, classClass, classClass, "SortedSetMetaClass", 0, -1);
sortedSetClass = QClass::create(*this, sortedSetMetaClass, sequenceClass, "SortedSet", 0, -1);
#endif
#ifndef NO_RANDOM
randomMetaClass = QClass::create(*this, classClass, classClass, "RandomMetaClass", 0, -1);
randomClass = QClass::create(*this, randomMetaClass, sequenceClass, "Random", 0, -1);
#endif
#ifndef NO_GRID
gridMetaClass = QClass::create(*this, classClass, classClass, "GridMetaClass", 0, -1);
gridClass = QClass::create(*this, gridMetaClass, sequenceClass, "Grid", 0, -1);
#endif
objectClass->type = classClass;
classClass->type = classClass;
init();
initBuiltInCode();
}

QVM::~QVM () {
deinit();
garbageCollect();
QObject* obj = firstGCObject.load(std::memory_order_relaxed);
while(obj){
QObject* next = reinterpret_cast<QObject*>(reinterpret_cast<uintptr_t>(obj->next) &~3);
delete obj;
obj=next;
}}

void QVM::reset () {
deinit();
garbageCollect();
init();
initBuiltInCode();
}

void QVM::deinit () {
LOCK_SCOPE(globalMutex)
for (auto pf: fiberThreads) {
if (*pf) (*pf)->lock(); // Never unlock, since we are going to delete them all anyway
*pf = nullptr;
}
globalVariables.clear();
globalSymbols.clear();
methodSymbols.clear();
imports.clear();
stringCache.clear();
foreignClassIds.clear();
keptHandles.clear();
fiberThreads.clear();
}

void QVM::init () {
LOCK_SCOPE(globalMutex)
initPlatformEncodings();

initBaseTypes();
initNumberType();
initSequenceType();
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
initPriorityQueueType();
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
}


void QVM::initBuiltInCode () {
auto& f = getActiveFiber();
f.loadString(string(BUILTIN_CODE, sizeof(BUILTIN_CODE)), "<builtIn>");
f.call(0);
f.pop();
}

