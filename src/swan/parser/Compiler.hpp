#ifndef _____COMPILER_HPP_____
#define _____COMPILER_HPP_____
#include "Constants.hpp"
#include "Token.hpp"
#include "Parser.hpp"
#include "../vm/OpCodeInfo.hpp"
#include "../vm/Function.hpp"
#include "../../include/cpprintf.hpp"
#include<string>
#include<vector>
#include<sstream>
#include<limits>

struct QVM;
struct QParser;
struct TypeAnalyzer;
struct ClassDeclaration;
struct FunctionDeclaration;

struct LocalVariable {
QToken name;
std::shared_ptr<struct TypeInfo> type;
std::shared_ptr<struct Expression> value;
int scope;
bool hasUpvalues ;
bool isConst;
LocalVariable (const QToken& n, int s, bool ic);
};

struct Loop {
enum { START, CONDITION, END };
int scope, startPos, condPos, endPos;
std::vector<std::pair<int,int>> jumpsToPatch;
Loop (int sc, int st): scope(sc), startPos(st), condPos(-1), endPos(-1) {}
};

enum class VarKind {
Local,
Upvalue,
Global,
};

struct FindVarResult {
int slot;
VarKind type;
};

struct QCompiler {
QVM& vm;
QParser& parser;
ClassDeclaration* curClass = nullptr;
FunctionDeclaration* curMethod = nullptr;
QCompiler* parent = nullptr;
std::shared_ptr<TypeAnalyzer> analyzer = nullptr, globalAnalyzer=nullptr;
std::ostringstream out;
std::vector<DebugItem> debugItems;
std::vector<LocalVariable> localVariables; //globalVariables;
std::vector<Upvariable> upvalues;
std::vector<Loop> loops;
std::vector<QV> constants;
QOpCode lastOp = OP_NOP, beforeLastOp = OP_NOP;
CompilationResult result = CR_SUCCESS;
int curScope = 0;

template<class T> void write (const T& x) { out.write(reinterpret_cast<const char*>(&x), sizeof(x)); }
template<class T> void write (const T* x, size_t sz) { out.write(reinterpret_cast<const char*>(x), sz); }
void writeOp (QOpCode op) { write<uint8_t>(op); beforeLastOp=lastOp; lastOp=op; }

template<class T> int writeOpArg (QOpCode op, const T& arg) { 
writeOp(op);
int pos = out.tellp();
write<T>(arg);
return pos;
}
template<class T, class U> inline void writeOpArgs (QOpCode op, const T&  t, const U& u) {
writeOp(op);
write<T>(t);
write<U>(u);
}
int writePosition () { return out.tellp(); }
int writeOpJump  (QOpCode op, uint_jump_offset_t arg = ~0) { return writeOpArg(op, arg); }
int writeOpJumpBackTo  (QOpCode op, int pos);
void patchJump (int pos, int reach=-1);
template<class T> void patch (int pos, const T& val) {
int curpos = out.tellp();
out.seekp(pos);
write<T>(val);
out.seekp(curpos);
}
void seek (int n) { out.seekp(n, std::ios_base::cur); }
void seekabs (int n) { out.seekp(n, std::ios_base::beg); }
void writeOpCallFunction (uint8_t nArgs) ;
void writeOpCallMethod (uint8_t nArgs, uint_method_symbol_t symbol) ;
void writeOpCallSuper  (uint8_t nArgs, uint_method_symbol_t symbol) ;
void writeOpLoadLocal (uint_local_index_t slot) ;
void writeOpStoreLocal (uint_local_index_t slot);

void writeDebugLine (const QToken& tk);

void pushLoop ();
void popLoop ();
void pushScope ();
void popScope ();

int countLocalVariablesInScope (int scope = -1);
int findLocalVariable (const QToken& name, bool forWrite = false);
int findUpvalue (const QToken& name, bool forWrite = false);
int findConstant (const QV& value);
int addUpvalue (int slot, bool upperUpvalue);
FindVarResult findVariable (const QToken& name, bool forWrite=false);
int createLocalVariable (const QToken& name, bool isConst = false);
int findGlobalVariable (const QToken& name, bool forWrite = false);
int findGlobalVariable (const std::string& name, bool forWrite = false);
int createGlobalVariable (const QToken& name, bool isConst = false);

struct ClassDeclaration* getCurClass (int* atLevel = nullptr);
struct FunctionDeclaration* getCurMethod ();

inline QToken createTempName (Expression& expr) { return parser.createTempName(expr); }
inline QToken createTempName (const QToken& expr) { return parser.createTempName(expr); }

template<class... A> inline void compileError (const QToken& token, const char* fmt, const A&... args) { parser.printMessage( token, Swan::CompilationMessage::Kind::ERROR, format(fmt, args...)); result = CR_FAILED; }
template<class... A> inline void compileWarn (const QToken& token, const char* fmt, const A&... args) { parser.printMessage( token, Swan::CompilationMessage::Kind::WARNING, format(fmt, args...)); }
template<class... A> inline void compileInfo (const QToken& token, const char* fmt, const A&... args) { parser.printMessage( token, Swan::CompilationMessage::Kind::INFO, format(fmt, args...)); }

QCompiler (QParser& p, QCompiler* pa = nullptr): vm(p.vm), parser(p), parent(pa), curClass(nullptr)  {}
void compile ();
struct QFunction* getFunction (int nArgs = 0);
bool bcIsAccessor (const std::string& bc, uint_field_index_t* fieldIndex = 0);
};

#endif
