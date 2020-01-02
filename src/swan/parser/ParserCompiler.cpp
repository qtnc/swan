#include "Parser.hpp"
#include "Compiler.hpp"
#include "../vm/ExtraAlgorithms.hpp"
#include "../vm/VM.hpp"
#include "../vm/Upvalue.hpp"
#include "../vm/NatSort.hpp"
#include "../../include/cpprintf.hpp"
#include<cmath>
#include<cstdlib>
#include<limits>
#include<memory>
#include<algorithm>
#include<unordered_map>
#include<unordered_set>
#include<boost/algorithm/string.hpp>
#include<utf8.h>
using namespace std;

extern const char* OPCODE_NAMES[];
extern double strtod_c  (const char*, char** = nullptr);

static const char 
*THIS = "this",
*EXPORTS = "exports",
*FIBER = "Fiber",
*ASYNC = "async",
*CONSTRUCTOR = "constructor";

OpCodeInfo OPCODE_INFO[] = {
#define OP(name, stackEffect, nArgs, argFormat) { stackEffect, nArgs, argFormat }
#include "../vm/OpCodes.hpp"
#undef OP
};

static const char* TOKEN_NAMES[] = {
#define TOKEN(name) "T_" #name
#include "TokenTypes.hpp"
#undef TOKEN
};

double dlshift (double, double);
double drshift (double, double);
double dintdiv (double, double);

template<class T> static inline uint32_t utf8inc (T& it, T end) {
if (it!=end) utf8::next(it, end);
return it==end? 0 : utf8::peek_next(it, end);
}

template<class T> static inline uint32_t utf8peek (T it, T end) {
return it==end? 0 : utf8::peek_next(it, end);
}

static inline ostream& operator<< (ostream& out, const QToken& token) {
return out << string(token.start, token.length);
}

static int findName (vector<string>& names, const string& name, bool createIfNotExist) {
auto it = find(names.begin(), names.end(), name);
if (it!=names.end()) return it-names.begin();
else if (createIfNotExist) {
int n = names.size();
names.push_back(name);
return n;
}
else return -1;
}

static inline void writeOpCallFunction (QCompiler& compiler, uint8_t nArgs) {
if (nArgs<8) compiler.writeOp(static_cast<QOpCode>(OP_CALL_FUNCTION_0 + nArgs));
else compiler.writeOpArg<uint_local_index_t>(OP_CALL_FUNCTION, nArgs);
}

static inline void writeOpCallMethod (QCompiler& compiler, uint8_t nArgs, uint_method_symbol_t symbol) {
if (nArgs<8) compiler.writeOpArg<uint_method_symbol_t>(static_cast<QOpCode>(OP_CALL_METHOD_1 + nArgs), symbol);
else compiler.writeOpArgs<uint_method_symbol_t, uint_local_index_t>(OP_CALL_METHOD, symbol, nArgs+1);
}

static inline void writeOpCallSuper  (QCompiler& compiler, uint8_t nArgs, uint_method_symbol_t symbol) {
if (nArgs<8) compiler.writeOpArg<uint_method_symbol_t>(static_cast<QOpCode>(OP_CALL_SUPER_1 + nArgs), symbol);
else compiler.writeOpArgs<uint_method_symbol_t, uint_local_index_t>(OP_CALL_SUPER, symbol, nArgs+1);
}

static inline void writeOpLoadLocal (QCompiler& compiler, uint_local_index_t slot) {
if (slot<8) compiler.writeOp(static_cast<QOpCode>(OP_LOAD_LOCAL_0 + slot));
else compiler.writeOpArg<uint_local_index_t>(OP_LOAD_LOCAL, slot);
}

static inline void writeOpStoreLocal (QCompiler& compiler, uint_local_index_t slot) {
if (slot<8) compiler.writeOp(static_cast<QOpCode>(OP_STORE_LOCAL_0 + slot));
else compiler.writeOpArg<uint_local_index_t>(OP_STORE_LOCAL, slot);
}

static inline bool isUnpack (const shared_ptr<Expression>& expr);
static inline bool isComprehension (const shared_ptr<Expression>& expr);
static inline void doCompileTimeImport (QVM& vm, const string& baseFile, shared_ptr<Expression> exprRequestedFile);

void QCompiler::writeDebugLine (const QToken& tk) {
if (parser.vm.compileDbgInfo) writeOpArg<int16_t>(OP_DEBUG_LINE, parser.getPositionOf(tk.start).first);
}

int QCompiler::writeOpJumpBackTo  (QOpCode op, int pos) { 
int distance = writePosition() -pos + sizeof(uint_jump_offset_t) +1;
if (distance >= std::numeric_limits<uint_jump_offset_t>::max()) compileError(parser.cur, "Jump too long");
return writeOpJump(op, distance); 
}

void QCompiler::patchJump (int pos, int reach) {
int curpos = out.tellp();
out.seekp(pos);
if (reach<0) reach = curpos;
int distance = reach -pos  - sizeof(uint_jump_offset_t);
if (distance >= std::numeric_limits<uint_jump_offset_t>::max()) compileError(parser.cur, "Jump too long");
write<uint_jump_offset_t>(distance);
out.seekp(curpos);
}

struct TypeInfo: std::enable_shared_from_this<TypeInfo> {
static shared_ptr<TypeInfo> ANY, MANY;
virtual bool isEmpty () { return false; }
virtual bool isNum (QVM& vm) { return false; }
virtual bool isBool (QVM& vm) { return false; }
virtual shared_ptr<TypeInfo> resolve (QCompiler& compiler) { return shared_from_this(); }
virtual shared_ptr<TypeInfo> merge (shared_ptr<TypeInfo> t, QCompiler& compiler) = 0;
virtual string toString () = 0;
};

struct AnyTypeInfo: TypeInfo {
bool isEmpty () override { return true; }
shared_ptr<TypeInfo> merge (shared_ptr<TypeInfo> t, QCompiler& compiler) override { return t?t:shared_from_this(); }
virtual string toString () override { return "<any>"; }
};
shared_ptr<TypeInfo> TypeInfo::ANY = make_shared<AnyTypeInfo>();

struct ManyTypeInfo: TypeInfo {
virtual shared_ptr<TypeInfo> merge (shared_ptr<TypeInfo> t, QCompiler& compiler) override { return shared_from_this(); }
virtual string toString () override { return "<many>"; }
};
shared_ptr<TypeInfo> TypeInfo::MANY = make_shared<ManyTypeInfo>();

struct ClassTypeInfo: TypeInfo {
QClass* type;
ClassTypeInfo (QClass* cls): type(cls) {}
virtual bool isNum (QVM& vm) override { return type==vm.numClass; }
virtual bool isBool (QVM& vm) override { return type==vm.boolClass; }
virtual string toString () override { return type->name.c_str(); }
shared_ptr<TypeInfo> merge (shared_ptr<TypeInfo> t0, QCompiler& compiler) override {
if (!t0 || t0->isEmpty()) return shared_from_this();
auto t = dynamic_pointer_cast<ClassTypeInfo>(t0);
if (!t) return TypeInfo::MANY;
if (t->type==type) return shared_from_this();
QClass* cls = findCommonParent(type, t->type);
if (cls) return make_shared<ClassTypeInfo>(cls);
else return TypeInfo::MANY;
}
QClass* findCommonParent (QClass* t1, QClass* t2) {
if (t1==t2) return t1;
for (auto p1=t1; p1; p1=p1->parent) {
for (auto p2=t2; p2; p2=p2->parent) {
if (p1==p2) return p1;
}}
return nullptr;
}};

struct NamedTypeInfo: TypeInfo {
QToken token;
NamedTypeInfo (const QToken& t): token(t) {}
shared_ptr<TypeInfo> resolve (QCompiler& compiler);
string toString () override { return string(token.start, token.length); }
shared_ptr<TypeInfo> merge (shared_ptr<TypeInfo> t, QCompiler& compiler) override { return shared_from_this(); }
};

struct Statement: std::enable_shared_from_this<Statement>  {
inline shared_ptr<Statement> shared_this () { return shared_from_this(); }
virtual const QToken& nearestToken () = 0;
virtual bool isExpression () { return false; }
virtual bool isDecorable () { return false; }
virtual bool isUsingExports () { return false; }
virtual shared_ptr<Statement> optimizeStatement () { return shared_this(); }
virtual void compile (QCompiler& compiler) {}
};

struct Expression: Statement {
shared_ptr<TypeInfo> type;
Expression(): type(TypeInfo::ANY) {}
bool isExpression () final override { return true; }
inline shared_ptr<Expression> shared_this () { return static_pointer_cast<Expression>(shared_from_this()); }
virtual shared_ptr<Expression> optimize () { return shared_this(); }
shared_ptr<Statement> optimizeStatement () override { return optimize(); }
virtual shared_ptr<TypeInfo> computeType (QCompiler& compiler) { 
return TypeInfo::MANY;
}
virtual shared_ptr<TypeInfo> getType (QCompiler& compiler) { 
if (!type || type->isEmpty()) type = computeType(compiler);
return type;
}
};

struct Assignable {
virtual void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) = 0;
virtual bool isAssignable () { return true; }
};

struct Decorable {
vector<shared_ptr<Expression>> decorations;
virtual bool isDecorable () { return true; }
};

struct Comprenable {
virtual void chain (const shared_ptr<Statement>& sta) = 0;
};

struct Functionnable {
virtual void makeFunctionParameters (vector<shared_ptr<struct Variable>>& params) = 0;
virtual bool isFunctionnable () = 0;
};

struct ConstantExpression: Expression {
QToken token;
ConstantExpression(QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override {
QV& value = token.value;
if (value.isUndefined()) compiler.writeOp(OP_LOAD_UNDEFINED);
else if (value.isNull()) compiler.writeOp(OP_LOAD_NULL);
else if (value.isFalse()) compiler.writeOp(OP_LOAD_FALSE);
else if (value.isTrue()) compiler.writeOp(OP_LOAD_TRUE);
else if (value.isInt8()) compiler.writeOpArg<int8_t>(OP_LOAD_INT8, static_cast<int>(value.d));
else compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(token.value));
}
shared_ptr<TypeInfo> computeType (QCompiler& compiler) override {  return make_shared<ClassTypeInfo>(&token.value.getClass(compiler.parser.vm));  }
};

struct DupExpression: Expression  {
QToken token;
DupExpression(QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override {
compiler.writeOp(OP_DUP);
}};

struct LiteralSequenceExpression: Expression, Assignable {
QToken type;
vector<shared_ptr<Expression>> items;
LiteralSequenceExpression (const QToken& t, const vector<shared_ptr<Expression>>& p = {}): type(t), items(p) {}
const QToken& nearestToken () { return type; }
shared_ptr<Expression> optimize () { for (auto& item: items) item = item->optimize(); return shared_this(); }
bool isVararg () { return any_of(items.begin(), items.end(), isUnpack); }
bool isSingleSequence ();
virtual bool isAssignable () override;
virtual void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) override;
};

struct LiteralListExpression: LiteralSequenceExpression, Functionnable {
LiteralListExpression (const QToken& t): LiteralSequenceExpression(t) {}
virtual void makeFunctionParameters (vector<shared_ptr<struct Variable>>& params) override;
virtual bool isFunctionnable () override;
virtual void compile (QCompiler& compiler) override {
compiler.writeDebugLine(nearestToken());
int listSymbol = compiler.vm.findGlobalSymbol(("List"), LV_EXISTING | LV_FOR_READ);
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, listSymbol);
items[0]->compile(compiler);
writeOpCallFunction(compiler, 1);
} else {
bool vararg = isVararg();
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, listSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else writeOpCallMethod(compiler, items.size(), ofSymbol);
}}
};

struct LiteralSetExpression: LiteralSequenceExpression {
LiteralSetExpression (const QToken& t): LiteralSequenceExpression(t) {}
void compile (QCompiler& compiler) override {
int setSymbol = compiler.vm.findGlobalSymbol(("Set"), LV_EXISTING | LV_FOR_READ);
compiler.writeDebugLine(nearestToken());
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, setSymbol);
items[0]->compile(compiler);
writeOpCallFunction(compiler, 1);
} else {
bool vararg = isVararg();
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, setSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else writeOpCallMethod(compiler, items.size(), ofSymbol);
}}
};

struct LiteralMapExpression: Expression, Assignable, Functionnable {
QToken type;
vector<pair<shared_ptr<Expression>, shared_ptr<Expression>>> items;
LiteralMapExpression (const QToken& t): type(t) {}
const QToken& nearestToken () override { return type; }
shared_ptr<Expression> optimize () override { for (auto& p: items) { p.first = p.first->optimize(); p.second = p.second->optimize(); } return shared_this(); }
virtual void makeFunctionParameters (vector<shared_ptr<struct Variable>>& params) override;
virtual bool isFunctionnable () override;
virtual bool isAssignable () override;
virtual void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) override;
void compile (QCompiler& compiler) override {
vector<shared_ptr<Expression>> unpacks;
int mapSymbol = compiler.vm.findGlobalSymbol(("Map"), LV_EXISTING | LV_FOR_READ);
int subscriptSetterSymbol = compiler.vm.findMethodSymbol(("[]="));
int callSymbol = compiler.vm.findMethodSymbol(("()"));
compiler.writeDebugLine(nearestToken());
if (items.size()==1 && isComprehension(items[0].first)) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, mapSymbol);
items[0].first->compile(compiler);
writeOpCallFunction(compiler, 1);
return;
}
for (auto it = items.begin(); it!=items.end(); ) {
auto expr = it->first;
if (isUnpack(expr)) {
unpacks.push_back(expr);
it = items.erase(it);
}
else ++it;
}
bool vararg = !unpacks.empty();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, mapSymbol);
for (auto item: unpacks) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else compiler.writeOp(OP_CALL_FUNCTION_0);
for (auto item: items) {
compiler.writeOp(OP_DUP);
compiler.writeDebugLine(item.first->nearestToken());
item.first->compile(compiler);
compiler.writeDebugLine(item.second->nearestToken());
item.second->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_3, subscriptSetterSymbol);
compiler.writeOp(OP_POP);
}}
};

struct LiteralTupleExpression: LiteralSequenceExpression, Functionnable {
LiteralTupleExpression (const QToken& t, const vector<shared_ptr<Expression>>& p = {}): LiteralSequenceExpression(t, p) {}
virtual void makeFunctionParameters (vector<shared_ptr<struct Variable>>& params) override;
virtual bool isFunctionnable () override;
virtual void compile (QCompiler& compiler) override {
int tupleSymbol = compiler.vm.findGlobalSymbol(("Tuple"), LV_EXISTING | LV_FOR_READ);
compiler.writeDebugLine(nearestToken());
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, tupleSymbol);
items[0]->compile(compiler);
writeOpCallFunction(compiler, 1);
} else {
bool vararg = any_of(items.begin(), items.end(), isUnpack);
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, tupleSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else writeOpCallMethod(compiler, items.size(), ofSymbol);
}}
};

struct LiteralGridExpression: Expression {
QToken token;
vector<vector<shared_ptr<Expression>>> data;
LiteralGridExpression (const QToken& t, const vector<vector<shared_ptr<Expression>>>& v): token(t), data(v) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override {
int gridSymbol = compiler.vm.findGlobalSymbol(("Grid"), LV_EXISTING | LV_FOR_READ);
int size = data.size() * data[0].size();
int argLimit = std::numeric_limits<uint_local_index_t>::max() -5;
compiler.writeDebugLine(nearestToken());
if (size>= argLimit) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, gridSymbol);
compiler.writeOpArg<uint8_t>(OP_LOAD_INT8, data[0].size());
compiler.writeOpArg<uint8_t>(OP_LOAD_INT8, data.size());
for (auto& row: data) {
for (auto& value: row) {
value->compile(compiler);
}}
if (size>argLimit) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else writeOpCallFunction(compiler, size+2);
}
};

struct LiteralRegexExpression: Expression {
QToken tok;
string pattern, options;
LiteralRegexExpression(const QToken& tk, const string& p, const string& o): tok(tk), pattern(p), options(o) {}
const QToken& nearestToken () override { return tok; }
virtual shared_ptr<TypeInfo>  computeType (QCompiler& compiler) override { return make_shared<ClassTypeInfo>(compiler.parser.vm.regexClass); }
void compile (QCompiler& compiler) override {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "Regex", 5, QV::UNDEFINED }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, pattern), QV_TAG_STRING)));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, options), QV_TAG_STRING)));
compiler.writeOp(OP_CALL_FUNCTION_2);
}
};

struct NameExpression: Expression, Assignable, Functionnable {
QToken token;
NameExpression (QToken x): token(x) {}
virtual const QToken& nearestToken () override { return token; }
virtual shared_ptr<TypeInfo> computeType (QCompiler& compiler) override;
virtual void compile (QCompiler& compiler) override ;
virtual void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue)override ;
virtual void makeFunctionParameters (vector<shared_ptr<Variable>>& params) override;
virtual bool isFunctionnable () override { return true; }
};

struct FieldExpression: Expression, Assignable {
QToken token;
FieldExpression (QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue)override ;
};

struct StaticFieldExpression: Expression, Assignable {
QToken token;
StaticFieldExpression (QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue)override ;
};

struct SuperExpression: Expression {
QToken superToken;
SuperExpression (const QToken& t): superToken(t) {}
const QToken& nearestToken () override { return superToken; }
void compile (QCompiler& compiler)  override;
};

struct AnonymousLocalExpression: Expression, Assignable  {
QToken token;
AnonymousLocalExpression (QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler)override  { writeOpLoadLocal(compiler, token.value.d); }
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue)override  { assignedValue->compile(compiler); writeOpStoreLocal(compiler, token.value.d); }
};

struct GenericMethodSymbolExpression: Expression {
QToken token;
GenericMethodSymbolExpression (const QToken& t): token(t) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override {
int symbol = compiler.parser.vm.findMethodSymbol(string(token.start, token.length));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(symbol | QV_TAG_GENERIC_SYMBOL_FUNCTION)));
}};

struct BinaryOperation: Expression {
shared_ptr<Expression> left, right;
QTokenType op;
BinaryOperation (shared_ptr<Expression> l, QTokenType o, shared_ptr<Expression> r): left(l), right(r), op(o)  {}
bool isComparison () { return op>=T_LT && op<=T_GTE; }
const QToken& nearestToken () override { return left->nearestToken(); }
shared_ptr<Expression> optimize ()override ;
shared_ptr<Expression> optimizeChainedComparisons ();
void compile (QCompiler& compiler)override ;
virtual shared_ptr<TypeInfo>  computeType (QCompiler& compiler) override;
};

struct UnaryOperation: Expression {
shared_ptr<Expression> expr;
QTokenType op;
UnaryOperation  (QTokenType op0, shared_ptr<Expression> e0): op(op0), expr(e0) {}
shared_ptr<Expression> optimize ()override ;
void compile (QCompiler& compiler)override ;
const QToken& nearestToken () override { return expr->nearestToken(); }
virtual shared_ptr<TypeInfo>  computeType (QCompiler& compiler) override;
};

struct ShortCircuitingBinaryOperation: BinaryOperation {
ShortCircuitingBinaryOperation (shared_ptr<Expression> l, QTokenType o, shared_ptr<Expression> r): BinaryOperation(l,o,r) {}
void compile (QCompiler& compiler)override ;
};

struct ConditionalExpression: Expression {
shared_ptr<Expression> condition, ifPart, elsePart;
ConditionalExpression (shared_ptr<Expression> cond, shared_ptr<Expression> ifp, shared_ptr<Expression> ep): condition(cond), ifPart(ifp), elsePart(ep) {}
const QToken& nearestToken () override { return condition->nearestToken(); }
shared_ptr<Expression> optimize () override { 
condition=condition->optimize(); 
ifPart=ifPart->optimize(); 
elsePart=elsePart->optimize(); 
if (auto cst = dynamic_pointer_cast<ConstantExpression>(condition)) {
if (cst->token.value.isFalsy()) return elsePart;
else return ifPart;
}
return shared_this(); 
}
virtual shared_ptr<TypeInfo>  computeType (QCompiler& compiler) override { 
if (!elsePart) return ifPart->getType(compiler);
auto tp1 = ifPart->getType(compiler), tp2 = elsePart->getType(compiler);
return tp1->merge(tp2, compiler);
}
void compile (QCompiler& compiler)override ;
};

struct SwitchExpression: Expression {
shared_ptr<Expression> expr, var;
vector<pair<vector<shared_ptr<Expression>>, shared_ptr<Expression>>> cases;
shared_ptr<Expression> defaultCase;
const QToken& nearestToken () override { return expr->nearestToken(); }
shared_ptr<Expression> optimize () override {
expr=expr->optimize();
if (var) var = var->optimize();
if (defaultCase) defaultCase=defaultCase->optimize();
for (auto& c: cases) {
for (auto& i: c.first) i=i->optimize();
c.second=c.second->optimize();
}
return shared_this();
}
virtual shared_ptr<TypeInfo>  computeType (QCompiler& compiler) override { 
shared_ptr<TypeInfo> type = TypeInfo::ANY;
if (defaultCase) type = type->merge(defaultCase->getType(compiler), compiler);
for (auto& p: cases) type = type->merge(p.second->getType(compiler), compiler);
return type;
}
void compile (QCompiler& compiler)override ;
};

struct ComprehensionExpression: Expression {
shared_ptr<Statement> rootStatement;
shared_ptr<Expression> loopExpression;
ComprehensionExpression (const shared_ptr<Statement>& rs, const shared_ptr<Expression>& le): rootStatement(rs), loopExpression(le)  {}
shared_ptr<Expression> optimize ()override ;
const QToken& nearestToken () override { return loopExpression->nearestToken(); }
void compile (QCompiler&)override ;
};

