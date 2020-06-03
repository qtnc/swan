#include "TypeAnalyzer.hpp"
#include "TypeInfo.hpp"
#include "FunctionInfo.hpp"
#include "Expression.hpp"
#include "../vm/VM.hpp"
using namespace std;


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



AnalyzedVariable::AnalyzedVariable (const QToken& n, int s): 
name(n), scope(s), type(TypeInfo::ANY), value(nullptr) {}

ClassDeclaration* TypeAnalyzer::getCurClass (int* atLevel) {
if (atLevel) ++(*atLevel);
if (curClass) return curClass;
else if (parent) return parent->getCurClass(atLevel);
else return nullptr;
}

FunctionDeclaration* TypeAnalyzer::getCurMethod () {
if (curMethod) return curMethod;
else if (parent) return parent->getCurMethod();
else return nullptr;
}

void TypeAnalyzer::pushScope () {
curScope++;
}

void TypeAnalyzer::popScope () {
auto newEnd = remove_if(variables.begin(), variables.end(), [&](auto& x){ return x.scope>=curScope; });
variables.erase(newEnd, variables.end());
curScope--;
}


shared_ptr<TypeInfo> TypeAnalyzer::mergeTypes (shared_ptr<TypeInfo> t1, shared_ptr<TypeInfo> t2) {
if (t1) return t1->resolve(*this)->merge(t2, *this);
else if (t2) return t2->resolve(*this)->merge(t1, *this);
else return nullptr;
}

bool TypeAnalyzer::isSameType (const std::shared_ptr<TypeInfo>& t1, const std::shared_ptr<TypeInfo>& t2) {
if (!t1 || !t2) return false;
if (t1==t2) return true;
return t1->equals(t2);
}

int TypeAnalyzer::assignType (Expression&  e, const std::shared_ptr<TypeInfo>& type) {
if (type==TypeInfo::ANY && e.type!=TypeInfo::ANY) {
typeWarn(e.nearestToken(), "Overwrite with any type");
}
if (type==TypeInfo::MANY && (e.type!=TypeInfo::ANY && e.type!=TypeInfo::MANY)) {
typeWarn(e.nearestToken(), "Overwrite with many type");
}
auto oldType = e.type;
e.type = type;
if (isSameType(type, oldType)) return false;
//if (oldType!=TypeInfo::ANY) typeInfo(e.nearestToken(), "Type changed from %s to %s in %s", oldType?oldType->toString():"<null>", type?type->toString():"<null>", typeid(e).name());
return true;
}

AnalyzedVariable* TypeAnalyzer::findVariable (const QToken& name, int flags) {
bool createNew = flags&LV_NEW;
auto rvar = find_if(variables.rbegin(), variables.rend(), [&](auto& x){
return x.name.length==name.length && strncmp(name.start, x.name.start, name.length)==0;
});
bool found = rvar!=variables.rend();
if (createNew) {
variables.emplace_back(name, curScope);
return &variables.back();
}
else if (found)  return &*rvar;
else if (parent) return parent->findVariable(name, flags);
else {
int index = vm.findGlobalSymbol(string(name.start, name.length), LV_EXISTING | LV_FOR_READ);
if (index<0) return nullptr;
QToken tkcst = { T_NAME, nullptr, 0, vm.globalVariables[index] };
variables.emplace_back(name, curScope);
auto var = &variables.back();
var->type = resolveValueType(vm.globalVariables[index]);
var->value = make_shared<ConstantExpression>(tkcst);
return var;
}}


