#include "Expression.hpp"
#include "Statement.hpp"
#include "TypeInfo.hpp"
#include "TypeAnalyzer.hpp"
#include "ParserRules.hpp"
#include "../vm/VM.hpp"
using namespace std;

extern const QToken THIS_TOKEN;

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
shared_ptr<TypeInfo> itemType = TypeInfo::ANY;
for (auto& item: items) {
re |= item->analyze(ta);
itemType = ta.mergeTypes(itemType, item->type);
}
vector<shared_ptr<TypeInfo>> subtypes = { itemType };
auto finalType = make_shared<ComposedTypeInfo>(seqtype, subtypes);
re |= ta.assignType(*this, finalType);
return re;
}

int LiteralMapExpression::analyze (TypeAnalyzer& ta) {
int re = 0;
auto seqtype = make_shared<ClassTypeInfo>(ta.parser.vm.mapClass);
shared_ptr<TypeInfo> keyType = TypeInfo::ANY, valueType = TypeInfo::ANY;
for (auto& item: items) {
re |= item.first->analyze(ta) | item.second->analyze(ta);
keyType = ta.mergeTypes(keyType, item.first->type);
valueType = ta.mergeTypes(valueType, item.second->type);
}
vector<shared_ptr<TypeInfo>> subtypes = { keyType, valueType };
auto finalType = make_shared<ComposedTypeInfo>(seqtype, subtypes);
re |= ta.assignType(*this, finalType);
return re;
}

int LiteralTupleExpression::analyze (TypeAnalyzer& ta) {
int re = 0;
auto seqtype = make_shared<ClassTypeInfo>(getSequenceClass(ta.parser.vm));
vector<shared_ptr<TypeInfo>> subtypes;
subtypes.resize(items.size());
for (int i=0, n=items.size(); i<n; i++) {
re |= items[i]->analyze(ta);
subtypes[i] = items[i]->type;
}
auto finalType = make_shared<ComposedTypeInfo>(seqtype, subtypes);
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
shared_ptr<TypeInfo> itemType = TypeInfo::ANY;
for (auto& row: data) {
for (auto& item: row) {
re |= item->analyze(ta);
itemType = ta.mergeTypes(itemType, item->type);
}}
vector<shared_ptr<TypeInfo>> subtypes = { itemType };
auto finalType = make_shared<ComposedTypeInfo>(seqtype, subtypes);
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
int re = expr->analyze(ta) | var->analyze(ta);
shared_ptr<TypeInfo> finalType = TypeInfo::ANY;
if (defaultCase) {
re |= defaultCase->analyze(ta);
finalType = ta.mergeTypes(finalType, defaultCase->type);
}
for (auto& p: cases) {
for (auto e: p.first) re |= e->analyze(ta);
if (p.second) re |= p.second->analyze(ta);
finalType = ta.mergeTypes(finalType, p.second->type);
}
re |= ta.assignType(*this, finalType);
return re;
}

int ComprehensionExpression::analyze (TypeAnalyzer& ta) {
int re = (rootStatement? rootStatement->analyze(ta) :0) | (loopExpression? loopExpression->analyze(ta) :0);
auto seqtype = make_shared<ClassTypeInfo>(ta.parser.vm.iterableClass);
vector<shared_ptr<TypeInfo>> subtypes = { loopExpression->type };
auto finalType = make_shared<ComposedTypeInfo>(seqtype, subtypes);
re |= ta.assignType(*this, finalType);
return re;
}

int SubscriptExpression::analyze (TypeAnalyzer& ta) {
int re = receiver->analyze(ta);
for (auto& arg: args) re |= arg->analyze(ta);
if (receiver->type && args.size()==1 && args[0]->type) {
auto cst = dynamic_pointer_cast<ConstantExpression>(args[0]);
auto cti = dynamic_pointer_cast<ComposedTypeInfo>(receiver->type);
auto ctp = cti && cti->type ? dynamic_pointer_cast<ClassTypeInfo>(cti->type) : nullptr;
if (cst && ctp && ctp->type==ta.vm.tupleClass && cst->token.type==T_NUM && cst->token.value.d>=0 && cst->token.value.d<cti->subtypes.size() ) {
auto finalType = cti->subtypes[static_cast<size_t>(cst->token.value.d)];
println("Found!type=%s", finalType->toString());
finalType = ta.mergeTypes(type, finalType);
re |= ta.assignType(*this, finalType);
return re;
}}
QToken subscriptToken = { T_NAME, "[]", 2, QV::UNDEFINED };
auto finalType = ta.resolveCallType(receiver, subscriptToken, args.size(), &args[0], CallFlag::None, &fd);
finalType = ta.mergeTypes(type, finalType);
re |= ta.assignType(*this, finalType);
return re;
}

