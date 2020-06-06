#include "Expression.hpp"
#include "Statement.hpp"
#include "TypeInfo.hpp"
#include "ParserRules.hpp"
#include "Compiler.hpp"
#include "../vm/VM.hpp"
#include "../vm/Function.hpp"
using namespace std;

unordered_map<int,int> BASE_OPTIMIZED_OPS = {
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



static inline bool isUnpack (shared_ptr<Expression> expr) {
return expr->isUnpack();
}

void ConstantExpression::compile (QCompiler& compiler) {
QV& value = token.value;
if (value.isUndefined()) compiler.writeOp(OP_LOAD_UNDEFINED);
else if (value.isNull()) compiler.writeOp(OP_LOAD_NULL);
else if (value.isFalse()) compiler.writeOp(OP_LOAD_FALSE);
else if (value.isTrue()) compiler.writeOp(OP_LOAD_TRUE);
else if (value.isInt8()) compiler.writeOpArg<int8_t>(OP_LOAD_INT8, static_cast<int>(value.d));
else compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(token.value));
}

void DupExpression::compile (QCompiler& compiler) {
compiler.writeOp(OP_DUP);
}

bool LiteralSequenceExpression::isVararg () { 
return any_of(items.begin(), items.end(), ::isUnpack); 
}

void LiteralListExpression::compile (QCompiler& compiler) {
compiler.writeDebugLine(nearestToken());
int listSymbol = compiler.vm.findGlobalSymbol(("List"), LV_EXISTING | LV_FOR_READ);
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, listSymbol);
items[0]->compile(compiler);
compiler.writeOpCallFunction(1);
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
else compiler.writeOpCallMethod(items.size(), ofSymbol);
}}


void LiteralSetExpression::compile (QCompiler& compiler) {
int setSymbol = compiler.vm.findGlobalSymbol(("Set"), LV_EXISTING | LV_FOR_READ);
compiler.writeDebugLine(nearestToken());
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, setSymbol);
items[0]->compile(compiler);
compiler.writeOpCallFunction(1);
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
else compiler.writeOpCallMethod(items.size(), ofSymbol);
}}

void LiteralMapExpression::compile (QCompiler& compiler) {
vector<std::shared_ptr<Expression>> unpacks;
int mapSymbol = compiler.vm.findGlobalSymbol(("Map"), LV_EXISTING | LV_FOR_READ);
int subscriptSetterSymbol = compiler.vm.findMethodSymbol(("[]="));
int callSymbol = compiler.vm.findMethodSymbol(("()"));
compiler.writeDebugLine(nearestToken());
if (items.size()==1 && items[0].first->isComprehension()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, mapSymbol);
items[0].first->compile(compiler);
compiler.writeOpCallFunction(1);
return;
}
for (auto it = items.begin(); it!=items.end(); ) {
auto expr = it->first;
if (expr->isUnpack()) {
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

void LiteralTupleExpression::compile (QCompiler& compiler) {
bool isVector = kind.type==T_SEMICOLON;
int tupleSymbol = compiler.vm.findGlobalSymbol((isVector?"Vector":"Tuple"), LV_EXISTING | LV_FOR_READ);
compiler.writeDebugLine(nearestToken());
if (isVector) {
for (auto item: items) if (item->isUnpack()) compiler.compileError(item->nearestToken(), "Spread expression not permitted here");
if (items.size()<1 || items.size()>4) compiler.compileError(kind, "A vector can't contain more than 4 elements");
}
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, tupleSymbol);
items[0]->compile(compiler);
compiler.writeOpCallFunction(1);
} else {
bool vararg = any_of(items.begin(), items.end(), ::isUnpack);
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, tupleSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else compiler.writeOpCallMethod(items.size(), ofSymbol);
}}

void LiteralGridExpression::compile (QCompiler& compiler) {
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
if (value->isUnpack()) compiler.compileError(value->nearestToken(), "Spread expression not permitted here");
value->compile(compiler);
}}
if (size>argLimit) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else compiler.writeOpCallFunction(size+2);
}
void LiteralRegexExpression::compile (QCompiler& compiler) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "Regex", 5, QV::UNDEFINED }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, pattern), QV_TAG_STRING)));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, options), QV_TAG_STRING)));
compiler.writeOp(OP_CALL_FUNCTION_2);
}


void AnonymousLocalExpression::compile (QCompiler& compiler) { 
compiler.writeOpLoadLocal(token.value.d); 
}