struct UnpackExpression: Expression {
shared_ptr<Expression> expr;
UnpackExpression   (shared_ptr<Expression> e0): expr(e0) {}
shared_ptr<Expression> optimize () override { expr=expr->optimize(); return shared_this(); }
void compile (QCompiler& compiler) override {
expr->compile(compiler);
compiler.writeOp(OP_UNPACK_SEQUENCE);
}
const QToken& nearestToken () override { return expr->nearestToken(); }
};

struct TypeHintExpression: Expression {
shared_ptr<Expression> expr;
shared_ptr<TypeInfo> type;
TypeHintExpression (shared_ptr<Expression> e, shared_ptr<TypeInfo> t): expr(e), type(t) {}
const QToken& nearestToken () override { return expr->nearestToken(); }
void compile (QCompiler& compiler)  override { 
//todo: actually exploit the type hint
//int pos = compiler.out.tellp();
//compiler.out.seekp(pos);
expr->compile(compiler); 
} 
shared_ptr<TypeInfo> computeType (QCompiler& compiler) override { return type->resolve(compiler); }
};

struct AbstractCallExpression: Expression {
shared_ptr<Expression> receiver;
QTokenType type;
std::vector<shared_ptr<Expression>> args;
AbstractCallExpression (shared_ptr<Expression> recv0, QTokenType tp, const std::vector<shared_ptr<Expression>>& args0): receiver(recv0), type(tp), args(args0) {}
const QToken& nearestToken () override { return receiver->nearestToken(); }
shared_ptr<Expression> optimize () override { receiver=receiver->optimize(); for (auto& arg: args) arg=arg->optimize(); return shared_this(); }
bool isVararg () { return isUnpack(receiver) || any_of(args.begin(), args.end(), isUnpack); }
void compileArgs (QCompiler& compiler) {
for (auto arg: args) arg->compile(compiler);
}
};

struct CallExpression: AbstractCallExpression {
CallExpression (shared_ptr<Expression> recv0, const std::vector<shared_ptr<Expression>>& args0): AbstractCallExpression(recv0, T_LEFT_PAREN, args0) {}
void compile (QCompiler& compiler)override ;
};

struct SubscriptExpression: AbstractCallExpression, Assignable  {
SubscriptExpression (shared_ptr<Expression> recv0, const std::vector<shared_ptr<Expression>>& args0): AbstractCallExpression(recv0, T_LEFT_BRACKET, args0) {}
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue)override ;
};

struct MemberLookupOperation: BinaryOperation, Assignable  {
MemberLookupOperation (shared_ptr<Expression> l, shared_ptr<Expression> r): BinaryOperation(l, T_DOT, r) {}
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue)override ;
};

struct MethodLookupOperation: BinaryOperation, Assignable  {
MethodLookupOperation (shared_ptr<Expression> l, shared_ptr<Expression> r): BinaryOperation(l, T_COLONCOLON, r) {}
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue)override ;
};

struct AssignmentOperation: BinaryOperation {
bool optimized;
AssignmentOperation (shared_ptr<Expression> l, QTokenType o, shared_ptr<Expression> r): BinaryOperation(l,o,r), optimized(false)  {}
shared_ptr<Expression> optimize ()override ;
void compile (QCompiler& compiler)override ;
};

struct YieldExpression: Expression {
QToken token;
shared_ptr<Expression> expr;
YieldExpression (const QToken& tk, shared_ptr<Expression> e): token(tk), expr(e) {}
const QToken& nearestToken () override { return token; }
shared_ptr<Expression> optimize () override { if (expr) expr=expr->optimize(); return shared_this(); }
void compile (QCompiler& compiler) override {
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
compiler.writeOp(OP_YIELD);
}
};

struct ImportExpression: Expression {
shared_ptr<Expression> from;
ImportExpression (shared_ptr<Expression> f): from(f) {}
shared_ptr<Expression> optimize () override { from=from->optimize(); return shared_this(); }
const QToken& nearestToken () override { return from->nearestToken(); }
void compile (QCompiler& compiler) override {
doCompileTimeImport(compiler.parser.vm, compiler.parser.filename, from);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "import", 6, QV::UNDEFINED }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(compiler.parser.vm, compiler.parser.filename)));
from->compile(compiler);
compiler.writeOp(OP_CALL_FUNCTION_2);
}};

struct Variable {
shared_ptr<Expression> name, value;
shared_ptr<TypeInfo> typeHint;
vector<shared_ptr<Expression>> decorations;
int flags;
Variable (const shared_ptr<Expression>& nm, const shared_ptr<Expression>& val = nullptr, int flgs = 0, const vector<shared_ptr<Expression>>& decos = {}):
name(nm), value(val), flags(flgs), decorations(decos), typeHint(nullptr) {}
void optimize () {
name = name->optimize();
if (value) value = value->optimize();
for (auto& d: decorations) d = d->optimize();
}};

struct SimpleStatement: Statement {
QToken token;
SimpleStatement (const QToken& t): token(t) {}
const QToken& nearestToken () override { return token; }
};

struct IfStatement: Statement, Comprenable   {
shared_ptr<Expression> condition;
shared_ptr<Statement> ifPart, elsePart;
QOpCode jumpType;
IfStatement (shared_ptr<Expression> cond, shared_ptr<Statement> ifp=nullptr, shared_ptr<Statement> ep = nullptr, QOpCode jt = OP_JUMP_IF_FALSY): condition(cond), ifPart(ifp), elsePart(ep), jumpType(jt) {}
void chain (const shared_ptr<Statement>& st) final override { 
if (!ifPart) ifPart=st; 
else if (!elsePart) elsePart = st;
}
shared_ptr<Statement> optimizeStatement () override { 
condition=condition->optimize(); 
ifPart=ifPart->optimizeStatement(); 
if (elsePart) elsePart=elsePart->optimizeStatement(); 
if (auto u = dynamic_pointer_cast<UnaryOperation>(condition)) if (u->op==T_EXCL) { jumpType=OP_JUMP_IF_TRUTY; condition = u->expr; }
if (auto c = dynamic_pointer_cast<ConstantExpression>(condition)) {
auto singlePart = c->token.value.isFalsy()? elsePart : ifPart;
if (!singlePart) singlePart = make_shared<SimpleStatement>(c->token);
return singlePart;
}
return shared_this(); 
}
const QToken& nearestToken () override { return condition->nearestToken(); }
void compile (QCompiler& compiler) {
compiler.writeDebugLine(condition->nearestToken());
condition->compile(compiler);
int skipIfJump = compiler.writeOpJump(jumpType);
compiler.pushScope();
compiler.writeDebugLine(ifPart->nearestToken());
ifPart->compile(compiler);
if (ifPart->isExpression()) compiler.writeOp(OP_POP);
compiler.lastOp = OP_LOAD_UNDEFINED;
compiler.popScope();
if (elsePart) {
int skipElseJump = compiler.writeOpJump(OP_JUMP);
compiler.patchJump(skipIfJump);
compiler.pushScope();
compiler.writeDebugLine(elsePart->nearestToken());
elsePart->compile(compiler);
if (elsePart->isExpression()) compiler.writeOp(OP_POP);
compiler.lastOp = OP_LOAD_UNDEFINED;
compiler.popScope();
compiler.patchJump(skipElseJump);
}
else {
compiler.patchJump(skipIfJump);
}}
};

struct SwitchStatement: Statement {
shared_ptr<Expression> expr, var;
vector<pair<shared_ptr<Expression>, vector<shared_ptr<Statement>>>> cases;
vector<shared_ptr<Statement>> defaultCase;
shared_ptr<Statement> optimizeStatement () override { 
expr = expr->optimize();
for (auto& p: cases) { 
p.first = p.first->optimize(); 
for (auto& s: p.second) s = s->optimizeStatement();
}
for (auto& s: defaultCase) s=s->optimizeStatement();
return shared_this(); 
}
const QToken& nearestToken () override { return expr->nearestToken(); }
void compile (QCompiler& compiler) override;
};

struct ForStatement: Statement, Comprenable  {
QToken token;
vector<shared_ptr<Variable>> loopVariables;
shared_ptr<Expression> inExpression, incrExpression;
shared_ptr<Statement> loopStatement;
bool traditional;
ForStatement (const QToken& tk): token(tk), loopVariables(), inExpression(nullptr), loopStatement(nullptr), incrExpression(nullptr), traditional(false)   {}
void chain (const shared_ptr<Statement>& st) final override { loopStatement=st; }
shared_ptr<Statement> optimizeStatement () override { 
if (inExpression) inExpression=inExpression->optimize(); 
if (loopStatement) loopStatement=loopStatement->optimizeStatement();
if (incrExpression) incrExpression=incrExpression->optimize();
for (auto& lv: loopVariables) lv->optimize();
return shared_this(); 
}
const QToken& nearestToken () override { return token; }
void parseHead (QParser& parser);
void compile (QCompiler& compiler)override ;
void compileForEach (QCompiler& compiler);
void compileTraditional (QCompiler& compiler);
};

struct WhileStatement: Statement {
shared_ptr<Expression> condition;
shared_ptr<Statement> loopStatement;
WhileStatement (shared_ptr<Expression> cond, shared_ptr<Statement> lst): condition(cond), loopStatement(lst) {}
shared_ptr<Statement> optimizeStatement () override { 
condition=condition->optimize(); 
loopStatement=loopStatement->optimizeStatement(); 
if (auto cst = dynamic_pointer_cast<ConstantExpression>(condition)) { if (cst->token.value.isFalsy()) return make_shared<SimpleStatement>(nearestToken()); }
return shared_this(); 
}
const QToken& nearestToken () override { return condition->nearestToken(); }
void compile (QCompiler& compiler) override {
compiler.writeDebugLine(condition->nearestToken());
compiler.pushLoop();
compiler.pushScope();
int loopStart = compiler.writePosition();
compiler.loops.back().condPos = compiler.writePosition();
condition->compile(compiler);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_FALSY) });
compiler.writeDebugLine(loopStatement->nearestToken());
loopStatement->compile(compiler);
if (loopStatement->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
compiler.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
}
};

struct RepeatWhileStatement: Statement {
shared_ptr<Expression> condition;
shared_ptr<Statement> loopStatement;
RepeatWhileStatement (shared_ptr<Expression> cond, shared_ptr<Statement> lst): condition(cond), loopStatement(lst) {}
shared_ptr<Statement> optimizeStatement () override { condition=condition->optimize(); loopStatement=loopStatement->optimizeStatement(); return shared_this(); }
const QToken& nearestToken () override { return loopStatement->nearestToken(); }
void compile (QCompiler& compiler) override {
compiler.writeDebugLine(loopStatement->nearestToken());
compiler.pushLoop();
compiler.pushScope();
int loopStart = compiler.writePosition();
loopStatement->compile(compiler);
if (loopStatement->isExpression()) compiler.writeOp(OP_POP);
compiler.loops.back().condPos = compiler.writePosition();
compiler.writeDebugLine(condition->nearestToken());
condition->compile(compiler);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_FALSY) });
compiler.popScope();
compiler.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
}
};

struct ContinueStatement: SimpleStatement {
int count;
ContinueStatement (const QToken& tk, int n = 1): SimpleStatement(tk), count(n) {}
void compile (QCompiler& compiler) override {
if (compiler.loops.empty()) compiler.compileError(token, ("Can't use 'continue' outside of a loop"));
else if (count>compiler.loops.size()) compiler.compileError(token, ("Can't continue on that many loops"));
else {
compiler.writeDebugLine(nearestToken());
Loop& loop = *(compiler.loops.end() -count);
int varCount = compiler.countLocalVariablesInScope(loop.scope);
if (varCount>0) compiler.writeOpArg<uint_local_index_t>(OP_POP_SCOPE, varCount);
if (loop.condPos>=0) compiler.writeOpJumpBackTo(OP_JUMP_BACK, loop.condPos);
else loop.jumpsToPatch.push_back({ Loop::CONDITION, compiler.writeOpJump(OP_JUMP) });
}}};

struct BreakStatement: SimpleStatement {
int count;
BreakStatement (const QToken& tk, int n=1): SimpleStatement(tk), count(n) {}
void compile (QCompiler& compiler) override {
if (compiler.loops.empty()) compiler.compileError(token, ("Can't use 'break' outside of a loop"));
else if (count>compiler.loops.size()) compiler.compileError(token, ("Can't break that many loops"));
else {
compiler.writeDebugLine(nearestToken());
Loop& loop = *(compiler.loops.end() -count);
int varCount = compiler.countLocalVariablesInScope(loop.scope);
if (varCount>0) compiler.writeOpArg<uint_local_index_t>(OP_POP_SCOPE, varCount);
loop.jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP) });
}}};

struct ReturnStatement: Statement {
QToken returnToken;
shared_ptr<Expression> expr;
ReturnStatement (const QToken& retk, shared_ptr<Expression> e0 = nullptr): returnToken(retk), expr(e0) {}
const QToken& nearestToken () override { return expr? expr->nearestToken() : returnToken; }
shared_ptr<Statement> optimizeStatement () override { if (expr) expr=expr->optimize(); return shared_this(); }
void compile (QCompiler& compiler) override {
compiler.writeDebugLine(nearestToken());
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
compiler.writeOp(OP_RETURN);
}
};

struct ThrowStatement: Statement {
QToken returnToken;
shared_ptr<Expression> expr;
ThrowStatement (const QToken& retk, shared_ptr<Expression> e0): returnToken(retk), expr(e0) {}
const QToken& nearestToken () override { return expr? expr->nearestToken() : returnToken; }
shared_ptr<Statement> optimizeStatement () override { if (expr) expr=expr->optimize(); return shared_this(); }
void compile (QCompiler& compiler) override {
compiler.writeDebugLine(nearestToken());
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
compiler.writeOp(OP_THROW);
}
};

