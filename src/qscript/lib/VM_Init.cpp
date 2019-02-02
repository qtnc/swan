#include "SwanLib.hpp"
#include<boost/algorithm/string.hpp>
#include<fstream>
using namespace std;

#include "builtin-code.h"

void initPlatformEncodings ();

static void defaultMessageReceiver (const QS::CompilationMessage& m) {
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
if (!in) return "";
ostringstream out(ios::binary);
out << in.rdbuf();
string s = out.str();
return s;
}


QVM::QVM ():
firstGCObject(nullptr),
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
#endif
#ifndef NO_RANDOM
randomMetaClass = QClass::create(*this, classClass, classClass, "RandomMetaClass", 0, -1);
randomClass = QClass::create(*this, randomMetaClass, sequenceClass, "Random", 0, -1);
#endif
objectClass->type = classClass;
classClass->type = classClass;
reset();
}

void QVM::reset () {
LOCK_SCOPE(globalMutex)
for (auto pf: fiberThreads) {
if (*pf) { (*pf)->lock(); (*pf)->unlock(); }
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
garbageCollect();
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
#endif

#ifndef NO_REGEX
initRegexTypes();
#endif

#ifndef NO_RANDOM
initRandomType();
#endif

initGlobals();
initMathFunctions();

auto& f = getActiveFiber();
f.loadString(string(BUILTIN_CODE, sizeof(BUILTIN_CODE)), "<builtIn>");
f.call(0);
f.pop();

}

