#include "Parser.hpp"
#include "Compiler.hpp"
#include "../vm/ExtraAlgorithms.hpp"
#include "../vm/VM.hpp"
#include "../vm/Upvalue.hpp"
#include "../vm/NatSort.hpp"
#include "../../include/cpprintf.hpp"
#include<cmath>
#include<cstdlib>
#include<memory>
#include<algorithm>
#include<unordered_map>
#include<boost/algorithm/string.hpp>
#include<utf8.h>
using namespace std;

extern const char* OPCODE_NAMES[];
extern double strtod_c  (const char*, char** = nullptr);

static const char 
*THIS = "this",
*EXPORTS = "exports";

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
if (nArgs<16) compiler.writeOp(static_cast<QOpCode>(OP_CALL_FUNCTION_0 + nArgs));
else compiler.writeOpArg<uint8_t>(OP_CALL_FUNCTION, nArgs);
}

static inline void writeOpCallMethod (QCompiler& compiler, uint8_t nArgs, uint_method_symbol_t symbol) {
if (nArgs<16) compiler.writeOpArg<uint_method_symbol_t>(static_cast<QOpCode>(OP_CALL_METHOD_1 + nArgs), symbol);
else compiler.writeOpArgs<uint_method_symbol_t, uint8_t>(OP_CALL_METHOD, symbol, nArgs+1);
}

static inline void writeOpCallSuper  (QCompiler& compiler, uint8_t nArgs, uint_method_symbol_t symbol) {
if (nArgs<16) compiler.writeOpArg<uint_method_symbol_t>(static_cast<QOpCode>(OP_CALL_SUPER_1 + nArgs), symbol);
else compiler.writeOpArgs<uint_method_symbol_t, uint8_t>(OP_CALL_SUPER, symbol, nArgs+1);
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

struct Statement: std::enable_shared_from_this<Statement>  {
inline shared_ptr<Statement> shared_this () { return shared_from_this(); }
virtual string print() = 0;
virtual const QToken& nearestToken () = 0;
virtual bool isExpression () { return false; }
virtual bool isDecorable () { return false; }
virtual bool isUsingExports () { return false; }
virtual shared_ptr<Statement> optimizeStatement () { return shared_this(); }
virtual void compile (QCompiler& compiler) {}
};

struct Expression: Statement {
bool isExpression () { return true; }
inline shared_ptr<Expression> shared_this () { return static_pointer_cast<Expression>(shared_from_this()); }
virtual shared_ptr<Expression> optimize () { return shared_this(); }
shared_ptr<Statement> optimizeStatement () { return optimize(); }
};

struct Assignable {
virtual void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) = 0;
virtual bool isAssignable () { return true; }
};

struct Decorable {
vector<shared_ptr<Expression>> decorations;
virtual bool isDecorable () { return true; }
};

struct ConstantExpression: Expression {
QToken token;
ConstantExpression(QToken x): token(x) {}
const QToken& nearestToken () { return token; }
string print () { return token.value.print(); }
void compile (QCompiler& compiler) {
QV& value = token.value;
if (value.isNull()) compiler.writeOp(OP_LOAD_NULL);
else if (value.isFalse()) compiler.writeOp(OP_LOAD_FALSE);
else if (value.isTrue()) compiler.writeOp(OP_LOAD_TRUE);
else if (value.isInt8()) compiler.writeOpArg<int8_t>(OP_LOAD_INT8, static_cast<int>(value.d));
else compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(token.value));
}
};

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

struct LiteralListExpression: LiteralSequenceExpression {
LiteralListExpression (const QToken& t): LiteralSequenceExpression(t) {}
void compile (QCompiler& compiler) {
compiler.writeDebugLine(nearestToken());
int listSymbol = compiler.vm.findGlobalSymbol(("List"), LV_EXISTING | LV_FOR_READ);
if (isSingleSequence()) {
int ofSymbol = compiler.vm.findMethodSymbol("of");
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, listSymbol);
items[0]->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, ofSymbol);
} else {
bool vararg = isVararg();
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, listSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
if (vararg) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else writeOpCallFunction(compiler, items.size());
}}
string print () {
string s = ("[");
bool first=true;
for (auto item: items) {
if (!first) s += (", ");
s += item->print();
first=false;
}
s+= ("]");
return s;
}};

struct LiteralSetExpression: LiteralSequenceExpression {
LiteralSetExpression (const QToken& t): LiteralSequenceExpression(t) {}
void compile (QCompiler& compiler) {
int setSymbol = compiler.vm.findGlobalSymbol(("Set"), LV_EXISTING | LV_FOR_READ);
compiler.writeDebugLine(nearestToken());
if (isSingleSequence()) {
int ofSymbol = compiler.vm.findMethodSymbol("of");
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, setSymbol);
items[0]->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, ofSymbol);
} else {
bool vararg = isVararg();
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, setSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
if (vararg) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else writeOpCallFunction(compiler, items.size());
}}
string print () {
string s = ("<");
bool first=true;
for (auto item: items) {
if (!first) s += (", ");
s += item->print();
first=false;
}
s+= (">");
return s;
}};


struct LiteralMapExpression: Expression, Assignable {
QToken type;
vector<pair<shared_ptr<Expression>, shared_ptr<Expression>>> items;
LiteralMapExpression (const QToken& t): type(t) {}
const QToken& nearestToken () { return type; }
shared_ptr<Expression> optimize () { for (auto& p: items) { p.first = p.first->optimize(); p.second = p.second->optimize(); } return shared_this(); }
virtual bool isAssignable () override;
virtual void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) override;
void compile (QCompiler& compiler) {
vector<shared_ptr<Expression>> unpacks;
int mapSymbol = compiler.vm.findGlobalSymbol(("Map"), LV_EXISTING | LV_FOR_READ);
int subscriptSetterSymbol = compiler.vm.findMethodSymbol(("[]="));
int callSymbol = compiler.vm.findMethodSymbol(("()"));
compiler.writeDebugLine(nearestToken());
if (items.size()==1 && isComprehension(items[0].first)) {
int ofSymbol = compiler.vm.findMethodSymbol("of");
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, mapSymbol);
items[0].first->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, ofSymbol);
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
if (vararg) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
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
string print () {
string s = ("{");
bool first=true;
for (auto item: items) {
if (!first) s += (", ");
s += item.first->print() + (": ") + item.second->print();
first=false;
}
s+= ("}");
return s;
}};

struct LiteralTupleExpression: LiteralSequenceExpression {
LiteralTupleExpression (const QToken& t, const vector<shared_ptr<Expression>>& p): LiteralSequenceExpression(t, p) {}
void compile (QCompiler& compiler) {
int tupleSymbol = compiler.vm.findGlobalSymbol(("Tuple"), LV_EXISTING | LV_FOR_READ);
compiler.writeDebugLine(nearestToken());
if (isSingleSequence()) {
int ofSymbol = compiler.vm.findMethodSymbol("of");
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, tupleSymbol);
items[0]->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, ofSymbol);
} else {
bool vararg = any_of(items.begin(), items.end(), isUnpack);
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, tupleSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
if (vararg) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else writeOpCallFunction(compiler, items.size());
}}
string print () {
string s = ("(");
bool first=true;
for (auto item: items) {
if (!first) s += (", ");
s += item->print();
first=false;
}
s+= (")");
return s;
}};

struct LiteralGridExpression: Expression {
QToken token;
vector<vector<shared_ptr<Expression>>> data;
LiteralGridExpression (const QToken& t, const vector<vector<shared_ptr<Expression>>>& v): token(t), data(v) {}
const QToken& nearestToken () { return token; }
void compile (QCompiler& compiler) {
int gridSymbol = compiler.vm.findGlobalSymbol(("Grid"), LV_EXISTING | LV_FOR_READ);
int size = data.size() * data[0].size();
compiler.writeDebugLine(nearestToken());
if (size>120) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, gridSymbol);
compiler.writeOpArg<uint8_t>(OP_LOAD_INT8, data[0].size());
compiler.writeOpArg<uint8_t>(OP_LOAD_INT8, data.size());
for (auto& row: data) {
for (auto& value: row) {
value->compile(compiler);
}}
if (size>120) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else writeOpCallFunction(compiler, size+2);
}
string print () {
string s = "\r\n";
for (auto& row: data) {
s += "| ";
bool first=true;
for (auto& expr: row) {
if (!first) s+=", ";
s += expr->print();
first=false;
}
s += " |\r\n";
}
return s;
}};


struct LiteralRegexExpression: Expression {
QToken tok;
string pattern, options;
LiteralRegexExpression(const QToken& tk, const string& p, const string& o): tok(tk), pattern(p), options(o) {}
const QToken& nearestToken () { return tok; }
void compile (QCompiler& compiler) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "Regex", 5, QV() }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, pattern), QV_TAG_STRING)));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, options), QV_TAG_STRING)));
compiler.writeOp(OP_CALL_FUNCTION_2);
}
string print () { return "/" + pattern + "/" + options; }
};

struct NameExpression: Expression, Assignable  {
QToken token;
NameExpression (QToken x): token(x) {}
const QToken& nearestToken () { return token; }
void compile (QCompiler& compiler);
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue);
string print () { return string(token.start, token.length); }
};

struct FieldExpression: Expression, Assignable  {
QToken token;
FieldExpression (QToken x): token(x) {}
const QToken& nearestToken () { return token; }
void compile (QCompiler& compiler);
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue);
string print () { return ("@")  + string(token.start, token.length); }
};

struct StaticFieldExpression: Expression, Assignable  {
QToken token;
StaticFieldExpression (QToken x): token(x) {}
const QToken& nearestToken () { return token; }
void compile (QCompiler& compiler);
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue);
string print () { return ("@@") + string(token.start, token.length); }
};

struct SuperExpression: Expression {
QToken superToken;
SuperExpression (const QToken& t): superToken(t) {}
const QToken& nearestToken () { return superToken; }
void compile (QCompiler& compiler) { compiler.writeOp(OP_LOAD_THIS); }
string print () { return ("super"); }
};

struct GenericMethodSymbolExpression: Expression {
QToken token;
GenericMethodSymbolExpression (const QToken& t): token(t) {}
const QToken& nearestToken () { return token; }
string print () { return "(::" + string(token.start, token.length) + ")"; }
void compile (QCompiler& compiler) {
int symbol = compiler.parser.vm.findMethodSymbol(string(token.start, token.length));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(symbol | QV_TAG_GENERIC_SYMBOL_FUNCTION)));
}};

struct BinaryOperation: Expression {
shared_ptr<Expression> left, right;
QTokenType op;
BinaryOperation (shared_ptr<Expression> l, QTokenType o, shared_ptr<Expression> r): left(l), right(r), op(o)  {}
const QToken& nearestToken () { return left->nearestToken(); }
shared_ptr<Expression> optimize ();
void compile (QCompiler& compiler);
string print();
};

struct UnaryOperation: Expression {
shared_ptr<Expression> expr;
QTokenType op;
UnaryOperation  (QTokenType op0, shared_ptr<Expression> e0): op(op0), expr(e0) {}
shared_ptr<Expression> optimize ();
void compile (QCompiler& compiler);
const QToken& nearestToken () { return expr->nearestToken(); }
string print();
};

