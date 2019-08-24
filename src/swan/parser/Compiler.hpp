#ifndef _____COMPILER_HPP_____
#define _____COMPILER_HPP_____
#include "Constants.hpp"
#include "Token.hpp"
#include "Parser.hpp"
#include "../vm/OpCodeInfo.hpp"
#include "../vm/Function.hpp"
#include<string>
#include<vector>
#include<sstream>
#include<limits>

struct QVM;
struct QParser;

struct LocalVariable {
QToken name;
int scope;
bool hasUpvalues ;
bool isConst;
};

struct Loop {
enum { START, CONDITION, END };
int scope, startPos, condPos, endPos;
std::vector<std::pair<int,int>> jumpsToPatch;
Loop (int sc, int st): scope(sc), startPos(st), condPos(-1), endPos(-1) {}
};

struct QCompiler {
QVM& vm;
QParser& parser;
struct ClassDeclaration* curClass = nullptr;
struct FunctionDeclaration* curMethod = nullptr;
QCompiler* parent = nullptr;
std::ostringstream out;
std::vector<LocalVariable> localVariables;
std::vector<QFunction::Upvalue> upvalues;
std::vector<Loop> loops;
std::vector<QV> constants;
QV additionalContextVar;
QOpCode lastOp = OP_LOAD_NULL;
CompilationResult result = CR_SUCCESS;
int curScope = 0;

template<class T> void write (const T& x) { out.write(reinterpret_cast<const char*>(&x), sizeof(x)); }
void writeOp (QOpCode op) { write<uint8_t>(op); lastOp=op; }

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

void writeDebugLine (const QToken& tk);

void pushLoop ();
void popLoop ();
void pushScope ();
void popScope ();
int countLocalVariablesInScope (int scope = -1);
int findLocalVariable (const QToken& name, int flags);
int findUpvalue (const QToken& name, int flags);
int findGlobalVariable (const QToken& name, int flags);
int findConstant (const QV& value);
int addUpvalue (int slot, bool upperUpvalue);

struct ClassDeclaration* getCurClass (int* atLevel = nullptr);
struct FunctionDeclaration* getCurMethod ();
inline QToken createTempName () { return parser.createTempName(); }

template<class... A> void compileError (const QToken& token, const char* fmt, const A&... args);
template<class... A> void compileWarn (const QToken& token, const char* fmt, const A&... args);
template<class... A> void compileInfo (const QToken& token, const char* fmt, const A&... args);
void dump ();

QCompiler (QParser& p): vm(p.vm), parser(p), parent(nullptr), curClass(nullptr)  {}
void compile ();
struct QFunction* getFunction (int nArgs = 0);
};

#endif
