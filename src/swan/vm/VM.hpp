#ifndef _____SWAN_VM_HPP_____
#define _____SWAN_VM_HPP_____
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "Mutex.hpp"
#include "Fiber.hpp"
#include <unordered_map>
#include<vector>
#include<string>

struct QVM: Swan::VM  {
static std::unordered_map<std::string, EncodingConversionFn> stringToBufferConverters;
static std::unordered_map<std::string, DecodingConversionFn> bufferToStringConverters;

std::vector<std::string> methodSymbols, globalSymbols;
std::vector<QV> globalVariables;
std::unordered_map<std::string,QV> imports;
std::unordered_map<std::pair<const char*, const char*>, QString*, StringCacheHasher, StringCacheEqualler> stringCache;
std::unordered_map<size_t, struct QForeignClass*> foreignClassIds;
std::vector<QFiber**> fiberThreads;
std::vector<QV> keptHandles;
PathResolverFn pathResolver;
FileLoaderFn fileLoader;
CompilationMessageFn messageReceiver;
Mutex globalMutex;
QObject* firstGCObject;
uint64_t gcAliveCount, gcTreshhold, gcTreshholdFactor;
QClass *boolClass, *classClass, *fiberClass, *functionClass, *listClass, *mapClass, *nullClass, *numClass, *objectClass, *rangeClass, *sequenceClass, *setClass, *stringClass, *systemClass, *tupleClass;
QClass *fiberMetaClass, *listMetaClass, *mapMetaClass, *numMetaClass, *rangeMetaClass, *setMetaClass, *stringMetaClass, *systemMetaClass, *tupleMetaClass;
#ifndef NO_BUFFER
QClass *bufferMetaClass, *bufferClass;
#endif
#ifndef NO_REGEX
QClass *regexClass, *regexMatchResultClass, *regexMetaClass, *regexIteratorClass, *regexTokenIteratorClass;
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
QClass *dictionaryClass, *dictionaryMetaClass, *linkedListClass, *linkedListMetaClass, *priorityQueueClass, *priorityQueueMetaClass, *sortedSetClass, *sortedSetMetaClass;
#endif
#ifndef NO_RANDOM
QClass *randomClass, *randomMetaClass;
#endif
#ifndef NO_GRID
QClass *gridClass, *gridMetaClass;
#endif
uint8_t varDeclMode = Option::VAR_STRICT;

QVM ();
virtual ~QVM ();
virtual QFiber& getActiveFiber () final override;

int findMethodSymbol (const std::string& name);
int findGlobalSymbol (const std::string& name, bool createNew);
void bindGlobal (const std::string& name, const QV& value);
QClass* createNewClass (const std::string& name, std::vector<QV>& parents, int nStaticFields, int nFields, bool foreign);
void addToGC (QObject* obj);

void initBaseTypes();
void initNumberType();
void initSequenceType();
void initStringType();
void initListType();
void initMapType();
void initTupleType();
void initSetType();
void initRangeType();
#ifndef NO_BUFFER
void initBufferType();
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
void initLinkedListType();
void initDictionaryType();
void initSortedSetType ();
void initPriorityQueueType ();
#endif
#ifndef NO_REGEX
void initRegexTypes();
#endif
#ifndef NO_RANDOM
void initRandomType();
#endif
#ifndef NO_GRID
void initGridType ();
#endif
void initGlobals();
void initMathFunctions();

virtual void garbageCollect () final override;
virtual void reset () final override;

virtual inline const PathResolverFn& getPathResolver () final override { return pathResolver; }
virtual inline void setPathResolver (const PathResolverFn& fn) final override { pathResolver=fn; }
virtual inline const FileLoaderFn& getFileLoader () final override { return fileLoader; }
virtual inline void setFileLoader (const FileLoaderFn& fn) final override { fileLoader=fn; }
virtual inline const CompilationMessageFn& getCompilationMessageReceiver () final override { return messageReceiver; }
virtual inline void setCompilationMessageReceiver (const CompilationMessageFn& fn) final override { messageReceiver = fn; }
virtual void setOption (Option opt, int value) final override;
virtual int getOption (Option opt) final override;
};

#include "FiberVM.hpp"

#endif