struct ShortCircuitingBinaryOperation: BinaryOperation {
ShortCircuitingBinaryOperation (shared_ptr<Expression> l, QTokenType o, shared_ptr<Expression> r): BinaryOperation(l,o,r) {}
void compile (QCompiler& compiler);
};

struct ConditionalExpression: Expression {
shared_ptr<Expression> condition, ifPart, elsePart;
ConditionalExpression (shared_ptr<Expression> cond, shared_ptr<Expression> ifp, shared_ptr<Expression> ep): condition(cond), ifPart(ifp), elsePart(ep) {}
const QToken& nearestToken () { return condition->nearestToken(); }
shared_ptr<Expression> optimize () { condition=condition->optimize(); ifPart=ifPart->optimize(); elsePart=elsePart->optimize(); return shared_this(); }
void compile (QCompiler& compiler);
string print () {
return condition->print() + ("? ") + ifPart->print() + (" : ") + elsePart->print();
}};

struct ComprehensionExpression: Expression {
vector<shared_ptr<struct ForStatement>> subCompr;
shared_ptr<Expression> filterExpression, loopExpression;
ComprehensionExpression (shared_ptr<Expression> lp): filterExpression(nullptr), loopExpression(lp) {}
shared_ptr<Expression> optimize ();
const QToken& nearestToken () { return loopExpression->nearestToken(); }
void compile (QCompiler&);
string print () {
string s = loopExpression->print();
if (filterExpression) s += (" if ") + filterExpression->print();
return s;
}};

struct UnpackExpression: Expression {
shared_ptr<Expression> expr;
UnpackExpression   (shared_ptr<Expression> e0): expr(e0) {}
shared_ptr<Expression> optimize () { expr=expr->optimize(); return shared_this(); }
void compile (QCompiler& compiler) {
expr->compile(compiler);
compiler.writeOp(OP_UNPACK_SEQUENCE);
}
const QToken& nearestToken () { return expr->nearestToken(); }
string print() { return "..."+expr->print(); }
};

struct AbstractCallExpression: Expression {
shared_ptr<Expression> receiver;
QTokenType type;
std::vector<shared_ptr<Expression>> args;
AbstractCallExpression (shared_ptr<Expression> recv0, QTokenType tp, const std::vector<shared_ptr<Expression>>& args0): receiver(recv0), type(tp), args(args0) {}
const QToken& nearestToken () { return receiver->nearestToken(); }
shared_ptr<Expression> optimize () { receiver=receiver->optimize(); for (auto& arg: args) arg=arg->optimize(); return shared_this(); }
bool isVararg () { return isUnpack(receiver) || any_of(args.begin(), args.end(), isUnpack); }
void compileArgs (QCompiler& compiler) {
for (auto arg: args) arg->compile(compiler);
}
string print();
};

struct CallExpression: AbstractCallExpression {
CallExpression (shared_ptr<Expression> recv0, const std::vector<shared_ptr<Expression>>& args0): AbstractCallExpression(recv0, T_LEFT_PAREN, args0) {}
void compile (QCompiler& compiler);
};

struct SubscriptExpression: AbstractCallExpression, Assignable  {
SubscriptExpression (shared_ptr<Expression> recv0, const std::vector<shared_ptr<Expression>>& args0): AbstractCallExpression(recv0, T_LEFT_BRACKET, args0) {}
void compile (QCompiler& compiler);
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue);
};

struct MemberLookupOperation: BinaryOperation, Assignable  {
MemberLookupOperation (shared_ptr<Expression> l, shared_ptr<Expression> r): BinaryOperation(l, T_DOT, r) {}
void compile (QCompiler& compiler);
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue);
};

struct MethodLookupOperation: BinaryOperation, Assignable  {
MethodLookupOperation (shared_ptr<Expression> l, shared_ptr<Expression> r): BinaryOperation(l, T_COLONCOLON, r) {}
void compile (QCompiler& compiler);
void compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue);
};

struct AssignmentOperation: BinaryOperation {
bool optimized;
AssignmentOperation (shared_ptr<Expression> l, QTokenType o, shared_ptr<Expression> r): BinaryOperation(l,o,r), optimized(false)  {}
shared_ptr<Expression> optimize ();
void compile (QCompiler& compiler);
};

struct YieldExpression: Expression {
QToken token;
shared_ptr<Expression> expr;
YieldExpression (const QToken& tk, shared_ptr<Expression> e): token(tk), expr(e) {}
const QToken& nearestToken () { return token; }
shared_ptr<Expression> optimize () { if (expr) expr=expr->optimize(); return shared_this(); }
void compile (QCompiler& compiler) {
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_NULL);
compiler.writeOp(OP_YIELD);
}
string print () { 
if (expr) return ("yield ") + expr->print();
else return ("yield");
}};

struct ImportExpression: Expression {
shared_ptr<Expression> from;
ImportExpression (shared_ptr<Expression> f): from(f) {}
shared_ptr<Expression> optimize () { from=from->optimize(); return shared_this(); }
const QToken& nearestToken () { return from->nearestToken(); }
string print () { return "import " + from->print(); }
void compile (QCompiler& compiler) {
doCompileTimeImport(compiler.parser.vm, compiler.parser.filename, from);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "import", 6, QV() }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, compiler.parser.filename), QV_TAG_STRING)));
from->compile(compiler);
compiler.writeOp(OP_CALL_FUNCTION_2);
}};

struct SimpleStatement: Statement {
QToken token;
SimpleStatement (const QToken& t): token(t) {}
const QToken& nearestToken () { return token; }
string print () { return TOKEN_NAMES[token.type]; }
};

struct IfStatement: Statement  {
shared_ptr<Expression> condition;
shared_ptr<Statement> ifPart, elsePart;
IfStatement (shared_ptr<Expression> cond, shared_ptr<Statement> ifp, shared_ptr<Statement> ep = nullptr): condition(cond), ifPart(ifp), elsePart(ep) {}
shared_ptr<Statement> optimizeStatement () { condition=condition->optimize(); ifPart=ifPart->optimizeStatement(); if (elsePart) elsePart=elsePart->optimizeStatement(); return shared_this(); }
const QToken& nearestToken () { return condition->nearestToken(); }
void compile (QCompiler& compiler) {
compiler.writeDebugLine(condition->nearestToken());
condition->compile(compiler);
int skipIfJump = compiler.writeOpJump(OP_JUMP_IF_FALSY);
compiler.pushScope();
compiler.writeDebugLine(ifPart->nearestToken());
ifPart->compile(compiler);
if (ifPart->isExpression()) compiler.writeOp(OP_POP);
compiler.lastOp = OP_LOAD_NULL;
compiler.popScope();
if (elsePart) {
int skipElseJump = compiler.writeOpJump(OP_JUMP);
compiler.patchJump(skipIfJump);
compiler.pushScope();
compiler.writeDebugLine(elsePart->nearestToken());
elsePart->compile(compiler);
if (elsePart->isExpression()) compiler.writeOp(OP_POP);
compiler.lastOp = OP_LOAD_NULL;
compiler.popScope();
compiler.patchJump(skipElseJump);
}
else {
compiler.patchJump(skipIfJump);
}}
string print () {
string s = ("if (") + condition->print() + (") ")  + ifPart->print();
if (elsePart) s+= (" else ") + elsePart->print();
return s;
}};

struct SwitchStatement: Statement {
shared_ptr<Expression> expr, comparator;
vector<pair<shared_ptr<Expression>, vector<shared_ptr<Statement>>>> cases;
vector<shared_ptr<Statement>> defaultCase;
shared_ptr<Statement> optimizeStatement () { 
expr = expr->optimize();
for (auto& p: cases) { 
p.first = p.first->optimize(); 
for (auto& s: p.second) s = s->optimizeStatement();
}
for (auto& s: defaultCase) s=s->optimizeStatement();
if (comparator) comparator = comparator->optimize();
return shared_this(); 
}
const QToken& nearestToken () { return expr->nearestToken(); }
void compile (QCompiler& compiler) {
vector<int> jumps;
compiler.pushLoop();
compiler.pushScope();
compiler.writeDebugLine(expr->nearestToken());
int caseSlot = compiler.findLocalVariable(compiler.createTempName(), LV_NEW | LV_CONST);
int compSlot = compiler.findLocalVariable(compiler.createTempName(), LV_NEW | LV_CONST);
expr->compile(compiler);
if (comparator) comparator->compile(compiler);
else compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(compiler.vm.findMethodSymbol("==") | QV_TAG_GENERIC_SYMBOL_FUNCTION)));
for (int i=0, n=cases.size(); i<n; i++) {
auto caseExpr = cases[i].first;
compiler.writeDebugLine(caseExpr->nearestToken());
writeOpLoadLocal(compiler, compSlot);
writeOpLoadLocal(compiler, caseSlot);
caseExpr->compile(compiler);
writeOpCallFunction(compiler, 2);
jumps.push_back(compiler.writeOpJump(OP_JUMP_IF_TRUTY));
}
auto defaultJump = compiler.writeOpJump(OP_JUMP);
for (int i=0, n=cases.size(); i<n; i++) {
compiler.patchJump(jumps[i]);
compiler.pushScope();
for (auto& s: cases[i].second) s->compile(compiler);
compiler.popScope();
}
compiler.patchJump(defaultJump);
compiler.pushScope();
for (auto& s: defaultCase) s->compile(compiler);
compiler.popScope();
compiler.popScope();
compiler.loops.back().condPos = compiler.writePosition();
compiler.loops.back().endPos = compiler.writePosition();
compiler.popLoop();
}
string print () {
string s = "switch(" + expr->print() + ") with(" + comparator->print() + ") {";
//for (auto& p: cases) s += "case " + p.first->print() + ":\r\n" + p.second->print() + "\r\n";
//if (defaultCase) s+= "else: \r\n" + defaultCase->print();
s += "}\r\n";
return s;
}};

struct ForStatement: Statement {
vector<QToken> loopVariables;
shared_ptr<Expression> inExpression;
shared_ptr<Statement> loopStatement;
bool loopVarIsConst;
QTokenType destructuring;
ForStatement (): loopVariables(), inExpression(nullptr), loopStatement(nullptr), loopVarIsConst(false), destructuring(T_END) {}
shared_ptr<Statement> optimizeStatement () { inExpression=inExpression->optimize(); if (loopStatement) loopStatement=loopStatement->optimizeStatement(); return shared_this(); }
const QToken& nearestToken () { return loopVariables[0]; }
void parseHead (QParser& parser);
void compile (QCompiler& compiler);
string print () {
return ("for (") + string(loopVariables[0].start, loopVariables[0].length) + (" in ") + inExpression->print() + (") ") + loopStatement->print();
}};