struct TryStatement: Statement, Comprenable  {
shared_ptr<Statement> tryPart, catchPart, finallyPart;
QToken catchVar;
TryStatement (shared_ptr<Statement> tp, shared_ptr<Statement> cp, shared_ptr<Statement> fp, const QToken& cv): tryPart(tp), catchPart(cp), finallyPart(fp), catchVar(cv)  {}
const QToken& nearestToken () override { return tryPart->nearestToken(); }
void chain (const shared_ptr<Statement>& st) final override { tryPart=st; }
shared_ptr<Statement> optimizeStatement () override { 
tryPart = tryPart->optimizeStatement();
if (catchPart) catchPart=catchPart->optimizeStatement();
if (finallyPart) finallyPart = finallyPart->optimizeStatement();
return shared_this(); }
void compile (QCompiler& compiler) override {
compiler.pushScope();
int jumpPos=-1, tryPos = compiler.writeOpArg<uint64_t>(OP_TRY, 0xFFFFFFFFFFFFFFFFULL);
tryPart->compile(compiler);
if (tryPart->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
if (catchPart) {
jumpPos = compiler.writeOpJump(OP_JUMP);
uint32_t curPos = compiler.out.tellp();
compiler.out.seekp(tryPos);
compiler.out.write(reinterpret_cast<const char*>(&curPos), sizeof(uint32_t));
compiler.out.seekp(curPos);
compiler.pushScope();
compiler.findLocalVariable(catchVar, LV_NEW);
catchPart->compile(compiler);
if (catchPart->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
}
if (jumpPos>=0) compiler.patchJump(jumpPos);
uint32_t curPos = compiler.out.tellp();
compiler.out.seekp(tryPos+sizeof(uint32_t));
compiler.out.write(reinterpret_cast<const char*>(&curPos), sizeof(uint32_t));
compiler.out.seekp(curPos);
if (finallyPart) {
compiler.pushScope();
finallyPart->compile(compiler);
if (finallyPart->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
}
compiler.writeOp(OP_END_FINALLY);
}
};

struct BlockStatement: Statement, Comprenable  {
vector<shared_ptr<Statement>> statements;
bool makeScope, optimized;
BlockStatement (const vector<shared_ptr<Statement>>& sta = {}, bool s = true): statements(sta), makeScope(s), optimized(false)   {}
void chain (const shared_ptr<Statement>& st) final override { statements.push_back(st); }
shared_ptr<Statement> optimizeStatement () override;
void doHoisting ();
const QToken& nearestToken () override { return statements[0]->nearestToken(); }
bool isUsingExports () override { return any_of(statements.begin(), statements.end(), [&](auto s){ return s && s->isUsingExports(); }); }
void compile (QCompiler& compiler) override {
if (makeScope) compiler.pushScope();
for (auto sta: statements) {
compiler.writeDebugLine(sta->nearestToken());
sta->compile(compiler);
if (sta->isExpression()) compiler.writeOp(OP_POP);
}
if (makeScope) compiler.popScope();
}
};

struct VariableDeclaration: Statement, Decorable {
vector<shared_ptr<Variable>> vars;
VariableDeclaration (const vector<shared_ptr<Variable>>& v = {}): vars(v) {}
const QToken& nearestToken () override { return vars[0]->name->nearestToken(); }
bool isDecorable () override { return true; }
shared_ptr<Statement> optimizeStatement () override { 
for (auto& v: vars) v->optimize();
return shared_this(); 
}
void compile (QCompiler& compiler)override ;
};

struct ExportDeclaration: Statement  {
vector<pair<QToken,shared_ptr<Expression>>> exports;
const QToken& nearestToken () override { return exports[0].first; }
shared_ptr<Statement> optimizeStatement () override { for (auto& v: exports) v.second=v.second->optimize(); return shared_this(); }
bool isUsingExports () override { return true; }
void compile (QCompiler& compiler)override ;
};

struct ImportDeclaration: Statement {
shared_ptr<Expression> from;
vector<shared_ptr<Variable>> imports;
shared_ptr<Statement> optimizeStatement () override { from=from->optimize(); return shared_this(); }
const QToken& nearestToken () override { return from->nearestToken(); }
void compile (QCompiler& compiler) override;
};

struct FunctionDeclaration: Expression, Decorable {
QToken name;
vector<shared_ptr<Variable>> params;
shared_ptr<Statement> body;
shared_ptr<TypeInfo> returnTypeHint;
int flags;
FunctionDeclaration (const QToken& nm, int fl = 0, const vector<shared_ptr<Variable>>& fp = {}, shared_ptr<Statement> b = nullptr): name(nm), params(fp), body(b), returnTypeHint(nullptr), flags(fl)     {}
const QToken& nearestToken () override { return name; }
void compileParams (QCompiler& compiler);
QFunction* compileFunction (QCompiler& compiler);
void compile (QCompiler& compiler) override { compileFunction(compiler); }
shared_ptr<Statement> optimizeStatement () override { 
body=body->optimizeStatement(); 
for (auto& param: params) param->optimize();
return shared_this(); 
}
virtual bool isDecorable () override { return true; }
};

struct ClassDeclaration: Expression, Decorable  {
struct Field {
int index;
QToken token;
shared_ptr<Expression> defaultValue;
shared_ptr<TypeInfo> type;
};
QToken name;
int flags;
vector<QToken> parents;
vector<shared_ptr<FunctionDeclaration>> methods;
unordered_map<string, Field> fields, staticFields;

ClassDeclaration (const QToken& name0, int flgs): name(name0), flags(flgs)  {}
int findField (unordered_map<string,Field>& flds, const QToken& name, shared_ptr<TypeInfo>** type = nullptr) {
auto it = flds.find(string(name.start, name.length));
int index = -1;
if (it!=flds.end()) {
index = it->second.index;
if (type) *type = &(it->second.type);
}
else {
index = flds.size();
flds[string(name.start, name.length)] = { index, name, nullptr, nullptr };
if (type) *type = &(flds[string(name.start, name.length)].type);
}
return index;
}
inline int findField (const QToken& name, shared_ptr<TypeInfo>** type = nullptr) {  return findField(fields, name, type); }
inline int findStaticField (const QToken& name, shared_ptr<TypeInfo>** type = nullptr) { return findField(staticFields, name, type); }
shared_ptr<FunctionDeclaration> findMethod (const QToken& name, bool isStatic);
void handleAutoConstructor (QCompiler& compiler, unordered_map<string,Field>& memberFields, bool isStatic);
const QToken& nearestToken () override { return name; }
shared_ptr<Expression> optimize () override { for (auto& m: methods) m=static_pointer_cast<FunctionDeclaration>(m->optimize()); return shared_this(); }
void compile (QCompiler&)override ;
virtual bool isDecorable () override { return true; }
};

LocalVariable::LocalVariable (const QToken& n, int s, bool ic): 
name(n), scope(s), type(TypeInfo::ANY), hasUpvalues(false), isConst(ic) {}

bool LiteralSequenceExpression::isSingleSequence () {
if (items.size()!=1) return false;
auto expr = items[0];
if (isComprehension(expr)) return true;
auto binop = dynamic_pointer_cast<BinaryOperation>(expr);
return binop && (binop->op==T_DOTDOT || binop->op==T_DOTDOTDOT);
}

static inline bool isUnpack (const shared_ptr<Expression>& expr) {
return !!dynamic_pointer_cast<UnpackExpression>(expr);
}

static inline bool isComprehension  (const shared_ptr<Expression>& expr) {
return !!dynamic_pointer_cast<ComprehensionExpression>(expr);
}

static inline void doCompileTimeImport (QVM& vm, const string& baseFile, shared_ptr<Expression> exprRequestedFile) {
auto expr = dynamic_pointer_cast<ConstantExpression>(exprRequestedFile);
if (expr && expr->token.value.isString()) {
QFiber& f = vm.getActiveFiber();
f.import(baseFile, expr->token.value.asString());
f.pop();
}}

struct ParserRule {
typedef shared_ptr<Expression>(QParser::*PrefixFn)(void);
typedef shared_ptr<Expression>(QParser::*InfixFn)(shared_ptr<Expression> left);
typedef shared_ptr<Statement>(QParser::*StatementFn)(void);
typedef void(QParser::*MemberFn)(ClassDeclaration&,int);
PrefixFn prefix = nullptr;
InfixFn infix = nullptr;
StatementFn statement = nullptr;
MemberFn member = nullptr;
const char* prefixOpName = nullptr;
const char* infixOpName = nullptr;
uint8_t priority = P_HIGHEST;
uint8_t flags = 0;
};

static std::unordered_map<int,ParserRule> rules = {
#define PREFIX(token, prefix, name) { T_##token, { &QParser::parse##prefix, nullptr, nullptr, nullptr, (#name), nullptr, P_PREFIX, P_LEFT  }}
#define PREFIX_OP(token, prefix, name) { T_##token, { &QParser::parse##prefix, nullptr, nullptr, &QParser::parseMethodDecl, (#name), nullptr, P_PREFIX, P_LEFT }}
#define INFIX(token, infix, name, priority, direction) { T_##token, { nullptr, &QParser::parse##infix, nullptr, nullptr, nullptr, (#name), P_##priority, P_##direction }}
#define INFIX_OP(token, infix, name, priority, direction) { T_##token, { nullptr, &QParser::parse##infix, nullptr, &QParser::parseMethodDecl, nullptr,  (#name), P_##priority, P_##direction  }}
#define MULTIFIX(token, prefix, infix, prefixName, infixName, priority, direction) { T_##token, { &QParser::parse##prefix, &QParser::parse##infix, nullptr, nullptr, (#prefixName), (#infixName), P_##priority, P_##direction }}
#define OPERATOR(token, prefix, infix, prefixName, infixName, priority, direction) { T_##token, { &QParser::parse##prefix, &QParser::parse##infix, nullptr, &QParser::parseMethodDecl, (#prefixName), (#infixName), P_##priority, P_##direction  }}
#define STATEMENT(token, func) { T_##token, { nullptr, nullptr, &QParser::parse##func, nullptr, nullptr, nullptr, P_PREFIX, P_LEFT  }}

OPERATOR(PLUS, PrefixOp, InfixOp, unp, +, TERM, LEFT),
OPERATOR(MINUS, PrefixOp, InfixOp, unm, -, TERM, LEFT),
INFIX_OP(STAR, InfixOp, *, FACTOR, LEFT),
INFIX_OP(BACKSLASH, InfixOp, \\, FACTOR, LEFT),
INFIX_OP(PERCENT, InfixOp, %, FACTOR, LEFT),
INFIX_OP(STARSTAR, InfixOp, **, EXPONENT, LEFT),
INFIX_OP(AMP, InfixOp, &, BITWISE, LEFT),
INFIX_OP(CIRC, InfixOp, ^, BITWISE, LEFT),
INFIX_OP(LTLT, InfixOp, <<, BITWISE, LEFT),
INFIX_OP(GTGT, InfixOp, >>, BITWISE, LEFT),
INFIX_OP(DOTDOT, InfixOp, .., RANGE, LEFT),
INFIX(DOTDOTDOT, InfixOp, ..., RANGE, LEFT),
INFIX(AMPAMP, InfixOp, &&, LOGICAL, LEFT),
INFIX(BARBAR, InfixOp, ||, LOGICAL, LEFT),
INFIX(QUESTQUEST, InfixOp, ??, LOGICAL, LEFT),

INFIX(AS, InfixOp, as, ASSIGNMENT, RIGHT),
INFIX(EQ, InfixOp, =, ASSIGNMENT, RIGHT),
INFIX(PLUSEQ, InfixOp, +=, ASSIGNMENT, RIGHT),
INFIX(MINUSEQ, InfixOp, -=, ASSIGNMENT, RIGHT),
INFIX(STAREQ, InfixOp, *=, ASSIGNMENT, RIGHT),
INFIX(SLASHEQ, InfixOp, /=, ASSIGNMENT, RIGHT),
INFIX(BACKSLASHEQ, InfixOp, \\=, ASSIGNMENT, RIGHT),
INFIX(PERCENTEQ, InfixOp, %=, ASSIGNMENT, RIGHT),
INFIX(STARSTAREQ, InfixOp, **=, ASSIGNMENT, RIGHT),
INFIX(BAREQ, InfixOp, |=, ASSIGNMENT, RIGHT),
INFIX(AMPEQ, InfixOp, &=, ASSIGNMENT, RIGHT),
INFIX(CIRCEQ, InfixOp, ^=, ASSIGNMENT, RIGHT),
INFIX(ATEQ, InfixOp, @=, ASSIGNMENT, RIGHT),
INFIX(LTLTEQ, InfixOp, <<=, ASSIGNMENT, RIGHT),
INFIX(GTGTEQ, InfixOp, >>=, ASSIGNMENT, RIGHT),
INFIX(AMPAMPEQ, InfixOp, &&=, ASSIGNMENT, RIGHT),
INFIX(BARBAREQ, InfixOp, ||=, ASSIGNMENT, RIGHT),
INFIX(QUESTQUESTEQ, InfixOp, ?\x3F=, ASSIGNMENT, RIGHT),

INFIX_OP(EQEQ, InfixOp, ==, COMPARISON, LEFT),
INFIX_OP(EXCLEQ, InfixOp, !=, COMPARISON, LEFT),
OPERATOR(LT, LiteralSet, InfixOp, <, <, COMPARISON, LEFT),
INFIX_OP(GT, InfixOp, >, COMPARISON, LEFT),
INFIX_OP(LTE, InfixOp, <=, COMPARISON, LEFT),
INFIX_OP(GTE, InfixOp, >=, COMPARISON, LEFT),
INFIX_OP(IS, InfixIs, is, COMPARISON, SWAP_OPERANDS),
INFIX_OP(IN, InfixOp, in, COMPARISON, SWAP_OPERANDS),

OPERATOR(QUEST, PrefixOp, Conditional, ?, ?, CONDITIONAL, LEFT),
OPERATOR(EXCL, PrefixOp, InfixNot, !, !, COMPARISON, LEFT),
PREFIX_OP(TILDE, PrefixOp, ~),
//PREFIX(DOLLAR, Lambda, $),

#ifndef NO_REGEX
OPERATOR(SLASH, LiteralRegex, InfixOp, /, /, FACTOR, LEFT),
#else
INFIX_OP(SLASH, InfixOp, /, FACTOR, LEFT),
#endif
#ifndef NO_GRID
OPERATOR(BAR, LiteralGrid, InfixOp, |, |, BITWISE, LEFT),
#else
INFIX_OP(BAR, InfixOp, |, BITWISE, LEFT),
#endif

{ T_LEFT_PAREN, { &QParser::parseGroupOrTuple, &QParser::parseMethodCall, nullptr, &QParser::parseMethodDecl, nullptr, ("()"), P_CALL, P_LEFT }},
{ T_LEFT_BRACKET, { &QParser::parseLiteralList, &QParser::parseSubscript, nullptr, &QParser::parseMethodDecl, nullptr, ("[]"), P_SUBSCRIPT, P_LEFT }},
{ T_LEFT_BRACE, { &QParser::parseLiteralMap, nullptr, &QParser::parseBlock, nullptr, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_AT, { &QParser::parseDecoratedExpression, &QParser::parseInfixOp, &QParser::parseDecoratedStatement, &QParser::parseDecoratedDecl, "@", "@", P_EXPONENT, P_LEFT }},
{ T_FUNCTION, { &QParser::parseLambda, nullptr, &QParser::parseFunctionDecl, &QParser::parseMethodDecl2, "function", "function", P_PREFIX, P_LEFT }},
{ T_NAME, { &QParser::parseName, nullptr, nullptr, &QParser::parseMethodDecl, nullptr, nullptr, P_PREFIX, P_LEFT }},
INFIX(MINUSGT, ArrowFunction, ->, COMPREHENSION, RIGHT),
INFIX(EQGT, ArrowFunction, =>, COMPREHENSION, RIGHT),
INFIX(DOT, InfixOp, ., MEMBER, LEFT),
INFIX(DOTQUEST, InfixOp, .?, MEMBER, LEFT),
MULTIFIX(COLONCOLON, GenericMethodSymbol, InfixOp, ::, ::, MEMBER, LEFT),
PREFIX(UND, Field, _),
PREFIX(UNDUND, StaticField, __),
PREFIX(SUPER, Super, super),
PREFIX(TRUE, Literal, true),
PREFIX(FALSE, Literal, false),
PREFIX(NULL, Literal, null),
PREFIX(UNDEFINED, Literal, undefined),
PREFIX(NUM, Literal, Num),
PREFIX(STRING, Literal, String),
PREFIX(YIELD, Yield, yield),
PREFIX(AWAIT, Yield, await),

STATEMENT(SEMICOLON, SimpleStatement),
STATEMENT(BREAK, Break),
STATEMENT(CLASS, ClassDecl),
STATEMENT(CONTINUE, Continue),
STATEMENT(EXPORT, ExportDecl),
STATEMENT(GLOBAL, GlobalDecl),
STATEMENT(IF, If),
STATEMENT(REPEAT, RepeatWhile),
STATEMENT(RETURN, Return),
STATEMENT(THROW, Throw),
STATEMENT(TRY, Try),
STATEMENT(WITH, With),
STATEMENT(WHILE, While),

{ T_FOR, { nullptr, &QParser::parseComprehension, &QParser::parseFor, nullptr, nullptr, "for", P_COMPREHENSION, P_LEFT }},
{ T_IMPORT, { &QParser::parseImportExpression, nullptr, &QParser::parseImportDecl, nullptr, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_VAR, { nullptr, nullptr, &QParser::parseVarDecl, &QParser::parseSimpleAccessor, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_CONST, { nullptr, nullptr, &QParser::parseVarDecl, &QParser::parseSimpleAccessor, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_SWITCH, { &QParser::parseSwitchExpression, nullptr, &QParser::parseSwitchStatement, nullptr, nullptr, nullptr, P_PREFIX, P_LEFT }},
{ T_ASYNC, { nullptr, nullptr, &QParser::parseAsync, &QParser::parseAsyncMethodDecl, nullptr, nullptr, P_PREFIX, P_LEFT }}
#undef PREFIX
#undef PREFIX_OP
#undef INFIX
#undef INFIX_OP
#undef OPERATOR
#undef STATEMENT
};

static std::unordered_map<string,QTokenType> KEYWORDS = {
#define TOKEN(name, keyword) { (#keyword), T_##name }
TOKEN(AMPAMP, and),
TOKEN(AS, as),
TOKEN(ASYNC, async),
TOKEN(AWAIT, await),
TOKEN(BREAK, break),
TOKEN(CASE, case),
TOKEN(CATCH, catch),
TOKEN(CLASS, class),
TOKEN(CONTINUE, continue),
TOKEN(CONST, const),
TOKEN(FUNCTION, def),
TOKEN(DEFAULT, default),
TOKEN(ELSE, else),
TOKEN(EXPORT, export),
TOKEN(FALSE, false),
TOKEN(FINALLY, finally),
TOKEN(FOR, for),
TOKEN(FUNCTION, function),
TOKEN(GLOBAL, global),
TOKEN(IF, if),
TOKEN(IMPORT, import),
TOKEN(IN, in),
TOKEN(IS, is),
TOKEN(VAR, let),
TOKEN(EXCL, not),
TOKEN(NULL, null),
TOKEN(BARBAR, or),
TOKEN(REPEAT, repeat),
TOKEN(RETURN, return),
TOKEN(STATIC, static),
TOKEN(SUPER, super),
TOKEN(SWITCH, switch),
TOKEN(THROW, throw),
TOKEN(TRUE, true),
TOKEN(TRY, try),
TOKEN(UNDEFINED, undefined),
TOKEN(VAR, var),
TOKEN(WHILE, while),
TOKEN(WITH, with),
TOKEN(YIELD, yield)
#undef TOKEN
};

static unordered_map<int, QV(*)(double,double)> BASE_NUMBER_BINOPS = {
#define OP(T,O) { T_##T, [](double a, double b){ return QV(a O b); } }
#define OPB(T,O) { T_##T, [](double a, double b){ return QV(static_cast<double>(static_cast<int64_t>(a) O static_cast<int64_t>(b))); } }
#define OPF(T,F) { T_##T, [](double a, double b){ return QV(F(a,b)); } }
OP(PLUS, +), OP(MINUS, -),
OP(STAR, *), OP(SLASH, /),
OP(LT, <), OP(GT, >),
OP(LTE, <=), OP(GTE, >=),
OP(EQEQ, ==), OP(EXCLEQ, !=), OP(IS, ==),
OPB(BAR, |), OPB(AMP, &), OPB(CIRC, ^),
OPF(LTLT, dlshift), OPF(GTGT, drshift),
OPF(PERCENT, fmod), OPF(STARSTAR, pow), OPF(BACKSLASH, dintdiv)
#undef OP
#undef OPF
#undef OPB
};

static unordered_map<int, double(*)(double)> BASE_NUMBER_UNOPS = {
{ T_MINUS, [](double x){ return -x; } },
{ T_PLUS, [](double x){ return +x; } },
{ T_TILDE, [](double x){ return static_cast<double>(~static_cast<int64_t>(x)); } }
#undef OP
};

static unordered_map<int,int> BASE_OPTIMIZED_OPS = {
#define OP(N,M) { T_##N, OP_##M }
OP(PLUS, ADD),
OP(MINUS, SUB),
OP(STAR, MUL),
OP(SLASH, DIV),
OP(BACKSLASH, INTDIV),
OP(PERCENT, MOD),
OP(STARSTAR, POW),
OP(LTLT, LSH),
OP(GTGT, RSH),
OP(BAR, BINOR),
OP(AMP, BINAND),
OP(CIRC, BINXOR),
OP(EQEQ, EQ),
OP(EXCLEQ, NEQ),
OP(LT, LT),
OP(GT, GT),
OP(LTE, LTE),
OP(GTE, GTE),
#undef OP
};

static inline bool isSpace (uint32_t c) {
return c==' ' || c=='\t' || c=='\r' || c==160;
}

static inline bool isLine (uint32_t c) {
return c=='\n';
}

bool isName (uint32_t c) {
return (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_' 
||(c>=0xC0 && c<0x2000 && c!=0xD7 && c!=0xF7)
|| (c>=0x2C00 && c<0x2E00)
|| (c>=0x2E80 && c<0xFFF0)
|| c>=0x10000;
}

bool isUpper (uint32_t c) {
return c>='A' && c<='Z';
}

static bool isSpaceOrIgnorableLine (uint32_t c, const char* in, const char* end) {
if (isSpace(c)) return true;
else if (!isLine(c)) return false;
while (in<end && (c=utf8::next(in, end)) && (isSpace(c) || isLine(c)));
return string(".+-/*&|").find(c)!=string::npos;
}

static inline double parseNumber (const char*& in) {
if (*in=='0') {
switch(in[1]){
case 'x': case 'X': return strtoll(in+2, const_cast<char**>(&in), 16);
case 'o': case 'O': return strtoll(in+2, const_cast<char**>(&in), 8);
case 'b': case 'B': return strtoll(in+2, const_cast<char**>(&in), 2);
case '1': case '2': case '3': case '4': case '5': case '6': case '7': return strtoll(in+1, const_cast<char**>(&in), 8);
default: break;
}}
double d = strtod_c(in, const_cast<char**>(&in));
if (in[-1]=='.') in--;
return d;
}

static void skipComment (const char*& in, const char* end, char delim) {
int c = utf8peek(in, end);
if (isSpace(c) || isName(c) || c==delim) {
while(c && in<end && !isLine(c)) c=utf8::next(in, end);
return;
}
else if (!c || isLine(c)) return;
int opening = c, closing = c, nesting = 1;
if (opening=='[' || opening=='{' || opening=='<') closing = opening+2; 
else if (opening=='(') closing=')';
while(c = utf8inc(in, end)){
if (c==delim) {
if (utf8inc(in, end)==opening) nesting++;
}
else if (c==closing) {
if (utf8inc(in, end)==delim && --nesting<=0) { utf8::next(in, end); break; }
}}}

static int parseCodePointValue (QParser& parser, const char*& in, const char* end, int n, int base) {
char buf[n+1];
memcpy(buf, in, n);
buf[n]=0;
in += n;
auto c = strtoul(buf, nullptr, base);
if (c>=0x110000) {
parser.cur = { T_STRING, in -n -2, static_cast<size_t>(n+2), QV::UNDEFINED };
parser.parseError("Invalid code point");
return 0xFFFD;
}
return c;
}

static int getStringEndingChar (int c) {
switch(c){
case 147: return 148;
case 171: return 187;
case 8220: return 8221;
default:  return c;
}}

static QV parseString (QParser& parser, QVM& vm, const char*& in, const char* end, int ending) {
string re;
auto out = back_inserter(re);
int c=0;
auto begin = in;
while(in<end && (c=utf8::next(in, end))!=ending && c) {
if (c=='\n' && !vm.multilineStrings) break;
if (c=='\\') {
const char* ebegin = in -1;
c = utf8::next(in, end);
switch(c){
case 'b': c='\b'; break;
case 'e': c='\x1B'; break;
case 'f': c='\f'; break;
case 'n': c='\n'; break;
case 'r': c='\r'; break;
case 't': c='\t'; break;
case 'u': c = parseCodePointValue(parser, in, end, 4, 16); break;
case 'U': c = parseCodePointValue(parser, in, end, 8, 16); break;
case 'v': c = '\v'; break;
case 'x': c = parseCodePointValue(parser, in, end, 2, 16); break;
case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': 
c = strtoul(--in, const_cast<char**>(&in), 0); 
if (c>=0x110000) { parser.cur = { T_STRING, ebegin, static_cast<size_t>(in-ebegin), QV::UNDEFINED }; parser.parseError("Invalid code point"); c=0xFFFD; }
break;
default:
if ((c>='a' && c<='z') || (c>='a' && c<='Z')) {
parser.cur = { T_STRING, ebegin, static_cast<size_t>(in-ebegin), QV::UNDEFINED };
parser.parseError("Invalid escape sequence");
}
break;
}}
utf8::append(c, out);
}
if (c!=ending) {
parser.cur = { T_STRING, begin, static_cast<size_t>(in-begin), QV::UNDEFINED };
parser.parseError("Unterminated string");
}
return QV(vm, re);
}

static shared_ptr<BinaryOperation> createBinaryOperation (shared_ptr<Expression> left, QTokenType op, shared_ptr<Expression> right) {
if (rules[op].flags&P_SWAP_OPERANDS) swap(left, right);
switch(op){
case T_EQ:
case T_PLUSEQ: case T_MINUSEQ: case T_STAREQ: case T_STARSTAREQ: case T_SLASHEQ: case T_BACKSLASHEQ: case T_PERCENTEQ: case T_ATEQ:
case T_BAREQ: case T_AMPEQ: case T_CIRCEQ: case T_AMPAMPEQ: case T_BARBAREQ: case T_LTLTEQ: case T_GTGTEQ: case T_QUESTQUESTEQ:
return make_shared<AssignmentOperation>(left, op, right);
case T_DOT: 
return make_shared<MemberLookupOperation>(left, right);
case T_COLONCOLON: 
return make_shared<MethodLookupOperation>(left, right);
case T_AMPAMP: case T_QUESTQUEST: case T_BARBAR:
return make_shared<ShortCircuitingBinaryOperation>(left, op, right);
case T_DOTQUEST:
return createBinaryOperation(left, T_AMPAMP, createBinaryOperation(left, T_DOT, right));
default: 
return make_shared<BinaryOperation>(left, op, right);
}}

const QToken& QParser::prevToken () {
stackedTokens.push_back(cur);
return cur=prev;
}

const QToken& QParser::nextNameToken (bool eatEq) {
prev=cur;
if (!stackedTokens.empty()) {
cur = stackedTokens.front();
stackedTokens.clear();
in = cur.start;
}
#define RET0(X) { cur = { X, start, static_cast<size_t>(in-start), QV::UNDEFINED}; return cur; }
#define RET RET0(T_NAME)
#define RET2(C) if (utf8peek(in, end)==C) utf8::next(in, end); RET
#define RET3(C1,C2) if(utf8peek(in, end)==C1 || utf8peek(in, end)==C2) utf8::next(in, end); RET
#define RET4(C1,C2,C3) if (utf8peek(in, end)==C1 || utf8peek(in, end)==C2 || utf8peek(in, end)==C3) utf8::next(in, end); RET
#define RET22(C1,C2) if (utf8peek(in, end)==C1) utf8::next(in, end); RET2(C2)
const char *start = in;
if (in>=end || !*in) RET0(T_END)
int c;
do {
c = utf8::next(in, end);
} while((isSpace(c) || isLine(c)) && *(start=in) && in<end);
switch(c){
case '\0': RET0(T_END)
case '(':
if (utf8peek(in, end)==')') {
utf8::next(in, end);
RET2('=')
}break;
case '[': 
if (utf8peek(in, end)==']') {
utf8::next(in, end);
RET2('=')
}break;
case '/': 
switch (utf8peek(in, end)) {
case '/': case '*': skipComment(in, end, '/'); return nextNameToken(eatEq);
default: RET
}
case '+': RET2('+')
case '-': RET2('-')
case '*': RET2('*')
case '\\': RET
case '%': RET
case '|': RET
case '&': RET
case '^': RET
case '~': RET
case '@': RET
case '!': RET2('=')
case '=': RET2('=')
case '<': RET3('<', '=')
case '>': RET3('>', '=')
case '#': skipComment(in, end, '#'); return nextNameToken(eatEq);
}
if (isName(c)) {
while(in<end && (c=utf8::peek_next(in, end)) && (isName(c) || isDigit(c))) utf8::next(in, end);
if (in<end && utf8::peek_next(in, end)=='=' && eatEq) utf8::next(in, end);
RET
}
cur = { T_END, in, 1, QV::UNDEFINED };
parseError("Unexpected character (%#0$2X)", c);
RET0(T_END)
#undef RET
#undef RET0
#undef RET2
#undef RET22
#undef RET3
#undef RET4
}

const QToken& QParser::nextToken () {
prev=cur;
if (!stackedTokens.empty()) {
cur = stackedTokens.back();
stackedTokens.pop_back();
return cur;
}
#define RET(X) { cur = { X, start, static_cast<size_t>(in-start), QV::UNDEFINED}; return cur; }
#define RETV(X,V) { cur = { X, start, static_cast<size_t>(in-start), V}; return cur; }
#define RET2(C,A,B) if (utf8peek(in, end)==C) { utf8::next(in, end); RET(A) } else RET(B)
#define RET3(C1,A,C2,B,C) if(utf8peek(in, end)==C1) { utf8::next(in, end); RET(A) } else if (utf8peek(in, end)==C2) { utf8::next(in, end); RET(B) } else RET(C)
#define RET4(C1,R1,C2,R2,C3,R3,C) if(utf8peek(in, end)==C1) { utf8::next(in, end); RET(R1) } else if (utf8peek(in, end)==C2) { utf8::next(in, end); RET(R2) } else if (utf8peek(in, end)==C3) { utf8::next(in, end); RET(R3) }  else RET(C)
#define RET22(C1,C2,R11,R12,R21,R22) if (utf8peek(in, end)==C1) { utf8::next(in, end); RET2(C2,R11,R12) } else RET2(C2,R21,R22)
const char *start = in;
if (in>=end || !*in) RET(T_END)
uint32_t c;
do {
c = utf8::next(in, end);
} while(isSpaceOrIgnorableLine(c, in, end) && *(start=in) && in<end);
switch(c){
case '\0': RET(T_END)
case '\n': RET(T_LINE)
case '(': RET(T_LEFT_PAREN)
case ')': RET(T_RIGHT_PAREN)
case '[': RET(T_LEFT_BRACKET)
case ']': RET(T_RIGHT_BRACKET)
case '{': RET(T_LEFT_BRACE)
case '}': RET(T_RIGHT_BRACE)
case ',': RET(T_COMMA)
case ';': RET(T_SEMICOLON)
case ':': RET2(':', T_COLONCOLON, T_COLON)
case '_': RET2('_', T_UNDUND, T_UND)
case '$': RET(T_DOLLAR)
case '+': RET3('+', T_PLUSPLUS, '=', T_PLUSEQ, T_PLUS)
case '-': RET4('-', T_MINUSMINUS, '=', T_MINUSEQ, '>', T_MINUSGT, T_MINUS)
case '*': RET22('*', '=', T_STARSTAREQ, T_STARSTAR, T_STAREQ, T_STAR)
case '\\': RET2('=', T_BACKSLASHEQ, T_BACKSLASH)
case '%': RET2('=', T_PERCENTEQ, T_PERCENT)
case '|': RET22('|', '=', T_BARBAREQ, T_BARBAR, T_BAREQ, T_BAR)
case '&': RET22('&', '=', T_AMPAMPEQ, T_AMPAMP, T_AMPEQ, T_AMP)
case '^': RET2('=', T_CIRCEQ, T_CIRC)
case '@': RET2('=', T_ATEQ, T_AT)
case '~': RET(T_TILDE)
case '!': RET2('=', T_EXCLEQ, T_EXCL)
case '=': RET3('=', T_EQEQ, '>', T_EQGT, T_EQ) 
case '<': RET22('<', '=', T_LTLTEQ, T_LTLT, T_LTE, T_LT) 
case '>': RET22('>', '=', T_GTGTEQ, T_GTGT, T_GTE, T_GT) 
case '?': 
if (utf8peek(in, end)=='.') { utf8::next(in, end); RET(T_DOTQUEST) } 
else { RET22('?', '=', T_QUESTQUESTEQ, T_QUESTQUEST, T_QUESTQUESTEQ, T_QUEST) }
case '/': 
switch (utf8peek(in, end)) {
case '/': case '*': skipComment(in, end, '/'); return nextToken();
default: RET2('=', T_SLASHEQ, T_SLASH)
}
case '.':
if (utf8peek(in, end)=='?') { utf8::next(in, end); RET(T_DOTQUEST) } 
else if (utf8peek(in, end)=='.') { utf8::next(in, end); RET2('.', T_DOTDOTDOT, T_DOTDOT) }
else RET(T_DOT)
case '"': case '\'': case '`':
case 146: case 147: case 171: case 8216: case 8217: case 8220: 
{
QV str = parseString(*this, vm, in, end, getStringEndingChar(c));
RETV(T_STRING, str)
}
case '#': skipComment(in, end, '#'); return nextToken();
case 183: case 215: case 8901: RET(T_STAR)
case 247: RET(T_SLASH)
case 8722: RET(T_MINUS)
case 8734: RETV(T_NUM, 1.0/0.0)
case 8800: RET(T_EXCLEQ)
case 8801: RET(T_EQEQ)
case 8804: RET(T_LTE)
case 8805: RET(T_GTE)
case 8712: RET(T_IN)
case 8743: RET(T_AMPAMP)
case 8744: RET(T_BARBAR)
case 8745: RET(T_AMP)
case 8746: RET(T_BAR)
case 8891: RET(T_CIRC)
}
if (isDigit(c)) {
double d = parseNumber(--in);
RETV(T_NUM, d)
}
else if (isName(c)) {
while(in<end && (c=utf8::peek_next(in, end)) && (isName(c) || isDigit(c))) utf8::next(in, end);
QTokenType type = T_NAME;
auto it = KEYWORDS.find(string(start, in-start));
if (it!=KEYWORDS.end()) type = it->second;
switch(type){
case T_TRUE: RETV(type, true)
case T_FALSE: RETV(type, false)
case T_NULL: RETV(type, QV::Null)
case T_UNDEFINED: RETV(type, QV::UNDEFINED)
default: RET(type)
}}
else if (isSpace(c)) RET(T_END)
cur = { T_END, in, 1, QV::UNDEFINED };
parseError("Unexpected character (%#0$2X)", c);
RET(T_END)
#undef RET
#undef RETV
#undef RET2
#undef RET22
#undef RET3
#undef RET4
}

pair<int,int> QParser::getPositionOf (const char* pos) {
if (pos<start || pos>=end) return { -1, -1 };
int line=1, column=1;
for (const char* c=start; c<pos && *c; c++) {
if (isLine(*c)) { line++; column=1; }
else column++;
}
return { line, column };
}

template<class... A> void QParser::parseError (const char* fmt, const A&... args) {
auto p = getPositionOf(cur.start);
int line = p.first, column = p.second;
Swan::CompilationMessage z = { Swan::CompilationMessage::Kind::ERROR, format(fmt, args...), string(cur.start, cur.length), displayName, line, column };
vm.messageReceiver(z);
result = cur.type==T_END? CR_INCOMPLETE : CR_FAILED;
}

bool QParser::match (QTokenType type) {
if (nextToken().type==type) return true;
prevToken();
return false;
}

template<class... T> bool QParser::matchOneOf (T... tokens) {
vector<QTokenType> vt = { tokens... };
nextToken();
if (vt.end()!=find(vt.begin(), vt.end(), cur.type)) return true;
prevToken();
return false;
}

bool QParser::consume (QTokenType type, const char* msg) {
if (nextToken().type==type) return true;
parseError(msg);
return false;
}

void QParser::skipNewlines () {
while(matchOneOf(T_LINE, T_SEMICOLON));
}

QToken QParser::createTempName () {
static int count = 0;
string name = format("$%d", count++);
QString* s = QString::create(vm,name);
return { T_NAME, s->data, s->length, QV(s) };
}

shared_ptr<Statement> QParser::parseSimpleStatement () {
return make_shared<SimpleStatement>(cur);
}

shared_ptr<Statement> QParser::parseBlock () {
vector<shared_ptr<Statement>> statements;
while(!match(T_RIGHT_BRACE)) {
shared_ptr<Statement> sta = parseStatement();
if (sta) statements.push_back(sta);
else { result=CR_INCOMPLETE; break; }
skipNewlines();
}
if (!statements.size()) statements.push_back(make_shared<SimpleStatement>(cur));
return make_shared<BlockStatement>(statements);
}

shared_ptr<Statement> QParser::parseIf () {
shared_ptr<Expression> condition = parseExpression();
match(T_COLON);
shared_ptr<Statement> ifPart = parseStatement();
if (!ifPart) result = CR_INCOMPLETE;
shared_ptr<Statement> elsePart = nullptr;
skipNewlines();
if (match(T_ELSE)) {
match(T_COLON);
elsePart = parseStatement();
if (!elsePart) result = CR_INCOMPLETE;
}
return make_shared<IfStatement>(condition, ifPart, elsePart);
}

shared_ptr<Statement> QParser::parseSwitchStatement () {
auto sw = make_shared<SwitchStatement>();
shared_ptr<Expression> activeCase;
vector<shared_ptr<Statement>> statements;
bool defaultDefined=false;
auto clearStatements = [&]()mutable{
if (activeCase) sw->cases.push_back(make_pair(activeCase, statements));
else sw->defaultCase = statements;
statements.clear();
};
sw->expr = parseExpression();
sw->var = make_shared<NameExpression>(createTempName());
skipNewlines();
consume(T_LEFT_BRACE, "Expected '{' to begin switch");
while(true){
skipNewlines();
if (match(T_CASE)) {
if (defaultDefined) parseError("Default case must be last");
clearStatements();
activeCase = parseSwitchCase(sw->var);
while(match(T_COMMA)) {
clearStatements();
activeCase = parseSwitchCase(sw->var);
}
match(T_COLON);
}
else if (match(T_DEFAULT)) {
if (defaultDefined) parseError("Duplicate default case");
defaultDefined=true;
clearStatements();
activeCase = nullptr;
match(T_COLON);
}
else if (match(T_RIGHT_BRACE)) break;
else {
auto sta = parseStatement();
if (!sta) { result=CR_INCOMPLETE; return nullptr; }
statements.push_back(sta);
}}
clearStatements();
return sw;
}

void ForStatement::parseHead (QParser& parser) {
parser.parseVarList(loopVariables);
if (loopVariables.size()==1 && (loopVariables[0]->flags&VD_NODEFAULT) && parser.match(T_IN)) {
parser.skipNewlines();
inExpression = parser.parseExpression(P_COMPREHENSION);
} else {
parser.consume(T_SEMICOLON, "Expected ';' after traditional for loop variables declarations");
traditional=true;
inExpression = parser.parseExpression();
parser.consume(T_SEMICOLON, "Expected ';' after traditional for loop condition");
incrExpression = parser.parseExpression();
}}

shared_ptr<Statement> QParser::parseFor () {
shared_ptr<ForStatement> forSta = make_shared<ForStatement>(cur);
forSta->parseHead(*this);
match(T_COLON);
forSta->loopStatement = parseStatement();
if (!forSta->inExpression || !forSta->loopStatement) result = CR_INCOMPLETE;
return forSta;
}

shared_ptr<Statement> QParser::parseWhile () {
shared_ptr<Expression> condition = parseExpression();
match(T_COLON);
shared_ptr<Statement> loopStatement = parseStatement();
if (!loopStatement) result = CR_INCOMPLETE;
return make_shared<WhileStatement>(condition, loopStatement);
}

shared_ptr<Statement> QParser::parseRepeatWhile () {
shared_ptr<Statement> loopStatement = parseStatement();
consume(T_WHILE, ("Expected 'while' after repeated statement"));
shared_ptr<Expression> condition = parseExpression();
if (!loopStatement || !condition) result = CR_INCOMPLETE;
return make_shared<RepeatWhileStatement>(condition, loopStatement);
}

shared_ptr<Statement> QParser::parseContinue () {
QToken cs = cur;
int count = 1;
if (match(T_NUM)) count = cur.value.d;
return make_shared<ContinueStatement>(cs, count);
}

shared_ptr<Statement> QParser::parseBreak () {
QToken bks = cur;
int count = 1;
if (match(T_NUM)) count = cur.value.d;
return make_shared<BreakStatement>(bks, count);
}

shared_ptr<Statement> QParser::parseReturn () {
QToken returnToken = cur;
shared_ptr<Expression> expr = nullptr;
if (matchOneOf(T_RIGHT_BRACE, T_LINE, T_SEMICOLON)) prevToken();
else expr = parseExpression();
return make_shared<ReturnStatement>(returnToken, expr);
}

shared_ptr<Expression> QParser::parseYield () {
shared_ptr<Expression> expr = nullptr;
QToken tk = cur;
skipNewlines();
if (matchOneOf(T_RIGHT_BRACE, T_LINE, T_SEMICOLON)) prevToken();
else expr = parseExpression();
return make_shared<YieldExpression>(tk, expr);
}

shared_ptr<Statement> QParser::parseThrow () {
QToken tk = cur;
return make_shared<ThrowStatement>(tk, parseExpression());
}

shared_ptr<Statement> QParser::parseTry () {
QToken catchVar = cur;
match(T_COLON);
shared_ptr<Statement> tryPart = parseStatement();
if (!tryPart) result = CR_INCOMPLETE;
shared_ptr<Statement> catchPart = nullptr, finallyPart = nullptr;
skipNewlines();
if (match(T_CATCH)) {
bool paren = match(T_LEFT_PAREN);
consume(T_NAME, "Expected variable name after 'catch'");
catchVar = cur;
if (paren) consume(T_RIGHT_PAREN, "Expected ')' after catch variable name");
match(T_COLON);
catchPart = parseStatement();
if (!catchPart) result = CR_INCOMPLETE;
}
skipNewlines();
if (match(T_FINALLY)) {
match(T_COLON);
finallyPart = parseStatement();
if (!finallyPart) result = CR_INCOMPLETE;
}
if (!catchPart && !finallyPart) result = CR_INCOMPLETE;
return make_shared<TryStatement>(tryPart, catchPart, finallyPart, catchVar);
}

shared_ptr<Statement> QParser::parseWith () {
QToken catchVar = cur;
skipNewlines();
shared_ptr<Expression> openExpr, varExpr = parseExpression(P_COMPREHENSION);
if (match(T_EQ)) openExpr = parseExpression(P_COMPREHENSION);
else {
openExpr = varExpr;
varExpr = make_shared<NameExpression>(createTempName());
}
//#######
if (!dynamic_pointer_cast<NameExpression>(varExpr)) {
varExpr = openExpr = nullptr;
parseError("Invalid variable name for 'with' expression");
}
if (!varExpr || !openExpr) { result=CR_INCOMPLETE; return nullptr; }
match(T_COLON);
shared_ptr<Statement> catchSta=nullptr, body = parseStatement();
if (!body) { result=CR_INCOMPLETE; return nullptr; }
skipNewlines();
if (match(T_CATCH)) {
bool paren = match(T_LEFT_PAREN);
consume(T_NAME, "Expected variable name after 'catch'");
catchVar = cur;
if (paren) consume(T_RIGHT_PAREN, "Expected ')' after catch variable name");
match(T_COLON);
catchSta = parseStatement();
if (!catchSta) { result=CR_INCOMPLETE; return nullptr; }
}
QString* closeName = QString::create(vm, ("close"), 5);
QToken closeToken = { T_NAME, closeName->data, closeName->length, QV(closeName, QV_TAG_STRING) };
vector<shared_ptr<Variable>> varDecls = { make_shared<Variable>(varExpr, openExpr) };
auto varDecl = make_shared<VariableDeclaration>(varDecls);
auto closeExpr = createBinaryOperation(varExpr, T_DOT, make_shared<NameExpression>(closeToken));
auto trySta = make_shared<TryStatement>(body, catchSta, closeExpr, catchVar);
vector<shared_ptr<Statement>> statements = { varDecl, trySta };
return make_shared<BlockStatement>(statements);
}

shared_ptr<Expression> QParser::parseDecoratedExpression () {
auto tp = nextToken().type;
if (tp==T_NUM) return make_shared<AnonymousLocalExpression>(cur);
else if (((tp>=T_PLUS && tp<=T_GTE) || (tp>=T_DOT && tp<=T_DOTQUEST)) && rules[tp].infix) {
prevToken();
cur.type=T_NAME;
prevToken();
auto func = make_shared<FunctionDeclaration>(cur);
auto nm = make_shared<NameExpression>(cur);
func->params.push_back(make_shared<Variable>(nm));
func->body = parseExpression(P_COMPREHENSION);
return func;
}
else prevToken();
auto decoration = parseExpression(P_PREFIX);
auto expr = parseExpression();
if (expr->isDecorable()) {
auto decorable = dynamic_pointer_cast<Decorable>(expr);
decorable->decorations.insert(decorable->decorations.begin(), decoration);
}
else parseError("Expression can't be decorated");
return expr;
}

shared_ptr<Statement> QParser::parseDecoratedStatement () {
auto decoration = parseExpression(P_PREFIX);
auto expr = parseStatement();
if (expr && expr->isDecorable()) {
auto decorable = dynamic_pointer_cast<Decorable>(expr);
decorable->decorations.insert(decorable->decorations.begin(), decoration);
}
else parseError("Expression can't be decorated");
return expr;
}

void QParser::parseVarList (vector<shared_ptr<Variable>>& vars, int flags) {
do {
auto var = make_shared<Variable>(nullptr, nullptr, flags);
skipNewlines();
while (match(T_AT)) {
var->decorations.insert(var->decorations.begin(), parseExpression(P_PREFIX));
skipNewlines();
}
if (!(var->flags&VD_CONST) && match(T_CONST)) var->flags |= VD_CONST;
if (!(var->flags&VD_CONST)) match(T_VAR);
if (!(var->flags&VD_VARARG) && match(T_DOTDOTDOT)) var->flags |= VD_VARARG;
switch(nextToken().type){
case T_NAME: var->name = parseName(); break;
case T_LEFT_PAREN: var->name = parseGroupOrTuple(); var->value=make_shared<LiteralTupleExpression>(cur); break;
case T_LEFT_BRACKET: var->name = parseLiteralList(); var->value=make_shared<LiteralListExpression>(cur); break;
case T_LEFT_BRACE:  var->name = parseLiteralMap(); var->value=make_shared<LiteralMapExpression>(cur); break;
case T_UND: var->name = parseField(); break;
case T_UNDUND: var->name = parseStaticField(); break;
default: parseError("Expecting identifier, '(', '[' or '{' in variable declaration"); break;
}
if (!(var->flags&VD_VARARG) && match(T_DOTDOTDOT)) var->flags |= VD_VARARG;
skipNewlines();
if (!(flags&VD_NODEFAULT) && match(T_EQ)) var->value = parseExpression(P_COMPREHENSION);
else var->flags |= VD_NODEFAULT;
if (match(T_AS)) var->typeHint = parseTypeInfo();
vars.push_back(var);
if (flags&VD_SINGLE) break;
} while(match(T_COMMA));
}

shared_ptr<Statement> QParser::parseVarDecl () {
return parseVarDecl(cur.type==T_CONST? VD_CONST : 0);
}

shared_ptr<Statement> QParser::parseVarDecl (int flags) {
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VD_GLOBAL;
auto decl = make_shared<VariableDeclaration>();
parseVarList(decl->vars, flags);
return decl;
}

void QParser::parseFunctionParameters (shared_ptr<FunctionDeclaration>& func) {
if (func->flags&FD_METHOD) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
func->params.push_back( make_shared<Variable>(make_shared<NameExpression>(thisToken)));
}
if (match(T_LEFT_PAREN) && !match(T_RIGHT_PAREN)) {
parseVarList(func->params);
consume(T_RIGHT_PAREN, ("Expected ')' to close parameter list"));
}
else if (match(T_NAME)) {
prevToken();
parseVarList(func->params, VD_SINGLE);
}
if (match(T_AS)) func->returnTypeHint = parseTypeInfo();
if (func->params.size()>=1 && (func->params[func->params.size() -1]->flags&VD_VARARG)) func->flags |= FD_VARARG;
}

void QParser::parseDecoratedDecl (ClassDeclaration& cls, int flags) {
vector<shared_ptr<Expression>> decorations;
int idxFrom = cls.methods.size();
bool parsed = false;
prevToken();
while (match(T_AT)) {
const char* c = in;
while(isSpace(*c) || isLine(*c)) c++;
if (*c=='(') {
parseMethodDecl(cls, flags);
parsed=true;
break;
}
skipNewlines();
auto decoration = parseExpression(P_PREFIX);
skipNewlines();
decorations.push_back(decoration);
}
if (!parsed) {
skipNewlines();
if (nextToken().type==T_STATIC) {
flags |= FD_STATIC;
nextToken();
}
const ParserRule& rule = rules[cur.type];
if (rule.member) (this->*rule.member)(cls, flags);
else { prevToken(); parseError("Expected declaration to decorate"); }
}
for (auto it=cls.methods.begin() + idxFrom, end = cls.methods.end(); it<end; ++it) (*it)->decorations = decorations;
}

void QParser::parseMethodDecl (ClassDeclaration& cls, int flags) {
prevToken();
QToken name = nextNameToken(true);
auto func = make_shared<FunctionDeclaration>(name, FD_METHOD | flags);
parseFunctionParameters(func);
if (*name.start=='[' && func->params.size()<=1) {
parseError(("Subscript operator must take at least one argument"));
return;
}
if (*name.start=='[' && name.start[name.length -1]=='=' && func->params.size()<=2) {
parseError(("Subscript operator setter must take at least two arguments"));
return;
}
if (*name.start!='[' && name.start[name.length -1]=='=' && func->params.size()!=2) {
parseError(("Setter methods must take exactly one argument"));
}
if (auto m = cls.findMethod(name, flags&FD_STATIC)) {
parseError("%s already defined in line %d", string(name.start, name.length), getPositionOf(m->name.start).first);
}
match(T_COLON);
if (match(T_SEMICOLON)) func->body = make_shared<SimpleStatement>(cur);
else func->body = parseStatement();
if (!func->body) func->body = make_shared<SimpleStatement>(cur);
cls.methods.push_back(func);
}

void QParser::parseMethodDecl2 (ClassDeclaration& cls, int flags) {
nextToken();
parseMethodDecl(cls, flags);
}

void QParser::parseAsyncMethodDecl (ClassDeclaration& cls, int flags) {
if (nextToken().type==T_STATIC && !(flags&FD_STATIC)) {
flags |= FD_STATIC;
nextToken();
}
if (cur.type==T_FUNCTION) nextToken();
parseMethodDecl(cls, flags | FD_ASYNC);
}

void QParser::parseSimpleAccessor (ClassDeclaration& cls, int flags) {
if (cur.type==T_CONST) flags |= FD_CONST;
if (flags&FD_CONST) match(T_VAR);
do {
consume(T_NAME, ("Expected field name after 'var'"));
QToken fieldToken = cur;
string fieldName = string(fieldToken.start, fieldToken.length);
if (auto m = cls.findMethod(fieldToken, flags&FD_STATIC)) parseError("%s already defined in line %d", fieldName, getPositionOf(m->name.start).first);
cls.findField(flags&FD_STATIC? cls.staticFields : cls.fields, fieldToken);
shared_ptr<TypeInfo> typeHint = nullptr;
if (match(T_EQ)) {
auto& f = (flags&FD_STATIC? cls.staticFields : cls.fields)[fieldName];
f.defaultValue = parseExpression(P_COMPREHENSION);
}
if (match(T_AS)) typeHint = parseTypeInfo();
QString* setterName = QString::create(vm, fieldName+ ("="));
QToken setterNameToken = { T_NAME, setterName->data, setterName->length, QV(setterName, QV_TAG_STRING)  };
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED};
shared_ptr<NameExpression> thisExpr = make_shared<NameExpression>(thisToken);
shared_ptr<Expression> field;
flags |= FD_METHOD;
if (flags&FD_STATIC) field = make_shared<StaticFieldExpression>(fieldToken);
else field = make_shared<FieldExpression>(fieldToken);
shared_ptr<Expression> param = make_shared<NameExpression>(fieldToken);
auto thisParam = make_shared<Variable>(thisExpr);
auto setterParam = make_shared<Variable>(param);
setterParam->typeHint = typeHint;
vector<shared_ptr<Variable>> empty = { thisParam }, setterParams = { thisParam, setterParam  };
shared_ptr<Expression> assignment = createBinaryOperation(field, T_EQ, param);
shared_ptr<FunctionDeclaration> getter = make_shared<FunctionDeclaration>(fieldToken, flags, empty, field);
shared_ptr<FunctionDeclaration> setter = make_shared<FunctionDeclaration>(setterNameToken, flags, setterParams, assignment);
getter->returnTypeHint = typeHint;
cls.methods.push_back(getter);
if (!(flags&FD_CONST)) cls.methods.push_back(setter);
} while (match(T_COMMA));
}

shared_ptr<Expression> QParser::parseLambda  () {
return parseLambda(0);
}

shared_ptr<Expression> QParser::parseLambda  (int flags) {
auto func = make_shared<FunctionDeclaration>(cur);
func->flags |= flags;
if (match(T_GT)) func->flags |= FD_METHOD;
if (match(T_STAR)) func->flags |= FD_FIBER;
else if (match(T_AMP)) flags |= FD_ASYNC;
parseFunctionParameters(func);
match(T_COLON);
func->body = parseStatement();
if (!func->body) func->body = make_shared<SimpleStatement>(cur);
return func;
}

shared_ptr<Expression> QParser::parseArrowFunction (shared_ptr<Expression> fargs) {
auto functionnable = dynamic_pointer_cast<Functionnable>(fargs);
if (!functionnable || !functionnable->isFunctionnable()) {
parseError("Expression can't be considered as the argument list for an anonymous function");
return fargs;
}
auto func = make_shared<FunctionDeclaration>(cur);
if (match(T_GT)) func->flags |= FD_METHOD;
if (match(T_STAR)) func->flags |= FD_FIBER;
else if (match(T_AMP)) func->flags |= FD_ASYNC;
if (func->flags&FD_METHOD) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
func->params.push_back( make_shared<Variable>(make_shared<NameExpression>(thisToken)));
}
functionnable->makeFunctionParameters(func->params);
func->body = parseStatement();
return func;
}

shared_ptr<Statement> QParser::parseAsync () {
return parseAsyncFunctionDecl(VD_CONST);
}

shared_ptr<Statement> QParser::parseAsyncFunctionDecl (int varFlags) {
consume(T_FUNCTION, "Expected 'function' after 'async'");
return parseFunctionDecl(varFlags, FD_ASYNC);
}

shared_ptr<Statement> QParser::parseFunctionDecl () {
return parseFunctionDecl(0);
}

shared_ptr<Statement> QParser::parseFunctionDecl (int varFlags, int funcFlags) {
bool hasName = matchOneOf(T_NAME, T_STRING);
QToken name = cur;
auto fnDecl = parseLambda(funcFlags);
if (!hasName) return fnDecl;
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) varFlags |= VD_GLOBAL;
vector<shared_ptr<Variable>> vars = { make_shared<Variable>( make_shared<NameExpression>(name), fnDecl, varFlags) };
return make_shared<VariableDeclaration>(vars);
}

shared_ptr<Statement> QParser::parseClassDecl () {
return parseClassDecl(0);
}

shared_ptr<Statement> QParser::parseClassDecl (int classFlags) {
if (!consume(T_NAME, ("Expected class name after 'class'"))) return nullptr;
shared_ptr<ClassDeclaration> classDecl = make_shared<ClassDeclaration>(cur, classFlags);
skipNewlines();
if (matchOneOf(T_IS, T_COLON, T_LT)) do {
skipNewlines();
consume(T_NAME, ("Expected class name after 'is'"));
classDecl->parents.push_back(cur);
} while (match(T_COMMA));
else classDecl->parents.push_back({ T_NAME, ("Object"), 6, QV::UNDEFINED });
if (match(T_LEFT_BRACE)) {
while(true) {
skipNewlines();
int memberFlags = 0;
if (nextToken().type==T_STATIC) {
memberFlags |= FD_STATIC;
nextToken();
}
const ParserRule& rule = rules[cur.type];
if (rule.member) (this->*rule.member)(*classDecl, memberFlags);
else { prevToken(); break; }
}
skipNewlines();
consume(T_RIGHT_BRACE, ("Expected '}' to close class body"));
}
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) classFlags |= VD_GLOBAL;
vector<shared_ptr<Variable>> vars = { make_shared<Variable>( make_shared<NameExpression>(classDecl->name), classDecl, classFlags) };
return make_shared<VariableDeclaration>(vars);
}

shared_ptr<Statement> QParser::parseGlobalDecl () {
if (matchOneOf(T_VAR, T_CONST)) {
return parseVarDecl(VD_GLOBAL);
}
else if (match(T_CLASS)) {
return parseClassDecl(VD_CONST | VD_GLOBAL);
}
else if (match(T_FUNCTION)) {
return parseFunctionDecl(VD_CONST | VD_GLOBAL);
}
else if (match(T_ASYNC)) {
return parseAsyncFunctionDecl(VD_CONST | VD_GLOBAL);
}
parseError(("Expected 'function', 'var' or 'class' after 'global'"));
return nullptr;
}

shared_ptr<Statement> QParser::parseExportDecl () {
auto exportDecl = make_shared<ExportDeclaration>();
shared_ptr<VariableDeclaration> varDecl;
if (matchOneOf(T_VAR, T_CONST)) varDecl = dynamic_pointer_cast<VariableDeclaration>(parseVarDecl(VD_CONST));
else if (match(T_CLASS)) varDecl = dynamic_pointer_cast<VariableDeclaration>(parseClassDecl(VD_CONST));
else if (match(T_FUNCTION)) varDecl = dynamic_pointer_cast<VariableDeclaration>(parseFunctionDecl(VD_CONST));
else if (match(T_ASYNC)) varDecl = dynamic_pointer_cast<VariableDeclaration>(parseAsyncFunctionDecl(VD_CONST));
if (varDecl) {
auto name = varDecl->vars[0]->name;
exportDecl->exports.push_back(make_pair(name->nearestToken(), name));
vector<shared_ptr<Statement>> sta = { varDecl, exportDecl };
return make_shared<BlockStatement>(sta, false);
}
else {
do {
shared_ptr<Expression> exportExpr = parseExpression();
if (!exportExpr) { parseError("Expected 'class', 'function', 'var' or expression after 'export'"); return nullptr; }
QToken nameToken = cur;
shared_ptr<NameExpression> nameExpr = dynamic_pointer_cast<NameExpression>(exportExpr);
if (nameExpr && !match(T_AS)) nameToken = nameExpr->token;
else {
if (!nameExpr) consume(T_AS, "Expected 'as' after export expression");
consume(T_NAME, "Expected export name after 'as'");
nameToken = cur;
}
exportDecl->exports.push_back(make_pair(nameToken, exportExpr));
} while (match(T_COMMA));
}
return exportDecl;
}

shared_ptr<Expression> QParser::parseImportExpression () {
bool parent = match(T_LEFT_PAREN);
auto result = make_shared<ImportExpression>(parseExpression());
if (parent) consume(T_RIGHT_PAREN, "Expected ')' to close function call");
return result;
}

static shared_ptr<Expression> nameExprToConstant (QParser& parser, shared_ptr<Expression> key) {
auto bop = dynamic_pointer_cast<BinaryOperation>(key);
if (bop && bop->op==T_EQ) key = bop->left;
while (bop && bop->op==T_DOT) {
key = bop->right;
bop = dynamic_pointer_cast<BinaryOperation>(key);
}
auto name = dynamic_pointer_cast<NameExpression>(key);
if (name) {
QToken token = name->token;
token.type = T_STRING;
token.value = QV(parser.vm, string(token.start, token.length));
key = make_shared<ConstantExpression>(token);
}
return key;
}

static void multiVarExprToSingleLiteralMap (QParser& parser, vector<shared_ptr<Variable>>& vars, int flags) {
if (vars.size()==1 && dynamic_pointer_cast<LiteralMapExpression>(vars[0]->name)) return;
auto map = make_shared<LiteralMapExpression>(vars[0]->name->nearestToken()), defmap = make_shared<LiteralMapExpression>(vars[0]->name->nearestToken());
for (auto& var: vars) {
shared_ptr<Expression> key=nullptr, value=nullptr, name = nameExprToConstant(parser, var->name);
if (var->value) {
key = createBinaryOperation(name, T_EQ, var->value);
value = createBinaryOperation(var->name, T_EQ, var->value);
}
else {
key = name;
value = var->name;
}
map->items.push_back(make_pair(key, value));
}
vars.clear();
vars.push_back(make_shared<Variable>(map, defmap, flags));
}

shared_ptr<Statement> QParser::parseImportDecl () {
auto importSta = make_shared<ImportDeclaration>();
int flags = 0;
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VD_GLOBAL;
parseVarList(importSta->imports, flags);
multiVarExprToSingleLiteralMap(*this, importSta->imports, flags);
consume(T_IN, "Expected 'in' after import variables");
importSta->from = parseExpression(P_COMPREHENSION);
return importSta;
}

shared_ptr<Statement> QParser::parseImportDecl2 () {
auto importSta = make_shared<ImportDeclaration>();
int flags = 0;
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VD_GLOBAL;
importSta->from = parseExpression(P_COMPREHENSION);
consume(T_IMPORT, "Expected 'import' after import source");
parseVarList(importSta->imports, flags);
multiVarExprToSingleLiteralMap(*this, importSta->imports, flags);
return importSta;
}

shared_ptr<Statement> QParser::parseStatement () {
skipNewlines();
const ParserRule& rule = rules[nextToken().type];
if (rule.statement) return (this->*rule.statement)();
else if (cur.type==T_END) return nullptr;
else {
prevToken();
return parseExpression();
}}

shared_ptr<Statement> QParser::parseStatements () {
vector<shared_ptr<Statement>> statements;
while(!matchOneOf(T_END, T_RIGHT_BRACE)) {
shared_ptr<Statement> sta = parseStatement();
if (sta) statements.push_back(sta);
else break;
}
return make_shared<BlockStatement>(statements);
}

shared_ptr<Expression> QParser::parsePrefixOp () {
QTokenType op =  cur.type;
shared_ptr<Expression> right = parseExpression(P_PREFIX);
return make_shared<UnaryOperation>(op, right);
}

shared_ptr<Expression> QParser::parseInfixOp (shared_ptr<Expression> left) {
QTokenType op =  cur.type;
auto& rule = rules[op];
auto priority = rule.priority;
if (rule.flags&P_RIGHT) --priority;
shared_ptr<Expression> right = parseExpression(priority);
return createBinaryOperation(left, op, right);
}

shared_ptr<Expression> QParser::parseInfixIs (shared_ptr<Expression> left) {
bool negate = match(T_EXCL);
auto& rule = rules[T_IS];
auto priority = rule.priority;
if (rule.flags&P_RIGHT) --priority;
shared_ptr<Expression> right = parseExpression(priority);
right = createBinaryOperation(left, T_IS, right);
if (negate) right = make_shared<UnaryOperation>(T_EXCL, right);
return right;
}

shared_ptr<Expression> QParser::parseInfixNot (shared_ptr<Expression> left) {
consume(T_IN, "Expected 'in' after infix not");
auto& rule = rules[T_IN];
auto priority = rule.priority;
if (rule.flags&P_RIGHT) --priority;
shared_ptr<Expression> right = parseExpression(priority);
return make_shared<UnaryOperation>(T_EXCL, createBinaryOperation(left, T_IN, right));
}

shared_ptr<Expression> QParser::parseConditional  (shared_ptr<Expression> cond) {
shared_ptr<Expression> ifPart = parseExpression();
skipNewlines();
consume(T_COLON, ("Expected ':' between conditional branches"));
shared_ptr<Expression> elsePart = parseExpression();
return make_shared<ConditionalExpression>(cond, ifPart, elsePart);
}

shared_ptr<Expression> QParser::parseComprehension (shared_ptr<Expression> loopExpr) {
skipNewlines();
shared_ptr<ForStatement> firstFor = make_shared<ForStatement>(cur);
firstFor->parseHead(*this);
shared_ptr<Comprenable> expr = firstFor;
shared_ptr<Statement> rootStatement = firstFor, leafExpr = make_shared<YieldExpression>(loopExpr->nearestToken(), loopExpr);
while(true){
skipNewlines();
if (match(T_FOR)) {
shared_ptr<ForStatement> forSta = make_shared<ForStatement>(cur);
forSta->parseHead(*this);
expr->chain(forSta);
expr = forSta;
}
else if (match(T_IF)) {
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
expr->chain(ifSta);
expr = ifSta;
}
else if (match(T_WHILE)) {
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
ifSta->elsePart = make_shared<ReturnStatement>(cur);
expr->chain(ifSta);
expr = ifSta;
}
else if (match(T_BREAK)) {
consume(T_IF, "Expected 'if' after 'break' in comprehension expression");
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
ifSta->chain(make_shared<BreakStatement>(cur));
expr->chain(ifSta);
expr = ifSta;
}
else if (match(T_RETURN)) {
consume(T_IF, "Expected 'if' after 'return' in comprehension expression");
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
ifSta->chain(make_shared<ReturnStatement>(cur));
expr->chain(ifSta);
expr = ifSta;
}
else if (match(T_CONTINUE)) {
if (match(T_IF)) {
auto ifSta = make_shared<IfStatement>(parseExpression(P_COMPREHENSION));
ifSta->chain(make_shared<ContinueStatement>(cur));
expr->chain(ifSta);
expr = ifSta;
} else {
consume(T_WHILE, "Expected 'if' or 'while' after 'continue' in comprehension expression");
auto cond = parseExpression(P_COMPREHENSION);
QToken trueToken = { T_TRUE, nullptr, 0, true }, falseToken = { T_FALSE, nullptr, 0, false };
auto var = make_shared<NameExpression>(createTempName());
vector<shared_ptr<Variable>> vars = { make_shared<Variable>(var, make_shared<ConstantExpression>(trueToken) ) };
vector<shared_ptr<Statement>> rootBlock = { make_shared<VariableDeclaration>(vars), rootStatement }, leafBlock = {
make_shared<IfStatement>(var, 
make_shared<IfStatement>(cond, make_shared<ContinueStatement>(cur), createBinaryOperation(var, T_EQ, make_shared<ConstantExpression>(falseToken) ) 
))};
auto bs = make_shared<BlockStatement>(leafBlock);
rootStatement = make_shared<BlockStatement>(rootBlock);
expr->chain(bs);
expr = bs;
}}
else if (match(T_WITH)) {
//#########
auto openExpr = parseExpression(P_COMPREHENSION);
QToken varToken;
if (match(T_AS)) {
consume(T_NAME, "Expected variable name after 'as'");
varToken = cur;
}
else varToken = createTempName();
QString* closeName = QString::create(vm, ("close"), 5);
QToken closeToken = { T_NAME, closeName->data, closeName->length, QV(closeName, QV_TAG_STRING) };
auto varExpr = make_shared<NameExpression>(varToken);
vector<shared_ptr<Variable>> varDecls = { make_shared<Variable>(varExpr, openExpr) };
auto varDecl = make_shared<VariableDeclaration>(varDecls);
auto closeExpr = createBinaryOperation(varExpr, T_DOT, make_shared<NameExpression>(closeToken));
auto trySta = make_shared<TryStatement>(nullptr, nullptr, closeExpr, cur); 
vector<shared_ptr<Statement>> statements = { varDecl, trySta };
auto bs = make_shared<BlockStatement>(statements);
expr->chain(bs);
expr = trySta;
}
else break;
}
expr->chain(leafExpr);
return make_shared<ComprehensionExpression>(rootStatement, loopExpr);
}

shared_ptr<Expression> QParser::parseMethodCall (shared_ptr<Expression> receiver) {
vector<shared_ptr<Expression>> args;
shared_ptr<LiteralMapExpression> mapArg = nullptr;
if (!match(T_RIGHT_PAREN)) {
do {
shared_ptr<Expression> arg = parseUnpackOrExpression();
if (match(T_COLON)) {
shared_ptr<Expression> val;
if (matchOneOf(T_COMMA, T_RIGHT_PAREN, T_RIGHT_BRACKET, T_RIGHT_BRACE)) { val=arg; prevToken(); }
else val = parseExpression();
if (!mapArg) { mapArg = make_shared<LiteralMapExpression>(cur); args.push_back(mapArg); }
arg = nameExprToConstant(*this, arg);
mapArg->items.push_back(make_pair(arg, val));
}
else if (arg) args.push_back(arg);
} while(match(T_COMMA));
skipNewlines();
consume(T_RIGHT_PAREN, ("Expected ')' to close method call"));
}
if (args.size() >= std::numeric_limits<uint_local_index_t>::max() -2) parseError("Too many arguments passed");
return make_shared<CallExpression>(receiver, args);
}

shared_ptr<Expression> QParser::parseSubscript  (shared_ptr<Expression> receiver) {
vector<shared_ptr<Expression>> args;
do {
shared_ptr<Expression> arg = parseExpression();
if (arg) args.push_back(arg);
} while(match(T_COMMA));
skipNewlines();
consume(T_RIGHT_BRACKET, ("Expected ']' to close subscript"));
if (args.size() >= std::numeric_limits<uint_local_index_t>::max() -2) parseError("Too many arguments passed");
return make_shared<SubscriptExpression>(receiver, args);
}

shared_ptr<Expression> QParser::parseSwitchCase (shared_ptr<Expression> left) {
auto& rule = rules[nextToken().type];
if (rule.infix && rule.priority>=P_COMPARISON) {
return (this->*rule.infix)(left);
}
else if (rule.prefix && rule.priority>=P_COMPARISON) {
shared_ptr<Expression> right = (this->*rule.prefix)();
return createBinaryOperation(left, T_EQEQ, right);
}
else parseError("Expected literal, identifier or infix expression after 'case'");
return nullptr;
}

shared_ptr<Expression> QParser::parseSwitchExpression () {
auto sw = make_shared<SwitchExpression>();
pair<vector<shared_ptr<Expression>>, shared_ptr<Expression>>* activeCase = nullptr;
shared_ptr<Expression>* activeExpr = nullptr;
bool defaultDefined = false;
sw->var = make_shared<DupExpression>(cur);
sw->expr = parseExpression(P_COMPREHENSION);
skipNewlines();
consume(T_LEFT_BRACE, "Expected '{' to begin switch");
while(true){
skipNewlines();
if (match(T_CASE)) {
if (defaultDefined) parseError("Default case must appear last");
sw->cases.emplace_back();
activeCase = &sw->cases.back();
activeExpr = &activeCase->second;
activeCase->first.push_back(parseSwitchCase(sw->var));
while(match(T_COMMA)) activeCase->first.push_back(parseSwitchCase(sw->var));
match(T_COLON);
}
else if (match(T_DEFAULT)) {
if (defaultDefined) parseError("Duplicated default case");
defaultDefined=true;
activeCase = nullptr;
activeExpr = &sw->defaultCase;
match(T_COLON);
}
else if (match(T_RIGHT_BRACE)) break;
else if (match(T_END)) { result=CR_INCOMPLETE; break; }
else {
if (!activeCase) { parseError("Expected 'case' after beginnig of switch expression"); return nullptr; }
if (!activeExpr || !*activeExpr) { result=CR_INCOMPLETE; return nullptr; }
*activeExpr = parseExpression();
}}
return sw;
}

shared_ptr<Expression> QParser::parseSuper () {
shared_ptr<Expression> superExpr = make_shared<SuperExpression>(cur);
if (match(T_LEFT_PAREN)) {
auto expr = make_shared<NameExpression>(curMethodNameToken);
auto call = parseMethodCall(expr);
return createBinaryOperation(superExpr, T_DOT, call);
}
consume(T_DOT, ("Expected '.' or '('  after 'super'"));
shared_ptr<Expression> expr = parseExpression();
return createBinaryOperation(superExpr, T_DOT, expr);
}

shared_ptr<Expression> QParser::parseUnpackOrExpression (int priority) {
if (match(T_DOTDOTDOT)) return make_shared<UnpackExpression>(parseExpression(priority));
else return parseExpression(priority);
}

shared_ptr<Expression> QParser::parseName () {
return make_shared<NameExpression>(cur);
}

shared_ptr<Expression> QParser::parseField () {
if (matchOneOf(T_COMMA, T_RIGHT_PAREN, T_RIGHT_BRACKET, T_RIGHT_BRACE, T_EQGT, T_MINUSGT, T_SEMICOLON, T_LINE)) {
prevToken();
cur.value = QV::UNDEFINED;
return make_shared<ConstantExpression>(cur);
}
consume(T_NAME, ("Expected field name after '_'"));
return make_shared<FieldExpression>(cur);
}

shared_ptr<Expression> QParser::parseStaticField () {
consume(T_NAME, ("Expected static field name after '@_'"));
return make_shared<StaticFieldExpression>(cur);
}

shared_ptr<Expression> QParser::parseGenericMethodSymbol () {
skipNewlines();
if (nextNameToken(true).type!=T_NAME) parseError("Expected method name after '::'");
return make_shared<GenericMethodSymbolExpression>(cur);
}

shared_ptr<Expression> QParser::parseLiteral () {
auto literal = make_shared<ConstantExpression>(cur);
if (cur.type==T_NUM && matchOneOf(T_NAME, T_LEFT_PAREN)) {
shared_ptr<Expression> expr = nullptr;
if (cur.type==T_NAME) {
prevToken();
expr = parseExpression(P_FACTOR);
}
else if (cur.type==T_LEFT_PAREN) expr = parseGroupOrTuple();
return createBinaryOperation(expr, T_STAR, literal);
}
return literal;
}

shared_ptr<Expression> QParser::parseLiteralList () {
shared_ptr<LiteralListExpression> list = make_shared<LiteralListExpression>(cur);
if (!match(T_RIGHT_BRACKET)) {
do {
list->items.push_back(parseUnpackOrExpression());
} while (match(T_COMMA));
skipNewlines();
consume(T_RIGHT_BRACKET, ("Expected ']' to close list literal"));
}
return list;
}

shared_ptr<Expression> QParser::parseLiteralSet () {
shared_ptr<LiteralSetExpression> list = make_shared<LiteralSetExpression>(cur);
if (!match(T_GT)) {
do {
list->items.push_back(parseUnpackOrExpression(P_COMPARISON));
} while (match(T_COMMA));
skipNewlines();
consume(T_GT, ("Expected '>' to close set literal"));
}
return list;
}

shared_ptr<Expression> QParser::parseLiteralMap () {
shared_ptr<LiteralMapExpression> map = make_shared<LiteralMapExpression>(cur);
if (!match(T_RIGHT_BRACE)) {
do {
bool computed = false;
shared_ptr<Expression> key, value;
if (match(T_LEFT_BRACKET)) {
computed=true;
key =  parseExpression();
skipNewlines();
consume(T_RIGHT_BRACKET, ("Expected ']' to close computed map key"));
}
else key = parseUnpackOrExpression();
if (!match(T_COLON)) value = key;
else value = parseExpression();
if (!computed) key = nameExprToConstant(*this, key);
map->items.push_back(make_pair(key, value));
} while (match(T_COMMA));
skipNewlines();
consume(T_RIGHT_BRACE, ("Expected '}' to close map literal"));
}
return map;
}

shared_ptr<Expression> QParser::parseLiteralGrid () {
QToken token = cur;
vector<vector<shared_ptr<Expression>>> data;
begin: do {
data.emplace_back();
auto& row = data.back();
do {
skipNewlines();
auto expr = parseExpression(P_BITWISE);
if (!expr) return nullptr;
row.push_back(expr);
} while(match(T_COMMA));
if (row.size() != data[0].size()) parseError("All rows must be of the same size (%d)", data[0].size());
if (match(T_SEMICOLON)) goto begin;
skipNewlines();
consume(T_BAR, "Expected '|' to close literal grid expression");
skipNewlines();
} while(match(T_BAR));
return make_shared<LiteralGridExpression>(token, data);
}

shared_ptr<Expression> QParser::parseLiteralRegex () {
string pattern, options;
while(*in && *in!='/') {
if (*in=='\n' || *in=='\r') { parseError("Unterminated regex literal"); return nullptr; }
if (*in=='\\' && in[1]=='/') ++in;
pattern.push_back(*in++);
}
while (*++in && ((*in>='a' && *in<='z') || (*in>='A' && *in<='Z'))) options.push_back(*in);
return make_shared<LiteralRegexExpression>(cur, pattern, options);
}

shared_ptr<Expression> QParser::parseGroupOrTuple () {
auto initial = cur;
if (match(T_RIGHT_PAREN)) return make_shared<LiteralTupleExpression>(initial, vector<shared_ptr<Expression>>() );
shared_ptr<Expression> expr = parseUnpackOrExpression();
bool isTuple = isUnpack(expr);
skipNewlines();
if (isTuple) consume(T_COMMA, "Expected ',' for single-item tuple");
else isTuple = match(T_COMMA);
if (isTuple) {
vector<shared_ptr<Expression>> items = { expr };
if (match(T_RIGHT_PAREN)) return make_shared<LiteralTupleExpression>(initial, items );
do {
items.push_back(parseUnpackOrExpression());
} while(match(T_COMMA));
consume(T_RIGHT_PAREN, ("Expected ')' to close tuple"));
return make_shared<LiteralTupleExpression>(initial, items);
}
else {
skipNewlines();
consume(T_RIGHT_PAREN, ("Expected ')' to close parenthesized expression"));
return expr;
}}

shared_ptr<Expression> QParser::parseExpression (int priority) {
skipNewlines();
if (priority == P_MEMBER)  nextNameToken(false); 
else nextToken();
const ParserRule* rule = &rules[cur.type];
if (!rule->prefix) {
parseError(("Expected expression"));
result = cur.type==T_END? CR_INCOMPLETE : CR_FAILED;
return nullptr;
}
shared_ptr<Expression> right, left = (this->*(rule->prefix))();
while(true){
rule = &rules[nextToken().type];
if (!rule->infix || priority>=rule->priority || !(right = (this->*(rule->infix))(left)) ) {
prevToken();
return left;
}
else left = right;
}}

shared_ptr<TypeInfo> QParser::parseTypeInfo () {
consume(T_NAME, "Expected type name");
return make_shared<NamedTypeInfo>(cur);
}

static string printFuncInfo (const QFunction& func) {
return format("%s (arity=%d, consts=%d, upvalues=%d, bc=%d, file=%s)", func.name, static_cast<int>(func.nArgs), func.constantsEnd-func.constants, func.upvaluesEnd-func.upvalues, func.bytecodeEnd-func.bytecode, func.file);
}

string QV::print () const {
if (isNull()) return ("null");
else if (isUndefined()) return "undefined";
else if (isTrue()) return ("true");
else if (isFalse()) return ("false");
else if (isNum()) return format("%.14G", d);
else if (isString()) return ("\"") + asString() + ("\"");
else if (isNativeFunction()) return format("%s@%#0$16llX", ("NativeFunction"), i);
else if (isNormalFunction()) return format("%s@%#0$16llX: %s", ("NormalFunction"), i, printFuncInfo(*asObject<QFunction>()));
else if (isClosure()) return format("%s@%#0$16llX: %s", ("Closure"), i, printFuncInfo(asObject<QClosure>()->func));
else if (isOpenUpvalue()) return format("%s@%#0$16llX=>%s", ("Upvalue"), i, asPointer<Upvalue>()->get().print() );
else if (i==QV_VARARG_MARK) return "<VarArgMark>"; 
else {
QObject* obj = asObject<QObject>();
QClass* cls = dynamic_cast<QClass*>(obj);
QString* str = dynamic_cast<QString*>(obj);
if (cls) return format("Class:%s@%#0$16llX", cls->name, i);
else if (str) return format("String:'%s'@%#0$16llX", str->data, i);
else return format("%s@%#0$16llX", obj->type->name, i);
}}

shared_ptr<Expression> AssignmentOperation::optimize () {
if (optimized) return shared_this();
if (op>=T_PLUSEQ && op<=T_BARBAREQ) {
QTokenType newOp = static_cast<QTokenType>(op + T_PLUS - T_PLUSEQ);
right = createBinaryOperation(left, newOp, right);
}
optimized=true;
return BinaryOperation::optimize();
}

shared_ptr<Expression> ComprehensionExpression::optimize () {
rootStatement = rootStatement->optimizeStatement();
return shared_this(); 
}

void ForStatement::compile (QCompiler& compiler) {
if (traditional) compileTraditional(compiler);
else compileForEach(compiler);
}

void ForStatement::compileForEach (QCompiler& compiler) {
compiler.pushScope();
int iteratorSlot = compiler.findLocalVariable(compiler.createTempName(), LV_NEW | LV_CONST);
int iteratorSymbol = compiler.vm.findMethodSymbol(("iterator"));
int nextSymbol = compiler.vm.findMethodSymbol(("next"));
int subscriptSymbol = compiler.vm.findMethodSymbol(("[]"));
shared_ptr<NameExpression> loopVariable = loopVariables.size()==1? dynamic_pointer_cast<NameExpression>(loopVariables[0]->name) : nullptr;
bool destructuring = !loopVariable;
if (destructuring) loopVariable = make_shared<NameExpression>(compiler.createTempName());
compiler.writeDebugLine(inExpression->nearestToken());
inExpression->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, iteratorSymbol);
compiler.pushLoop();
compiler.pushScope();
int valueSlot = compiler.findLocalVariable(loopVariable->token, LV_NEW);
int loopStart = compiler.writePosition();
compiler.loops.back().condPos = compiler.writePosition();
compiler.writeDebugLine(inExpression->nearestToken());
writeOpLoadLocal(compiler, iteratorSlot);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, nextSymbol);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_UNDEFINED) });
if (destructuring) {
loopVariables[0]->value = loopVariable;
compiler.writeDebugLine(inExpression->nearestToken());
make_shared<VariableDeclaration>(loopVariables)->optimizeStatement()->compile(compiler);
}
compiler.writeDebugLine(loopStatement->nearestToken());
loopStatement->compile(compiler);
if (loopStatement->isExpression()) compiler.writeOp(OP_POP);
compiler.popScope();
compiler.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
compiler.popScope();
}

