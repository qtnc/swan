#include "Expression.hpp"
#include "TypeInfo.hpp"
#include "Compiler.hpp"
#include "ParserRules.hpp"
#include "../vm/VM.hpp"
using namespace std;

Expression::Expression(): 
type(TypeInfo::ANY) 
{}

shared_ptr<TypeInfo> Expression::computeType (QCompiler& compiler) { 
return TypeInfo::MANY;
}

shared_ptr<TypeInfo> Expression::getType (QCompiler& compiler) { 
if (!type || type->isEmpty()) type = computeType(compiler);
return type;
}

shared_ptr<TypeInfo> ConstantExpression::computeType (QCompiler& compiler) { 
 return make_shared<ClassTypeInfo>(&token.value.getClass(compiler.parser.vm));  
}

shared_ptr<TypeInfo> LiteralSequenceExpression::computeType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(compiler.parser.vm));
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = TypeInfo::ANY;
for (auto& item: items) type = type->merge(item->getType(compiler), compiler);
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

shared_ptr<TypeInfo> LiteralMapExpression::computeType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(compiler.parser.vm.mapClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(2);
shared_ptr<TypeInfo> keyType = TypeInfo::ANY, valueType = TypeInfo::ANY;
for (auto& item: items) {
keyType = keyType->merge(item.first->getType(compiler), compiler);
valueType = valueType->merge(item.second->getType(compiler), compiler);
}
subtypes[0] = keyType;
subtypes[1] = valueType;
return make_shared<ComposedTypeInfo>(seqtype, 2, std::move(subtypes));
}


shared_ptr<TypeInfo> LiteralTupleExpression::computeType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(compiler.parser.vm));
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(items.size());
for (int i=0, n=items.size(); i<n; i++) subtypes[i] = items[i]->getType(compiler);
return make_shared<ComposedTypeInfo>(seqtype, items.size(), std::move(subtypes));
}

shared_ptr<TypeInfo>  LiteralRegexExpression::computeType (QCompiler& compiler) { 
return make_shared<ClassTypeInfo>(compiler.parser.vm.regexClass); 
}

shared_ptr<TypeInfo> LiteralGridExpression::computeType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(compiler.parser.vm.gridClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = TypeInfo::ANY;
for (auto& row: data) for (auto& item: row) type = type->merge(item->getType(compiler), compiler);
subtypes[0] = type;
return make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
}

shared_ptr<TypeInfo>  ConditionalExpression::computeType (QCompiler& compiler) { 
if (!elsePart) return ifPart->getType(compiler);
auto tp1 = ifPart->getType(compiler), tp2 = elsePart->getType(compiler);
return tp1->merge(tp2, compiler);
}

shared_ptr<TypeInfo>  SwitchExpression::computeType (QCompiler& compiler) { 
shared_ptr<TypeInfo> type = TypeInfo::ANY;
if (defaultCase) type = type->merge(defaultCase->getType(compiler), compiler);
for (auto& p: cases) type = type->merge(p.second->getType(compiler), compiler);
return type;
}

std::shared_ptr<TypeInfo> ComprehensionExpression::computeType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(compiler.parser.vm.iterableClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = loopExpression->getType(compiler);
subtypes[0] = type;
return make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
}

shared_ptr<TypeInfo> TypeHintExpression::computeType (QCompiler& compiler) { 
return type->resolve(compiler); 
}

shared_ptr<TypeInfo> AssignmentOperation::computeType (QCompiler& compiler) { 
return right->getType(compiler); 
}

shared_ptr<TypeInfo> SubscriptExpression::computeType (QCompiler& compiler) {
return findMethodReturnType(receiver, { T_NAME, "[]", 2, QV::UNDEFINED }, false, compiler);
}

shared_ptr<TypeInfo> ClassDeclaration::computeType (QCompiler& compiler) { 
auto thisPtr = static_pointer_cast<ClassDeclaration>(shared_from_this());
return make_shared<ClassDeclTypeInfo>(thisPtr); 
}

shared_ptr<TypeInfo>  BinaryOperation::computeType (QCompiler& compiler) {
if (op>=T_EQEQ && op<=T_GTE) return make_shared<ClassTypeInfo>(compiler.parser.vm.boolClass);
else if (op>=T_EQ && op<=T_BARBAREQ) return right->getType(compiler);
else return left->getType(compiler);
}

shared_ptr<TypeInfo>  UnaryOperation::computeType (QCompiler& compiler) {
if (op==T_EXCL) return make_shared<ClassTypeInfo>(compiler.parser.vm.boolClass);
return expr->getType(compiler);
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
if (cls) return findMethodReturnType(make_shared<ClassDeclTypeInfo>(cls), token, false, compiler);
return TypeInfo::MANY;
}

shared_ptr<TypeInfo> MemberLookupOperation::computeType (QCompiler& compiler) {
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
if (getter->token.type==T_END) getter->token = compiler.parser.curMethodNameToken;
return findMethodReturnType(left, getter->token, !!super, compiler);
}
shared_ptr<CallExpression> call = dynamic_pointer_cast<CallExpression>(right);
if (call) {
getter = dynamic_pointer_cast<NameExpression>(call->receiver);
if (getter) {
if (getter->token.type==T_END) getter->token = compiler.parser.curMethodNameToken;
return findMethodReturnType(left, getter->token, !!super, compiler);
}}
return TypeInfo::MANY;
}

shared_ptr<TypeInfo> CallExpression::computeType (QCompiler& compiler) {
if (auto name=dynamic_pointer_cast<NameExpression>(receiver)) {
if (compiler.findLocalVariable(name->token, LV_EXISTING | LV_FOR_READ)<0 && compiler.findUpvalue(name->token, LV_FOR_READ)<0 && compiler.findGlobalVariable(name->token, LV_FOR_READ)<0 && compiler.getCurClass()) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto expr = BinaryOperation::create(make_shared<NameExpression>(thisToken), T_DOT, shared_this())->optimize();
return expr->getType(compiler);
}}
return findMethodReturnType(receiver, { T_NAME, "()", 2, QV::UNDEFINED }, false, compiler);
}

shared_ptr<TypeInfo> DebugExpression::computeType (QCompiler& compiler) { 
return expr->computeType(compiler); 
}

shared_ptr<TypeInfo> FunctionDeclaration::computeType (QCompiler& compiler) {
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(params.size()+1);
auto funcTI = make_shared<ClassTypeInfo>(compiler.parser.vm.functionClass);
for (int i=0, n=params.size(); i<n; i++) {
auto& p = params[i];
auto t = p->typeHint;
subtypes[i] = t?t: TypeInfo::MANY;
}
subtypes[params.size()] = returnTypeHint? returnTypeHint : TypeInfo::MANY;
return make_shared<ComposedTypeInfo>(funcTI, params.size()+1, std::move(subtypes));
}