struct WhileStatement: Statement {
shared_ptr<Expression> condition;
shared_ptr<Statement> loopStatement;
WhileStatement (shared_ptr<Expression> cond, shared_ptr<Statement> lst): condition(cond), loopStatement(lst) {}
shared_ptr<Statement> optimizeStatement () { condition=condition->optimize(); loopStatement=loopStatement->optimizeStatement(); return shared_this(); }
const QToken& nearestToken () { return condition->nearestToken(); }
void compile (QCompiler& compiler) {
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
string print () {
return ("while (") + condition->print() + (") ") + loopStatement->print();
}};

struct RepeatWhileStatement: Statement {
shared_ptr<Expression> condition;
shared_ptr<Statement> loopStatement;
RepeatWhileStatement (shared_ptr<Expression> cond, shared_ptr<Statement> lst): condition(cond), loopStatement(lst) {}
shared_ptr<Statement> optimizeStatement () { condition=condition->optimize(); loopStatement=loopStatement->optimizeStatement(); return shared_this(); }
const QToken& nearestToken () { return loopStatement->nearestToken(); }
void compile (QCompiler& compiler) {
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
string print () {
return ("repeat ") + loopStatement->print() + (" while ") + condition->print();
}};

struct ContinueStatement: SimpleStatement {
int count;
ContinueStatement (const QToken& tk, int n): SimpleStatement(tk), count(n) {}
void compile (QCompiler& compiler) {
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
BreakStatement (const QToken& tk, int n): SimpleStatement(tk), count(n) {}
void compile (QCompiler& compiler) {
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
ReturnStatement (const QToken& retk, shared_ptr<Expression> e0): returnToken(retk), expr(e0) {}
const QToken& nearestToken () { return expr? expr->nearestToken() : returnToken; }
shared_ptr<Statement> optimizeStatement () { if (expr) expr=expr->optimize(); return shared_this(); }
void compile (QCompiler& compiler) {
compiler.writeDebugLine(nearestToken());
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_NULL);
compiler.writeOp(OP_RETURN);
}
string print () { if (expr) return ("return ") + expr->print(); else return ("return null"); }
};

struct ThrowStatement: Statement {
QToken returnToken;
shared_ptr<Expression> expr;
ThrowStatement (const QToken& retk, shared_ptr<Expression> e0): returnToken(retk), expr(e0) {}
const QToken& nearestToken () { return expr? expr->nearestToken() : returnToken; }
shared_ptr<Statement> optimizeStatement () { if (expr) expr=expr->optimize(); return shared_this(); }
void compile (QCompiler& compiler) {
compiler.writeDebugLine(nearestToken());
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_NULL);
compiler.writeOp(OP_THROW);
}
string print () { return "throw " + expr->print(); }
};

struct TryStatement: Statement {
shared_ptr<Statement> tryPart, catchPart, finallyPart;
QToken catchVar;
TryStatement (shared_ptr<Statement> tp, shared_ptr<Statement> cp, shared_ptr<Statement> fp, const QToken& cv): tryPart(tp), catchPart(cp), finallyPart(fp), catchVar(cv)  {}
const QToken& nearestToken () { return tryPart->nearestToken(); }
shared_ptr<Statement> optimizeStatement () { 
tryPart = tryPart->optimizeStatement();
if (catchPart) catchPart=catchPart->optimizeStatement();
if (finallyPart) finallyPart = finallyPart->optimizeStatement();
return shared_this(); }
void compile (QCompiler& compiler) {
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
string print () {
string s = "try " + tryPart->print();
if (catchPart) s += " catch " + catchPart->print();
if (finallyPart) s+=" finally " + finallyPart->print();
return s;
}
};

struct BlockStatement: Statement {
vector<shared_ptr<Statement>> statements;
BlockStatement (const vector<shared_ptr<Statement>>& sta): statements(sta) {}
shared_ptr<Statement> optimizeStatement () { for (auto& sta: statements) sta=sta->optimizeStatement(); return shared_this(); }
const QToken& nearestToken () { return statements[0]->nearestToken(); }
bool isUsingExports () override { return any_of(statements.begin(), statements.end(), [&](auto s){ return s && s->isUsingExports(); }); }
void compile (QCompiler& compiler) {
compiler.pushScope();
for (auto sta: statements) {
compiler.writeDebugLine(sta->nearestToken());
sta->compile(compiler);
if (sta->isExpression()) compiler.writeOp(OP_POP);
}
compiler.popScope();
}
string print () {
string s = ("{\r\n");
for (auto sta: statements) s += sta->print() + ("\r\n");
s += ("}\r\n");
return s;
}};

struct VariableDeclaration: Statement, Decorable {
vector<pair<QToken,shared_ptr<Expression>>> vars;
int flags;
VariableDeclaration (const vector<pair<QToken,shared_ptr<Expression>>>& vd, int flgs): vars(vd), flags(flgs)   {}
const QToken& nearestToken () { return vars[0].first; }
shared_ptr<Statement> optimizeStatement () { for (auto& v: vars) v.second=v.second->optimize(); return shared_this(); }
virtual bool isDecorable () override { return true; }
virtual bool isUsingExports () override { return flags&VD_EXPORT; }
void compile (QCompiler& compiler);
string print () {
string s;
if (flags&VD_EXPORT) s+=("export ");
if (flags&VD_GLOBAL) s+=("global ");
s += (flags&VD_CONST)? ("const ") : ("var ");
for (int i=0, n=vars.size(); i<n; i++) {
QToken& var = vars[i].first;
shared_ptr<Expression> val = vars[i].second;
if (i>0) s+=(", ");
s += string(var.start, var.length) + (" = ") + val->print();
}
return s;
}};

struct ExportDeclaration: Statement  {
vector<pair<QToken,shared_ptr<Expression>>> exports;
const QToken& nearestToken () { return exports[0].first; }
shared_ptr<Statement> optimizeStatement () { for (auto& v: exports) v.second=v.second->optimize(); return shared_this(); }
bool isUsingExports () override { return true; }
void compile (QCompiler& compiler);
string print () { return "export!"; }
};

struct ImportDeclaration: Statement {
shared_ptr<Expression> from;
vector<pair<QToken,QToken>> imports;
ImportDeclaration (shared_ptr<Expression> f): from(f) {}
shared_ptr<Statement> optimizeStatement () { from=from->optimize(); return shared_this(); }
const QToken& nearestToken () { return from->nearestToken(); }
string print () {
string s = "import " + from->print() + " for ";
bool first=true;
for (auto& p: imports) {
if (!first) s+=", ";
first=false;
s += string(p.first.start, p.first.length) + " as " + string(p.second.start, p.second.length);
}
return s;
}
void compile (QCompiler& compiler) {
doCompileTimeImport(compiler.parser.vm, compiler.parser.filename, from);
int subscriptSymbol = compiler.parser.vm.findMethodSymbol("[]");
vector<int> varSlots;
compiler.writeDebugLine(nearestToken());
for (auto& p: imports) {
varSlots.push_back(compiler.findLocalVariable(p.second, LV_NEW | LV_CONST));
compiler.writeOp(OP_LOAD_NULL);
}
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "import", 6, QV() }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, compiler.parser.filename), QV_TAG_STRING)));
from->compile(compiler);
compiler.writeOp(OP_CALL_FUNCTION_2);
for (int i=0, n=imports.size(); i<n; i++) {
auto& p = imports[i];
auto slot = varSlots[i];
if (n>1) compiler.writeOp(OP_DUP);
compiler.writeDebugLine(p.second);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, p.first.start, p.first.length), QV_TAG_STRING)));
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, subscriptSymbol);
writeOpStoreLocal(compiler, slot);
compiler.writeOp(OP_POP);
}
if (imports.size()>1) compiler.writeOp(OP_POP);
}};

struct FunctionParameter: Expression, Decorable {
QToken arg;
shared_ptr<Expression> defaultValue;
shared_ptr<Expression> typeCheck;
int flags;
FunctionParameter (const QToken& a, shared_ptr<Expression> dv = nullptr, shared_ptr<Expression> tc = nullptr, int fl=0): arg(a), defaultValue(dv), typeCheck(tc), flags(fl) {}
const QToken& nearestToken () { return arg; }
void compile (QCompiler& fc);
virtual bool isDecorable () { return true; }
shared_ptr<Expression> optimize () { if (defaultValue) defaultValue=defaultValue->optimize(); if (typeCheck) typeCheck=typeCheck->optimize(); return shared_this(); }
string print () {
string s = "";
if (flags&FP_CONST) s += "const ";
if (flags&FP_FIELD_ASSIGN) s+="_";
else if (flags&FP_STATIC_FIELD_ASSIGN) s+="__";
s += string(arg.start, arg.length);
if (typeCheck) s += (" is ") + typeCheck->print();
if (defaultValue) s += (" = ") + defaultValue->print();
return s;
}};

struct FunctionDeclaration: Expression, Decorable {
QToken name;
vector<shared_ptr<FunctionParameter>> params;
shared_ptr<Statement> body;
int flags;
FunctionDeclaration (const QToken& nm, const vector<shared_ptr<FunctionParameter>>& fp, shared_ptr<Statement> b, int fl): name(nm), params(fp), body(b), flags(fl)     {}
const QToken& nearestToken () { return name; }
QFunction* compileFunction (QCompiler& compiler);
void compile (QCompiler& compiler) { compileFunction(compiler); }
shared_ptr<Statement> optimizeStatement () { body=body->optimizeStatement(); for (auto& fp: params) fp=static_pointer_cast<FunctionParameter>(fp->optimize()); return shared_this(); }
virtual bool isDecorable () { return true; }
string print () {
string s = string(name.start, name.length) + (" (");
for (int i=0, n=params.size(); i<n; i++) {
if (i>0) s+=(", ");
s += params[i]->print();
}
s += (" ) ");
s += body->print();
return s;
}};

struct ClassDeclaration: Expression, Decorable  {
QToken name;
int flags;
vector<string> fields, staticFields;
vector<QToken> parents;
vector<shared_ptr<FunctionDeclaration>> methods;
ClassDeclaration (const QToken& name0, int flgs): name(name0), flags(flgs)  {}
int findField (const string& name) {  return findName(fields, name, true);  }
int findStaticField (const string& name) { return findName(staticFields, name, true); }
const QToken& nearestToken () { return name; }
shared_ptr<Expression> optimize () { for (auto& m: methods) m=static_pointer_cast<FunctionDeclaration>(m->optimize()); return shared_this(); }
void compile (QCompiler&);
virtual bool isDecorable () { return true; }
string print () {
string s = ("class ") + (name.start? string(name.start, name.length) : string(("<anonymous>"))) + (" {\r\n");
for (auto method: methods) s += method->print() + ("\r\n");
s += ("}\r\n");
return s;
}};

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
typedef void(QParser::*MemberFn)(ClassDeclaration&,bool);
PrefixFn prefix = nullptr;
InfixFn infix = nullptr;
StatementFn statement = nullptr;
MemberFn member = nullptr;
const char* prefixOpName = nullptr;
const char* infixOpName = nullptr;
int priority = P_HIGHEST;
};

