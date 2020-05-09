#include "Expression.hpp"
#include "Statement.hpp"
#include "TypeInfo.hpp"
#include "TypeAnalyzer.hpp"
#include "ParserRules.hpp"
#include "../vm/VM.hpp"
using namespace std;

int ConstantExpression::analyze (TypeAnalyzer& ta) { 
auto tp = ta.resolveValueType(token.value);
return ta.assignType(*this, tp);
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

int LiteralSequenceExpression::analyze (TypeAnalyzer& ta) {
int re = 0;
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(ta.parser.vm));
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> itemType = TypeInfo::ANY;
for (auto& item: items) {
re |= item->analyze(ta);
itemType = ta.mergeTypes(itemType, item->type);
}
subtypes[0] = itemType;
auto finalType = make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
re |= ta.assignType(*this, finalType);
return re;
}

int LiteralMapExpression::analyze (TypeAnalyzer& ta) {
int re = 0;
auto seqtype = make_shared<ClassTypeInfo>(ta.parser.vm.mapClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(2);
shared_ptr<TypeInfo> keyType = TypeInfo::ANY, valueType = TypeInfo::ANY;
for (auto& item: items) {
re |= item.first->analyze(ta) | item.second->analyze(ta);
keyType = ta.mergeTypes(keyType, item.first->type);
valueType = ta.mergeTypes(valueType, item.second->type);
}
subtypes[0] = keyType;
subtypes[1] = valueType;
auto finalType = make_shared<ComposedTypeInfo>(seqtype, 2, std::move(subtypes));
re |= ta.assignType(*this, finalType);
return re;
}

int LiteralTupleExpression::analyze (TypeAnalyzer& ta) {
int re = 0;
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(ta.parser.vm));
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(items.size());
for (int i=0, n=items.size(); i<n; i++) {
re |= items[i]->analyze(ta);
subtypes[i] = items[i]->type;
}
auto finalType = make_shared<ComposedTypeInfo>(seqtype, items.size(), std::move(subtypes));
re |= ta.assignType(*this, finalType);
return re;
}

int LiteralRegexExpression::analyze (TypeAnalyzer& ta) { 
auto tp = make_shared<ClassTypeInfo>(ta.parser.vm.regexClass); 
return ta.assignType(*this, tp);
}

int LiteralGridExpression::analyze (TypeAnalyzer& ta) {
int re = 0;
auto seqtype = make_shared<ClassTypeInfo>(ta.parser.vm.gridClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
shared_ptr<TypeInfo> itemType = TypeInfo::ANY;
for (auto& row: data) {
for (auto& item: row) {
re |= item->analyze(ta);
itemType = ta.mergeTypes(itemType, item->type);
}}
subtypes[0] = itemType;
auto finalType = make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
re |= ta.assignType(*this, finalType);
return re;
}

int ConditionalExpression::analyze (TypeAnalyzer& ta) { 
int re = 0;
shared_ptr<TypeInfo> finalType;
if (ifPart) re |= ifPart->analyze(ta);
if (elsePart) re |= elsePart->analyze(ta);
if (!elsePart) finalType = ifPart->type;
else if (!ifPart) finalType = elsePart->type;
else finalType = ta.mergeTypes(ifPart->type, elsePart->type);
re |= ta.assignType(*this, finalType);
return re;
}

int SwitchExpression::analyze (TypeAnalyzer& ta) { 
int re = 0;
shared_ptr<TypeInfo> finalType = TypeInfo::ANY;
if (defaultCase) {
re |= defaultCase->analyze(ta);
finalType = ta.mergeTypes(finalType, defaultCase->type);
}
for (auto& p: cases) {
re |= p.second->analyze(ta);
finalType = ta.mergeTypes(finalType, p.second->type);
}
re |= ta.assignType(*this, finalType);
return re;
}

int ComprehensionExpression::analyze (TypeAnalyzer& ta) {
int re = (rootStatement? rootStatement->analyze(ta) :0) | (loopExpression? loopExpression->analyze(ta) :0);
auto seqtype = make_shared<ClassTypeInfo>(ta.parser.vm.iterableClass);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(1);
subtypes[0] = loopExpression->type;
auto finalType = make_shared<ComposedTypeInfo>(seqtype, 1, std::move(subtypes));
re |= ta.assignType(*this, finalType);
return re;
}

int SubscriptExpression::analyze (TypeAnalyzer& ta) {
int re = receiver->analyze(ta);
for (auto& arg: args) re |= arg->analyze(ta);
auto rectype = receiver->type;
shared_ptr<TypeInfo> finalType = nullptr;
if (auto cti = dynamic_pointer_cast<ComposedTypeInfo>(rectype)) {
if (auto ct = dynamic_pointer_cast<ClassTypeInfo>(cti->type)) {
if (ct->type==ta.vm.listClass && cti->nSubtypes==1) finalType = cti->subtypes[0];
else if (ct->type==ta.vm.mapClass && cti->nSubtypes==2) finalType = cti->subtypes[1];
else if (ct->type==ta.vm.tupleClass && args.size()==1) if (auto cxe = dynamic_pointer_cast<ConstantExpression>(args[0])) if (cti->nSubtypes>=cxe->token.value.d) finalType = cti->subtypes[cxe->token.value.d];
}}
if (!finalType) finalType = ta.resolveCallType(receiver, { T_NAME, "[]", 2, QV::UNDEFINED });
re |= ta.assignType(*this, finalType);
return re;
}

int ClassDeclaration::analyze (TypeAnalyzer& ta) { 
int re = 0;
for (auto& m: methods) re |= m->analyze(ta);
auto thisPtr = static_pointer_cast<ClassDeclaration>(shared_from_this());
auto finalType = make_shared<ClassDeclTypeInfo>(thisPtr); 
re |= ta.assignType(*this, finalType);
return re;
}

int BinaryOperation::analyze (TypeAnalyzer& ta) {
shared_ptr<TypeInfo> finalType = nullptr;
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0);
if (op>=T_EQEQ && op<=T_GTE) finalType = make_shared<ClassTypeInfo>(ta.parser.vm.boolClass);
else if (op>=T_EQ && op<=T_BARBAREQ) finalType = right->type;
else if (op==T_LTEQGT) finalType = make_shared<ClassTypeInfo>(ta.parser.vm.numClass);
else finalType = left->type;
re |= ta.assignType(*this, finalType);
return re;
}