void AnonymousLocalExpression::compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue) { 
assignedValue->compile(compiler); 
compiler.writeOpStoreLocal(token.value.d); 
}

void GenericMethodSymbolExpression::compile (QCompiler& compiler) {
int symbol = compiler.parser.vm.findMethodSymbol(std::string(token.start, token.length));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(symbol | QV_TAG_GENERIC_SYMBOL_FUNCTION)));
}

void UnpackExpression::compile (QCompiler& compiler) {
expr->compile(compiler);
compiler.writeOp(OP_UNPACK_SEQUENCE);
}

void TypeHintExpression::compile (QCompiler& compiler)  { 
//todo: actually exploit the type hint
//int pos = compiler.out.tellp();
//compiler.out.seekp(pos);
expr->compile(compiler); 
} 

bool AbstractCallExpression::isVararg () { 
return  receiver->isUnpack()  || any_of(args.begin(), args.end(), ::isUnpack); 
}

void AbstractCallExpression::compileArgs (QCompiler& compiler) {
for (auto arg: args) arg->compile(compiler);
}

void YieldExpression::compile (QCompiler& compiler) {
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
compiler.writeOp(OP_YIELD);
auto method = compiler.getCurMethod();
//if (method && expr) method->returnTypeHint = compiler.mergeTypes(method->returnTypeHint, expr->getType(compiler));
}

void ImportExpression::compile (QCompiler& compiler) {
doCompileTimeImport(compiler.parser.vm, compiler.parser.filename, from);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "import", 6, QV::UNDEFINED }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(compiler.parser.vm, compiler.parser.filename)));
from->compile(compiler);
compiler.writeOp(OP_CALL_FUNCTION_2);
}

void ComprehensionExpression::compile (QCompiler  & compiler) {
QCompiler fc(compiler.parser);
fc.parent = &compiler;
rootStatement->optimizeStatement()->compile(fc);
QFunction* func = fc.getFunction(0);
func->name = "<comprehension>";
//###set argtypes
compiler.result = fc.result;
int funcSlot = compiler.findConstant(QV(func, QV_TAG_NORMAL_FUNCTION));
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.vm.findGlobalSymbol("Fiber", LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CLOSURE, funcSlot);
compiler.writeOp(OP_CALL_FUNCTION_1);
}


void NameExpression::compile (QCompiler& compiler) {
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
LocalVariable* lv = nullptr;
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_READ, &lv);
if (slot==0 && compiler.getCurClass()) {
compiler.writeOp(OP_LOAD_THIS);
return;
}
else if (slot>=0) { 
compiler.writeOpLoadLocal(slot);
return;
}
slot = compiler.findUpvalue(token, LV_FOR_READ, &lv);
if (slot>=0) { 
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, slot);
return;
}
slot = compiler.findGlobalVariable(token, LV_EXISTING | LV_FOR_READ, &lv);
if (slot>=0) { 
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, slot);
return;
}
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (cls) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
if (atLevel<=2) compiler.writeOp(OP_LOAD_THIS);
else compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, compiler.findUpvalue(thisToken, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(string(token.start, token.length)));
return;
}
compiler.compileError(token, ("Undefined variable"));
}

void NameExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
LocalVariable* lv = nullptr;
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
assignedValue->compile(compiler);
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_WRITE, &lv);
if (slot>=0) {
compiler.writeOpStoreLocal(slot);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
slot = compiler.findUpvalue(token, LV_FOR_WRITE, &lv);
if (slot>=0) {
compiler.writeOpArg<uint_upvalue_index_t>(OP_STORE_UPVALUE, slot);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
slot = compiler.findGlobalVariable(token, LV_EXISTING | LV_FOR_WRITE, &lv);
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
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
if (atLevel<=2) compiler.writeOp(OP_LOAD_THIS);
else compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, compiler.findUpvalue(thisToken, LV_EXISTING | LV_FOR_READ));
char setterName[token.length+2];
memcpy(&setterName[0], token.start, token.length);
setterName[token.length+1] = 0;
setterName[token.length] = '=';
QToken setterNameToken = { T_NAME, setterName, token.length+1, QV::UNDEFINED };
assignedValue->compile(compiler);
//todo: update type of field var
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, compiler.vm.findMethodSymbol(setterName));
return;
}
compiler.compileError(token, ("Undefined variable"));
}