static std::unordered_map<int,ParserRule> rules = {
#define PREFIX(token, prefix, name) { T_##token, { &QParser::parse##prefix, nullptr, nullptr, nullptr, (#name), nullptr, P_PREFIX }}
#define PREFIX_OP(token, prefix, name) { T_##token, { &QParser::parse##prefix, nullptr, nullptr, &QParser::parseMethodDecl, (#name), nullptr, P_PREFIX }}
#define INFIX(token, infix, name, priority) { T_##token, { nullptr, &QParser::parse##infix, nullptr, nullptr, nullptr, (#name), P_##priority }}
#define INFIX_OP(token, infix, name, priority) { T_##token, { nullptr, &QParser::parse##infix, nullptr, &QParser::parseMethodDecl, nullptr,  (#name), P_##priority }}
#define MULTIFIX(token, prefix, infix, prefixName, infixName, priority) { T_##token, { &QParser::parse##prefix, &QParser::parse##infix, nullptr, nullptr, (#prefixName), (#infixName), P_##priority }}
#define OPERATOR(token, prefix, infix, prefixName, infixName, priority) { T_##token, { &QParser::parse##prefix, &QParser::parse##infix, nullptr, &QParser::parseMethodDecl, (#prefixName), (#infixName), P_##priority }}
#define STATEMENT(token, func) { T_##token, { nullptr, nullptr, &QParser::parse##func, nullptr, nullptr, nullptr, P_PREFIX }}

OPERATOR(PLUS, PrefixOp, InfixOp, unp, +, TERM),
OPERATOR(MINUS, PrefixOp, InfixOp, unm, -, TERM),
INFIX_OP(STAR, InfixOp, *, FACTOR),
INFIX_OP(BACKSLASH, InfixOp, \\, FACTOR),
INFIX_OP(PERCENT, InfixOp, %, FACTOR),
INFIX_OP(STARSTAR, InfixOp, **, EXPONENT),
OPERATOR(AMP, Lambda, InfixOp, &, &, BITWISE),
INFIX_OP(CIRC, InfixOp, ^, BITWISE),
INFIX_OP(LTLT, InfixOp, <<, BITWISE),
INFIX_OP(GTGT, InfixOp, >>, BITWISE),
INFIX_OP(DOTDOT, InfixOp, .., RANGE),
OPERATOR(DOTDOTDOT, Unpack, InfixOp, ..., ..., RANGE),
INFIX(AMPAMP, InfixOp, &&, LOGICAL),
INFIX(BARBAR, InfixOp, ||, LOGICAL),
INFIX(QUESTQUEST, InfixOp, ??, LOGICAL),

INFIX(EQ, InfixOp, =, ASSIGNMENT),
INFIX(PLUSEQ, InfixOp, +=, ASSIGNMENT),
INFIX(MINUSEQ, InfixOp, -=, ASSIGNMENT),
INFIX(STAREQ, InfixOp, *=, ASSIGNMENT),
INFIX(SLASHEQ, InfixOp, /=, ASSIGNMENT),
INFIX(BACKSLASHEQ, InfixOp, \\=, ASSIGNMENT),
INFIX(PERCENTEQ, InfixOp, %=, ASSIGNMENT),
INFIX(STARSTAREQ, InfixOp, **=, ASSIGNMENT),
INFIX(BAREQ, InfixOp, |=, ASSIGNMENT),
INFIX(AMPEQ, InfixOp, &=, ASSIGNMENT),
INFIX(CIRCEQ, InfixOp, ^=, ASSIGNMENT),
INFIX(ATEQ, InfixOp, @=, ASSIGNMENT),
INFIX(LTLTEQ, InfixOp, <<=, ASSIGNMENT),
INFIX(GTGTEQ, InfixOp, >>=, ASSIGNMENT),
INFIX(AMPAMPEQ, InfixOp, &&=, ASSIGNMENT),
INFIX(BARBAREQ, InfixOp, ||=, ASSIGNMENT),
INFIX(QUESTQUESTEQ, InfixOp, ?\x3F=, ASSIGNMENT),

INFIX_OP(EQEQ, InfixOp, ==, COMPARISON),
INFIX_OP(EXCLEQ, InfixOp, !=, COMPARISON),
OPERATOR(LT, LiteralSet, InfixOp, <, <, COMPARISON),
INFIX_OP(GT, InfixOp, >, COMPARISON),
INFIX_OP(LTE, InfixOp, <=, COMPARISON),
INFIX_OP(GTE, InfixOp, >=, COMPARISON),
INFIX_OP(IS, InfixOp, is, COMPARISON),
INFIX_OP(IN, InfixOp, in, COMPARISON),

OPERATOR(QUEST, PrefixOp, Conditional, ?, ?, CONDITIONAL),
PREFIX_OP(EXCL, PrefixOp, !),
PREFIX_OP(TILDE, PrefixOp, ~),
PREFIX(DOLLAR, Lambda, $),

#ifndef NO_REGEX
OPERATOR(SLASH, LiteralRegex, InfixOp, /, /, FACTOR),
#else
INFIX_OP(SLASH, InfixOp, /, FACTOR),
#endif
#ifndef NO_GRID
OPERATOR(BAR, LiteralGrid, InfixOp, |, |, BITWISE),
#else
INFIX_OP(BAR, InfixOp, |, BITWISE),
#endif

{ T_LEFT_PAREN, { &QParser::parseGroupOrTuple, &QParser::parseMethodCall, nullptr, &QParser::parseMethodDecl, nullptr, ("()"), P_CALL }},
{ T_LEFT_BRACKET, { &QParser::parseLiteralList, &QParser::parseSubscript, nullptr, &QParser::parseMethodDecl, nullptr, ("[]"), P_SUBSCRIPT }},
{ T_LEFT_BRACE, { &QParser::parseLiteralMap, nullptr, &QParser::parseBlock, nullptr, nullptr, nullptr, P_PREFIX }},
{ T_AT, { &QParser::parseDecoratedExpression, &QParser::parseInfixOp, &QParser::parseDecoratedStatement, &QParser::parseDecoratedDecl, "@", "@", P_EXPONENT }},
{ T_FUNCTION, { &QParser::parseLambda, nullptr, &QParser::parseFunctionDecl, nullptr, "function", "function", P_PREFIX }},
{ T_NAME, { &QParser::parseName, nullptr, nullptr, &QParser::parseMethodDecl, nullptr, nullptr, P_PREFIX }},
INFIX(DOT, InfixOp, ., MEMBER),
INFIX(DOTQUEST, InfixOp, .?, MEMBER),
MULTIFIX(COLONCOLON, GenericMethodSymbol, InfixOp, ::, ::, MEMBER),
PREFIX(UND, Field, _),
PREFIX(UNDUND, StaticField, __),
PREFIX(SUPER, Super, super),
PREFIX(TRUE, Literal, true),
PREFIX(FALSE, Literal, false),
PREFIX(NULL, Literal, null),
PREFIX(NUM, Literal, Num),
PREFIX(STRING, Literal, String),
PREFIX(YIELD, Yield, yield),

STATEMENT(SEMICOLON, SimpleStatement),
STATEMENT(BREAK, Break),
STATEMENT(CLASS, ClassDecl),
STATEMENT(CONST, VarDecl),
STATEMENT(CONTINUE, Continue),
STATEMENT(EXPORT, ExportDecl),
{ T_FOR, { nullptr, &QParser::parseComprehension, &QParser::parseFor, nullptr, nullptr, "for", P_COMPREHENSION }},
STATEMENT(GLOBAL, GlobalDecl),
STATEMENT(IF, If),
{ T_IMPORT, { &QParser::parseImportExpression, nullptr, &QParser::parseImportDecl, nullptr, nullptr, nullptr, P_PREFIX }},
STATEMENT(REPEAT, RepeatWhile),
STATEMENT(RETURN, Return),
STATEMENT(SWITCH, Switch),
STATEMENT(THROW, Throw),
STATEMENT(TRY, Try),
{ T_VAR, { nullptr, nullptr, &QParser::parseVarDecl, &QParser::parseSimpleAccessor, nullptr, nullptr, P_PREFIX }},
STATEMENT(WITH, With),
STATEMENT(WHILE, While)
#undef PREFIX
#undef PREFIX_OP
#undef INFIX
#undef INFIX_OP
#undef OPERATOR
#undef STATEMENT
};