void ForStatement::compileTraditional (QCompiler& compiler) {
compiler.pushScope();
make_shared<VariableDeclaration>(loopVariables)->optimizeStatement()->compile(compiler);
compiler.pushLoop();
compiler.pushScope();
int loopStart = compiler.writePosition();
inExpression->compile(compiler);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_FALSY) });
loopStatement->compile(compiler);
if (loopStatement->isExpression()) compiler.writeOp(OP_POP);
compiler.loops.back().condPos = compiler.writePosition();
incrExpression->compile(compiler);
compiler.writeOp(OP_POP);
compiler.writeOpJumpBackTo(OP_JUMP_BACK, loopStart);
compiler.loops.back().endPos = compiler.writePosition();
compiler.popScope();
compiler.popLoop();
compiler.popScope();
}

void ComprehensionExpression::compile (QCompiler  & compiler) {
QCompiler fc(compiler.parser);
fc.parent = &compiler;
rootStatement->optimizeStatement()->compile(fc);
QFunction* func = fc.getFunction(0);
func->name = "<comprehension>";
compiler.result = fc.result;
int funcSlot = compiler.findConstant(QV(func, QV_TAG_NORMAL_FUNCTION));
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.vm.findGlobalSymbol("Fiber", LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CLOSURE, funcSlot);
compiler.writeOp(OP_CALL_FUNCTION_1);
}