void FieldExpression::compile (QCompiler& compiler) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (!cls) {
compiler.compileError(token, ("Can't use field outside of a class"));
return;
}
if (compiler.getCurMethod()->flags & FuncDeclFlag::Static) {
compiler.compileError(token, ("Can't use field in a static method"));
return;
}
int fieldSlot = cls->findField(token);
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
if (compiler.getCurMethod()->flags & FuncDeclFlag::Static) {
compiler.compileError(token, ("Can't use field in a static method"));
return;
}
int fieldSlot = cls->findField(token, &fieldType);
assignedValue->compile(compiler);
//if (fieldType) *fieldType = compiler.mergeTypes(*fieldType, assignedValue->getType(compiler));
if (atLevel<=2) compiler.writeOpArg<uint_field_index_t>(OP_STORE_THIS_FIELD, fieldSlot);
else  {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
compiler.writeOpArg<uint_field_index_t>(OP_STORE_FIELD, fieldSlot);
}
}

void StaticFieldExpression::compile (QCompiler& compiler) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (!cls) {
compiler.compileError(token, ("Can't use static field oustide of a class"));
return;
}
bool isStatic = static_cast<bool>(compiler.getCurMethod()->flags & FuncDeclFlag::Static);
int fieldSlot = cls->findStaticField(token);
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
bool isStatic = static_cast<bool>( compiler.getCurMethod()->flags & FuncDeclFlag::Static);
int fieldSlot = cls->findStaticField(token, &fieldType);
assignedValue->compile(compiler);
//if (fieldType) *fieldType = compiler.mergeTypes(*fieldType, assignedValue->getType(compiler));
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
auto th = dynamic_pointer_cast<TypeHintExpression>(item);
if (bop && bop->op==T_EQ) {
expr = bop->left;
th = dynamic_pointer_cast<TypeHintExpression>(expr);
}
if (th) expr = th->expr;
if (!dynamic_pointer_cast<Assignable>(expr) && !dynamic_pointer_cast<UnpackExpression>(expr)) {
if (auto cst = dynamic_pointer_cast<ConstantExpression>(expr)) return cst->token.value.i == QV::UNDEFINED.i;
else return false;
}
}
return true;
}

void LiteralSequenceExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
compiler.pushScope();
QToken tmpToken = compiler.createTempName(*this);
auto tmpVar = make_shared<NameExpression>(tmpToken);
int slot = compiler.findLocalVariable(tmpToken, LV_NEW | LV_CONST);
assignedValue->compile(compiler);
for (int i=0, n=items.size(); i<n; i++) {
shared_ptr<Expression> item = items[i], defaultValue = nullptr;
shared_ptr<TypeInfo> typeHint = nullptr;
bool unpack = false;
if (auto bop = dynamic_pointer_cast<BinaryOperation>(item)) {
if (bop->op==T_EQ) {
item = bop->left;
defaultValue = bop->right;
if (auto th = dynamic_pointer_cast<TypeHintExpression>(defaultValue)) typeHint = th->type;
}}
else if (auto th = dynamic_pointer_cast<TypeHintExpression>(item)) {
item = th->expr;
typeHint = th->type;
}
auto assignable = dynamic_pointer_cast<Assignable>(item);
if (!assignable) {
if (auto unpackExpr = dynamic_pointer_cast<UnpackExpression>(item)) {
assignable = dynamic_pointer_cast<Assignable>(unpackExpr->expr);
unpack = true;
if (i+1!=items.size()) compiler.compileError(unpackExpr->nearestToken(), "Unpack expression must appear last in assignment expression");
}}
if (!assignable || !assignable->isAssignable()) {
compiler.compileWarn(item->nearestToken(), "Ignoring unassignable target");
continue;
}
QToken indexToken = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(i)) };
shared_ptr<Expression> index = make_shared<ConstantExpression>(indexToken);
if (unpack) {
QToken minusOneToken = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(-1)) };
shared_ptr<Expression> minusOne = make_shared<ConstantExpression>(minusOneToken);
index = BinaryOperation::create(index, T_DOTDOTDOT, minusOne);
}
vector<shared_ptr<Expression>> indices = { index };
auto subscript = make_shared<SubscriptExpression>(tmpVar, indices);
if (defaultValue) defaultValue = BinaryOperation::create(subscript, T_QUESTQUEST, defaultValue)->optimize();
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
auto th = dynamic_pointer_cast<TypeHintExpression>(item.second);
if (bop && bop->op==T_EQ) {
expr = bop->left;
th = dynamic_pointer_cast<TypeHintExpression>(expr);
}
if (th) expr = th->expr;
if (!dynamic_pointer_cast<Assignable>(expr) && !dynamic_pointer_cast<GenericMethodSymbolExpression>(expr) && !dynamic_pointer_cast<UnpackExpression>(expr)) return false;
}
return true;
}

void LiteralMapExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
compiler.pushScope();
QToken tmpToken = compiler.createTempName(*this);
int tmpSlot = compiler.findLocalVariable(tmpToken, LV_NEW | LV_CONST);
int count = -1;
auto tmpVar = make_shared<NameExpression>(tmpToken);
assignedValue->compile(compiler);
vector<shared_ptr<Expression>> allKeys;
for (auto& item: items) {
count++;
shared_ptr<Expression> assigned = item.second, defaultValue = nullptr;
shared_ptr<TypeInfo> typeHint = nullptr;
if (auto bop = dynamic_pointer_cast<BinaryOperation>(assigned)) {
if (bop->op==T_EQ) {
assigned = bop->left;
defaultValue = bop->right;
if (auto th = dynamic_pointer_cast<TypeHintExpression>(defaultValue)) typeHint = th->type;
}}
else if (auto th = dynamic_pointer_cast<TypeHintExpression>(assigned)) {
assigned = th->expr;
typeHint = th->type;
}
auto assignable = dynamic_pointer_cast<Assignable>(assigned);
if (!assignable) {
if (auto mh = dynamic_pointer_cast<GenericMethodSymbolExpression>(assigned)) assignable = make_shared<NameExpression>(mh->token);
else if (auto unp = dynamic_pointer_cast<UnpackExpression>(assigned))  assignable = dynamic_pointer_cast<Assignable>(unp->expr);
}
if (!assignable || !assignable->isAssignable()) {
compiler.compileWarn(item.second->nearestToken(), "Ignoring unassignable target: %s", typeid(*assigned).name() );
continue;
}
if (count>0) compiler.writeOp(OP_POP);
shared_ptr<Expression> value = nullptr;
if (auto method = dynamic_pointer_cast<GenericMethodSymbolExpression>(item.first))  value = BinaryOperation::create(tmpVar, T_DOT, make_shared<NameExpression>(method->token));
else if (auto unp = dynamic_pointer_cast<UnpackExpression>(item.first)) {
if (count+1!=items.size()) compiler.compileError(unp->nearestToken(), "Unpack expression must appear last in assignment expression");
auto excludeKeys = make_shared<LiteralTupleExpression>(unp->nearestToken(), allKeys);
value = BinaryOperation::create(tmpVar, T_MINUS, excludeKeys);
}
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
allKeys.push_back(subscript);
vector<shared_ptr<Expression>> indices = { subscript };
value = make_shared<SubscriptExpression>(tmpVar, indices);
}
if (defaultValue) value = BinaryOperation::create(value, T_QUESTQUEST, defaultValue)->optimize();
if (typeHint) value = make_shared<TypeHintExpression>(value, typeHint)->optimize();
assignable->compileAssignment(compiler, value);
}
compiler.popScope();
}

void UnaryOperation::compile (QCompiler& compiler) {
expr->compile(compiler);
if (op==T_MINUS && type->isNum()) compiler.writeOp(OP_NEG);
else if (op==T_TILDE && type->isNum()) compiler.writeOp(OP_BINNOT);
else if (op==T_EXCL) compiler.writeOp(OP_NOT);
else compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(rules[op].prefixOpName));
}

void BinaryOperation::compile  (QCompiler& compiler) {
if ((rules[op].flags&P_SWAP_OPERANDS) && !!dynamic_pointer_cast<DupExpression>(right)) {
right->compile(compiler);
left->compile(compiler);
compiler.writeOpArg<uint8_t>(OP_SWAP, 0xFE);
}
else {
left->compile(compiler);
right->compile(compiler);
}
if (left->type->isNum() && right->type->isNum() && BASE_OPTIMIZED_OPS[op]) {
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
if (c.second) c.second->compile(compiler);
endJumps.push_back(compiler.writeOpJump(OP_JUMP));
compiler.patchJump(ifJump);
}
compiler.writeOp(OP_POP);
if (defaultCase) defaultCase->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
for (auto pos: endJumps) compiler.patchJump(pos);
}

void SubscriptExpression::compile  (QCompiler& compiler) {
int subscriptSymbol = compiler.vm.findMethodSymbol("[]");
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
for (auto arg: args) arg->compile(compiler);
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, subscriptSymbol);
else compiler.writeOpCallMethod(args.size(), subscriptSymbol);
}