static std::unordered_map<string,QTokenType> KEYWORDS = {
#define TOKEN(name, keyword) { (#keyword), T_##name }
TOKEN(BARBAR, and),
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
TOKEN(NULL, null),
TOKEN(AMPAMP, or),
TOKEN(REPEAT, repeat),
TOKEN(RETURN, return),
TOKEN(STATIC, static),
TOKEN(SUPER, super),
TOKEN(SWITCH, switch),
TOKEN(THROW, throw),
TOKEN(TRUE, true),
TOKEN(TRY, try),
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
OP(EQEQ, ==), OP(EXCLEQ, !=),
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

static inline bool isSpace (uint32_t c) {
return c==' ' || c=='\t' || c=='\r';
}

static inline bool isLine (uint32_t c) {
return c=='\n';
}

bool isName (uint32_t c) {
return (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_' || c>=128;
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
int c = utf8::peek_next(in, end);
if (isSpace(c) || isName(c) || c==delim) {
while(c && in<end && !isLine(c)) c=utf8::next(in, end);
return;
}
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

static int parseCodePointValue (const char*& in, const char* end, int n, int base) {
char buf[n+1];
memcpy(buf, in, n);
buf[n]=0;
in += n;
return strtoul(buf, nullptr, base);
}

static int getStringEndingChar (int c) {
switch(c){
case 147: return 148;
case 171: return 187;
case 8220: return 8221;
default:  return c;
}}

static QV parseString (QVM& vm, const char*& in, const char* end, int ending) {
string re;
auto out = back_inserter(re);
int c;
while((c=utf8::next(in, end))!=ending && c) {
if (c=='\\') {
c = utf8::next(in, end);
switch(c){
case 'b': c='\b'; break;
case 'e': c='\x1B'; break;
case 'f': c='\f'; break;
case 'n': c='\n'; break;
case 'r': c='\r'; break;
case 't': c='\t'; break;
case 'u': c = parseCodePointValue(in, end, 4, 16); break;
case 'U': c = parseCodePointValue(in, end, 8, 16); break;
case 'x': c = parseCodePointValue(in, end, 2, 16); break;
case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': c = strtoul(--in, const_cast<char**>(&in), 0); break;
}}
utf8::append(c, out);
}
return QV(vm, re);
}

static shared_ptr<BinaryOperation> createBinaryOperation (shared_ptr<Expression> left, QTokenType op, shared_ptr<Expression> right) {
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
case T_IN: case T_IS:
return make_shared<BinaryOperation>(right, op, left);
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
#define RET0(X) { cur = { X, start, static_cast<size_t>(in-start), QV()}; return cur; }
#define RET RET0(T_NAME)
#define RET2(C) if (utf8::peek_next(in, end)==C) utf8::next(in, end); RET
#define RET3(C1,C2) if(utf8::peek_next(in, end)==C1 || utf8::peek_next(in, end)==C2) utf8::next(in, end); RET
#define RET4(C1,C2,C3) if (utf8::peek_next(in, end)==C1 || utf8::peek_next(in, end)==C2 || utf8::peek_next(in, end)==C3) utf8::next(in, end); RET
#define RET22(C1,C2) if (utf8::peek_next(in, end)==C1) utf8::next(in, end); RET2(C2)
const char *start = in;
if (in>=end || !*in) RET0(T_END)
int c;
do {
c = utf8::next(in, end);
} while((isSpace(c) || isLine(c)) && *(start=in) && in<end);
switch(c){
case '\0': RET0(T_END)
case '(':
if (utf8::peek_next(in, end)==')') {
utf8::next(in, end);
RET2('=')
}break;
case '[': 
if (utf8::peek_next(in, end)==']') {
utf8::next(in, end);
RET2('=')
}break;
case '/': 
switch (utf8::peek_next(in, end)) {
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
auto p = getPositionOf(in);
int line = p.first, column = p.second;
println(std::cerr, "Line %d, column %d: unexpected character '%c' (%<#0$2X)", line, column, c);
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
#define RET(X) { cur = { X, start, static_cast<size_t>(in-start), QV()}; return cur; }
#define RETV(X,V) { cur = { X, start, static_cast<size_t>(in-start), V}; return cur; }
#define RET2(C,A,B) if (utf8::peek_next(in, end)==C) { utf8::next(in, end); RET(A) } else RET(B)
#define RET3(C1,A,C2,B,C) if(utf8::peek_next(in, end)==C1) { utf8::next(in, end); RET(A) } else if (utf8::peek_next(in, end)==C2) { utf8::next(in, end); RET(B) } else RET(C)
#define RET4(C1,R1,C2,R2,C3,R3,C) if(utf8::peek_next(in, end)==C1) { utf8::next(in, end); RET(R1) } else if (utf8::peek_next(in, end)==C2) { utf8::next(in, end); RET(R2) } else if (utf8::peek_next(in, end)==C3) { utf8::next(in, end); RET(R3) }  else RET(C)
#define RET22(C1,C2,A,B,C,D) if (utf8::peek_next(in, end)==C1) { utf8::next(in, end); RET2(C2,A,B) } else RET2(C2,C,D)
const char *start = in;
if (in>=end || !*in) RET(T_END)
int c;
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
case '?': RET22('?', '=', T_QUESTQUESTEQ, T_QUESTQUEST, T_QUESTQUESTEQ, T_QUEST) 
case '/': 
switch (utf8::peek_next(in, end)) {
case '/': case '*': skipComment(in, end, '/'); return nextToken();
default: RET2('=', T_SLASHEQ, T_SLASH)
}
case '.':
if (utf8::peek_next(in, end)=='?') { utf8::next(in, end); RET(T_DOTQUEST) } 
else if (utf8::peek_next(in, end)=='.') { utf8::next(in, end); RET2('.', T_DOTDOTDOT, T_DOTDOT) }
else RET(T_DOT)
case '"': case '\'': case '`':
case 146: case 147: case 171: case 8216: case 8217: case 8220: 
{
QV str = parseString(vm, in, end, getStringEndingChar(c));
RETV(T_STRING, str)
}
case '#': skipComment(in, end, '#'); return nextToken();
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
default: RET(type)
}}
else if (isSpace(c)) RET(T_END)
auto p = getPositionOf(in);
int line = p.first, column = p.second;
println(std::cerr, "Line %d, column %d: unexpected character '%c' (%<#0$2X)", line, column, c);
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

shared_ptr<Statement> QParser::parseSwitch () {
auto sw = make_shared<SwitchStatement>();
shared_ptr<Expression> activeCase;
vector<shared_ptr<Statement>> statements;
auto clearStatements = [&]()mutable{
if (activeCase) sw->cases.push_back(make_pair(activeCase, statements));
else sw->defaultCase = statements;
statements.clear();
};
sw->expr = parseExpression();
if (match(T_WITH)) sw->comparator = parseExpression();
skipNewlines();
consume(T_LEFT_BRACE, "Expected '{' to begin switch");
while(true){
skipNewlines();
if (match(T_CASE)) {
clearStatements();
activeCase = parseExpression();
while(match(T_COMMA)) {
clearStatements();
activeCase = parseExpression();
}
matchOneOf(T_COLON, T_EQGT, T_MINUSGT);
}
else if (matchOneOf(T_DEFAULT, T_ELSE)) {
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
if (parser.match(T_CONST)) loopVarIsConst = true;
else parser.match(T_VAR);
if (parser.matchOneOf(T_LEFT_BRACE, T_LEFT_PAREN, T_LEFT_BRACKET)) destructuring = static_cast<QTokenType>(parser.cur.type +1);
do {
parser.consume(T_NAME, ("Expected loop variable name after 'for'"));
loopVariables.push_back(parser.cur);
} while (parser.match(T_COMMA));
if (destructuring!=T_END) parser.consume(destructuring, "Expecting close of destructuring variable declaration");
parser.consume(T_IN, ("Expected 'in' after loop variable name"));
inExpression = parser.parseExpression(P_COMPREHENSION);
}

shared_ptr<Statement> QParser::parseFor () {
shared_ptr<ForStatement> forSta = make_shared<ForStatement>();
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
skipNewlines();
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
QToken varToken = cur, catchVar = cur;
skipNewlines();
shared_ptr<Expression> openExpr = parseExpression(P_COMPREHENSION);
shared_ptr<Statement> catchSta = nullptr;
if (!openExpr) { result=CR_INCOMPLETE; return nullptr; }
if (match(T_AS)) {
consume(T_NAME, "Expected variable name after 'as'");
varToken = cur;
}
match(T_COLON);
shared_ptr<Statement> body = parseStatement();
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
vector<pair<QToken,shared_ptr<Expression>>> varDecls = { make_pair(varToken, openExpr) };
auto varDecl = make_shared<VariableDeclaration>(varDecls, 0);
auto varExpr = make_shared<NameExpression>(varToken);
auto closeExpr = createBinaryOperation(varExpr, T_DOT, make_shared<NameExpression>(closeToken));
auto trySta = make_shared<TryStatement>(body, catchSta, closeExpr, catchVar);
vector<shared_ptr<Statement>> statements = { varDecl, trySta };
return make_shared<BlockStatement>(statements);
}

shared_ptr<Expression> QParser::parseDecoratedExpression () {
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
if (expr->isDecorable()) {
auto decorable = dynamic_pointer_cast<Decorable>(expr);
decorable->decorations.insert(decorable->decorations.begin(), decoration);
}
else parseError("Expression can't be decorated");
return expr;
}

shared_ptr<Statement> QParser::parseVarDecl () {
vector<pair<QToken,shared_ptr<Expression>>> vars;
bool isConst = cur.type==T_CONST;
QTokenType destructuring = T_END;
if (matchOneOf(T_LEFT_BRACE, T_LEFT_PAREN, T_LEFT_BRACKET)) destructuring = static_cast<QTokenType>(cur.type +1);
do {
skipNewlines();
consume(T_NAME, ("Expected variable name after 'var'"));
QToken name = cur;
shared_ptr<Expression> value;
if (match(T_EQ)) value = parseExpression();
else value = make_shared<ConstantExpression>(name);
vars.push_back(make_pair(name, value));
} while(match(T_COMMA));
if (destructuring!=T_END) {
consume(destructuring, "Expecting close of destructuring variable declaration");
consume(T_EQ, "Expecting '=' after destructuring variable declaration");
shared_ptr<Expression> source = parseExpression();
for (int i=0, n=vars.size(); i<n; i++) {
QToken& name =  vars[i].first;
QToken index = { T_NAME, name.start, name.length, destructuring==T_RIGHT_BRACE? QV(QString::create(vm, name.start, name.length), QV_TAG_STRING) : QV(static_cast<double>(i)) };
vector<shared_ptr<Expression>> indices = { make_shared<ConstantExpression>(index) };
auto subscript = make_shared<SubscriptExpression>(source, indices);
vars[i].second = createBinaryOperation(subscript, T_QUESTQUEST, vars[i].second);
}}
return make_shared<VariableDeclaration>(vars, isConst? VD_CONST : 0);
}

static shared_ptr<Statement> makeMethodPrebody (QParser& parser, vector<shared_ptr<FunctionParameter>>& params) {
vector<shared_ptr<Statement>> stats;
int count = 0;
while(true){
auto mapDestr = find_consecutive(params.begin(), params.end(), [](auto p){ return p->flags&(FP_MAP_DESTRUCTURING | FP_ARRAY_DESTRUCTURING); });
if (mapDestr.first==params.end()) break;
int flags = FP_MAP_DESTRUCTURING | FP_ARRAY_DESTRUCTURING | FP_CONST;
vector<pair<QToken,shared_ptr<Expression>>> vars;
string mapName = format("$destr%d", ++count);
QString* mapNameStr = QString::create(parser.vm, mapName);
QToken mapNameToken = { T_NAME, mapNameStr->data, mapNameStr->length, QV(mapNameStr, QV_TAG_STRING) };
auto mapNameExpr = make_shared<NameExpression>(mapNameToken);
auto source = mapNameExpr;
for (auto it=mapDestr.first; it!=mapDestr.second; ++it) {
auto param = *it;
QToken& name =  param->arg;
QToken index = { T_NAME, name.start, name.length, param->flags&FP_MAP_DESTRUCTURING? QV(QString::create(parser.vm, name.start, name.length), QV_TAG_STRING) : QV(static_cast<double>(it-mapDestr.first)) };
vector<shared_ptr<Expression>> indices = { make_shared<ConstantExpression>(index) };
auto subscript = make_shared<SubscriptExpression>(source, indices);
if (param->defaultValue) param->defaultValue = createBinaryOperation(subscript, T_QUESTQUESTEQ, param->defaultValue);
else param->defaultValue =  subscript;
for (auto it = param->decorations.rbegin(), end=param->decorations.rend(); it!=end; ++it) param->defaultValue = make_shared<CallExpression>(*it, vector<shared_ptr<Expression>>({ param->defaultValue }) );
vars.push_back(make_pair(param->arg, param->defaultValue));
flags &= param->flags;
}
shared_ptr<Expression> paramDefaultValue = nullptr;
if (flags&FP_MAP_DESTRUCTURING) paramDefaultValue = make_shared<LiteralMapExpression>(mapNameToken);
else paramDefaultValue = make_shared<LiteralListExpression>(mapNameToken);
auto it = params.erase(mapDestr.first, mapDestr.second);
params.insert(it, make_shared<FunctionParameter>(mapNameToken, paramDefaultValue));
stats.push_back(make_shared<VariableDeclaration>(vars, (flags&FP_CONST? VD_CONST : 0) ));
}
if (stats.empty()) return nullptr;
else return make_shared<BlockStatement>(stats);
}

shared_ptr<Statement> concatStatements (shared_ptr<Statement> s1, shared_ptr<Statement> s2) {
if (!s1) return s2;
else if (!s2) return s1;
auto b1 = dynamic_pointer_cast<BlockStatement>(s1), b2 = dynamic_pointer_cast<BlockStatement>(s2);
if (b1&&b2) {
b1->statements.insert(b1->statements.end(), b2->statements.begin(), b2->statements.end());
return b1;
}
else if (b1) {
b1->statements.push_back(s2);
return b1;
}
else if (b2) {
b2->statements.insert(b2->statements.begin(), s1);
return b2;
}
else {
vector<shared_ptr<Statement>> v = { s1, s2 };
return make_shared<BlockStatement>(v);
}}

vector<shared_ptr<FunctionParameter>> QParser::parseFunctionParameters (bool implicitThis) {
vector<shared_ptr<FunctionParameter>> params;
vector<shared_ptr<Expression>> decorations;
int  destructuring = 0;
if (implicitThis) {
QToken thisToken = { T_NAME, THIS, 4, QV() };
params.push_back(make_shared<FunctionParameter>(thisToken, nullptr, nullptr, FP_CONST));
}
if (match(T_LEFT_PAREN) && !match(T_RIGHT_PAREN)) {
do {
int flags = 0;
skipNewlines();
if (!destructuring && match(T_LEFT_BRACE)) destructuring=FP_MAP_DESTRUCTURING;
else if (!destructuring && matchOneOf(T_LEFT_PAREN, T_LEFT_BRACKET)) destructuring =  FP_ARRAY_DESTRUCTURING; 
while(match(T_AT)) decorations.push_back(parseExpression(P_PREFIX));
if (match(T_CONST)) flags |= FP_CONST;
else if (match(T_VAR)) flags &= ~FP_CONST;
if (!destructuring) {
if (match(T_DOTDOTDOT)) flags |= FP_VARARG;
if (match(T_UND)) flags |= FP_FIELD_ASSIGN;
else if (match(T_UNDUND)) flags |= FP_STATIC_FIELD_ASSIGN;
}
consume(T_NAME, ("Expected parameter name"));
QToken arg = cur;
shared_ptr<Expression> defaultValue = nullptr, typeCheck = nullptr;
if (!destructuring && !(flags&FP_VARARG) && match(T_DOTDOTDOT)) flags |= FP_VARARG;
if (!(flags&FP_VARARG)) {
if (match(T_EQ)) defaultValue = parseExpression();
if (!destructuring && match(T_AS)) typeCheck = parseExpression();
}
params.push_back(make_shared<FunctionParameter>(arg, defaultValue, typeCheck, flags | destructuring));
params.back()->decorations = decorations;
decorations.clear();
if (flags&FP_VARARG) break;
if (destructuring && matchOneOf(T_RIGHT_BRACE, T_RIGHT_BRACKET, T_RIGHT_PAREN, T_GT)) destructuring=0;
} while(match(T_COMMA));
skipNewlines();
if (destructuring && !matchOneOf(T_RIGHT_BRACE, T_RIGHT_BRACKET, T_RIGHT_PAREN)) parseError("Expected '}', ']' or ')' to close parameter destructuration");
consume(T_RIGHT_PAREN, ("Expected ')' to close parameter list"));
}
else if (match(T_NAME)) params.push_back(make_shared<FunctionParameter>(cur));
return params;
}

void QParser::parseDecoratedDecl (ClassDeclaration& cls, bool isStatic) {
vector<shared_ptr<Expression>> decorations;
int idxFrom = cls.methods.size();
bool parsed = false;
prevToken();
while (match(T_AT)) {
const char* c = in;
while(isSpace(*c) || isLine(*c)) c++;
if (*c=='(') {
parseMethodDecl(cls, isStatic);
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
bool isStatic2 = nextToken().type==T_STATIC;
if (isStatic2) nextToken();
if (cur.type==T_FUNCTION) nextToken();
const ParserRule& rule = rules[cur.type];
if (rule.member) (this->*rule.member)(cls, isStatic||isStatic2);
else { prevToken(); parseError("Expected declaration to decorate"); }
}
for (auto it=cls.methods.begin() + idxFrom, end = cls.methods.end(); it<end; ++it) (*it)->decorations = decorations;
}

void QParser::parseMethodDecl (ClassDeclaration& cls, bool isStatic) {
prevToken();
QToken name = nextNameToken(true);
vector<shared_ptr<FunctionParameter>> params = parseFunctionParameters(true);
shared_ptr<Statement> prebody = makeMethodPrebody(*this, params);
int flags = FD_METHOD;
if (isStatic) flags |= FD_STATIC;
if (*name.start=='[' && params.size()<=0) {
parseError(("Subscript operator must take at least one argument"));
return;
}
if (*name.start=='[' && name.start[name.length -1]=='=' && params.size()<=1) {
parseError(("Subscript operator setter must take at least two arguments"));
return;
}
if (*name.start!='[' && name.start[name.length -1]=='=' && params.size()!=1) {
parseError(("Setter methods must take exactly one argument"));
}
if (params.size()>=1 && (params[params.size() -1]->flags&FP_VARARG)) flags |= FD_VARARG;
shared_ptr<Statement> body;
matchOneOf(T_COLON, T_EQGT, T_MINUSGT);
if (match(T_SEMICOLON)) body = make_shared<SimpleStatement>(cur);
else body = parseStatement();
body = concatStatements(prebody, body);
if (!body) body = make_shared<SimpleStatement>(cur);
shared_ptr<FunctionDeclaration> funcdecl = make_shared<FunctionDeclaration>(name, params, body, flags);
cls.methods.push_back(funcdecl);
}

void QParser::parseSimpleAccessor (ClassDeclaration& cls, bool isStatic) {
do {
consume(T_NAME, ("Expected field name after 'var'"));
QString* setterName = QString::create(vm, string(cur.start, cur.length) + ("="));
QToken setterNameToken = { T_NAME, setterName->data, setterName->length, QV(setterName, QV_TAG_STRING)  };
QToken thisToken = { T_NAME, THIS, 4, QV()};
shared_ptr<Expression> field;
int flags = FD_METHOD;
if (isStatic) flags |= FD_STATIC;
if (isStatic) field = make_shared<StaticFieldExpression>(cur);
else field = make_shared<FieldExpression>(cur);
shared_ptr<Expression> param = make_shared<NameExpression>(cur);
shared_ptr<FunctionParameter> thisParam = make_shared<FunctionParameter>(thisToken, nullptr, nullptr, FP_CONST);
vector<shared_ptr<FunctionParameter>> empty = { thisParam }, params = { thisParam, make_shared<FunctionParameter>(cur) };
shared_ptr<Expression> assignment = createBinaryOperation(field, T_EQ, param);
shared_ptr<FunctionDeclaration> getter = make_shared<FunctionDeclaration>(cur, empty, field, flags);
shared_ptr<FunctionDeclaration> setter = make_shared<FunctionDeclaration>(setterNameToken, params, assignment, flags);
cls.methods.push_back(getter);
cls.methods.push_back(setter);
} while (match(T_COMMA));
}

shared_ptr<Expression> QParser::parseLambda  () {
QToken name = cur;
int flags = 0;
if (matchOneOf(T_DOLLAR, T_UND)) flags |= FD_METHOD;
if (match(T_STAR)) flags |= FD_FIBER;
vector<shared_ptr<FunctionParameter>> params = parseFunctionParameters(flags&FD_METHOD);
shared_ptr<Statement> prebody = makeMethodPrebody(*this, params);
matchOneOf(T_COLON, T_EQGT, T_MINUSGT);
shared_ptr<Statement> body = parseStatement();
body = concatStatements(prebody, body);
if (!body) body = make_shared<SimpleStatement>(cur);
if (params.size()>=1 && (params[params.size() -1]->flags&FP_VARARG)) flags |= FD_VARARG;
return make_shared<FunctionDeclaration>(name, params, body, flags);
}

shared_ptr<Statement> QParser::parseFunctionDecl () {
return parseFunctionDecl(VD_CONST);
}

shared_ptr<Statement> QParser::parseFunctionDecl (int flags) {
consume(T_NAME, "Expected function name after 'function'");
QToken name = cur;
auto fnDecl = parseLambda();
vector<pair<QToken,shared_ptr<Expression>>> vd = {{ name, fnDecl }};
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VD_GLOBAL;
return make_shared<VariableDeclaration>(vd, flags);
}

shared_ptr<Statement> QParser::parseClassDecl () {
return parseClassDecl(VD_CONST);
}

shared_ptr<Statement> QParser::parseClassDecl (int flags) {
if (!consume(T_NAME, ("Expected class name after 'class'"))) return nullptr;
shared_ptr<ClassDeclaration> classDecl = make_shared<ClassDeclaration>(cur, flags);
skipNewlines();
if (matchOneOf(T_IS, T_COLON)) do {
skipNewlines();
consume(T_NAME, ("Expected class name after 'is'"));
classDecl->parents.push_back(cur);
} while (match(T_COMMA));
else classDecl->parents.push_back({ T_NAME, ("Object"), 6, QV() });
if (match(T_LEFT_BRACE)) {
while(true) {
skipNewlines();
bool isStatic = nextToken().type==T_STATIC;
if (isStatic) nextToken();
if (cur.type==T_FUNCTION) nextToken();
const ParserRule& rule = rules[cur.type];
if (rule.member) (this->*rule.member)(*classDecl, isStatic);
else { prevToken(); break; }
}
skipNewlines();
consume(T_RIGHT_BRACE, ("Expected '}' to close class body"));
}
vector<pair<QToken,shared_ptr<Expression>>> vd = {{ classDecl->name, classDecl }};
if (vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) flags |= VD_GLOBAL;
return make_shared<VariableDeclaration>(vd, flags);
}

shared_ptr<Statement> QParser::parseGlobalDecl () {
if (matchOneOf(T_VAR, T_CONST)) {
shared_ptr<VariableDeclaration> vd = static_pointer_cast<VariableDeclaration>(parseVarDecl());
vd->flags |= VD_GLOBAL;
return vd;
}
else if (match(T_CLASS)) {
return parseClassDecl(VD_CONST | VD_GLOBAL);
}
else if (matchOneOf(T_DOLLAR, T_FUNCTION)) {
return parseFunctionDecl(VD_CONST | VD_GLOBAL);
}
parseError(("Expected 'function', 'var' or 'class' after 'global'"));
return nullptr;
}

shared_ptr<Statement> QParser::parseExportDecl () {
if (matchOneOf(T_VAR, T_CONST)) {
shared_ptr<VariableDeclaration> vd = static_pointer_cast<VariableDeclaration>(parseVarDecl());
vd->flags |= VD_EXPORT;
return vd;
}
else if (match(T_CLASS)) {
return parseClassDecl(VD_CONST | VD_EXPORT);
}
else if (matchOneOf(T_DOLLAR, T_FUNCTION)) {
return parseFunctionDecl(VD_CONST | VD_EXPORT);
}
else {
auto exportDecl = make_shared<ExportDeclaration>();
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
return exportDecl;
}}

shared_ptr<Expression> QParser::parseImportExpression () {
bool parent = match(T_LEFT_PAREN);
auto result = make_shared<ImportExpression>(parseExpression());
if (parent) consume(T_RIGHT_PAREN, "Expected ')' to close function call");
return result;
}

shared_ptr<Statement> QParser::parseImportDecl () {
bool parent = match(T_LEFT_PAREN);
shared_ptr<Expression> from = parseExpression(P_COMPREHENSION);
if (parent) consume(T_RIGHT_PAREN, "Expected ')' to close function call");
if (!match(T_FOR)) return make_shared<ImportExpression>(from);
auto im = make_shared<ImportDeclaration>(from);
do {
consume(T_NAME, "Expected variable name after 'for'");
QToken t1 = cur, t2 = cur;
if (match(T_AS)) {
consume(T_NAME, "Expected variable name after 'as'");
t2 = cur;
}
im->imports.emplace_back(t1,t2);
} while(match(T_COMMA));
return im;
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
shared_ptr<Expression> right = parseExpression(rules[op].priority);
return createBinaryOperation(left, op, right);
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
auto compr = make_shared<ComprehensionExpression>(loopExpr);
do {
skipNewlines();
shared_ptr<ForStatement> forSta = make_shared<ForStatement>();
forSta->parseHead(*this);
compr->subCompr.push_back(forSta);
} while(match(T_FOR));
if (match(T_IF)) compr->filterExpression = parseExpression(P_COMPREHENSION);
return compr;
}

static shared_ptr<Expression> nameExprToConstant (QParser& parser, shared_ptr<Expression> key) {
shared_ptr<NameExpression> name = dynamic_pointer_cast<NameExpression>(key);
if (name) {
QToken token = name->token;
token.type = T_STRING;
token.value = QV(parser.vm, string(token.start, token.length));
key = make_shared<ConstantExpression>(token);
}
return key;
}

shared_ptr<Expression> QParser::parseMethodCall (shared_ptr<Expression> receiver) {
vector<shared_ptr<Expression>> args;
shared_ptr<LiteralMapExpression> mapArg = nullptr;
if (!match(T_RIGHT_PAREN)) {
do {
shared_ptr<Expression> arg = parseExpression();
if (match(T_COLON)) {
shared_ptr<Expression> val = parseExpression();
if (!mapArg) { mapArg = make_shared<LiteralMapExpression>(cur); args.push_back(mapArg); }
arg = nameExprToConstant(*this, arg);
mapArg->items.push_back(make_pair(arg, val));
}
else if (arg) args.push_back(arg);
} while(match(T_COMMA));
skipNewlines();
consume(T_RIGHT_PAREN, ("Expected ')' to close method call"));
}
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
return make_shared<SubscriptExpression>(receiver, args);
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

shared_ptr<Expression> QParser::parseUnpack () {
return make_shared<UnpackExpression>(parseExpression());
}

shared_ptr<Expression> QParser::parseName () {
return make_shared<NameExpression>(cur);
}

shared_ptr<Expression> QParser::parseField () {
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
list->items.push_back(parseExpression());
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
list->items.push_back(parseExpression(P_COMPARISON));
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
key = parseExpression();
skipNewlines();
consume(T_RIGHT_BRACKET, ("Expected ']' to close computed map key"));
}
else key = parseExpression();
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
shared_ptr<Expression> expr = parseExpression();
if (match(T_COMMA)) {
vector<shared_ptr<Expression>> items = { expr };
if (match(T_RIGHT_PAREN)) return make_shared<LiteralTupleExpression>(initial, items );
do {
items.push_back(parseExpression());
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

static string printFuncInfo (const QFunction& func) {
return format("%s (arity=%d, consts=%d, upvalues=%d, bc=%d, file=%s)", func.name, static_cast<int>(func.nArgs), func.constants.size(), func.upvalues.size(), func.bytecode.size(), func.file);
}

string QV::print () const {
if (isNull()) return ("null");
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
loopExpression=loopExpression->optimize(); 
for (auto& e: subCompr) e=static_pointer_cast<ForStatement>(e->optimizeStatement());
if (filterExpression) filterExpression=filterExpression->optimize(); 
return shared_this(); 
}

void ForStatement::compile (QCompiler& compiler) {
compiler.pushScope();
int iteratorSlot = compiler.findLocalVariable(compiler.createTempName(), LV_NEW | LV_CONST);
int iteratorSymbol = compiler.vm.findMethodSymbol(("iterator"));
int nextSymbol = compiler.vm.findMethodSymbol(("next"));
int hasNextSymbol = compiler.vm.findMethodSymbol(("hasNext"));
int subscriptSymbol = compiler.vm.findMethodSymbol(("[]"));
compiler.writeDebugLine(inExpression->nearestToken());
inExpression->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, iteratorSymbol);
compiler.pushLoop();
compiler.pushScope();
vector<int> valueSlots;
compiler.writeDebugLine(loopVariables[0]);
for (auto& loopVariable: loopVariables) valueSlots.push_back(compiler.findLocalVariable(loopVariable, LV_NEW | (loopVarIsConst? LV_CONST : 0)));
int loopStart = compiler.writePosition();
compiler.loops.back().condPos = compiler.writePosition();
writeOpLoadLocal(compiler, iteratorSlot);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, hasNextSymbol);
compiler.loops.back().jumpsToPatch.push_back({ Loop::END, compiler.writeOpJump(OP_JUMP_IF_FALSY) });
writeOpLoadLocal(compiler, iteratorSlot);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, nextSymbol);
if (destructuring==T_RIGHT_PAREN || destructuring==T_RIGHT_BRACKET || (destructuring==T_END && valueSlots.size()>1)) {
for (int i=1; i<valueSlots.size(); i++) {
writeOpLoadLocal(compiler, valueSlots[0]);
compiler.writeOpArg<uint8_t>(OP_LOAD_INT8, i);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, subscriptSymbol);
}
writeOpLoadLocal(compiler, valueSlots[0]);
compiler.writeOpArg<uint8_t>(OP_LOAD_INT8, 0);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, subscriptSymbol);
writeOpStoreLocal(compiler, valueSlots[0]);
compiler.writeOp(OP_POP);
}
else if (destructuring==T_RIGHT_BRACE) {
for (int i=1; i<valueSlots.size() && i<loopVariables.size(); i++) {
writeOpLoadLocal(compiler, valueSlots[0]);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QString::create(compiler.parser.vm, loopVariables[i].start, loopVariables[i].length)));
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, subscriptSymbol);
}
writeOpLoadLocal(compiler, valueSlots[0]);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QString::create(compiler.parser.vm, loopVariables[0].start, loopVariables[0].length)));
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, subscriptSymbol);
writeOpStoreLocal(compiler, valueSlots[0]);
compiler.writeOp(OP_POP);
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

void ComprehensionExpression::compile (QCompiler  & compiler) {
auto yieldSta = make_shared<YieldExpression>(loopExpression->nearestToken(), loopExpression);
auto ifSta = filterExpression? make_shared<IfStatement>(filterExpression, yieldSta, nullptr) : nullptr;
auto finalSta = ifSta? static_pointer_cast<Statement>(ifSta) : static_pointer_cast<Statement>(yieldSta);
shared_ptr<ForStatement> forSta = nullptr;
for (auto& sc: subCompr) {
if (forSta) forSta->loopStatement = sc;
forSta = sc;
}
forSta->loopStatement = finalSta;
QCompiler fc(compiler.parser);
fc.parent = &compiler;
subCompr[0]->optimizeStatement()->compile(fc);
QFunction* func = fc.getFunction(0);
func->name = "<comprehension>";
compiler.result = fc.result;
int funcSlot = compiler.findConstant(QV(func, QV_TAG_NORMAL_FUNCTION));
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.vm.findGlobalSymbol("Fiber", LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CLOSURE, funcSlot);
compiler.writeOp(OP_CALL_FUNCTION_1);
}

void NameExpression::compile (QCompiler& compiler) {
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_READ);
if (slot==0 && compiler.getCurClass()) {
compiler.writeOp(OP_LOAD_THIS);
return;
}
else if (slot>=0) { 
writeOpLoadLocal(compiler, slot);
return;
}
slot = compiler.findUpvalue(token, LV_FOR_READ);
if (slot>=0) { 
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, slot);
return;
}
slot = compiler.vm.findGlobalSymbol(string(token.start, token.length), LV_EXISTING | LV_FOR_READ);
if (slot>=0) { 
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, slot);
return;
}
ClassDeclaration* cls = compiler.getCurClass();
if (!cls) compiler.compileError(token, ("Undefined variable"));
else {
compiler.writeOp(OP_LOAD_THIS);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(string(token.start, token.length)));
}}

void NameExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
assignedValue->compile(compiler);
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_WRITE);
if (slot>=0) {
writeOpStoreLocal(compiler, slot);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
slot = compiler.findUpvalue(token, LV_FOR_WRITE);
if (slot>=0) {
compiler.writeOpArg<uint_upvalue_index_t>(OP_STORE_UPVALUE, slot);
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
compiler.compileError(token, ("Undefined variable"));
}

void FieldExpression::compile (QCompiler& compiler) {
ClassDeclaration* cls = compiler.getCurClass();
if (!cls) {
compiler.compileError(token, ("Can't use field oustide of a class"));
return;
}
int slot = cls->findField(string(token.start, token.length));
compiler.writeOpArg<uint_field_index_t>(OP_LOAD_FIELD, slot);
}

void FieldExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
ClassDeclaration* cls = compiler.getCurClass();
if (!cls) {
compiler.compileError(token, ("Can't use field oustide of a class"));
return;
}
int slot = cls->findField(string(token.start, token.length));
assignedValue->compile(compiler);
compiler.writeOpArg<uint_field_index_t>(OP_STORE_FIELD, slot);
}

void StaticFieldExpression::compile (QCompiler& compiler) {
ClassDeclaration* cls = compiler.getCurClass();
if (!cls) {
compiler.compileError(token, ("Can't use static field oustide of a class"));
return;
}
int slot = cls->findStaticField(string(token.start, token.length));
compiler.writeOpArg<uint_field_index_t>(OP_LOAD_STATIC_FIELD, slot);
}

void StaticFieldExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
ClassDeclaration* cls = compiler.getCurClass();
if (!cls) {
compiler.compileError(token, ("Can't use static field oustide of a class"));
return;
}
int slot = cls->findStaticField(string(token.start, token.length));
assignedValue->compile(compiler);
compiler.writeOpArg<uint_field_index_t>(OP_STORE_STATIC_FIELD, slot);
}

bool LiteralSequenceExpression::isAssignable () {
if (items.size()<1) return false;
for (auto& item: items) if (!dynamic_pointer_cast<Assignable>(item)) return false;
return true;
}

void LiteralSequenceExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
compiler.pushScope();
QToken tmpToken = compiler.createTempName();
auto tmpVar = make_shared<NameExpression>(tmpToken);
int slot = compiler.findLocalVariable(tmpToken, LV_NEW | LV_CONST);
assignedValue->compile(compiler);
for (int i=0, n=items.size(); i<n; i++) {
auto& item = items[i];
auto assignable = dynamic_pointer_cast<Assignable>(item);
if (!assignable || !assignable->isAssignable()) continue;
QToken index = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(i)) };
vector<shared_ptr<Expression>> indices = { make_shared<ConstantExpression>(index) };
auto subscript = make_shared<SubscriptExpression>(tmpVar, indices);
assignable->compileAssignment(compiler, subscript);
if (i+1<n) compiler.writeOp(OP_POP);
}
compiler.popScope();
}

