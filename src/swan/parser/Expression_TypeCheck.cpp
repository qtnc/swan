#include "Expression.hpp"
#include "TypeInfo.hpp"
#include "TypeChecker.hpp"
#include "ParserRules.hpp"
#include "../vm/VM.hpp"
using namespace std;

shared_ptr<TypeInfo> Expression::getType (TypeChecker& checker) {
return TypeInfo::ANY;
} 

shared_ptr<TypeInfo> ConstantExpression::getType (TypeChecker& checker) { 
 return make_shared<ClassTypeInfo>(&token.value.getClass(checker.vm));  
}

shared_ptr<TypeInfo> LiteralSequenceExpression::getType (TypeChecker& checker) {
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(checker.vm));
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = TypeInfo::ANY;
for (auto& item: items) type = type->merge(item->getType(checker), checker);
subtypes[0] = type;
return make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
}

QClass* LiteralTupleExpression::getSequenceClass (QVM& vm) { 
return vm.tupleClass; 
}

QClass* LiteralListExpression::getSequenceClass (QVM& vm) { 
return vm.listClass; 
}

QClass* LiteralSetExpression::getSequenceClass (QVM& vm) { 
return vm.setClass; 
}

shared_ptr<TypeInfo> LiteralMapExpression::getType (TypeChecker& checker) {
auto seqtype = make_shared<ClassTypeInfo>(checker.vm.mapClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(2);
shared_ptr<TypeInfo> keyType = TypeInfo::ANY, valueType = TypeInfo::ANY;
for (auto& item: items) {
keyType = keyType->merge(item.first->getType(checker), checker);
valueType = valueType->merge(item.second->getType(checker), checker);
}
subtypes[0] = keyType;
subtypes[1] = valueType;
return make_shared<ComposedTypeInfo>(seqtype, 2, std::move(subtypes));
}

shared_ptr<TypeInfo> LiteralTupleExpression::getType (TypeChecker& checker) {
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(checker.vm));
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(items.size());
for (int i=0, n=items.size(); i<n; i++) subtypes[i] = items[i]->getType(checker);
return make_shared<ComposedTypeInfo>(seqtype, items.size(), std::move(subtypes));
}

shared_ptr<TypeInfo>  LiteralRegexExpression::getType (TypeChecker& checker) { 
return make_shared<ClassTypeInfo>(checker.vm.regexClass); 
}

