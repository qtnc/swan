#include "Expression.hpp"
#include "TypeInfo.hpp"
#include "Compiler.hpp"
#include "ParserRules.hpp"
#include "../vm/VM.hpp"
using namespace std;

shared_ptr<TypeInfo> ConstantExpression::getType (QCompiler& compiler) { 
return compiler.resolveValueType(token.value);
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

shared_ptr<TypeInfo> LiteralSequenceExpression::getType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(compiler.parser.vm));
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = TypeInfo::ANY;
for (auto& item: items) type = compiler.mergeTypes(type, item->getType(compiler));
subtypes[0] = type;
return make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
}

shared_ptr<TypeInfo> LiteralMapExpression::getType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(compiler.parser.vm.mapClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(2);
shared_ptr<TypeInfo> keyType = TypeInfo::ANY, valueType = TypeInfo::ANY;
for (auto& item: items) {
keyType = compiler.mergeTypes(keyType, item.first->getType(compiler));
valueType = compiler.mergeTypes(valueType, item.second->getType(compiler));
}
subtypes[0] = keyType;
subtypes[1] = valueType;
return make_shared<ComposedTypeInfo>(seqtype, 2, std::move(subtypes));
}


shared_ptr<TypeInfo> LiteralTupleExpression::getType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(compiler.parser.vm));
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(items.size());
for (int i=0, n=items.size(); i<n; i++) subtypes[i] = items[i]->getType(compiler);
return make_shared<ComposedTypeInfo>(seqtype, items.size(), std::move(subtypes));
}

shared_ptr<TypeInfo>  LiteralRegexExpression::getType (QCompiler& compiler) { 
return make_shared<ClassTypeInfo>(compiler.parser.vm.regexClass); 
}

shared_ptr<TypeInfo> LiteralGridExpression::getType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(compiler.parser.vm.gridClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = TypeInfo::ANY;
for (auto& row: data) for (auto& item: row) type = compiler.mergeTypes(type, item->getType(compiler));
subtypes[0] = type;
return make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
}

shared_ptr<TypeInfo>  ConditionalExpression::getType (QCompiler& compiler) { 
if (!elsePart) return ifPart->getType(compiler);
auto tp1 = ifPart->getType(compiler), tp2 = elsePart->getType(compiler);
return compiler.mergeTypes(tp1, tp2);
}

shared_ptr<TypeInfo>  SwitchExpression::getType (QCompiler& compiler) { 
shared_ptr<TypeInfo> type = TypeInfo::ANY;
if (defaultCase) type = compiler.mergeTypes(type, defaultCase->getType(compiler));
for (auto& p: cases) type = compiler.mergeTypes(type, p.second->getType(compiler));
return type;
}

std::shared_ptr<TypeInfo> ComprehensionExpression::getType (QCompiler& compiler) {
auto seqtype = make_shared<ClassTypeInfo>(compiler.parser.vm.iterableClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> type = loopExpression->getType(compiler);
subtypes[0] = type;
return make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
}

shared_ptr<TypeInfo> SubscriptExpression::getType (QCompiler& compiler) {
auto rectype = receiver->getType(compiler);
if (auto cti = dynamic_pointer_cast<ComposedTypeInfo>(rectype)) {
if (auto ct = dynamic_pointer_cast<ClassTypeInfo>(cti->type)) {
if (ct->type==compiler.vm.listClass && cti->nSubtypes==1) return cti->subtypes[0];
else if (ct->type==compiler.vm.mapClass && cti->nSubtypes==2) return cti->subtypes[1];
else if (ct->type==compiler.vm.tupleClass && args.size()==1) if (auto cxe = dynamic_pointer_cast<ConstantExpression>(args[0])) if (cti->nSubtypes>=cxe->token.value.d) return cti->subtypes[cxe->token.value.d];
}}
return compiler.resolveCallType(receiver, { T_NAME, "[]", 2, QV::UNDEFINED });
}

shared_ptr<TypeInfo> ClassDeclaration::getType (QCompiler& compiler) { 
auto thisPtr = static_pointer_cast<ClassDeclaration>(shared_from_this());
return make_shared<ClassDeclTypeInfo>(thisPtr); 
}

shared_ptr<TypeInfo>  BinaryOperation::getType (QCompiler& compiler) {
if (op>=T_EQEQ && op<=T_GTE) return make_shared<ClassTypeInfo>(compiler.parser.vm.boolClass);
else if (op>=T_EQ && op<=T_BARBAREQ) return right->getType(compiler);
else return left->getType(compiler);
}

shared_ptr<TypeInfo>  UnaryOperation::getType (QCompiler& compiler) {
if (op==T_EXCL) return make_shared<ClassTypeInfo>(compiler.parser.vm.boolClass);
return expr->getType(compiler);
}

shared_ptr<TypeInfo> FunctionDeclaration::getType (QCompiler& compiler) {
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(params.size()+1);
auto funcTI = make_shared<ClassTypeInfo>(compiler.parser.vm.functionClass);
for (int i=0, n=params.size(); i<n; i++) {
auto& p = params[i];
auto t = p->typeHint;
if (!t && p->value) t = p->value->getType(compiler);
subtypes[i] = t?t: TypeInfo::MANY;
}
subtypes[params.size()] = returnTypeHint? returnTypeHint : TypeInfo::MANY;
return make_shared<ComposedTypeInfo>(funcTI, params.size()+1, std::move(subtypes));
}

shared_ptr<TypeInfo>  FieldExpression::getType (QCompiler& compiler) {
ClassDeclaration* cls = compiler.getCurClass();
shared_ptr<TypeInfo>* fieldType = nullptr;
if (cls) cls->findField(token, &fieldType);
return fieldType? *fieldType : nullptr;
}

shared_ptr<TypeInfo>  StaticFieldExpression::getType (QCompiler& compiler) {
ClassDeclaration* cls = compiler.getCurClass();
shared_ptr<TypeInfo>* fieldType = nullptr;
if (cls) cls->findStaticField(token, &fieldType);
return fieldType? *fieldType : nullptr;
}

shared_ptr<TypeInfo>  MethodLookupOperation::getType (QCompiler& compiler) {
return TypeInfo::MANY;
}

shared_ptr<TypeInfo>  GenericMethodSymbolExpression::getType (QCompiler& compiler) {
return TypeInfo::MANY;
}

shared_ptr<TypeInfo>  AnonymousLocalExpression::getType (QCompiler& compiler) {
return TypeInfo::MANY;
}

shared_ptr<TypeInfo>  DupExpression::getType (QCompiler& compiler) {
return TypeInfo::MANY;
}

shared_ptr<TypeInfo>  SuperExpression::getType (QCompiler& compiler) {
return TypeInfo::MANY;
}

shared_ptr<TypeInfo>  UnpackExpression::getType (QCompiler& compiler) {
return TypeInfo::MANY;
}

shared_ptr<TypeInfo>  YieldExpression::getType (QCompiler& compiler) {
return TypeInfo::MANY;
}

shared_ptr<TypeInfo>  ImportExpression::getType (QCompiler& compiler) {
return TypeInfo::MANY;
}