bool LiteralMapExpression::isAssignable () {
if (items.size()<1) return false;
for (auto& item: items) if (!dynamic_pointer_cast<Assignable>(item.second)) return false;
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
auto assignable = dynamic_pointer_cast<Assignable>(item.second);
if (!assignable || !assignable->isAssignable()) continue;
if (!first) compiler.writeOp(OP_POP);
first=false;
vector<shared_ptr<Expression>> indices = { item.first };
auto subscript = make_shared<SubscriptExpression>(tmpVar, indices);
assignable->compileAssignment(compiler, subscript);
}
compiler.popScope();
}

shared_ptr<Expression> UnaryOperation::optimize () { 
expr=expr->optimize();
shared_ptr<ConstantExpression> cst = dynamic_pointer_cast<ConstantExpression>(expr);
if (cst) {
QV& value = cst->token.value;
if (value.isNum() && BASE_NUMBER_UNOPS[op]) {
QToken token = cst->token;
token.value = BASE_NUMBER_UNOPS[op](value.d);
return make_shared<ConstantExpression>(token);
}
//other operations on non-number
}
return shared_this(); 
}

string UnaryOperation::print () {
return ("(") + string(rules[op].prefixOpName) + expr->print() + (")");
}

void UnaryOperation::compile (QCompiler& compiler) {
expr->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(rules[op].prefixOpName));
}

shared_ptr<Expression> BinaryOperation::optimize () { 
left=left->optimize(); 
right=right->optimize();
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
return shared_this(); 
}

string BinaryOperation::print () {
return ("(") + left->print() +  rules[op].infixOpName + right->print() + (")");
}

void BinaryOperation::compile  (QCompiler& compiler) {
left->compile(compiler);
right->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, compiler.vm.findMethodSymbol(rules[op].infixOpName));
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
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, symbol);
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

string AbstractCallExpression::print () {
const char* ch = rules[type].infixOpName;
string s = receiver->print() + *ch;
for (int i=0, n=args.size(); i<n; i++) {
if (i>0) s += (", ");
s += args[i]->print();
}
s += *++ch;
return s;
}

void CallExpression::compile (QCompiler& compiler) {
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
QOpCode op = OP_CALL_FUNCTION_0;
if (compiler.lastOp==OP_LOAD_THIS) op = OP_CALL_METHOD_0;
compileArgs(compiler);
if (vararg) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else compiler.writeOp(static_cast<QOpCode>(op+args.size()));
}

void AssignmentOperation::compile (QCompiler& compiler) {
shared_ptr<Assignable> target = dynamic_pointer_cast<Assignable>(left);
if (target && target->isAssignable()) {
target->compileAssignment(compiler, right);
return;
}
compiler.compileError(left->nearestToken(), ("Invalid target for assignment"));
}

void VariableDeclaration::compile (QCompiler& compiler) {
bool isConst = flags&VD_CONST;
bool isGlobal = flags&VD_GLOBAL;
bool exporting = flags&VD_EXPORT;
if (compiler.parent || compiler.curScope>1) {
if (isGlobal) compiler.compileError(nearestToken(), "Cannot declare global in a local scope");
if (exporting) compiler.compileError(nearestToken(), ("Cannot declare export in a local scope"));
}
for (auto& p: vars) {
int slot = compiler.findLocalVariable(p.first, LV_NEW | (isConst? LV_CONST : 0));
compiler.writeDebugLine(p.first);
for (auto decoration: decorations) decoration->compile(compiler);
auto vd = p.second;
auto fd = dynamic_pointer_cast<FunctionDeclaration>(vd);
if (fd) {
auto func = fd->compileFunction(compiler);
func->name = string(p.first.start, p.first.length);
}
else vd->compile(compiler);
for (auto decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
if (isGlobal) {
int globalSlot = compiler.findGlobalVariable(p.first, LV_NEW | (isConst? LV_CONST : 0));
writeOpLoadLocal(compiler, slot);
compiler.writeOpArg<uint_global_symbol_t>(OP_STORE_GLOBAL, globalSlot);
compiler.writeOp(OP_POP);
}
else if (exporting) {
int subscriptSetterSymbol = compiler.parser.vm.findMethodSymbol(("[]="));
int exportsSlot = compiler.findExportsVariable(true);
writeOpLoadLocal(compiler, exportsSlot);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, p.first.start, p.first.length), QV_TAG_STRING)));
writeOpLoadLocal(compiler, slot);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_3, subscriptSetterSymbol);
compiler.writeOp(OP_POP);
}}}

void ClassDeclaration::compile (QCompiler& compiler) {
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
auto func = method->compileFunction(compiler);
func->name = string(name.start, name.length) + "::" + string(method->name.start, method->name.length);
compiler.writeDebugLine(method->name);
compiler.writeOpArg<uint_method_symbol_t>(OP_STORE_METHOD, methodSymbol);
compiler.writeOp(OP_POP);
}
for (auto decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
fieldInfo.nFields = fields.size();
fieldInfo.nStaticFields = staticFields.size();
compiler.patch<FieldInfo>(fieldInfoPos, fieldInfo);
compiler.curClass = oldClassDecl;
}

void ExportDeclaration::compile (QCompiler& compiler) {
int subscriptSetterSymbol = compiler.parser.vm.findMethodSymbol(("[]="));
bool multi = exports.size()>1;
int exportsSlot = compiler.findExportsVariable(true);
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

void FunctionParameter::compile (QCompiler& fc) {
int slot = fc.localVariables.size();
LocalVariable lv = { arg, fc.curScope, false, (flags&FP_CONST)  };
fc.localVariables.push_back(lv);
if (defaultValue) {
fc.writeDebugLine(defaultValue->nearestToken());
make_shared<AssignmentOperation>(make_shared<NameExpression>(arg), T_QUESTQUESTEQ, defaultValue) ->optimize() ->compile(fc);
fc.writeOp(OP_POP);
}
if (decorations.size()) {
shared_ptr<Expression> nameExpr = make_shared<NameExpression>(arg), argExpr = nameExpr;
for (auto it=decorations.rbegin(); it!=decorations.rend(); ++it) argExpr = make_shared<CallExpression>(*it, vector<shared_ptr<Expression>>({ argExpr }) );
make_shared<AssignmentOperation>(nameExpr, T_EQ, argExpr) ->optimize() ->compile(fc);
fc.writeOp(OP_POP);
}
if (typeCheck) {
//todo
}
if (flags&(FP_FIELD_ASSIGN|FP_STATIC_FIELD_ASSIGN)) {
shared_ptr<Expression> field;
if (flags&FP_FIELD_ASSIGN) field = make_shared<FieldExpression>(arg);
else if (flags&FP_STATIC_FIELD_ASSIGN) field = make_shared<StaticFieldExpression>(arg);
shared_ptr<Expression> value = make_shared<NameExpression>(arg);
shared_ptr<Expression> assignment = createBinaryOperation(field, T_EQ, value);
assignment->compile(fc);
fc.writeOp(OP_POP);
}
}

QFunction* FunctionDeclaration::compileFunction (QCompiler& compiler) {
QCompiler fc(compiler.parser);
fc.parent = &compiler;
compiler.parser.curMethodNameToken = name;
for (int i=0, n=params.size(); i<n; i++) {
params[i] = static_pointer_cast<FunctionParameter>(params[i]->optimize());
params[i]->compile(fc);
}
body=body->optimizeStatement();
fc.writeDebugLine(body->nearestToken());
body->compile(fc);
if (body->isExpression()) fc.writeOp(OP_POP);
QFunction* func = fc.getFunction(params.size());
compiler.result = fc.result;
func->vararg = (flags&FD_VARARG);
func->name = string(name.start, name.length);
int funcSlot = compiler.findConstant(QV(func, QV_TAG_NORMAL_FUNCTION));
bool fiber = flags&FD_FIBER;
if (fiber) compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.vm.findGlobalSymbol("Fiber", LV_EXISTING | LV_EXISTING));
for (auto decoration: decorations) decoration->compile(compiler);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CLOSURE, funcSlot);
for (auto decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
if (fiber) compiler.writeOp(OP_CALL_FUNCTION_1);
return func;
}

ClassDeclaration* QCompiler::getCurClass () {
if (curClass) return curClass;
else if (parent) return parent->getCurClass();
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

int QCompiler::findLocalVariable (const QToken& name, int flags) {
bool createNew = flags&LV_NEW, isConst = flags&LV_CONST;
auto rvar = find_if(localVariables.rbegin(), localVariables.rend(), [&](auto& x){
return x.name.length==name.length && strncmp(name.start, x.name.start, name.length)==0;
});
if (rvar==localVariables.rend() && !createNew && parser.vm.getOption(QVM::Option::VAR_DECL_MODE)!=QVM::Option::VAR_IMPLICIT) return -1;
else if (rvar==localVariables.rend() || (createNew && rvar->scope<curScope)) {
int n = localVariables.size();
localVariables.push_back({ name, curScope, false, isConst });
return n;
}
else if (!createNew)  {
if (rvar->isConst && isConst) return LV_ERR_CONST;
int n = rvar.base() -localVariables.begin() -1;
return n;
}
else compileError(name, ("Variable already defined"));
return -1;
}

int QCompiler::findUpvalue (const QToken& token, int flags) {
if (!parent) return -1;
int slot = parent->findLocalVariable(token, flags);
if (slot>=0) {
parent->localVariables[slot].hasUpvalues=true;
return addUpvalue(slot, false);
}
else if (slot==LV_ERR_CONST) return slot;
slot = parent->findUpvalue(token, flags);
if (slot>=0) return addUpvalue(slot, true);
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
QToken exportsToken = { T_NAME, EXPORTS, 7, QV()};
auto exs = make_shared<ReturnStatement>(exportsToken, make_shared<NameExpression>(exportsToken));
auto bs = dynamic_pointer_cast<BlockStatement>(sta);
if (bs) bs->statements.push_back(exs);
else bs = make_shared<BlockStatement>(vector<shared_ptr<Statement>>({ sta, exs }));
return bs;
}

int QCompiler::findExportsVariable (bool createIfNotExist) {
QToken exportsToken = { T_NAME, EXPORTS, 7, QV()};
int slot = findLocalVariable(exportsToken, LV_EXISTING | LV_FOR_READ);
if (slot<0 && createIfNotExist) {
int mapSlot = vm.findGlobalSymbol(("Map"), LV_EXISTING | LV_FOR_READ);
slot = findLocalVariable(exportsToken, LV_NEW);
writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, mapSlot);
writeOp(OP_CALL_FUNCTION_0);
writeOpStoreLocal(*this, slot);
}
return slot;
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
globalVariables.push_back(QV());
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
if (lastOp==OP_POP) {
seek(-1);
writeOp(OP_RETURN);
}
else if (lastOp!=OP_RETURN) {
writeOp(OP_LOAD_NULL);
writeOp(OP_RETURN);
}
}
}

QFunction* QCompiler::getFunction (int nArgs) {
compile();
QFunction* function = vm.construct<QFunction>(vm);
function->nArgs = nArgs;
function->constants.clear();
function->constants.insert(function->constants.end(), constants.begin(), constants.end());
function->bytecode = out.str();
function->upvalues.clear();
function->upvalues.insert(function->upvalues.end(), upvalues.begin(), upvalues.end());
function->file = parent&&!vm.compileDbgInfo? "" : parser.filename;
result = result? result : parser.result;
return function;
}