int ClassDeclaration::analyze (TypeAnalyzer& ta) { 
int re = 0;
auto oldCls = ta.curClass;
ta.curClass = this;
auto classType = make_shared<ClassDeclTypeInfo>(this, TypeInfoFlag::Exact | TypeInfoFlag::Static); 
re |= ta.assignType(*this, classType);
if (!(flags & VarFlag::Optimized)) {
handleAutoConstructor(ta, fields, false);
handleAutoConstructor(ta, staticFields, true);
flags |= VarFlag::Optimized;
}
if (auto lv = ta.findVariable(name)) {
lv->value = shared_this();
lv->type = type;
}
for (auto& p: parents) {
auto parent = make_shared<NameExpression>(p);
parent->analyze(ta);
parent->type = parent->type->resolve(ta);
if (auto cdt = dynamic_pointer_cast<ClassDeclTypeInfo>(parent->type))  cdt->cls->flags |= VarFlag::Inherited;
}
for (auto& m: methods) {
ta.curMethod = m.get();
re |= m->analyze(ta);
ta.curMethod = nullptr;
}
ta.curClass = oldCls;
return re;
}

int BinaryOperation::analyze (TypeAnalyzer& ta) {
shared_ptr<TypeInfo> finalType = nullptr;
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0);
if (op>=T_EQEQ && op<=T_GTE) finalType = make_shared<ClassTypeInfo>(ta.vm.boolClass);
else if (op>=T_EQ && op<=T_BARBAREQ) finalType = right->type;
else if (op==T_LTEQGT) finalType = make_shared<ClassTypeInfo>(ta.vm.numClass);
if ((op==T_DOTDOT || op==T_DOTDOTDOT) && left->type && right->type && left->type->isNum() && right->type->isNum()) finalType = make_shared<ClassTypeInfo>(ta.parser.vm.rangeClass);
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

std::shared_ptr<TypeInfo> FunctionDeclaration::getArgTypeInfo (int n, int nPassedArgs, shared_ptr<TypeInfo>* passedArgs) {
auto& p = params[n];
if (p->type) return p->type;
else return TypeInfo::ANY;
}

int FieldExpression::analyze (TypeAnalyzer& ta) {
int re = 0;
ClassDeclaration* cls = ta.getCurClass();
shared_ptr<TypeInfo>* fieldType = nullptr;
if (cls) cls->findField(token, &fieldType);
if (fieldType) re |= ta.assignType(*this, *fieldType);
return re;
}

int FieldExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
int re = assignedValue->analyze(ta);
ClassDeclaration* cls = ta.getCurClass();
if (!cls) return re;
if (ta.getCurMethod()->flags & VarFlag::Static) return re;
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
if (auto lv = ta.findVariable(token)) {
pure = true;
auto finalType = ta.mergeTypes(type, lv->type);
return ta.assignType(*this, finalType);
}
if (auto cls = ta.getCurClass()) {
pure = false;
auto resolvedType = ta.resolveCallType(make_shared<NameExpression>(THIS_TOKEN), make_shared<ClassDeclTypeInfo>(cls), token);
auto finalType = ta.mergeTypes(type, resolvedType);
return ta.assignType(*this, finalType);
}
return 0;
}

int NameExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
if (token.type==T_END) token = ta.parser.curMethodNameToken;
int re = assignedValue->analyze(ta);
if (auto lv = ta.findVariable(token)) {
lv->type = ta.mergeTypes(lv->type, assignedValue->type);
re |= ta.assignType(*this, lv->type);
}
else if (auto cls = ta.getCurClass()) {
char setterName[token.length+2];
memcpy(&setterName[0], token.start, token.length);
setterName[token.length+1] = 0;
setterName[token.length] = '=';
QToken setterNameToken = { T_NAME, setterName, token.length+1, QV::UNDEFINED };
auto resolvedType = ta.resolveCallType(make_shared<NameExpression>(THIS_TOKEN), make_shared<ClassDeclTypeInfo>(cls), setterNameToken, 1, &assignedValue);
resolvedType = ta.mergeTypes(type, resolvedType);
re |= ta.assignType(*this, resolvedType);
}
pure = false;
return re;
}