void SubscriptExpression::compileAssignment  (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
int subscriptSetterSymbol = compiler.vm.findMethodSymbol("[]=");
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
for (auto arg: args) arg->compile(compiler);
assignedValue->compile(compiler);
vector<shared_ptr<Expression>> tmpargs = args; tmpargs.push_back(assignedValue);
//todo: update generic subtype if possible
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, subscriptSetterSymbol);
else compiler.writeOpCallMethod(args.size() +1, subscriptSetterSymbol);
}

inline bool isInlinableAccessor (shared_ptr<TypeInfo> type, QFunction* func) {
return type && func && (func->flags & FunctionFlag::Accessor)
&& (type->isExact() || !(func->flags & FunctionFlag::Overridden) );
}

void MemberLookupOperation::compile (QCompiler& compiler) {
auto func = fd.getFunc();
auto super = dynamic_pointer_cast<SuperExpression>(left);
auto getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
if (getter->token.type==T_END) getter->token = compiler.parser.curMethodNameToken;
int symbol = compiler.vm.findMethodSymbol(string(getter->token.start, getter->token.length));
left->compile(compiler);
if (isInlinableAccessor(type, func)) compiler.writeOpArg<uint_field_index_t>(OP_LOAD_FIELD, func->fieldIndex);
else compiler.writeOpArg<uint_method_symbol_t>(super? OP_CALL_SUPER_1 : OP_CALL_METHOD_1, symbol);
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
else if (super) compiler.writeOpCallSuper(call->args.size(), symbol);
else if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, symbol);
else compiler.writeOpCallMethod(call->args.size(), symbol);
return;
}}
compiler.compileError(right->nearestToken(), ("Bad operand for '.' operator"));
}

void MemberLookupOperation::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
auto super = dynamic_pointer_cast<SuperExpression>(left);
auto setter = dynamic_pointer_cast<NameExpression>(right);
auto func = fd.getFunc();
if (setter) {
string sName = string(setter->token.start, setter->token.length) + ("=");
int symbol = compiler.vm.findMethodSymbol(sName);
QToken sToken = { T_NAME, sName.data(), sName.size(), QV::UNDEFINED };
left->compile(compiler);
assignedValue->compile(compiler);
if (isInlinableAccessor(type, func)) {
compiler.writeOpArg<uint8_t>(OP_SWAP, 0xFE);
compiler.writeOpArg<uint_field_index_t>(OP_STORE_FIELD, func->fieldIndex);
}
else   compiler.writeOpArg<uint_method_symbol_t>(super? OP_CALL_SUPER_2 : OP_CALL_METHOD_2, symbol);
return;
}
compiler.compileError(right->nearestToken(), "Bad operand for '.' operator in assignment");
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
LocalVariable* lv = nullptr;
auto func = fd.getFunc();
int globalIndex = -1;
if (auto name=dynamic_pointer_cast<NameExpression>(receiver)) {
if (compiler.findLocalVariable(name->token, LV_EXISTING | LV_FOR_READ, &lv)<0 && compiler.findUpvalue(name->token, LV_FOR_READ, &lv)<0 && (globalIndex=compiler.findGlobalVariable(name->token, LV_FOR_READ, &lv))<0 && compiler.getCurClass()) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto thisExpr = make_shared<NameExpression>(thisToken);
auto expr = BinaryOperation::create(thisExpr, T_DOT, shared_this())->optimize();
expr->compile(compiler);
return;
}}
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
compileArgs(compiler);
if (vararg) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else compiler.writeOpCallFunction(args.size());
}

void AssignmentOperation::compile (QCompiler& compiler) {
shared_ptr<Assignable> target = dynamic_pointer_cast<Assignable>(left);
if (target && target->isAssignable()) {
target->compileAssignment(compiler, right);
return;
}
compiler.compileError(left->nearestToken(), ("Invalid target for assignment"));
}




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
compiler.curMethod = method.get();
auto func = method->compileFunction(compiler);
compiler.curMethod = nullptr;
func->name = string(name.start, name.length) + "::" + string(method->name.start, method->name.length);
compiler.writeDebugLine(method->name);
if (method->flags &FuncDeclFlag::Static) compiler.writeOpArg<uint_method_symbol_t>(OP_STORE_STATIC_METHOD, methodSymbol);
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

