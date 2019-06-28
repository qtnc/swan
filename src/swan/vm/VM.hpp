#ifndef _____SWAN_VM_HPP_____
#define _____SWAN_VM_HPP_____
#include "Value.hpp"
#include "HasherAndEqualler.hpp"
#include "GIL.hpp"
#include "Fiber.hpp"
#include <unordered_map>
#include<vector>
#include<string>

struct GlobalVariable {
int index;
bool isConst;
};

struct QVM: Swan::VM  {
static std::unordered_map<std::string, EncodingConversionFn> stringToBufferConverters;
static std::unordered_map<std::string, DecodingConversionFn> bufferToStringConverters;

std::vector<std::string> methodSymbols;
std::vector<QV> globalVariables;
std::unordered_map<std::string,QV> imports;
std::unordered_map<std::string, GlobalVariable> globalSymbols;
std::unordered_map<std::pair<const char*, const char*>, QString*, StringCacheHasher, StringCacheEqualler> stringCache;
std::unordered_map<size_t, struct QForeignClass*> foreignClassIds;
std::vector<QV> keptHandles;
std::vector<QFiber*> fibers;
QFiber *activeFiber;
PathResolverFn pathResolver;
FileLoaderFn fileLoader;
CompilationMessageFn messageReceiver;
ImportHookFn importHook;
GIL gil;
size_t gcMemUsage, gcTreshhold;
uint16_t gcTreshholdFactor, gcLock;
QObject* firstGCObject;

QClass *boolClass, *classClass, *fiberClass, *functionClass, *nullClass, *numClass, *objectClass, *undefinedClass;
QClass *listClass, *mapClass, *rangeClass, *setClass, *stringClass, *tupleClass;
QClass *iterableClass, *iteratorClass, *mappingClass, *listIteratorClass, *stringIteratorClass, *setIteratorClass, *mapIteratorClass, *rangeIteratorClass, *tupleIteratorClass;
#ifndef NO_BUFFER
QClass *bufferClass, *bufferIteratorClass;
#endif
#ifndef NO_REGEX
QClass *regexClass, *regexMatchResultClass, *regexIteratorClass, *regexTokenIteratorClass;
#endif
#ifndef NO_OPTIONAL_COLLECTIONS
QClass *dictionaryClass, *linkedListClass, *heapClass, *sortedSetClass;
QClass *dictionaryIteratorClass, *linkedListIteratorClass, *sortedSetIteratorClass, *heapIteratorClass;
#endif
#ifndef NO_RANDOM
QClass *randomClass;
#endif
#ifndef NO_GRID
QClass *gridClass, *gridIteratorClass;
#endif

uint8_t varDeclMode = Option::VAR_STRICT;
bool compileDbgInfo = true;
bool multilineStrings = true;

QVM ();
virtual ~QVM ();
void init ();
virtual QFiber& createFiber () final override;
virtual inline QFiber& getActiveFiber () final override { return *activeFiber; }

int findMethodSymbol (const std::string& name);
int findGlobalSymbol (const std::string& name, int flags);
void bindGlobal (const std::string& name, const QV& value, bool isConst=false);
QClass* createNewClass (const std::string& name, std::vector<QV>& parents, int nStaticFields, int nFields, bool foreign);

void addToGC (QObject* obj);

void initBaseTypes();
void initNumberType();
void initIterableType();
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
void initHeapType ();
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

virtual inline void lock () final override { gil.lock(); }
virtual inline void unlock () final override { gil.unlock(); }

virtual void garbageCollect () final override;
virtual inline void destroy () final override { delete this; }

virtual inline const PathResolverFn& getPathResolver () final override { return pathResolver; }
virtual inline void setPathResolver (const PathResolverFn& fn) final override { pathResolver=fn; }
virtual inline const FileLoaderFn& getFileLoader () final override { return fileLoader; }
virtual inline void setFileLoader (const FileLoaderFn& fn) final override { fileLoader=fn; }
virtual inline const CompilationMessageFn& getCompilationMessageReceiver () final override { return messageReceiver; }
virtual inline void setCompilationMessageReceiver (const CompilationMessageFn& fn) final override { messageReceiver = fn; }
virtual inline const ImportHookFn& getImportHook () final override { return importHook; }
virtual inline void setImportHook (const ImportHookFn& fn) final override { importHook=fn; }
virtual void setOption (Option opt, int value) final override;
virtual int getOption (Option opt) final override;

void* allocate (size_t n);
void deallocate (void* p, size_t n) ;

template<class T, class... A> inline T* construct (A&&... args) {
gcMemUsage += sizeof(T);
return new(allocate(sizeof(T)))  T( std::forward<A>(args)... );
}

template<class T, class U, class... A> inline T* constructVLS (int nU, A&&... args) {
char* ptr = reinterpret_cast<char*>(allocate(sizeof(T) + nU*sizeof(U)));
U* uPtr = reinterpret_cast<U*>(ptr + sizeof(T));
new(ptr) T( std::forward<A>(args)... );
//new(uPtr) U[nU];
std::fill(uPtr, uPtr+nU, U());
return reinterpret_cast<T*>(ptr);
}

};

struct GCLocker {
QVM& vm;
GCLocker (QVM& vm0): vm(vm0) { ++vm.gcLock; }
~GCLocker () { --vm.gcLock; }
};

#include "FiberVM.hpp"

#endif