int MemberLookupOperation::analyze (TypeAnalyzer& ta) {
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0);
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
if (getter->token.type==T_END) getter->token = ta.parser.curMethodNameToken;
auto finalType = ta.resolveCallType(left, getter->token, 0, nullptr, (super? CallFlag::Super : CallFlag::None), &fd);
finalType = ta.mergeTypes(type, finalType);
re |= ta.assignType(*this, finalType);
return re;
}
shared_ptr<CallExpression> call = dynamic_pointer_cast<CallExpression>(right);
if (call) {
getter = dynamic_pointer_cast<NameExpression>(call->receiver);
if (getter) {
if (getter->token.type==T_END) getter->token = ta.parser.curMethodNameToken;
auto finalType = ta.resolveCallType(left, getter->token, call->args.size(), &(call->args[0]), (super? CallFlag::Super : CallFlag::None), &fd);
finalType = ta.mergeTypes(type, finalType);
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
string sName = string(setter->token.start, setter->token.length) + ("=");
QToken sToken = { T_NAME, sName.data(), sName.size(), QV::UNDEFINED };
auto finalType = ta.resolveCallType(left, sToken, 1, &assignedValue, (super? CallFlag::Super : CallFlag::None), &fd);
finalType = ta.mergeTypes(type, finalType);
re |= ta.assignType(*this, finalType);
return re;
}
return re;
}

int LiteralSequenceExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
ta.pushScope();
int re = assignedValue? assignedValue->analyze(ta) :0;
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
auto subscript = make_shared<SubscriptExpression>( assignedValue, indices);
if (defaultValue) defaultValue = BinaryOperation::create(subscript, T_QUESTQUEST, defaultValue)->optimize();
else defaultValue = subscript;
if (typeHint) defaultValue = make_shared<TypeHintExpression>(defaultValue, typeHint)->optimize();
assignable->analyzeAssignment(ta, defaultValue);
}
ta.popScope();
return re;
}

int LiteralMapExpression::analyzeAssignment (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
ta.pushScope();
int count = -1;
int re = assignedValue? assignedValue->analyze(ta) :0;
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
if (!assignable || !assignable->isAssignable()) continue;
shared_ptr<Expression> value = nullptr;
if (auto method = dynamic_pointer_cast<GenericMethodSymbolExpression>(item.first))  value = BinaryOperation::create(assignedValue, T_DOT, make_shared<NameExpression>(method->token));
else if (auto unp = dynamic_pointer_cast<UnpackExpression>(item.first)) {
auto excludeKeys = make_shared<LiteralTupleExpression>(unp->nearestToken(), allKeys);
value = BinaryOperation::create(assignedValue, T_MINUS, excludeKeys);
}
else {
shared_ptr<Expression> subscript = item.first;
if (auto field = dynamic_pointer_cast<FieldExpression>(subscript))  {
field->token.value = QV(ta.vm, field->token.start, field->token.length);
subscript = make_shared<ConstantExpression>(field->token);
}
else if (auto field = dynamic_pointer_cast<StaticFieldExpression>(subscript))  {
field->token.value = QV(ta.vm, field->token.start, field->token.length);
subscript = make_shared<ConstantExpression>(field->token);
}
allKeys.push_back(subscript);
vector<shared_ptr<Expression>> indices = { subscript };
value = make_shared<SubscriptExpression>(assignedValue, indices);
}
if (defaultValue) value = BinaryOperation::create(value, T_QUESTQUEST, defaultValue)->optimize();
if (typeHint) value = make_shared<TypeHintExpression>(value, typeHint)->optimize();
assignable->analyzeAssignment(ta, value);
}
ta.popScope();
return re;
}

int MethodLookupOperation::analyze (TypeAnalyzer& ta) {
int re = (left? left->analyze(ta) :0) | (right? right->analyze(ta) :0);
auto name = dynamic_pointer_cast<NameExpression>(right);
if (!name) return re;
shared_ptr<TypeInfo> finalType = TypeInfo::ANY;
auto receiver = left;
FuncOrDecl fd;
ta.resolveCallType(receiver, name->token, 0, nullptr, CallFlag::None, &fd);
auto fi = fd.getFunctionInfo(ta);
if (fi) finalType = fi->getFunctionTypeInfo();
finalType = ta.mergeTypes(type, finalType);
re |= ta.assignType(*this, finalType);
return re;
}