void FunctionDeclaration::compileParams (QCompiler& compiler) {
vector<shared_ptr<Variable>> destructuring;
compiler.writeDebugLine(nearestToken());
for (auto& var: params) {
shared_ptr<NameExpression> name = nullptr; 
LocalVariable* lv = nullptr;
int slot;
if (name = dynamic_pointer_cast<NameExpression>(var->name)) {
slot = compiler.findLocalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0), &lv);
if (var->value) {
auto value = BinaryOperation::create(name, T_QUESTQUESTEQ, var->value)->optimize();
value->compile(compiler);
compiler.writeOp(OP_POP);
}}
else {
name = make_shared<NameExpression>(compiler.createTempName(*var->name));
slot = compiler.findLocalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0), &lv);
if (!(var->flags&VD_OPTIMFLAG)) var->value = var->value? BinaryOperation::create(name, T_QUESTQUESTEQ, var->value)->optimize() : name;
destructuring.push_back(var);
var->flags |= VD_OPTIMFLAG;
}
if (var->decorations.size()) {
for (auto& decoration: var->decorations) decoration->compile(compiler);
compiler.writeOpLoadLocal(slot);
for (auto& decoration: var->decorations) compiler.writeOpCallFunction(1);
compiler.writeOpStoreLocal(slot);
var->decorations.clear();
}
if (var->type) {
auto typeHint = make_shared<TypeHintExpression>(name, var->type)->optimize();
//if (lv) lv->type = compiler.mergeTypes(typeHint->getType(compiler), lv->type);
//todo: use the type hint
//typeHint->compile(compiler);
//compiler.writeOp(OP_POP);
}
else if (var->value) {
//var->typeHint = var->value->getType(compiler);
//if (lv) lv->type = compiler.mergeTypes(var->typeHint, lv->type);
}
}
if (destructuring.size()) {
make_shared<VariableDeclaration>(destructuring)->optimizeStatement()->compile(compiler);
}
}

QFunction* FunctionDeclaration::compileFunction (QCompiler& compiler) {
QCompiler fc(compiler.parser, &compiler);
fc.curMethod = this;
compiler.parser.curMethodNameToken = name;
compileParams(fc);
body=body->optimizeStatement();
fc.writeDebugLine(body->nearestToken());
body->compile(fc);
fc.writeDebugLine(body->nearestToken());
if (body->isExpression()) fc.writeOp(OP_POP);
QFunction* func = fc.getFunction(params.size());
compiler.result = fc.result;
func->flags
.set(FunctionFlag::Vararg, flags &FuncDeclFlag::Vararg)
.set(FunctionFlag::Pure, flags & flags & FuncDeclFlag::Pure)
.set(FunctionFlag::Final, flags & FuncDeclFlag::Final);
int funcSlot = compiler.findConstant(QV(func, QV_TAG_NORMAL_FUNCTION));
if (flags & FuncDeclFlag::Accessor) {
func->flags |= FunctionFlag::Accessor;
func->fieldIndex = fieldIndex;
}
else if (func->flags & FunctionFlag::Accessor) {
flags |= FuncDeclFlag::Accessor;
fieldIndex = func->fieldIndex;
}
if (name.type==T_NAME) func->name = string(name.start, name.length);
else func->name = "<closure>";
string sType = type->toBinString(compiler.vm);
func->typeInfo.assign(sType.begin() +3, sType.end() -1);
if (flags & FuncDeclFlag::Fiber) {
QToken fiberToken = { T_NAME, FIBER, 5, QV::UNDEFINED };
decorations.insert(decorations.begin(), make_shared<NameExpression>(fiberToken));
}
else if (flags &FuncDeclFlag::Async) {
QToken asyncToken = { T_NAME, ASYNC, 5, QV::UNDEFINED };
decorations.insert(decorations.begin(), make_shared<NameExpression>(asyncToken));
}
for (auto decoration: decorations) decoration->compile(compiler);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CLOSURE, funcSlot);
for (auto decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
this->func = func;
return func;
}

QV doCompileTimeImport (QVM& vm, const string& baseFile, shared_ptr<Expression> exprRequestedFile) {
QV re = QV::UNDEFINED;
auto expr = dynamic_pointer_cast<ConstantExpression>(exprRequestedFile);
if (expr && expr->token.value.isString()) {
QFiber& f = vm.getActiveFiber();
f.import(baseFile, expr->token.value.asString());
re = f.top();
f.pop();
}
return re;
}



void DebugExpression::compile (QCompiler& compiler) {
expr->compile(compiler);
compiler.compileInfo(nearestToken(), "Type = %s", expr&&expr->type? expr->type->toString() : "<null>");
compiler.writeOp(OP_DEBUG);
}