std::shared_ptr<TypeInfo> TypeAnalyzer::resolveValueType (QV value) {
std::unique_ptr<FunctionInfo> funcInfo;
if (value.isClosure()) {
QClosure& closure = *value.asObject<QClosure>();
if (!closure.func.typeInfo.empty()) funcInfo = make_unique<StringFunctionInfo>(*this, closure.func.typeInfo.c_str());
}
else if (value.isNativeFunction()) {
auto ptr = value.asNativeFunction();
auto it = vm.nativeFuncTypeInfos.find(ptr);
if (it!=vm.nativeFuncTypeInfos.end()) funcInfo = make_unique<StringFunctionInfo>(*this, it->second.c_str());
}
if (funcInfo) return funcInfo->getFunctionTypeInfo();
return make_shared<ClassTypeInfo>(&value.getClass(vm));
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType   (std::shared_ptr<Expression> receiver, QV func, int nArgs, shared_ptr<Expression>* args, FuncOrDecl* fd) {
std::unique_ptr<FunctionInfo> funcInfo;
if (func.isClosure()) {
QClosure& closure = *func.asObject<QClosure>();
if (!closure.func.typeInfo.empty()) funcInfo = make_unique<StringFunctionInfo>(*this, closure.func.typeInfo.c_str());
if (fd) fd->func = &closure.func;
}
else if (func.isNativeFunction()) {
auto ptr = func.asNativeFunction();
auto it = vm.nativeFuncTypeInfos.find(ptr);
if (it!=vm.nativeFuncTypeInfos.end()) funcInfo = make_unique<StringFunctionInfo>(*this, it->second.c_str());
}
if (funcInfo) {
shared_ptr<TypeInfo> argtypes[nArgs+1];
receiver->analyze(*this);
argtypes[0] = receiver->type;
for (int i=0; i<nArgs; i++) {
args[i]->analyze(*this);
argtypes[i+1] = args[i]->type;
}
return funcInfo->getReturnTypeInfo(nArgs+1, argtypes);
}
return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType (std::shared_ptr<Expression> receiver, const QToken& name, int nArgs, shared_ptr<Expression>* args, bool super, FuncOrDecl* fd) {
receiver->analyze(*this);
shared_ptr<TypeInfo> receiverType = receiver->type->resolve(*this);
if (auto cti = dynamic_pointer_cast<ComposedTypeInfo>(receiverType)) receiverType = cti->type;
if (auto clt = dynamic_pointer_cast<ClassTypeInfo>(receiverType)) {
auto cls = clt->type;
if (super) cls = cls->parent;
QV method = cls->findMethod(parser.vm.findMethodSymbol(string(name.start, name.length)));
return resolveCallType(receiver, method, nArgs, args, fd);
}
else if (auto cdt = dynamic_pointer_cast<ClassDeclTypeInfo>(receiverType)) {
shared_ptr<FunctionDeclaration> m;
if (!super) {
m = cdt->cls->findMethod(name, false);
if (!m) m = cdt->cls->findMethod(name, true);
}
if (!m) for (auto& parentToken: cdt->cls->parents) {
auto parentTI = make_shared<NamedTypeInfo>(parentToken) ->resolve(*this);
//println("RT=%s,%s, PT=%s,%s,%s", receiverType->toString(), typeid(*receiverType).name(), string(parentToken.start, parentToken.length), parentTI->toString(), typeid(*parentTI).name() );
if (cdt = dynamic_pointer_cast<ClassDeclTypeInfo>(parentTI)) {
m = cdt->cls->findMethod(name, false);
if (!m) m = cdt->cls->findMethod(name, true);
}
else if (auto clt = dynamic_pointer_cast<ClassTypeInfo>(parentTI)) {
QV method = clt->type->findMethod(parser.vm.findMethodSymbol(string(name.start, name.length)));
return resolveCallType(receiver, method, nArgs, args, fd);
}
if (m) break;
}
if (m) {
if (fd) fd->method = m.get();
return m->returnType;
}}
return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType   (std::shared_ptr<Expression> receiver, int nArgs, shared_ptr<Expression>* args, FuncOrDecl* fd) {
receiver->analyze(*this);
auto funcType = receiver->type;
if (auto cdt = dynamic_pointer_cast<ClassDeclTypeInfo>(funcType)) {
return cdt;
}
else if (auto ct = dynamic_pointer_cast<ComposedTypeInfo>(funcType)) {
int n = ct->countSubtypes();
if (n>0) return ct->subtypes[n -1];
}
return TypeInfo::MANY;
}

void TypeAnalyzer::report (shared_ptr<Expression> expr) {
typeInfo(expr->nearestToken(), "Expr type = %s", expr->type? expr->type->toString() : "<null>");
}