int CallExpression::analyze (TypeAnalyzer& ta) {
shared_ptr<TypeInfo> finalType = nullptr;
int re = 0;
if (receiver) re |= receiver->analyze(ta);
for (auto& arg: args) re |= arg->analyze(ta);
if (auto name=dynamic_pointer_cast<NameExpression>(receiver)) {
auto lv = ta.findVariable(name->token);
auto cls = ta.getCurClass();
if (!lv && cls) finalType = ta.resolveCallType(make_shared<NameExpression>(THIS_TOKEN), make_shared<ClassDeclTypeInfo>(cls), name->token, args.size(), &args[0], CallFlag::None, &fd);
}
if (!finalType) finalType = ta.resolveCallType(receiver, args.size(), &args[0], CallFlag::None, &fd);
finalType = ta.mergeTypes(type, finalType);
re |= ta.assignType(*this, finalType);
return re;
}

int SubscriptExpression::analyzeAssignment  (TypeAnalyzer& ta, shared_ptr<Expression> assignedValue) {
int re = receiver->analyze(ta);
for (auto& arg: args) re |= arg->analyze(ta);
re |= assignedValue->analyze(ta);
QToken subscriptSetterToken = { T_NAME, "[]=", 3, QV::UNDEFINED };
auto finalType = ta.resolveCallType(receiver, subscriptSetterToken, args.size(), &args[0], CallFlag::None, &fd);
finalType = ta.mergeTypes(type, finalType);
re |= ta.assignType(*this, finalType);
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
re |= ta.assignType(*this, assignedValue->type);
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
if (body->isExpression()) lastExpr = static_pointer_cast<Expression>(body);
else if (auto bs = dynamic_pointer_cast<BlockStatement>(body)) {
if (bs->statements.size()>=1 && bs->statements.back()->isExpression()) lastExpr = static_pointer_cast<Expression>(bs->statements.back());
}
if (lastExpr) returnType = ta.mergeTypes(returnType, lastExpr->type);
return re | ta.assignType(*this, getFunctionTypeInfo());
}

int FunctionDeclaration::analyzeParams (TypeAnalyzer& ta) {
int re = 0;
vector<shared_ptr<Variable>> destructuring;
for (auto& var: params) {
if (var->value) re |= var->value->analyze(ta);
shared_ptr<NameExpression> name = nullptr; 
AnalyzedVariable* lv = nullptr;
if (name = dynamic_pointer_cast<NameExpression>(var->name)) {
lv = ta.createVariable(name->token);
if (var->value) lv->type = ta.mergeTypes(lv->type, var->value->type);
}
else {
name = make_shared<NameExpression>(ta.createTempName(*var->name));
lv = ta.createVariable(name->token);
if (!(var->flags & VarFlag::Optimized)) {
var->value = var->value? BinaryOperation::create(name, T_QUESTQUESTEQ, var->value)->optimize() : name;
re |= var->value->analyze(ta);
}
destructuring.push_back(var);
var->flags |= VarFlag::Optimized;
lv->type = ta.mergeTypes(lv->type, var->value->type);
}
if (var->decorations.size()) {
for (auto& decoration: var->decorations) re |= decoration->analyze(ta);
}
if (var->type) {
if (lv) lv->type = ta.mergeTypes(lv->type, var->type);
}
else if (var->value) {
var->value->analyze(ta);
var->type  = ta.mergeTypes(var->type, var->value->type);
if (lv) lv->type = ta.mergeTypes(lv->type, var->value->type); 
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
int re = expr->analyze(ta);
return re | ta.assignType(*this, expr->type);
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

int DebugExpression::analyze (TypeAnalyzer& ta) {
return expr->analyze(ta) | ta.assignType(*this, expr->type);
}

int TypeHintExpression::analyze (TypeAnalyzer& ta) {
return 
ta.assignType(*this, ta.mergeTypes(type, type->resolve(ta)))
| expr->analyze(ta) 
| ta.assignType(*expr, type);
}

QFunction* FuncOrDecl::getFunc () {
if (func) return func;
else if (method) return method->func;
else return nullptr;
}