shared_ptr<TypeInfo> LiteralGridExpression::getType (TypeChecker& checker) {
auto seqtype = make_shared<ClassTypeInfo>(checker.vm.gridClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = TypeInfo::ANY;
for (auto& row: data) for (auto& item: row) type = type->merge(item->getType(checker), checker);
subtypes[0] = type;
return make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
}

shared_ptr<TypeInfo>  ConditionalExpression::getType (TypeChecker& checker) { 
if (!elsePart) return ifPart->getType(checker);
auto tp1 = ifPart->getType(checker), tp2 = elsePart->getType(checker);
return tp1->merge(tp2, checker);
}

shared_ptr<TypeInfo>  SwitchExpression::getType (TypeChecker& checker) { 
shared_ptr<TypeInfo> type = TypeInfo::ANY;
if (defaultCase) type = type->merge(defaultCase->getType(checker), checker);
for (auto& p: cases) type = type->merge(p.second->getType(checker), checker);
return type;
}

std::shared_ptr<TypeInfo> ComprehensionExpression::getType (TypeChecker& checker) {
auto seqtype = make_shared<ClassTypeInfo>(checker.vm.iterableClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = loopExpression->getType(checker);
subtypes[0] = type;
return make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
}

shared_ptr<TypeInfo> TypeHintExpression::getType (TypeChecker& checker) { 
return type->resolve(checker); 
}

shared_ptr<TypeInfo> AssignmentOperation::getType (TypeChecker& checker) { 
return right->getType(checker); 
}

shared_ptr<TypeInfo> SubscriptExpression::getType (TypeChecker& checker) {
return checker.resolveCallType(receiver, { T_NAME, "[]", 2, QV::UNDEFINED });
}

shared_ptr<TypeInfo> ClassDeclaration::getType (TypeChecker& checker) { 
auto thisPtr = static_pointer_cast<ClassDeclaration>(shared_from_this());
return make_shared<ClassDeclTypeInfo>(thisPtr); 
}

shared_ptr<TypeInfo>  BinaryOperation::getType (TypeChecker& checker) {
if (op>=T_EQEQ && op<=T_GTE) return make_shared<ClassTypeInfo>(checker.vm.boolClass);
else if (op>=T_EQ && op<=T_BARBAREQ) return right->getType(checker);
else return left->getType(checker);
}

shared_ptr<TypeInfo>  UnaryOperation::getType (TypeChecker& checker) {
if (op==T_EXCL) return make_shared<ClassTypeInfo>(checker.vm.boolClass);
return expr->getType(checker);
}

shared_ptr<TypeInfo> NameExpression::getType (TypeChecker& checker) {
if (token.type==T_END) token = checker.parser.curMethodNameToken;
auto var = checker.findVariable(token, LV_EXISTING | LV_FOR_READ);
if (var) return type->merge(var->type, checker);
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
ClassDeclaration* cls = checker.getCurClass();
if (cls) return checker.resolveCallType(make_shared<NameExpression>(thisToken), token);
return TypeInfo::MANY;
}


shared_ptr<TypeInfo> MemberLookupOperation::getType (TypeChecker& checker) {
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
if (getter->token.type==T_END) getter->token = checker.parser.curMethodNameToken;
return checker.resolveCallType(left, getter->token, 0, nullptr, !!super);
}
shared_ptr<CallExpression> call = dynamic_pointer_cast<CallExpression>(right);
if (call) {
getter = dynamic_pointer_cast<NameExpression>(call->receiver);
if (getter) {
if (getter->token.type==T_END) getter->token = checker.parser.curMethodNameToken;
return checker.resolveCallType(left, getter->token, call->args.size(), &(call->args[0]), !!super);
}}
return TypeInfo::MANY;
}

shared_ptr<TypeInfo> CallExpression::getType (TypeChecker& checker) {
if (auto name=dynamic_pointer_cast<NameExpression>(receiver)) {
if (!checker.findVariable(name->token, LV_EXISTING | LV_FOR_READ) && checker.getCurClass()) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto expr = BinaryOperation::create(make_shared<NameExpression>(thisToken), T_DOT, shared_this())->optimize();
return expr->getType(checker);
}}
return checker.resolveCallType(receiver, { T_NAME, "()", 2, QV::UNDEFINED }, args.size(), &args[0]);
}


shared_ptr<TypeInfo> DebugExpression::getType (TypeChecker& checker) { 
return expr->getType(checker); 
}

shared_ptr<TypeInfo> FunctionDeclaration::getType (TypeChecker& checker) {
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(params.size()+1);
auto funcTI = make_shared<ClassTypeInfo>(checker.vm.functionClass);
for (int i=0, n=params.size(); i<n; i++) {
auto& p = params[i];
auto t = p->typeHint;
subtypes[i] = t?t: TypeInfo::MANY;
}
subtypes[params.size()] = returnTypeHint? returnTypeHint : TypeInfo::MANY;
return make_shared<ComposedTypeInfo>(funcTI, params.size()+1, std::move(subtypes));
}


static inline bool isUnpack (shared_ptr<Expression> expr) {
return expr->isUnpack();
}

void LiteralSequenceExpression::typeCheck (TypeChecker& checker) {
for (auto item: items) item->typeCheck(checker);
}

void LiteralMapExpression::typeCheck (TypeChecker& checker) {
for (auto& item: items) {
item.first->typeCheck(checker);
item.second->typeCheck(checker);
}
}

void LiteralGridExpression::typeCheck (TypeChecker& checker) {
for (auto& row: data) {
for (auto& value: row) {
value->typeCheck(checker);
}}
}

void ConditionalExpression::typeCheck (TypeChecker& checker) {
if (condition) condition->typeCheck(checker);
if (ifPart) ifPart->typeCheck(checker);
if (elsePart) elsePart->typeCheck(checker);
}

void UnpackExpression::typeCheck (TypeChecker& checker) {
if (expr) expr->typeCheck(checker);
}

void TypeHintExpression::typeCheck (TypeChecker& checker)  { 
if (expr) expr->typeCheck(checker); 
} 

void YieldExpression::typeCheck (TypeChecker& checker) {
if (expr) expr->typeCheck(checker);
}

void ImportExpression::typeCheck (TypeChecker& checker) {
doCompileTimeImport(checker.parser.vm, checker.parser.filename, from);
from->typeCheck(checker);
}