int UnaryOperation::analyze (TypeAnalyzer& ta) {
int re = expr? expr->analyze(ta) :0;
shared_ptr<TypeInfo> finalType = nullptr;
if (op==T_EXCL) finalType = make_shared<ClassTypeInfo>(ta.parser.vm.boolClass);
else finalType = expr->type;
re |= ta.assignType(*this, finalType);
return re;
}

std::shared_ptr<TypeInfo> FunctionDeclaration::getArgTypeInfo (int n) {
auto& p = params[n];
if (p->type) return p->type;
else return TypeInfo::MANY;
}

int FieldExpression::analyze (TypeAnalyzer& ta) {
ClassDeclaration* cls = ta.getCurClass();
shared_ptr<TypeInfo>* fieldType = nullptr;
if (cls) cls->findField(token, &fieldType);
return fieldType? ta.assignType(*this, *fieldType) :0;
}

int FieldExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
int re = assignedValue->analyze(ta);
ClassDeclaration* cls = ta.getCurClass();
if (!cls) return re;
if (ta.getCurMethod()->flags&FD_STATIC) return re;
shared_ptr<TypeInfo>* fieldType = nullptr;
cls->findField(token, &fieldType);
if (fieldType) {
*fieldType = ta.mergeTypes(*fieldType, assignedValue->type);
re |= ta.assignType(*this, *fieldType);
}
return re;
}

int StaticFieldExpression::analyze (TypeAnalyzer& ta) {
ClassDeclaration* cls = ta.getCurClass();
shared_ptr<TypeInfo>* fieldType = nullptr;
if (cls) cls->findStaticField(token, &fieldType);
return fieldType? ta.assignType(*this, *fieldType) :0;
}

int StaticFieldExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
int re = assignedValue->analyze(ta);
ClassDeclaration* cls = ta.getCurClass();
if (!cls) return re;
shared_ptr<TypeInfo>* fieldType = nullptr;
cls->findStaticField(token, &fieldType);
if (fieldType) {
*fieldType = ta.mergeTypes(*fieldType, assignedValue->type);
re |= ta.assignType(*this, *fieldType);
}
return re;
}

int NameExpression::analyze (TypeAnalyzer& ta) {
if (token.type==T_END) token = ta.parser.curMethodNameToken;
if (auto lv = ta.findVariable(token, LV_EXISTING | LV_FOR_READ)) {
auto finalType = ta.mergeTypes(type, lv->type);
return ta.assignType(*this, finalType);
}
ClassDeclaration* cls = ta.getCurClass();
if (cls) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto finalType = ta.resolveCallType(make_shared<NameExpression>(thisToken), token);
finalType = ta.mergeTypes(type, finalType);
return ta.assignType(*this, finalType);
}
return 0;
}

int NameExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
if (token.type==T_END) token = ta.parser.curMethodNameToken;
int re = assignedValue->analyze(ta);
if (auto lv = ta.findVariable(token, LV_EXISTING | LV_FOR_WRITE)) {
lv->type = ta.mergeTypes(lv->type, assignedValue->type);
re |= ta.assignType(*this, lv->type);
}
else if (auto cls = ta.getCurClass()) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
char setterName[token.length+2];
memcpy(&setterName[0], token.start, token.length);
setterName[token.length+1] = 0;
setterName[token.length] = '=';
QToken setterNameToken = { T_NAME, setterName, token.length+1, QV::UNDEFINED };
auto finalType = ta.resolveCallType(make_shared<NameExpression>(thisToken), setterNameToken, 1, &assignedValue);
re |= ta.assignType(*this, finalType);
}
return re;
}

int MemberLookupOperation::analyze (TypeAnalyzer& ta) {
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0);
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
if (getter->token.type==T_END) getter->token = ta.parser.curMethodNameToken;
uint64_t fflags = 0;
auto finalType = ta.mergeTypes(type, ta.resolveCallType(left, getter->token, 0, nullptr, !!super, &fflags));
//if (fflags&2) ta.writeOpArg<uint_local_index_t>(OP_LOAD_FIELD, (fflags>>8) );
//else ta.writeOpArg<uint_method_symbol_t>(super? OP_CALL_SUPER_1 : OP_CALL_METHOD_1, symbol);
re |= ta.assignType(*this, finalType);
return re;
}
shared_ptr<CallExpression> call = dynamic_pointer_cast<CallExpression>(right);
if (call) {
getter = dynamic_pointer_cast<NameExpression>(call->receiver);
if (getter) {
if (getter->token.type==T_END) getter->token = ta.parser.curMethodNameToken;
uint64_t fflags = 0;
auto finalType = ta.mergeTypes(type, ta.resolveCallType(left, getter->token, call->args.size(), &(call->args[0]), !!super, &fflags));
re |= ta.assignType(*this, finalType);
return re;
}}
return re;
}

int MemberLookupOperation::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0) | (assignedValue? assignedValue->analyze(ta) :0);
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> setter = dynamic_pointer_cast<NameExpression>(right);
if (setter) {
uint64_t fflags = 0;
string sName = string(setter->token.start, setter->token.length) + ("=");
QToken sToken = { T_NAME, sName.data(), sName.size(), QV::UNDEFINED };
auto finalType = ta.resolveCallType(left, sToken, 1, &assignedValue, !!super, &fflags);
//if (fflags&4) {
//ta.writeOpArg<uint8_t>(OP_SWAP, 0xFE);
//ta.writeOpArg<uint_local_index_t>(OP_STORE_FIELD,  (fflags>>8) );
//}
//else ta.writeOpArg<uint_method_symbol_t>(super? OP_CALL_SUPER_2 : OP_CALL_METHOD_2, symbol);
re |= ta.assignType(*this, finalType);
return re;
}
return re;
}

int LiteralSequenceExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
//###todo
return 0;
}

int LiteralMapExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
//###todo
return 0;
}

int MethodLookupOperation::analyze (TypeAnalyzer& ta) {
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0);
re |= ta.assignType(*this, TypeInfo::MANY);
return re;
}

int CallExpression::analyze (TypeAnalyzer& ta) {
int re = 0;
if (receiver) re |= receiver->analyze(ta);
for (auto& arg: args) re |= arg->analyze(ta);
QV func = QV::UNDEFINED;
if (auto name=dynamic_pointer_cast<NameExpression>(receiver)) {
auto lv = ta.findVariable(name->token, LV_EXISTING | LV_FOR_READ);
if (!lv && ta.getCurClass()) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto thisExpr = make_shared<NameExpression>(thisToken);
auto expr = BinaryOperation::create(thisExpr, T_DOT, shared_this())->optimize();
re |= expr->analyze(ta);
auto finalType = ta.resolveCallType(thisExpr, name->token, args.size(), &args[0]);
re |= ta.assignType(*this, finalType);
return re;
}}
/*if (globalIndex>=0) {
auto gval = ta.parser.vm.globalVariables[globalIndex];
QToken tmptok = { T_NAME, 0, 0, gval  };
type = ta.resolveCallType(make_shared<ConstantExpression>(tmptok), gval, args.size(), &args[0]);
}
else */
auto finalType = ta.resolveCallType(receiver, args.size(), &args[0]);
re |= ta.assignType(*this, finalType);
return re;
}

