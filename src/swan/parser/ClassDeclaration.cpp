#include "Constants.hpp"
#include "Expression.hpp"
#include "Statement.hpp"
#include "TypeInfo.hpp"
#include "NativeFuncTypeInfo.hpp"
#include "ParserRules.hpp"
#include "Compiler.hpp"
#include "../vm/VM.hpp"
using namespace std;

int ClassDeclaration::findField (unordered_map<string,Field>& flds, const QToken& name, shared_ptr<TypeInfo>** type) {
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
auto assignment = BinaryOperation::create(fieldExpr, T_QUESTQUESTEQ, f.defaultValue) ->optimize();
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
ctor->body = BinaryOperation::create(make_shared<SuperExpression>(ctorToken), T_DOT, make_shared<CallExpression>(make_shared<NameExpression>(ctorToken), vector<shared_ptr<Expression>>({ make_shared<UnpackExpression>(arg) }) ));
}
methods.push_back(ctor);
}
if (ctor && inits->statements.size()) {
inits->chain(ctor->body);
ctor->body = inits;
}}


ClassDeclTypeInfo::ClassDeclTypeInfo (ClassDeclaration* c1): 
cls(std::static_pointer_cast<ClassDeclaration>(c1->shared_this())) {}

string ClassDeclTypeInfo::toString () { 
return string(cls->name.start, cls->name.length); 
}

shared_ptr<TypeInfo> ClassDeclTypeInfo::merge (shared_ptr<TypeInfo> t0, QCompiler& compiler) { 
t0 = t0? t0->resolve(compiler) :t0;
if (!t0 || t0->isEmpty()) return shared_from_this();
auto t = dynamic_pointer_cast<ClassDeclTypeInfo>(t0);
if (t && t->cls==cls) return shared_from_this();
else if (t) for (auto& p1: cls->parents) {
for (auto& p2: t->cls->parents) {
auto t3 = make_shared<NamedTypeInfo>(p1), t4 = make_shared<NamedTypeInfo>(p2);
auto re = t3->merge(t4, compiler);
if (re && re!=TypeInfo::MANY && re!=TypeInfo::ANY) return re;
}}
else for (auto& p1: cls->parents) {
auto t3 = make_shared<NamedTypeInfo>(p1);
auto re = t3->merge(t0, compiler);
if (re && re!=TypeInfo::MANY && re!=TypeInfo::ANY) return re;
}
return TypeInfo::MANY;
}

shared_ptr<TypeInfo> NamedTypeInfo::resolve (QCompiler& compiler) {
LocalVariable* lv = nullptr;
do {
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_READ, &lv);
if (slot>=0) break;
slot = compiler.findUpvalue(token, LV_FOR_READ, &lv);
if (slot>=0) break;
slot = compiler.vm.findGlobalSymbol(string(token.start, token.length), LV_EXISTING | LV_FOR_READ);
if (slot>=0) { 
auto value = compiler.parser.vm.globalVariables[slot];
if (!value.isInstanceOf(compiler.parser.vm.classClass)) break;
QClass* cls = value.asObject<QClass>();
return make_shared<ClassTypeInfo>(cls);
}
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (cls) {
auto m = cls->findMethod(token, false);
if (!m) m = cls->findMethod(token, true);
if (!m) break;
return m->returnTypeHint;
}
}while(false);
if (lv && lv->value) {
//println("CDTI::resolve, type found: %s", typeid(*lv->value).name());
if (auto cd = dynamic_pointer_cast<ClassDeclaration>(lv->value)) {
return make_shared<ClassDeclTypeInfo>(cd);
}}
return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> QCompiler::resolveValueType (QV value) {
if (value.isClosure()) {
//todo
}
else if (value.isNativeFunction()) {
intptr_t ptr = (value.i &~QV_TAGMASK);
auto it = vm.nativeFuncTypeInfos.find(ptr);
if (it!=vm.nativeFuncTypeInfos.end()) return it->second->getFunctionType(vm);
}
return make_shared<ClassTypeInfo>(&value.getClass(vm));
}

std::shared_ptr<TypeInfo> QCompiler::resolveCallType   (std::shared_ptr<Expression> receiver, QV func, int nArgs, shared_ptr<Expression>* args) {
if (func.isClosure()) {
//todo
}
else if (func.isNativeFunction()) {
intptr_t ptr = (func.i &~QV_TAGMASK);
auto it = vm.nativeFuncTypeInfos.find(ptr);
if (it!=vm.nativeFuncTypeInfos.end()) return it->second->getReturnType();
}
return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> QCompiler::resolveCallType (std::shared_ptr<Expression> receiver, const QToken& name, int nArgs, shared_ptr<Expression>* args, bool super) {
shared_ptr<TypeInfo> receiverType = receiver->getType(*this) ->resolve(*this);
if (auto cti = dynamic_pointer_cast<ComposedTypeInfo>(receiverType)) receiverType = cti->type;
if (auto clt = dynamic_pointer_cast<ClassTypeInfo>(receiverType)) {
auto cls = clt->type;
if (super) cls = cls->parent;
QV method = cls->findMethod(parser.vm.findMethodSymbol(string(name.start, name.length)));
return resolveCallType(receiver, method, nArgs, args);
}
else if (auto cdt = dynamic_pointer_cast<ClassDeclTypeInfo>(receiverType)) {
shared_ptr<FunctionDeclaration> m;
if (!super) {
m = cdt->cls->findMethod(name, false);
if (!m) m = cdt->cls->findMethod(name, true);
}
if (!m) for (auto& parentToken: cdt->cls->parents) {
auto parentTI = make_shared<NamedTypeInfo>(parentToken) ->resolve(*this);
if (cdt = dynamic_pointer_cast<ClassDeclTypeInfo>(parentTI)) {
m = cdt->cls->findMethod(name, false);
if (!m) m = cdt->cls->findMethod(name, true);
}
else if (auto clt = dynamic_pointer_cast<ClassTypeInfo>(parentTI)) {
QV method = clt->type->findMethod(parser.vm.findMethodSymbol(string(name.start, name.length)));
return resolveCallType(receiver, method, nArgs, args);
}
if (m) break;
}
if (m) return m->returnTypeHint;
}
return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> QCompiler::resolveCallType   (std::shared_ptr<Expression> receiver, int nArgs, shared_ptr<Expression>* args) {
auto funcType = receiver->getType(*this);
if (auto cdt = dynamic_pointer_cast<ClassDeclTypeInfo>(funcType)) {
return cdt;
}
else if (auto ct = dynamic_pointer_cast<ComposedTypeInfo>(funcType)) {
if (ct->nSubtypes>0) return ct->subtypes[ct->nSubtypes -1];
}
return TypeInfo::MANY;
}