void ComprehensionExpression::typeCheck (TypeChecker& checker) {
TypeChecker fc(checker.parser, &checker);
rootStatement->optimizeStatement()->typeCheck(fc);
}

void NameExpression::typeCheckAssignment (TypeChecker& checker, shared_ptr<Expression> assignedValue) {
if (token.type==T_END) token = checker.parser.curMethodNameToken;
auto var = checker.findVariable(token, LV_EXISTING | LV_FOR_WRITE);
assignedValue->typeCheck(checker);
if (var) var->type = var->type->merge(assignedValue->getType(checker), checker);
else {
ClassDeclaration* cls = checker.getCurClass();
if (cls) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
char setterName[token.length+2];
memcpy(&setterName[0], token.start, token.length);
setterName[token.length+1] = 0;
setterName[token.length] = '=';
QToken setterNameToken = { T_NAME, setterName, token.length+1, QV::UNDEFINED };
checker.resolveCallType(make_shared<NameExpression>(thisToken), setterNameToken, 1, &assignedValue);
}}
}

void FieldExpression::typeCheckAssignment (TypeChecker& checker, shared_ptr<Expression> assignedValue) {
auto cls = checker.getCurClass();
auto method = checker.getCurMethod();
shared_ptr<TypeInfo>* fieldType = nullptr;
if (!cls || !method) return;
if (checker.getCurMethod()->flags&FD_STATIC) return;
cls->findField(token, &fieldType);
assignedValue->typeCheck(checker);
if (fieldType) *fieldType = (*fieldType)->merge(assignedValue->getType(checker), checker);
}

void StaticFieldExpression::typeCheckAssignment (TypeChecker& checker, shared_ptr<Expression> assignedValue) {
auto cls = checker.getCurClass();
shared_ptr<TypeInfo>* fieldType = nullptr;
if (!cls) return;
cls->findStaticField(token, &fieldType);
assignedValue->typeCheck(checker);
if (fieldType) *fieldType = (*fieldType)->merge(assignedValue->getType(checker), checker);
}

void LiteralSequenceExpression::typeCheckAssignment (TypeChecker& checker, shared_ptr<Expression> assignedValue) {
checker.pushScope();
assignedValue->typeCheck(checker);
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
}}
if (!assignable || !assignable->isAssignable()) continue;
QToken indexToken = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(i)) };
shared_ptr<Expression> index = make_shared<ConstantExpression>(indexToken);
if (unpack) {
QToken minusOneToken = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(-1)) };
shared_ptr<Expression> minusOne = make_shared<ConstantExpression>(minusOneToken);
index = BinaryOperation::create(index, T_DOTDOTDOT, minusOne);
}
vector<shared_ptr<Expression>> indices = { index };
auto subscript = make_shared<SubscriptExpression>(assignedValue, indices);
if (defaultValue) defaultValue = BinaryOperation::create(subscript, T_QUESTQUEST, defaultValue)->optimize();
else defaultValue = subscript;
if (typeHint) defaultValue = make_shared<TypeHintExpression>(defaultValue, typeHint)->optimize();
assignable->typeCheckAssignment(checker, defaultValue);
}
checker.popScope();
}


void LiteralMapExpression::typeCheckAssignment (TypeChecker& checker, shared_ptr<Expression> assignedValue) {
checker.pushScope();
assignedValue->typeCheck(checker);
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
shared_ptr<Expression> value = nullptr;
if (auto method = dynamic_pointer_cast<GenericMethodSymbolExpression>(item.first))  value = BinaryOperation::create(assignedValue, T_DOT, make_shared<NameExpression>(method->token));
else {
shared_ptr<Expression> subscript = item.first;
if (auto field = dynamic_pointer_cast<FieldExpression>(subscript))  {
field->token.value = QV(checker.vm, field->token.start, field->token.length);
subscript = make_shared<ConstantExpression>(field->token);
}
else if (auto field = dynamic_pointer_cast<StaticFieldExpression>(subscript))  {
field->token.value = QV(checker.vm, field->token.start, field->token.length);
subscript = make_shared<ConstantExpression>(field->token);
}
vector<shared_ptr<Expression>> indices = { subscript };
value = make_shared<SubscriptExpression>(assignedValue, indices);
}
if (defaultValue) value = BinaryOperation::create(value, T_QUESTQUEST, defaultValue)->optimize();
if (typeHint) value = make_shared<TypeHintExpression>(value, typeHint)->optimize();
assignable->typeCheckAssignment(checker, value);
}
checker.popScope();
}