int SubscriptExpression::analyzeAssignment  (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
int re = receiver->analyze(ta);
for (auto& arg: args) re |= arg->analyze(ta);
re |= assignedValue->analyze(ta);
//todo: update generic subtype if possible
return re;
}

int AssignmentOperation::analyze (TypeAnalyzer& ta) {
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0);
shared_ptr<Assignable> target = dynamic_pointer_cast<Assignable>(left);
if (target && target->isAssignable()) {
re |= target->analyzeAssignment(ta, right);
re |= ta.assignType(*this, right->type);
}
return re;
}


int MethodLookupOperation::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0) | (assignedValue? assignedValue->analyze(ta) :0);
re |= ta.assignType(*this, TypeInfo::MANY);
return re;
}

int FunctionDeclaration::analyze  (TypeAnalyzer& ta) {
int re = 0;
TypeAnalyzer fta(ta.parser, &ta);
shared_ptr<Expression> lastExpr = nullptr;
fta.curMethod = this;
fta.parser.curMethodNameToken = name;
re |= analyzeParams(fta);
body=body->optimizeStatement();
body->analyze(fta);
if (body->isExpression()) {
lastExpr = static_pointer_cast<Expression>(body);
if (auto fe = dynamic_pointer_cast<FieldExpression>(body)) {
flags|=FD_GETTER;
auto cls = ta.getCurClass();
if (cls) iField = cls->findField(fe->token);
}}
else if (auto bs = dynamic_pointer_cast<BlockStatement>(body)) {
if (bs->statements.size()>=1 && bs->statements.back()->isExpression()) lastExpr = static_pointer_cast<Expression>(bs->statements.back());
}
if (lastExpr) returnType = ta.mergeTypes(returnType, lastExpr->type);
return re | ta.assignType(*this, getFunctionTypeInfo()->resolve(ta));
}

int FunctionDeclaration::analyzeParams (TypeAnalyzer& ta) {
int re = 0;
vector<shared_ptr<Variable>> destructuring;
for (auto& var: params) {
if (var->value) re |= var->value->analyze(ta);
auto name = dynamic_pointer_cast<NameExpression>(var->name);
AnalyzedVariable* lv = nullptr;
if (!name) {
name = make_shared<NameExpression>(ta.createTempName());
lv = ta.findVariable(name->token, LV_NEW);
if (!(var->flags&VD_OPTIMFLAG)) {
var->value = var->value? BinaryOperation::create(name, T_QUESTQUESTEQ, var->value)->optimize() : name;
re |= var->value->analyze(ta);
}
destructuring.push_back(var);
var->flags |= VD_OPTIMFLAG;
lv->type = var->value->type;
}
else {
lv = ta.findVariable(name->token, LV_NEW);
if (var->value) lv->type = var->value->type;
}
if (var->decorations.size()) {
for (auto& decoration: var->decorations) re |= decoration->analyze(ta);
}
if (var->type) {
if (lv) lv->type = var->type; //ta.mergeTypes(var->type, lv->type);
}
else if (var->value) {
var->type  = var->value->type;
if (lv) lv->type = var->value->type; //ta.mergeTypes(var->type, lv->type);
}
}
if (destructuring.size()) {
re |= make_shared<VariableDeclaration>(destructuring)->optimizeStatement()->analyze(ta);
}
return re;
}

int GenericMethodSymbolExpression::analyze (TypeAnalyzer& ta) {
return ta.assignType(*this, TypeInfo::MANY);
}

int AnonymousLocalExpression::analyze (TypeAnalyzer& ta) {
return ta.assignType(*this, TypeInfo::MANY);
}

int AnonymousLocalExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
return assignedValue->analyze(ta) | ta.assignType(*this, assignedValue->type);
}

int DupExpression::analyze (TypeAnalyzer& ta) {
return ta.assignType(*this, TypeInfo::MANY);
}

int SuperExpression::analyze (TypeAnalyzer& ta) {
return ta.assignType(*this, TypeInfo::MANY);
}

int UnpackExpression::analyze (TypeAnalyzer& ta) {
int re = expr? expr->analyze(ta) :0;
re |= ta.assignType(*this, TypeInfo::MANY);
return re;
}

int YieldExpression::analyze (TypeAnalyzer& ta) {
int re = expr? expr->analyze(ta) :0;
re |= ta.assignType(*this, TypeInfo::MANY);
return re;
}

int ImportExpression::analyze (TypeAnalyzer& ta) {
int re = from? from->analyze(ta) :0;
re |= ta.assignType(*this, TypeInfo::MANY);
return re;
}
