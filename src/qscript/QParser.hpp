#ifndef ____Q_PARSE_H_1___
#define ____Q_PARSE_H_1___
#include "QValue.hpp"
#include<string>
#include<sstream>
#include<vector>
#include<memory>

#define LV_EXISTING 0
#define LV_NEW 1
#define LV_CONST 2
#define LV_FOR_READ 0
#define LV_FOR_WRITE 2
#define LV_ERR_CONST -127
#define VD_LOCAL 1
#define VD_CONST 2
#define VD_GLOBAL 4
#define VD_EXPORT 8
#define FP_VARARG 1
#define FP_CONST 2
#define FP_FIELD_ASSIGN 4
#define FP_STATIC_FIELD_ASSIGN 8
#define FP_MAP_DESTRUCTURING 16
#define FP_ARRAY_DESTRUCTURING 32
#define FD_VARARG 1
#define FD_FIBER 2
#define FD_METHOD 4
#define FD_STATIC 8

enum QOperatorPriority {
P_LOWEST,
P_ASSIGNMENT,
P_COMPREHENSION,
P_CONDITIONAL,
P_LOGICAL,
P_COMPARISON,
P_RANGE,
P_BITWISE,
P_TERM,
P_FACTOR,
P_EXPONENT,
P_PREFIX,
P_POSTFIX,
P_SUBSCRIPT,
P_MEMBER,
P_CALL,
P_HIGHEST
};

enum QTokenType {
#define TOKEN(name) T_##name
#include "QTokenTypes.hpp"
#undef TOKEN
};

struct QToken {
QTokenType type;
const char *start;
size_t length;
QV value;
};

struct Expression;
struct Statement;

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

enum CompilationResult {
CR_SUCCESS,
CR_FAILED,
CR_INCOMPLETE
};

struct QParser  {
const char *in, *start, *end;
std::string filename, displayName;
QToken cur, prev;
QToken curMethodNameToken = { T_END, "#", 1,  QV() };
std::vector<QToken> stackedTokens;
QVM& vm;
CompilationResult result = CR_SUCCESS;

QParser (QVM& vm0, const std::string& source, const std::string& filename0, const std::string& displayName0): vm(vm0), in(source.data()), start(source.data()), end(source.data() + source.length()), filename(filename0), displayName(displayName0)  {}

const QToken& nextToken ();
const QToken& nextNameToken (bool);
const QToken& prevToken();
std::pair<int,int> getPositionOf (const char*);
template<class... A> void parseError (const char* fmt, const A&... args);

void skipNewlines ();
bool match (QTokenType type);
bool consume (QTokenType type, const char* msg);
template<class... T> bool matchOneOf (T... tokens);

std::shared_ptr<Expression> parseExpression (int priority = P_LOWEST);
std::shared_ptr<Expression> parsePrefixOp ();
std::shared_ptr<Expression> parseInfixOp (std::shared_ptr<Expression> left);
std::shared_ptr<Expression> parseConditional  (std::shared_ptr<Expression> condition);
std::shared_ptr<Expression> parseComprehension   (std::shared_ptr<Expression> body);
std::shared_ptr<Expression> parseMethodCall (std::shared_ptr<Expression> receiver);
std::shared_ptr<Expression> parseSubscript (std::shared_ptr<Expression> receiver);
std::shared_ptr<Expression> parseGroupOrTuple ();
std::shared_ptr<Expression> parseLiteral ();
std::shared_ptr<Expression> parseLiteralList ();
std::shared_ptr<Expression> parseLiteralSet ();
std::shared_ptr<Expression> parseLiteralMap ();
std::shared_ptr<Expression> parseLiteralRegex ();
std::shared_ptr<Expression> parseName ();
std::shared_ptr<Expression> parseField ();
std::shared_ptr<Expression> parseStaticField  ();
std::shared_ptr<Expression> parseLambda ();
std::shared_ptr<Expression> parseSuper ();
std::shared_ptr<Expression> parseUnpack ();
std::shared_ptr<Expression> parseGenericMethodSymbol ();
std::vector<std::shared_ptr<struct FunctionParameter>> parseFunctionParameters (bool implicitThis);

std::shared_ptr<Statement> parseStatement ();
std::shared_ptr<Statement> parseStatements ();
std::shared_ptr<Statement> parseSimpleStatement ();
std::shared_ptr<Statement> parseBlock ();
std::shared_ptr<Statement> parseIf ();
std::shared_ptr<Statement> parseFor ();
std::shared_ptr<Statement> parseWhile ();
std::shared_ptr<Statement> parseRepeatWhile ();
std::shared_ptr<Statement> parseContinue ();
std::shared_ptr<Statement> parseBreak ();
std::shared_ptr<Statement> parseReturn ();
std::shared_ptr<Statement> parseTry ();
std::shared_ptr<Statement> parseThrow ();
std::shared_ptr<Statement> parseWith ();
std::shared_ptr<Expression> parseYield ();
std::shared_ptr<Statement> parseVarDecl ();
std::shared_ptr<Statement> parseExportDecl ();
std::shared_ptr<Statement> parseImportDecl (bool expressionOnly);
std::shared_ptr<Statement> parseImportDecl ();
std::shared_ptr<Expression> parseImportExpression ();
std::shared_ptr<Statement> parseGlobalDecl ();
std::shared_ptr<Statement> parseClassDecl (int flags);
std::shared_ptr<Statement> parseClassDecl ();

void parseMethodDecl (struct ClassDeclaration&, bool);
void parseSimpleAccessor (struct ClassDeclaration&, bool);
};

struct QCompiler {
QVM& vm;
QParser& parser;
struct ClassDeclaration* curClass;
QCompiler* parent;
std::ostringstream out;
std::vector<LocalVariable> localVariables;
std::vector<QFunction::Upvalue> upvalues;
std::vector<Loop> loops;
std::vector<QV> constants;
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
int writeOpJumpBackTo  (QOpCode op, int pos) { return writeOpJump(op, writePosition() -pos + sizeof(uint_jump_offset_t) +1); }
void patchJump (int pos, int reach=-1) {
int curpos = out.tellp();
out.seekp(pos);
if (reach<0) reach = curpos;
write<uint_jump_offset_t>(reach -pos  - sizeof(uint_jump_offset_t));
out.seekp(curpos);
}
template<class T> void patch (int pos, const T& val) {
int curpos = out.tellp();
out.seekp(pos);
write<T>(val);
out.seekp(curpos);
}
void seek (int n) { out.seekp(n, std::ios_base::cur); }

void writeDebugLine (const QToken& tk) {
writeOpArg<int16_t>(OP_DEBUG_LINE, parser.getPositionOf(tk.start).first);
}

void pushLoop ();
void popLoop ();
void pushScope ();
void popScope ();
int countLocalVariablesInScope (int scope = -1);
int findLocalVariable (const QToken& name, int flags);
int findUpvalue (const QToken& name, int flags);
int findGlobalVariable (const QToken& name, int flags);
int findExportsVariable (bool createIfNotExist=true);
int findConstant (const QV& value);
int addUpvalue (int slot, bool upperUpvalue);

struct ClassDeclaration* getCurClass ();

template<class... A> void compileError (const QToken& token, const char* fmt, const A&... args);
void dump ();

QCompiler (QParser& p): vm(p.vm), parser(p), parent(nullptr), curClass(nullptr)  {}
void compile ();
QFunction* getFunction (int nArgs = 0);
};

#endif