shared_ptr<TypeInfo> NameExpression::computeType (QCompiler& compiler) {
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
LocalVariable* lv = nullptr;
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_READ, &lv);
if (slot>=0) return type->merge(lv->type, compiler);
slot = compiler.findUpvalue(token, LV_FOR_READ, &lv);
if (slot>=0) return type->merge(lv->type, compiler);
slot = compiler.vm.findGlobalSymbol(string(token.start, token.length), LV_EXISTING | LV_FOR_READ);
if (slot>=0) { 
auto curType = make_shared<ClassTypeInfo>(&(compiler.parser.vm.globalVariables[slot].getClass(compiler.parser.vm)));
return type->merge(curType, compiler);
}
ClassDeclaration* cls = compiler.getCurClass();
if (cls) {
//todo
return TypeInfo::MANY;
}
return TypeInfo::MANY;
}

void NameExpression::compile (QCompiler& compiler) {
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
LocalVariable* lv = nullptr;
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_READ, &lv);
if (slot==0 && compiler.getCurClass()) {
compiler.writeOp(OP_LOAD_THIS);
type = type->merge(lv->type, compiler);
return;
}
else if (slot>=0) { 
writeOpLoadLocal(compiler, slot);
type = type->merge(lv->type, compiler);
return;
}
slot = compiler.findUpvalue(token, LV_FOR_READ, &lv);
if (slot>=0) { 
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, slot);
type = type->merge(lv->type, compiler);
return;
}
slot = compiler.vm.findGlobalSymbol(string(token.start, token.length), LV_EXISTING | LV_FOR_READ);
if (slot>=0) { 
auto curType = make_shared<ClassTypeInfo>(&(compiler.parser.vm.globalVariables[slot].getClass(compiler.parser.vm)));
type = type->merge(curType, compiler);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, slot);
return;
}
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (cls) {
if (atLevel<=2) compiler.writeOp(OP_LOAD_THIS);
else compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, compiler.findUpvalue({ T_NAME, THIS, 4, QV::UNDEFINED }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(string(token.start, token.length)));
return;
}
compiler.compileError(token, ("Undefined variable"));
}

void NameExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
LocalVariable* lv = nullptr;
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
assignedValue->compile(compiler);
type = type->merge(assignedValue->getType(compiler), compiler);
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_WRITE, &lv);
if (slot>=0) {
writeOpStoreLocal(compiler, slot);
lv->type = type->merge(lv->type, compiler);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
slot = compiler.findUpvalue(token, LV_FOR_WRITE, &lv);
if (slot>=0) {
compiler.writeOpArg<uint_upvalue_index_t>(OP_STORE_UPVALUE, slot);
lv->type = type->merge(lv->type, compiler);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
slot = compiler.vm.findGlobalSymbol(string(token.start, token.length), LV_EXISTING | LV_FOR_WRITE);
if (slot>=0) {
compiler.writeOpArg<uint_global_symbol_t>(OP_STORE_GLOBAL, slot);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
else if (slot==LV_ERR_ALREADY_EXIST) {
compiler.compileError(token, ("Already existing variable"));
return;
}
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (cls) {
if (atLevel<=2) compiler.writeOp(OP_LOAD_THIS);
else compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, compiler.findUpvalue({ T_NAME, THIS, 4, QV::UNDEFINED }, LV_EXISTING | LV_FOR_READ));
string setterName(token.length+1, '=');
memcpy(const_cast<char*>(setterName.data()), token.start, token.length);
assignedValue->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, compiler.vm.findMethodSymbol(setterName));
return;
}
compiler.compileError(token, ("Undefined variable"));
}

void FieldExpression::compile (QCompiler& compiler) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
shared_ptr<TypeInfo>* fieldType = nullptr;
if (!cls) {
compiler.compileError(token, ("Can't use field outside of a class"));
return;
}
if (compiler.getCurMethod()->flags&FD_STATIC) {
compiler.compileError(token, ("Can't use field in a static method"));
return;
}
int fieldSlot = cls->findField(token, &fieldType);
if (fieldType) type = type->merge(*fieldType, compiler);
if (atLevel<=2) compiler.writeOpArg<uint_field_index_t>(OP_LOAD_THIS_FIELD, fieldSlot);
else {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
compiler.writeOpArg<uint_field_index_t>(OP_LOAD_FIELD, fieldSlot);
}}

void FieldExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
shared_ptr<TypeInfo>* fieldType = nullptr;
if (!cls) {
compiler.compileError(token, ("Can't use field outside of a class"));
return;
}
if (compiler.getCurMethod()->flags&FD_STATIC) {
compiler.compileError(token, ("Can't use field in a static method"));
return;
}
int fieldSlot = cls->findField(token, &fieldType);
assignedValue->compile(compiler);
type = type->merge(assignedValue->getType(compiler), compiler);
if (fieldType) *fieldType = type;
if (atLevel<=2) compiler.writeOpArg<uint_field_index_t>(OP_STORE_THIS_FIELD, fieldSlot);
else {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
compiler.writeOpArg<uint_field_index_t>(OP_STORE_FIELD, fieldSlot);
}}

void StaticFieldExpression::compile (QCompiler& compiler) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
shared_ptr<TypeInfo>* fieldType = nullptr;
if (!cls) {
compiler.compileError(token, ("Can't use static field oustide of a class"));
return;
}
bool isStatic = compiler.getCurMethod()->flags&FD_STATIC;
int fieldSlot = cls->findStaticField(token, &fieldType);
if (fieldType) type = type->merge(*fieldType, compiler);
if (atLevel<=2 && !isStatic) compiler.writeOpArg<uint_field_index_t>(OP_LOAD_THIS_STATIC_FIELD, fieldSlot);
else if (atLevel<=2) {
compiler.writeOp(OP_LOAD_THIS);
compiler.writeOpArg<uint_field_index_t>(OP_LOAD_STATIC_FIELD, fieldSlot);
}
else {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
if (!isStatic) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol("class"));
compiler.writeOpArg<uint_field_index_t>(OP_LOAD_STATIC_FIELD, fieldSlot);
}}

void StaticFieldExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
shared_ptr<TypeInfo>* fieldType = nullptr;
if (!cls) {
compiler.compileError(token, ("Can't use static field oustide of a class"));
return;
}
bool isStatic = compiler.getCurMethod()->flags&FD_STATIC;
int fieldSlot = cls->findStaticField(token, &fieldType);
assignedValue->compile(compiler);
type = type->merge(assignedValue->getType(compiler), compiler);
if (fieldType) *fieldType = type;
if (atLevel<=2 && !isStatic) compiler.writeOpArg<uint_field_index_t>(OP_STORE_THIS_STATIC_FIELD, fieldSlot);
else if (atLevel<=2) {
compiler.writeOp(OP_LOAD_THIS);
compiler.writeOpArg<uint_field_index_t>(OP_STORE_STATIC_FIELD, fieldSlot);
}
else {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
if (!isStatic) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol("class"));
compiler.writeOpArg<uint_field_index_t>(OP_STORE_STATIC_FIELD, fieldSlot);
}}

void SuperExpression::compile (QCompiler& compiler) {
auto cls = compiler.getCurClass();
if (!cls) compiler.compileError(superToken, "Can't use 'super' outside of a class");
else if (cls->parents.empty()) compiler.compileError(superToken, "Can't use 'super' when having no superclass");
else {
make_shared<NameExpression>(cls->parents[0])->optimize()->compile(compiler);
compiler.writeOp(OP_LOAD_THIS); 
}}

bool LiteralSequenceExpression::isAssignable () {
if (items.size()<1) return false;
for (auto& item: items) {
shared_ptr<Expression> expr = item;
auto bop = dynamic_pointer_cast<BinaryOperation>(item);
if (bop && bop->op==T_EQ) expr = bop->left;
if (!dynamic_pointer_cast<Assignable>(expr) && !dynamic_pointer_cast<UnpackExpression>(expr)) {
if (auto cst = dynamic_pointer_cast<ConstantExpression>(expr)) return cst->token.value.i == QV::UNDEFINED.i;
else return false;
}
}
return true;
}

void LiteralSequenceExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
compiler.pushScope();
QToken tmpToken = compiler.createTempName();
auto tmpVar = make_shared<NameExpression>(tmpToken);
int slot = compiler.findLocalVariable(tmpToken, LV_NEW | LV_CONST);
assignedValue->compile(compiler);
for (int i=0, n=items.size(); i<n; i++) {
shared_ptr<Expression> item = items[i], defaultValue = nullptr;
shared_ptr<TypeInfo> typeHint = nullptr;
bool unpack = false;
auto bop = dynamic_pointer_cast<BinaryOperation>(item);
auto th = dynamic_pointer_cast<TypeHintExpression>(item);
if (bop && bop->op==T_EQ) {
item = bop->left;
defaultValue = bop->right;
th = dynamic_pointer_cast<TypeHintExpression>(defaultValue);
}
if (th) {
item = th->expr;
typeHint = th->type;
}
auto assignable = dynamic_pointer_cast<Assignable>(item);
if (!assignable) {
auto unpackExpr = dynamic_pointer_cast<UnpackExpression>(item);
if (unpackExpr) {
assignable = dynamic_pointer_cast<Assignable>(unpackExpr->expr);
unpack = true;
if (i+1!=items.size()) compiler.compileError(unpackExpr->nearestToken(), "Unpack expression must appear last in assignment expression");
}}
if (!assignable || !assignable->isAssignable()) continue;
QToken indexToken = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(i)) };
shared_ptr<Expression> index = make_shared<ConstantExpression>(indexToken);
if (unpack) {
QToken minusOneToken = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(-1)) };
shared_ptr<Expression> minusOne = make_shared<ConstantExpression>(minusOneToken);
index = createBinaryOperation(index, T_DOTDOTDOT, minusOne);
}
vector<shared_ptr<Expression>> indices = { index };
auto subscript = make_shared<SubscriptExpression>(tmpVar, indices);
if (defaultValue) defaultValue = createBinaryOperation(subscript, T_QUESTQUEST, defaultValue)->optimize();
else defaultValue = subscript;
if (typeHint) defaultValue = make_shared<TypeHintExpression>(defaultValue, typeHint)->optimize();
assignable->compileAssignment(compiler, defaultValue);
if (i+1<n) compiler.writeOp(OP_POP);
}
compiler.popScope();
}

bool LiteralMapExpression::isAssignable () {
if (items.size()<1) return false;
for (auto& item: items) {
shared_ptr<Expression> expr = item.second;
auto bop = dynamic_pointer_cast<BinaryOperation>(item.second);
if (bop && bop->op==T_EQ) expr = bop->left;
if (!dynamic_pointer_cast<Assignable>(expr) && !dynamic_pointer_cast<GenericMethodSymbolExpression>(expr)) return false;
}
return true;
}

void LiteralMapExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
compiler.pushScope();
QToken tmpToken = compiler.createTempName();
int tmpSlot = compiler.findLocalVariable(tmpToken, LV_NEW | LV_CONST);
auto tmpVar = make_shared<NameExpression>(tmpToken);
assignedValue->compile(compiler);
bool first = true;
for (auto& item: items) {
shared_ptr<Expression> assigned = item.second, defaultValue = nullptr;
shared_ptr<TypeInfo> typeHint = nullptr;
auto bop = dynamic_pointer_cast<BinaryOperation>(assigned);
auto th = dynamic_pointer_cast<TypeHintExpression>(assigned);
if (bop && bop->op==T_EQ) {
assigned = bop->left;
defaultValue = bop->right;
th = dynamic_pointer_cast<TypeHintExpression>(defaultValue);
}
if (th) {
assigned = th->expr;
typeHint = th->type;
}
auto assignable = dynamic_pointer_cast<Assignable>(assigned);
if (!assignable) {
auto mh = dynamic_pointer_cast<GenericMethodSymbolExpression>(assigned);
if (mh) assignable = make_shared<NameExpression>(mh->token);
}
if (!assignable || !assignable->isAssignable()) continue;
if (!first) compiler.writeOp(OP_POP);
first=false;
shared_ptr<Expression> value = nullptr;
if (auto method = dynamic_pointer_cast<GenericMethodSymbolExpression>(item.first))  value = createBinaryOperation(tmpVar, T_DOT, make_shared<NameExpression>(method->token));
else {
shared_ptr<Expression> subscript = item.first;
if (auto field = dynamic_pointer_cast<FieldExpression>(subscript))  {
field->token.value = QV(compiler.vm, field->token.start, field->token.length);
subscript = make_shared<ConstantExpression>(field->token);
}
else if (auto field = dynamic_pointer_cast<StaticFieldExpression>(subscript))  {
field->token.value = QV(compiler.vm, field->token.start, field->token.length);
subscript = make_shared<ConstantExpression>(field->token);
}
vector<shared_ptr<Expression>> indices = { subscript };
value = make_shared<SubscriptExpression>(tmpVar, indices);
}
if (defaultValue) value = createBinaryOperation(value, T_QUESTQUEST, defaultValue)->optimize();
if (typeHint) value = make_shared<TypeHintExpression>(value, typeHint)->optimize();
assignable->compileAssignment(compiler, value);
}
compiler.popScope();
}

bool isItemFunctionnable (shared_ptr<Expression> item) {
shared_ptr<Expression> expr = item;
auto bop = dynamic_pointer_cast<BinaryOperation>(item);
if (bop && bop->op==T_EQ) expr = bop->left;
auto functionnable = dynamic_pointer_cast<Functionnable>(expr);
return functionnable && functionnable->isFunctionnable();
}

bool LiteralTupleExpression::isFunctionnable () {
return all_of(items.begin(), items.end(), isItemFunctionnable);
}

void LiteralTupleExpression::makeFunctionParameters (vector<shared_ptr<Variable>>& params) {
for (auto& item: items) {
auto var = make_shared<Variable>(nullptr, nullptr);
auto bx = dynamic_pointer_cast<BinaryOperation>(item);
if (bx && bx->op==T_EQ) {
var->name = bx->left;
var->value = bx->right;
}
else {
var->name = item;
var->flags |= VD_NODEFAULT;
}
params.push_back(var);
}}

bool LiteralListExpression::isFunctionnable () {
return all_of(items.begin(), items.end(), isItemFunctionnable);
}

void LiteralListExpression::makeFunctionParameters (vector<shared_ptr<Variable>>& params) {
auto var = make_shared<Variable>(shared_this(), make_shared<LiteralListExpression>(nearestToken()));
params.push_back(var);
}

bool LiteralMapExpression::isFunctionnable () {
return all_of(items.begin(), items.end(), [&](auto& item){ return isItemFunctionnable(item.second); });
}

void LiteralMapExpression::makeFunctionParameters (vector<shared_ptr<Variable>>& params) {
auto var = make_shared<Variable>(shared_this(), make_shared<LiteralMapExpression>(nearestToken()));
params.push_back(var);
}

void NameExpression::makeFunctionParameters (vector<shared_ptr<Variable>>& params) {
params.push_back(make_shared<Variable>(shared_this(), nullptr, VD_NODEFAULT));
}

shared_ptr<TypeInfo>  UnaryOperation::computeType (QCompiler& compiler) {
if (op==T_EXCL) return make_shared<ClassTypeInfo>(compiler.parser.vm.boolClass);
return expr->getType(compiler);
}

shared_ptr<Expression> UnaryOperation::optimize () { 
expr=expr->optimize();
if (auto cst = dynamic_pointer_cast<ConstantExpression>(expr)) {
QV& value = cst->token.value;
if (value.isNum() && BASE_NUMBER_UNOPS[op]) {
QToken token = cst->token;
token.value = BASE_NUMBER_UNOPS[op](value.d);
return make_shared<ConstantExpression>(token);
}
else if (op==T_EXCL) {
cst->token.value = value.isFalsy()? QV::TRUE : QV::FALSE;
return cst;
}
//other operations on non-number
}
return shared_this(); 
}

void UnaryOperation::compile (QCompiler& compiler) {
expr->compile(compiler);
auto type = expr->getType(compiler);
if (op==T_MINUS && type->isNum(compiler.parser.vm)) compiler.writeOp(OP_NEG);
else if (op==T_TILDE && type->isNum(compiler.parser.vm)) compiler.writeOp(OP_BINNOT);
else if (op==T_EXCL && type->isBool(compiler.parser.vm)) compiler.writeOp(OP_NOT);
else compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(rules[op].prefixOpName));
}

shared_ptr<TypeInfo>  BinaryOperation::computeType (QCompiler& compiler) {
if (op>=T_EQEQ && op<=T_GTE) return make_shared<ClassTypeInfo>(compiler.parser.vm.boolClass);
else if (op>=T_EQ && op<=T_BARBAREQ) return right->getType(compiler);
else return left->getType(compiler);
}

shared_ptr<Expression> BinaryOperation::optimize () { 
if (left && right && isComparison()) if (auto cc = optimizeChainedComparisons()) return cc->optimize();
if (left) left=left->optimize(); 
if (right) right=right->optimize();
if (!left || !right) return shared_this();
shared_ptr<ConstantExpression> c1 = dynamic_pointer_cast<ConstantExpression>(left), c2 = dynamic_pointer_cast<ConstantExpression>(right);
if (c1 && c2) {
QV &v1 = c1->token.value, &v2 = c2->token.value;
if (v1.isNum() && v2.isNum() && BASE_NUMBER_BINOPS[op]) {
QToken token = c1->token;
token.value = BASE_NUMBER_BINOPS[op](v1.d, v2.d);
return make_shared<ConstantExpression>(token);
}
//other operations on non-number
}
else if (c1 && op==T_BARBAR) return c1->token.value.isFalsy()? right : left;
else if (c1 && op==T_QUESTQUEST) return c1->token.value.isNullOrUndefined()? right : left;
else if (c1 && op==T_AMPAMP) return c1->token.value.isFalsy()? left: right;
return shared_this(); 
}

shared_ptr<Expression> BinaryOperation::optimizeChainedComparisons () {
vector<shared_ptr<Expression>> all = { right };
vector<QTokenType> ops = { op };
shared_ptr<Expression> next = left;
while(true){
auto bop = dynamic_pointer_cast<BinaryOperation>(next);
if (bop && bop->left && bop->right && bop->isComparison()) { 
all.push_back(bop->right);
ops.push_back(bop->op);
next = bop->left;
}
else {
all.push_back(next);
break;
}
}
if (all.size()>=3) {
shared_ptr<Expression> expr = all.back();
all.pop_back(); 
expr = createBinaryOperation(expr, ops.back(), all.back());
ops.pop_back();
while(all.size() && ops.size()) {
shared_ptr<Expression> l = all.back();
all.pop_back();
shared_ptr<Expression> r = all.back();
auto b = createBinaryOperation(l, ops.back(), r);
ops.pop_back();
expr = createBinaryOperation(expr, T_AMPAMP, b);
}
return expr->optimize();
}
return nullptr;
}

void BinaryOperation::compile  (QCompiler& compiler) {
left->compile(compiler);
right->compile(compiler);
if (left->getType(compiler)->isNum(compiler.parser.vm) && right->getType(compiler)->isNum(compiler.parser.vm) && BASE_OPTIMIZED_OPS[op]) {
compiler.writeOp(static_cast<QOpCode>(BASE_OPTIMIZED_OPS[op]));
}
else compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, compiler.vm.findMethodSymbol(rules[op].infixOpName));
}

void ShortCircuitingBinaryOperation::compile (QCompiler& compiler) {
QOpCode op = this->op==T_AMPAMP? OP_AND : (this->op==T_BARBAR? OP_OR : OP_NULL_COALESCING);
left->compile(compiler);
int pos = compiler.writeOpJump(op);
right->compile(compiler);
compiler.patchJump(pos);
}

void ConditionalExpression::compile (QCompiler& compiler) {
condition->compile(compiler);
int ifJump = compiler.writeOpJump(OP_JUMP_IF_FALSY);
ifPart->compile(compiler);
int elseJump = compiler.writeOpJump(OP_JUMP);
compiler.patchJump(ifJump);
elsePart->compile(compiler);
compiler.patchJump(elseJump);
}

shared_ptr<TypeInfo> NamedTypeInfo::resolve (QCompiler& compiler) {
LocalVariable* lv = nullptr;
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_READ, &lv);
if (slot>=0) {
//todo
return shared_from_this();
}
slot = compiler.findUpvalue(token, LV_FOR_READ, &lv);
if (slot>=0) { 
//todo
return shared_from_this();
}
slot = compiler.vm.findGlobalSymbol(string(token.start, token.length), LV_EXISTING | LV_FOR_READ);
if (slot>=0) { 
auto value = compiler.parser.vm.globalVariables[slot];
if (!value.isInstanceOf(compiler.parser.vm.classClass)) return shared_from_this();
QClass* cls = value.asObject<QClass>();
return make_shared<ClassTypeInfo>(cls);
}
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (cls) {
//todo
return shared_from_this();
}
return shared_from_this();
}

void SwitchExpression::compile (QCompiler& compiler) {
vector<int> endJumps;
expr->compile(compiler);
for (auto& c: cases) {
bool notFirst=false;
vector<int> condJumps;
for (auto& item: c.first) {
if (notFirst) condJumps.push_back(compiler.writeOpJump(OP_OR));
notFirst=true;
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
for (auto pos: condJumps) compiler.patchJump(pos);
int ifJump = compiler.writeOpJump(OP_JUMP_IF_FALSY);
compiler.writeOp(OP_POP);
c.second->compile(compiler);
endJumps.push_back(compiler.writeOpJump(OP_JUMP));
compiler.patchJump(ifJump);
}
compiler.writeOp(OP_POP);
if (defaultCase) defaultCase->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
for (auto pos: endJumps) compiler.patchJump(pos);
}

void SwitchStatement::compile (QCompiler& compiler) {
vector<int> jumps;
compiler.pushLoop();
compiler.pushScope();
compiler.writeDebugLine(expr->nearestToken());
make_shared<VariableDeclaration>(vector<shared_ptr<Variable>>({ make_shared<Variable>(var, expr) }))->optimizeStatement()->compile(compiler);
for (int i=0, n=cases.size(); i<n; i++) {
auto caseExpr = cases[i].first;
compiler.writeDebugLine(caseExpr->nearestToken());
caseExpr->compile(compiler);
jumps.push_back(compiler.writeOpJump(OP_JUMP_IF_TRUTY));
}
auto defaultJump = compiler.writeOpJump(OP_JUMP);
for (int i=0, n=cases.size(); i<n; i++) {
compiler.patchJump(jumps[i]);
compiler.pushScope();
for (auto& s: cases[i].second) {
s->compile(compiler);
if (s->isExpression()) compiler.writeOp(OP_POP);
}
compiler.popScope();
}
compiler.patchJump(defaultJump);
compiler.pushScope();
for (auto& s: defaultCase) {
s->compile(compiler);
if (s->isExpression()) compiler.writeOp(OP_POP);
}
compiler.popScope();
compiler.popScope();
compiler.loops.back().condPos = compiler.writePosition();
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
}

void SubscriptExpression::compile  (QCompiler& compiler) {
int subscriptSymbol = compiler.vm.findMethodSymbol("[]");
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
for (auto arg: args) arg->compile(compiler);
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, subscriptSymbol);
else writeOpCallMethod(compiler, args.size(), subscriptSymbol);
}

void SubscriptExpression::compileAssignment  (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
int subscriptSetterSymbol = compiler.vm.findMethodSymbol("[]=");
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
for (auto arg: args) arg->compile(compiler);
assignedValue->compile(compiler);
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, subscriptSetterSymbol);
else writeOpCallMethod(compiler, args.size() +1, subscriptSetterSymbol);
}

void MemberLookupOperation::compile (QCompiler& compiler) {
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
if (getter->token.type==T_END) getter->token = compiler.parser.curMethodNameToken;
int symbol = compiler.vm.findMethodSymbol(string(getter->token.start, getter->token.length));
left->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(super? OP_CALL_SUPER_1 : OP_CALL_METHOD_1, symbol);
return;
}
shared_ptr<CallExpression> call = dynamic_pointer_cast<CallExpression>(right);
if (call) {
getter = dynamic_pointer_cast<NameExpression>(call->receiver);
if (getter) {
if (getter->token.type==T_END) getter->token = compiler.parser.curMethodNameToken;
int symbol = compiler.vm.findMethodSymbol(string(getter->token.start, getter->token.length));
bool vararg = call->isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
left->compile(compiler);
call->compileArgs(compiler);
if (super&&vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_SUPER_VARARG, symbol);
else if (super) writeOpCallSuper(compiler, call->args.size(), symbol);
else if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, symbol);
else writeOpCallMethod(compiler, call->args.size(), symbol);
return;
}}
compiler.compileError(right->nearestToken(), ("Bad operand for '.' operator"));
}

void MemberLookupOperation::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> setter = dynamic_pointer_cast<NameExpression>(right);
if (setter) {
string sName = string(setter->token.start, setter->token.length) + ("=");
int symbol = compiler.vm.findMethodSymbol(sName);
left->compile(compiler);
assignedValue->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(super? OP_CALL_SUPER_2 : OP_CALL_METHOD_2, symbol);
return;
}
compiler.compileError(right->nearestToken(), ("Bad operand for '.' operator in assignment"));
}

void MethodLookupOperation::compile (QCompiler& compiler) {
left->compile(compiler);
shared_ptr<NameExpression> getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
int symbol = compiler.vm.findMethodSymbol(string(getter->token.start, getter->token.length));
compiler.writeOpArg<uint_method_symbol_t>(OP_LOAD_METHOD, symbol);
return;
}
compiler.compileError(right->nearestToken(), ("Bad operand for '::' operator"));
}

void MethodLookupOperation::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
left->compile(compiler);
shared_ptr<NameExpression> setter = dynamic_pointer_cast<NameExpression>(right);
if (setter) {
int symbol = compiler.vm.findMethodSymbol(string(setter->token.start, setter->token.length));
assignedValue->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_STORE_METHOD, symbol);
compiler.writeOp(OP_POP);
return;
}
compiler.compileError(right->nearestToken(), ("Bad operand for '::' operator in assignment"));
}

void CallExpression::compile (QCompiler& compiler) {
if (auto name=dynamic_pointer_cast<NameExpression>(receiver)) {
if (compiler.findLocalVariable(name->token, LV_EXISTING | LV_FOR_READ)<0 && compiler.findUpvalue(name->token, LV_FOR_READ)<0 && compiler.findGlobalVariable(name->token, LV_FOR_READ)<0 && compiler.getCurClass()) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
createBinaryOperation(make_shared<NameExpression>(thisToken), T_DOT, shared_this())->optimize()->compile(compiler);
return;
}}
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
QOpCode op = OP_CALL_FUNCTION_0;
if (compiler.lastOp==OP_LOAD_THIS) op = OP_CALL_METHOD_0;
compileArgs(compiler);
if (vararg) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else writeOpCallFunction(compiler, args.size());
}

void AssignmentOperation::compile (QCompiler& compiler) {
shared_ptr<Assignable> target = dynamic_pointer_cast<Assignable>(left);
if (target && target->isAssignable()) {
target->compileAssignment(compiler, right);
return;
}
compiler.compileError(left->nearestToken(), ("Invalid target for assignment"));
}

static vector<shared_ptr<NameExpression>>& decompose (QCompiler& compiler, shared_ptr<Expression> expr, vector<shared_ptr<NameExpression>>& names) {
if (auto name = dynamic_pointer_cast<NameExpression>(expr)) {
names.push_back(name);
return names;
}
if (auto seq = dynamic_pointer_cast<LiteralSequenceExpression>(expr)) {
for (auto& item: seq->items) decompose(compiler, item, names);
return names;
}
if (auto map = dynamic_pointer_cast<LiteralMapExpression>(expr)) {
for (auto& item: map->items) decompose(compiler, item.second, names);
return names;
}
auto bop = dynamic_pointer_cast<BinaryOperation>(expr);
if (bop && bop->op==T_EQ) {
decompose(compiler, bop->left, names);
return names;
}
if (auto prop = dynamic_pointer_cast<GenericMethodSymbolExpression>(expr)) {
names.push_back(make_shared<NameExpression>(prop->token));
return names;
}
if (auto unpack = dynamic_pointer_cast<UnpackExpression>(expr)) {
decompose(compiler, unpack->expr, names);
return names;
}
if (!dynamic_cast<FieldExpression*>(&*expr) && !dynamic_cast<StaticFieldExpression*>(&*expr)) compiler.compileError(expr->nearestToken(), "Invalid target for assignment in destructuring");
return names;
}

void VariableDeclaration::compile (QCompiler& compiler) {
vector<shared_ptr<Variable>> destructured;
for (auto& var: vars) {
if (!var->name) continue;
auto name = dynamic_pointer_cast<NameExpression>(var->name);
if (!name) {
destructured.push_back(var);
vector<shared_ptr<NameExpression>> names;
for (auto& nm: decompose(compiler, var->name, names)) {
if (var->flags&VD_GLOBAL) compiler.findGlobalVariable(nm->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0));
else { compiler.findLocalVariable(nm->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0)); compiler.writeOp(OP_LOAD_UNDEFINED); }
}
continue;
}
int slot = -1;
if ((var->flags&VD_GLOBAL)) slot = compiler.findGlobalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0));
else slot = compiler.findLocalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0));
for (auto& decoration: decorations) decoration->compile(compiler);
for (auto& decoration: var->decorations) decoration->compile(compiler);
if (auto fdecl = dynamic_pointer_cast<FunctionDeclaration>(var->value)) {
auto func = fdecl->compileFunction(compiler);
func->name = string(name->token.start, name->token.length);
}
else if (var->value) var->value->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
for (auto& decoration: var->decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
for (auto& decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
if (var->flags&VD_GLOBAL) {
compiler.writeOpArg<uint_global_symbol_t>(OP_STORE_GLOBAL, slot);
compiler.writeOp(OP_POP);
}
}//for decompose
for (auto& var: destructured) {
auto assignable = dynamic_pointer_cast<Assignable>(var->name);
if (!assignable || !assignable->isAssignable()) continue;
assignable->compileAssignment(compiler, var->value);
compiler.writeOp(OP_POP);
}
}//end VariableDeclaration::compile

void ImportDeclaration::compile (QCompiler& compiler) {
doCompileTimeImport(compiler.parser.vm, compiler.parser.filename, from);
QToken importToken = { T_NAME, "import", 6, QV::UNDEFINED }, fnToken = { T_STRING, "", 0, QV(QString::create(compiler.parser.vm, compiler.parser.filename), QV_TAG_STRING) };
auto importName = make_shared<NameExpression>(importToken);
auto fnConst = make_shared<ConstantExpression>(fnToken);
vector<shared_ptr<Expression>> importArgs = { fnConst, from };
auto importCall = make_shared<CallExpression>(importName, importArgs);
imports[0]->value = importCall;
make_shared<VariableDeclaration>(imports)->optimizeStatement()->compile(compiler);
}