void UnaryOperation::typeCheck (TypeChecker& checker) {
expr->typeCheck(checker);
}

void BinaryOperation::typeCheck (TypeChecker& checker) {
left->typeCheck(checker);
right->typeCheck(checker);
}

void SwitchExpression::typeCheck (TypeChecker& checker) {
expr->typeCheck(checker);
for (auto& c: cases) {
for (auto& item: c.first) item->typeCheck(checker);
c.second->typeCheck(checker);
}
if (defaultCase) defaultCase->typeCheck(checker);
}

void AbstractCallExpression::typeCheck  (TypeChecker& checker) {
receiver->typeCheck(checker);
for (auto arg: args) arg->typeCheck(checker);
}

void SubscriptExpression::typeCheckAssignment  (TypeChecker& checker, shared_ptr<Expression> assignedValue) {
receiver->typeCheck(checker);
for (auto arg: args) arg->typeCheck(checker);
assignedValue->typeCheck(checker);
vector<shared_ptr<Expression>> tmpargs = args; tmpargs.push_back(assignedValue);
checker.resolveCallType(receiver, { T_NAME, "[]=", 3, QV::UNDEFINED }, tmpargs.size(), &tmpargs[0]);
}

void MemberLookupOperation::typeCheckAssignment (TypeChecker& checker, shared_ptr<Expression> assignedValue) {
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> setter = dynamic_pointer_cast<NameExpression>(right);
if (setter) {
string sName = string(setter->token.start, setter->token.length) + ("=");
int symbol = checker.vm.findMethodSymbol(sName);
left->typeCheck(checker);
assignedValue->typeCheck(checker);
checker.resolveCallType(left, setter->token, 1, &assignedValue, !!super);
return;
}
}

void AssignmentOperation::typeCheck (TypeChecker& checker) {
shared_ptr<Assignable> target = dynamic_pointer_cast<Assignable>(left);
if (target && target->isAssignable()) {
target->typeCheckAssignment(checker, right);
return;
}
}



void ClassDeclaration::typeCheck (TypeChecker& checker) {
for (auto decoration: decorations) decoration->typeCheck(checker);
ClassDeclaration* oldClassDecl = checker.curClass;
checker.curClass = this;
for (auto method: methods) {
checker.parser.curMethodNameToken = method->name;
checker.curMethod = method.get();
method->typeCheck(checker);
}
checker.curClass = oldClassDecl;
}

void FunctionDeclaration::typeCheckParams (TypeChecker& checker) {
vector<shared_ptr<Variable>> destructuring;
for (auto& var: params) {
auto name = dynamic_pointer_cast<NameExpression>(var->name);
if (!name) destructuring.push_back(var);
else {
lv = checker.findVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0));
if (var->value) {
auto value = BinaryOperation::create(name, T_QUESTQUESTEQ, var->value)->optimize();
value->typeCheck(checker);
if (lv) lv->type = value->getType(checker)->merge(lv->type, checker);
}}
if (var->decorations.size()) {
for (auto& decoration: var->decorations) decoration->typeCheck(checker);
}
//if (var->typeHint && lv) lv->type = var->typeHint;
}
if (destructuring.size()) {
make_shared<VariableDeclaration>(destructuring)->optimizeStatement()->typeCheck(checker);
}
}


void FunctionDeclaration::typeCheck  (TypeChecker& checker) {
TypeChecker tc(checker.parser, &checker);
checker.parser.curMethodNameToken = name;
typeCheckParams(tc);
body=body->optimizeStatement();
body->typeCheck(tc);
if (flags&FD_FIBER) {
QToken fiberToken = { T_NAME, FIBER, 5, QV::UNDEFINED };
decorations.insert(decorations.begin(), make_shared<NameExpression>(fiberToken));
}
else if (flags&FD_ASYNC) {
QToken asyncToken = { T_NAME, ASYNC, 5, QV::UNDEFINED };
decorations.insert(decorations.begin(), make_shared<NameExpression>(asyncToken));
}
for (auto decoration: decorations) decoration->typeCheck(checker);
}



void DebugExpression::typeCheck (TypeChecker& checker) {
expr->typeCheck(checker);
checker.parser.printMessage(nearestToken(), Swan::CompilationMessage::Kind::INFO, format("Type = %s", expr->getType(checker)->toString()));
}