shared_ptr<FunctionDeclaration> ClassDeclaration::findMethod (const QToken& name, bool isStatic) {
auto it = find_if(methods.begin(), methods.end(), [&](auto& m){ 
return m->name.length==name.length && strncmp(name.start, m->name.start, name.length)==0
&& ((!!(m->flags&FD_STATIC))==isStatic);
});
return it==methods.end()? nullptr : *it;
}

void ClassDeclaration::handleAutoConstructor (QCompiler& compiler, unordered_map<string,Field>& memberFields, bool isStatic) {
if (all_of(methods.begin(), methods.end(), [&](auto& m){ return isStatic!=!!(m->flags&FD_STATIC); })) return;
auto inits = make_shared<BlockStatement>();
vector<pair<string,Field>> initFields;
for (auto& field: memberFields) if (field.second.defaultValue) initFields.emplace_back(field.first, field.second);
sort(initFields.begin(), initFields.end(), [&](auto& a, auto& b){ return a.second.index<b.second.index; });
for (auto& fp: initFields) {
auto& f = fp.second;
shared_ptr<Expression> fieldExpr;
if (isStatic) fieldExpr = make_shared<StaticFieldExpression>(f.token);
else fieldExpr = make_shared<FieldExpression>(f.token);
auto assignment = createBinaryOperation(fieldExpr, T_QUESTQUESTEQ, f.defaultValue) ->optimize();
inits->statements.push_back(assignment);
}
QToken ctorToken = { T_NAME, CONSTRUCTOR, 11, QV::UNDEFINED };
auto ctor = findMethod(ctorToken, isStatic);
if (!ctor && (!isStatic || inits->statements.size() )) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto thisExpr = make_shared<NameExpression>(thisToken);
ctor = make_shared<FunctionDeclaration>(ctorToken);
ctor->flags = (isStatic? FD_STATIC : 0);
ctor->params.push_back(make_shared<Variable>(thisExpr));
if (isStatic) ctor->body = make_shared<SimpleStatement>(ctorToken);
else {
auto arg = make_shared<NameExpression>(compiler.parser.createTempName()); 
ctor->params.push_back(make_shared<Variable>(arg, nullptr, VD_VARARG)); 
ctor->flags |= FD_VARARG;
ctor->body = createBinaryOperation(make_shared<SuperExpression>(ctorToken), T_DOT, make_shared<CallExpression>(make_shared<NameExpression>(ctorToken), vector<shared_ptr<Expression>>({ make_shared<UnpackExpression>(arg) }) ));
}
methods.push_back(ctor);
}
if (ctor && inits->statements.size()) {
inits->chain(ctor->body);
ctor->body = inits;
}}

void ClassDeclaration::compile (QCompiler& compiler) {
handleAutoConstructor(compiler, fields, false);
handleAutoConstructor(compiler, staticFields, true);
for (auto decoration: decorations) decoration->compile(compiler);
struct FieldInfo {  uint_field_index_t nParents, nStaticFields, nFields; } fieldInfo = { static_cast<uint_field_index_t>(parents.size()), 0, 0 };
ClassDeclaration* oldClassDecl = compiler.curClass;
compiler.curClass = this;
int nameConstant = compiler.findConstant(QV(compiler.vm, string(name.start, name.length)));
compiler.writeDebugLine(name);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, nameConstant);
for (auto& parent: parents) NameExpression(parent) .compile(compiler);
int fieldInfoPos = compiler.writeOpArg<FieldInfo>(OP_NEW_CLASS, fieldInfo);
for (auto method: methods) {
int methodSymbol = compiler.vm.findMethodSymbol(string(method->name.start, method->name.length));
compiler.parser.curMethodNameToken = method->name;
//println("Compiling %s: flags=%#0$2X", string(name.start, name.length) + "::" + string(method->name.start, method->name.length), method->flags);
compiler.curMethod = method.get();
auto func = method->compileFunction(compiler);
compiler.curMethod = nullptr;
func->name = string(name.start, name.length) + "::" + string(method->name.start, method->name.length);
compiler.writeDebugLine(method->name);
if (method->flags&FD_STATIC) compiler.writeOpArg<uint_method_symbol_t>(OP_STORE_STATIC_METHOD, methodSymbol);
else compiler.writeOpArg<uint_method_symbol_t>(OP_STORE_METHOD, methodSymbol);
compiler.writeOp(OP_POP);
}
for (auto decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
if (findMethod({ T_NAME, CONSTRUCTOR, 11, QV::UNDEFINED }, true)) {
compiler.writeOp(OP_DUP);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(CONSTRUCTOR));
compiler.writeOp(OP_POP);
}
fieldInfo.nFields = fields.size();
fieldInfo.nStaticFields = staticFields.size();
compiler.patch<FieldInfo>(fieldInfoPos, fieldInfo);
compiler.curClass = oldClassDecl;
if (fields.size() >= std::numeric_limits<uint_field_index_t>::max()) compiler.compileError(nearestToken(), "Too many member fields");
if (staticFields.size() >= std::numeric_limits<uint_field_index_t>::max()) compiler.compileError(nearestToken(), "Too many static member fields");
}

void ExportDeclaration::compile (QCompiler& compiler) {
QToken exportsToken = { T_NAME, EXPORTS, 7, QV::UNDEFINED};
int subscriptSetterSymbol = compiler.parser.vm.findMethodSymbol(("[]="));
bool multi = exports.size()>1;
int exportsSlot = compiler.findLocalVariable(exportsToken, LV_EXISTING | LV_FOR_READ);
writeOpLoadLocal(compiler, exportsSlot);
for (auto& p: exports) {
if (multi) compiler.writeOp(OP_DUP);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, p.first.start, p.first.length), QV_TAG_STRING)));
p.second->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_3, subscriptSetterSymbol);
compiler.writeOp(OP_POP);
}
if (multi) compiler.writeOp(OP_POP);
}

void FunctionDeclaration::compileParams (QCompiler& compiler) {
vector<shared_ptr<Variable>> destructuring;
compiler.writeDebugLine(nearestToken());
for (auto& var: params) {
auto name = dynamic_pointer_cast<NameExpression>(var->name);
LocalVariable* lv = nullptr;
int slot;
if (!name) {
name = make_shared<NameExpression>(compiler.createTempName());
slot = compiler.findLocalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0), &lv);
var->value = var->value? createBinaryOperation(name, T_QUESTQUESTEQ, var->value)->optimize() : name;
destructuring.push_back(var);
if (lv) lv->type = var->value->getType(compiler)->merge(lv->type, compiler);
}
else {
slot = compiler.findLocalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0), &lv);
if (var->value) {
auto value = createBinaryOperation(name, T_QUESTQUESTEQ, var->value)->optimize();
value->compile(compiler);
compiler.writeOp(OP_POP);
if (lv) lv->type = value->getType(compiler)->merge(lv->type, compiler);
}}
if (var->decorations.size()) {
for (auto& decoration: var->decorations) decoration->compile(compiler);
writeOpLoadLocal(compiler, slot);
for (auto& decoration: var->decorations) writeOpCallFunction(compiler, 1);
writeOpStoreLocal(compiler, slot);
var->decorations.clear();
}
if (var->typeHint) {
auto typeHint = make_shared<TypeHintExpression>(name, var->typeHint)->optimize();
if (lv) lv->type = typeHint->getType(compiler)->merge(lv->type, compiler);
//todo: use the type hint
//typeHint->compile(compiler);
//compiler.writeOp(OP_POP);
}
}
if (destructuring.size()) {
make_shared<VariableDeclaration>(destructuring)->optimizeStatement()->compile(compiler);
}
}

QFunction* FunctionDeclaration::compileFunction (QCompiler& compiler) {
QCompiler fc(compiler.parser);
fc.parent = &compiler;
compiler.parser.curMethodNameToken = name;
compileParams(fc);
body=body->optimizeStatement();
fc.writeDebugLine(body->nearestToken());
body->compile(fc);
if (body->isExpression()) fc.writeOp(OP_POP);
QFunction* func = fc.getFunction(params.size());
compiler.result = fc.result;
func->vararg = (flags&FD_VARARG);
int funcSlot = compiler.findConstant(QV(func, QV_TAG_NORMAL_FUNCTION));
if (name.type==T_NAME) func->name = string(name.start, name.length);
else func->name = "<closure>";
if (flags&FD_FIBER) {
QToken fiberToken = { T_NAME, FIBER, 5, QV::UNDEFINED };
decorations.insert(decorations.begin(), make_shared<NameExpression>(fiberToken));
}
else if (flags&FD_ASYNC) {
QToken asyncToken = { T_NAME, ASYNC, 5, QV::UNDEFINED };
decorations.insert(decorations.begin(), make_shared<NameExpression>(asyncToken));
}
for (auto decoration: decorations) decoration->compile(compiler);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CLOSURE, funcSlot);
for (auto decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
return func;
}

shared_ptr<Statement> BlockStatement::optimizeStatement () { 
if (optimized) return shared_this(); 
for (auto& sta: statements) sta=sta->optimizeStatement(); 
doHoisting();
optimized = true;
return shared_this(); 
}

void BlockStatement::doHoisting () {
vector<shared_ptr<Statement>> statementsToAdd;
for (auto& sta: statements) {
auto vd = dynamic_pointer_cast<VariableDeclaration>(sta);
if (vd  && vd->vars.size()==1
&& !(vd->vars[0]->flags&VD_CONST)
&& dynamic_pointer_cast<NameExpression>(vd->vars[0]->name) 
&& (dynamic_pointer_cast<FunctionDeclaration>(vd->vars[0]->value) || dynamic_pointer_cast<ClassDeclaration>(vd->vars[0]->value))
) {
auto name = vd->vars[0]->name;
auto value = vd->vars[0]->value;
vd->vars[0]->value = nullptr;
statementsToAdd.push_back(vd->optimizeStatement());
sta = createBinaryOperation(name, T_EQ, value) ->optimize();
}}
if (statementsToAdd.size()) statements.insert(statements.begin(), statementsToAdd.begin(), statementsToAdd.end());
}

ClassDeclaration* QCompiler::getCurClass (int* atLevel) {
if (atLevel) ++(*atLevel);
if (curClass) return curClass;
else if (parent) return parent->getCurClass(atLevel);
else return nullptr;
}

FunctionDeclaration* QCompiler::getCurMethod () {
if (curMethod) return curMethod;
else if (parent) return parent->getCurMethod();
else return nullptr;
}

void QCompiler::pushLoop () {
pushScope();
loops.emplace_back( curScope, writePosition() );
}

void QCompiler::popLoop () {
popScope();
Loop& loop = loops.back();
for (auto p: loop.jumpsToPatch) {
switch(p.first){
case Loop::CONDITION: patchJump(p.second, loop.condPos); break;
case Loop::END: patchJump(p.second, loop.endPos); break;
}}
while (curScope>loop.scope) popScope();
loops.pop_back();
}

void QCompiler::pushScope () {
curScope++;
}

void QCompiler::popScope () {
auto newEnd = remove_if(localVariables.begin(), localVariables.end(), [&](auto& x){ return x.scope>=curScope; });
int nVars = localVariables.end() -newEnd;
localVariables.erase(newEnd, localVariables.end());
curScope--;
if (nVars>0) writeOpArg<uint_local_index_t>(OP_POP_SCOPE, nVars);
}

int QCompiler::countLocalVariablesInScope (int scope) {
if (scope<0) scope = curScope;
return count_if(localVariables.begin(), localVariables.end(), [&](auto& x){ return x.scope>=scope; });
}

int QCompiler::findLocalVariable (const QToken& name, int flags, LocalVariable** ptr) {
bool createNew = flags&LV_NEW, isConst = flags&LV_CONST;
auto rvar = find_if(localVariables.rbegin(), localVariables.rend(), [&](auto& x){
return x.name.length==name.length && strncmp(name.start, x.name.start, name.length)==0;
});
if (rvar==localVariables.rend() && !createNew && parser.vm.getOption(QVM::Option::VAR_DECL_MODE)!=QVM::Option::VAR_IMPLICIT) return -1;
else if (rvar==localVariables.rend() || (createNew && rvar->scope<curScope)) {
if (createNew && rvar!=localVariables.rend()) compileWarn(name, "Shadowig %s declared at line %d", string(rvar->name.start, rvar->name.length), parser.getPositionOf(rvar->name.start).first);
if (localVariables.size() >= std::numeric_limits<uint_local_index_t>::max()) compileError(name, "Too many local variables");
int n = localVariables.size();
localVariables.emplace_back(name, curScope, isConst);
if (ptr) *ptr = &localVariables[n];
return n;
}
else if (!createNew)  {
if (rvar->isConst && isConst) return LV_ERR_CONST;
int n = rvar.base() -localVariables.begin() -1;
if (ptr) *ptr = &localVariables[n];
return n;
}
else compileError(name, ("Variable already defined"));
return -1;
}

int QCompiler::findUpvalue (const QToken& token, int flags, LocalVariable** ptr) {
if (!parent) return -1;
int slot = parent->findLocalVariable(token, flags, ptr);
if (slot>=0) {
parent->localVariables[slot].hasUpvalues=true;
int upslot = addUpvalue(slot, false);
if (upslot >= std::numeric_limits<uint_upvalue_index_t>::max()) compileError(token, "Too many upvalues");
return upslot;
}
else if (slot==LV_ERR_CONST) return slot;
slot = parent->findUpvalue(token, flags, ptr);
if (slot>=0) return addUpvalue(slot, true);
if (slot >= std::numeric_limits<uint_upvalue_index_t>::max()) compileError(token, "Too many upvalues");
return slot;
}

int QCompiler::addUpvalue (int slot, bool upperUpvalue) {
auto it = find_if(upvalues.begin(), upvalues.end(), [&](auto& x){ return x.slot==slot && x.upperUpvalue==upperUpvalue; });
if (it!=upvalues.end()) return it - upvalues.begin();
int i = upvalues.size();
upvalues.push_back({ slot, upperUpvalue });
return i;
}

int QCompiler::findGlobalVariable (const QToken& name, int flags) {
return parser.vm.findGlobalSymbol(string(name.start, name.length), flags);
}

static shared_ptr<Statement> addReturnExports (shared_ptr<Statement> sta) {
if (!sta->isUsingExports()) return sta;
QToken exportsToken = { T_NAME, EXPORTS, 7, QV::UNDEFINED};
QToken exportMTToken = { T_LEFT_BRACE, EXPORTS, 7, QV::UNDEFINED};
auto exn = make_shared<NameExpression>(exportsToken);
auto exs = make_shared<ReturnStatement>(exportsToken, exn);
vector<shared_ptr<Variable>> exv = { make_shared<Variable>(exn, make_shared<LiteralMapExpression>(exportMTToken), VD_CONST) }; 
auto exvd = make_shared<VariableDeclaration>(exv);
auto bs = dynamic_pointer_cast<BlockStatement>(sta);
if (bs) {
bs->statements.insert(bs->statements.begin(), exvd);
bs->statements.push_back(exs);
}
else bs = make_shared<BlockStatement>(vector<shared_ptr<Statement>>({ exvd, sta, exs }));
return bs;
}

int QCompiler::findConstant (const QV& value) {
auto it = find_if(constants.begin(), constants.end(), [&](const auto& v){
return value.i == v.i;
});
if (it!=constants.end()) return it - constants.begin();
else {
int n = constants.size();
constants.push_back(value);
return n;
}}

int QVM::findMethodSymbol (const string& name) {
auto it = find(methodSymbols.begin(), methodSymbols.end(), name);
if (it!=methodSymbols.end()) return it - methodSymbols.begin();
else {
int n = methodSymbols.size();
methodSymbols.push_back(name);
return n;
}}

int QVM::findGlobalSymbol (const string& name, int flags) {
auto it = globalSymbols.find(name);
if (it!=globalSymbols.end()) {
auto& gv = it->second;
if (flags&LV_NEW && varDeclMode!=Option::VAR_IMPLICIT_GLOBAL) return LV_ERR_ALREADY_EXIST;
else if ((flags&LV_FOR_WRITE) && gv.isConst) return LV_ERR_CONST;
return gv.index;
}
else if (!(flags&LV_NEW) && varDeclMode!=Option::VAR_IMPLICIT_GLOBAL) return -1;
else {
int n = globalSymbols.size();
globalSymbols[name] = { n, flags&LV_CONST };
globalVariables.push_back(QV::UNDEFINED);
return n;
}}

const uint8_t* printOpCode (const uint8_t* bc) {
if (!OPCODE_NAMES[*bc]) {
println(std::cerr, "Unknown opcode: %d(%<#0$2X)", *bc);
println(std::cerr, "Following bytes: %0$2X %0$2X %0$2X %0$2X %0$2X %0$2X %0$2X %0$2X", bc[0], bc[1], bc[2], bc[3], bc[4], bc[5], bc[6], bc[7]);
return bc+8;
}
int nArgs = OPCODE_INFO[*bc].nArgs, argFormat = OPCODE_INFO[*bc].argFormat;
print("%s", OPCODE_NAMES[*bc]);
bc++;
for (int i=0; i<nArgs; i++) {
int arglen = argFormat&0x0F;
argFormat>>=4;
switch(arglen){
case 1:
print(", %d", static_cast<int>(*bc));
bc++;
break;
case 2:
print(", %d", *reinterpret_cast<const uint16_t*>(bc));
bc+=2;
break;
case 4:
print(", %d", *reinterpret_cast<const uint32_t*>(bc));
bc+=4;
break;
case 8:
print(", %d", *reinterpret_cast<const uint64_t*>(bc));
bc+=8;
break;
}}
println("");
return bc;
}

void QCompiler::dump () {
if (!parent) {
println("\nOpcode list: ");
for (int i=0; OPCODE_NAMES[i]; i++) {
println("%d (%#0$2X). %s", i, i, OPCODE_NAMES[i]);
}
println("");

println("\n%d method symbols:", vm.methodSymbols.size());
for (int i=0, n=vm.methodSymbols.size(); i<n; i++) println("%d (%<#0$4X). %s", i, vm.methodSymbols[i]);
println("\n%d global symbols:", vm.globalSymbols.size());
//for (int i=0, n=vm.globalSymbols.size(); i<n; i++) println("%d (%<#0$4X). %s", i, vm.globalSymbols[i]);
println("\n%d constants:", constants.size());
for (int i=0, n=constants.size(); i<n; i++) println("%d (%<#0$4X). %s", i, constants[i].print());
}
else print("\nBytecode of method:");

string bcs = out.str();
println("\nBytecode length: %d bytes", bcs.length());
for (const uint8_t *bc = reinterpret_cast<const uint8_t*>( bcs.data() ), *end = bc + bcs.length(); bc<end; ) bc = printOpCode(bc);
}

template<class... A> void QCompiler::compileError (const QToken& token, const char* fmt, const A&... args) {
auto p = parser.getPositionOf(token.start);
int line = p.first, column = p.second;
parser.vm.messageReceiver({ Swan::CompilationMessage::Kind::ERROR, format(fmt, args...), string(token.start, token.length), parser.displayName, line, column });
result = CR_FAILED;
}

template<class... A> void QCompiler::compileWarn (const QToken& token, const char* fmt, const A&... args) {
auto p = parser.getPositionOf(token.start);
int line = p.first, column = p.second;
parser.vm.messageReceiver({ Swan::CompilationMessage::Kind::WARNING, format(fmt, args...), string(token.start, token.length), parser.displayName, line, column });
}

template<class... A> void QCompiler::compileInfo (const QToken& token, const char* fmt, const A&... args) {
auto p = parser.getPositionOf(token.start);
int line = p.first, column = p.second;
parser.vm.messageReceiver({ Swan::CompilationMessage::Kind::INFO, format(fmt, args...), string(token.start, token.length), parser.displayName, line, column });
}

void QCompiler::compile () {
shared_ptr<Statement> sta = parser.parseStatements();
if (sta && !parser.result) {
//println("Code before optimization:");
//println("%s", sta->print() );
sta = addReturnExports(sta);
sta=sta->optimizeStatement();
//println("Code after optimization:");
//println("%s", sta->print());
sta->compile(*this);
if (constants.size() >= std::numeric_limits<uint_constant_index_t>::max()) compileError(sta->nearestToken(), "Too many constant values");
if (vm.methodSymbols.size()  >= std::numeric_limits<uint_method_symbol_t>::max()) compileError(sta->nearestToken(), "Too many method symbols");
if (vm.globalVariables.size()  >= std::numeric_limits<uint_global_symbol_t>::max()) compileError(sta->nearestToken(), "Too many global variables");
// Implicit return last expression
if (lastOp==OP_POP) {
seek(-1);
writeOp(OP_RETURN);
}
else if (lastOp!=OP_RETURN) {
writeOp(OP_LOAD_UNDEFINED);
writeOp(OP_RETURN);
}
}
}

QFunction* QCompiler::getFunction (int nArgs) {
compile();

string bc = out.str();
size_t nConsts = constants.size(), nUpvalues = upvalues.size(), bcSize = bc.size();

QFunction* function = QFunction::create(vm, nConsts, nUpvalues, bcSize);
function->nArgs = nArgs;
copy(make_move_iterator(constants.begin()), make_move_iterator(constants.end()), function->constants);
copy(make_move_iterator(upvalues.begin()), make_move_iterator(upvalues.end()), function->upvalues);
memmove(function->bytecode, bc.data(), bcSize);
function->file = parent&&!vm.compileDbgInfo? "" : parser.filename;
result = result? result : parser.result;
return function;
}
