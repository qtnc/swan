#include "TypeAnalyzer.hpp"
#include "TypeInfo.hpp"
#include "FunctionInfo.hpp"
#include "Expression.hpp"
#include "../vm/VM.hpp"
#include<boost/algorithm/find_backward.hpp>
using namespace std;
using namespace boost::algorithm;


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
if (type==TypeInfo::ANY && e.type && e.type!=TypeInfo::ANY) {
typeWarn(e.nearestToken(), "Overwrite with any type, was %s", e.type?e.type->toString():"<null>");
}
if (type==TypeInfo::MANY && e.type && e.type!=TypeInfo::ANY && e.type!=TypeInfo::MANY) {
typeWarn(e.nearestToken(), "Overwrite with many type, was %s", e.type?e.type->toString():"<null>");
}
auto oldType = e.type;
e.type = type? type ->resolve(*this) : nullptr;
if (isSameType(type, oldType)) return false;
//if (oldType!=TypeInfo::ANY) typeInfo(e.nearestToken(), "Type changed from %s to %s in %s", oldType?oldType->toString():"<null>", type?type->toString():"<null>", typeid(e).name());
return true;
}

inline auto findLV (vector<AnalyzedVariable>& localVariables, const QToken& name) {
return find_if_backward(localVariables.begin(), localVariables.end(), [&](auto& x){
return x.name.length==name.length && strncmp(name.start, x.name.start, name.length)==0;
});
}

AnalyzedVariable* TypeAnalyzer::createVariable (const QToken& name) {
variables.emplace_back(name, curScope);
return &variables.back();
}

AnalyzedVariable* TypeAnalyzer::findVariable (const QToken& name) {
auto it = findLV(variables, name);
if (it!=variables.end()) return &*it;
else if (parent) return parent->findVariable(name);
else {
int index = vm.findGlobalSymbol(string(name.start, name.length));
if (index>=0) {
auto var = createVariable(name);
var->type = resolveValueType(vm.globalVariables[index]);
QToken tkcst = { T_NAME, nullptr, 0, vm.globalVariables[index] };
var->value = make_shared<ConstantExpression>(tkcst);
return var;
}}
return nullptr;
}

shared_ptr<FunctionInfo> TypeAnalyzer::resolveFunctionInfo (const char* str) {
return make_shared<StringFunctionInfo>(*this, str);
}

shared_ptr<FunctionInfo> FuncOrDecl::getFunctionInfo (TypeAnalyzer& ta) {
shared_ptr<FunctionInfo> fi = nullptr;
if (method) fi = static_pointer_cast<FunctionInfo>(static_pointer_cast<FunctionDeclaration>(method->shared_this()));
else if (func) fi = ta.resolveFunctionInfo(func->typeInfo.c_str());
else if (nativeFunc) {
auto it = ta.vm.nativeFuncTypeInfos.find(nativeFunc);
if (it!=ta.vm.nativeFuncTypeInfos.end()) fi = ta.resolveFunctionInfo(it->second.c_str());
}
return fi;
}

shared_ptr<FunctionInfo> TypeAnalyzer::resolveFunctionInfo  (QV value, FuncOrDecl* fd) {
shared_ptr<FunctionInfo> fi = nullptr;
if (value.isClosure()) {
QClosure& closure = *value.asObject<QClosure>();
if (!closure.func.typeInfo.empty()) fi = resolveFunctionInfo(closure.func.typeInfo.c_str());
if (fd) fd->func = &closure.func;
}
else if (value.isNativeFunction()) {
auto nativeFunc = value.asNativeFunction();
auto it = vm.nativeFuncTypeInfos.find(nativeFunc);
if (it!=vm.nativeFuncTypeInfos.end()) fi = resolveFunctionInfo(it->second.c_str());
if (fd) fd->nativeFunc = nativeFunc;
}
//other cases ?
return fi;
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveValueType (QV value) {
auto fi = resolveFunctionInfo(value);
if (fi) return fi->getFunctionTypeInfo();
return make_shared<ClassTypeInfo>(&value.getClass(vm));
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType (std::shared_ptr<Expression> receiver, std::shared_ptr<FunctionInfo> fi, int nArgs, shared_ptr<Expression>* args) {
receiver->analyze(*this);
shared_ptr<TypeInfo> argtypes[nArgs+1];
argtypes[0] = receiver->type;
for (int i=0; i<nArgs; i++) {
args[i]->analyze(*this);
argtypes[i+1] = args[i]->type;
}
return fi->getReturnTypeInfo(nArgs+1, argtypes);
}
 
std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType   (std::shared_ptr<Expression> receiver, QV value, int nArgs, shared_ptr<Expression>* args, FuncOrDecl* fd) {
auto fi = resolveFunctionInfo(value, fd);
if (fi) return resolveCallType(receiver, fi, nArgs, args);
else return TypeInfo::ANY;
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType (std::shared_ptr<Expression> receiver, std::shared_ptr<ClassDeclTypeInfo> cdti, const QToken& name, int nArgs, std::shared_ptr<Expression>* args, bitmask<CallFlag> flags, FuncOrDecl* pfd) {
shared_ptr<TypeInfo> ti = TypeInfo::ANY;
shared_ptr<FunctionDeclaration> fd = nullptr;
if (!(flags & CallFlag::Super)) {
fd = cdti->cls->findMethod(name, static_cast<bool>( cdti->flags & TypeInfoFlag::Static));
}
if (fd) {
ti = resolveCallType(receiver, fd, nArgs, args);
if (pfd) pfd->method = fd.get();
}
else if (!(cdti->flags & TypeInfoFlag::Static)) {
for (auto& parentToken: cdti->cls->parents) {
auto parent = make_shared<NamedTypeInfo>(parentToken) ->resolve(*this);
if (auto pcdti = dynamic_pointer_cast<ClassDeclTypeInfo>(parent)) ti = resolveCallType(receiver, pcdti, name, nArgs, args, flags &~CallFlag::Super, pfd);
else if (auto cti = dynamic_pointer_cast<ClassTypeInfo>(parent)) ti = resolveCallType(receiver, cti, name, nArgs, args, flags &~CallFlag::Super, pfd);
if (ti) break;
}}
return ti;
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType (std::shared_ptr<Expression> receiver, std::shared_ptr<ClassTypeInfo> cti, const QToken& name, int nArgs, std::shared_ptr<Expression>* args, bitmask<CallFlag> flags, FuncOrDecl* fd) {
auto cls = cti->type;
if (flags & CallFlag::Super) cls = cls->parent;
if (cti->flags & TypeInfoFlag::Static) cls = cls->type;
QV method = cls->findMethod(vm.findMethodSymbol(string(name.start, name.length)));
return resolveCallType(receiver, method, nArgs, args, fd);
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType (std::shared_ptr<Expression> receiver, const QToken& name, int nArgs, shared_ptr<Expression>* args, bitmask<CallFlag> flags, FuncOrDecl* fd) {
receiver->analyze(*this);
shared_ptr<TypeInfo> receiverType = receiver->type->resolve(*this);
if (auto cti = dynamic_pointer_cast<ComposedTypeInfo>(receiverType)) receiverType = cti->type;
if (auto cti = dynamic_pointer_cast<ClassTypeInfo>(receiverType)) return resolveCallType(receiver, cti, name, nArgs, args, flags, fd);
else if (auto cdti = dynamic_pointer_cast<ClassDeclTypeInfo>(receiverType)) return resolveCallType(receiver, cdti, name, nArgs, args, flags, fd);
else return TypeInfo::ANY;
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveCallType   (std::shared_ptr<Expression> receiver, int nArgs, shared_ptr<Expression>* args, bitmask<CallFlag> flags, FuncOrDecl* fd) {
receiver->analyze(*this);
auto funcType = receiver->type;
if (auto cdt = dynamic_pointer_cast<ClassDeclTypeInfo>(funcType)) {
if (cdt->flags & TypeInfoFlag::Static) {
return make_shared<ClassDeclTypeInfo>(cdt->cls, TypeInfoFlag::Exact);
}}
else if (auto cti = dynamic_pointer_cast<ComposedTypeInfo>(funcType)) {
int n = cti->countSubtypes();
if (n>0) return cti->subtypes[n -1];
}
//other cases
return TypeInfo::ANY;
}

std::shared_ptr<TypeInfo> TypeAnalyzer::resolveSubscriptType   (std::shared_ptr<Expression> receiver, int nArgs, shared_ptr<Expression>* args, bitmask<CallFlag> flags, FuncOrDecl* fd) {
if (receiver->type && nArgs==1 && args[0]->type) {
auto cti = dynamic_pointer_cast<ComposedTypeInfo>(receiver->type);
if (cti && cti->type && cti->type->isTuple()) {
auto cst = dynamic_pointer_cast<ConstantExpression>(args[0]);
if (cst && cst->token.type==T_NUM && cst->token.value.d>=0 && cst->token.value.d<cti->subtypes.size() ) {
return cti->subtypes[static_cast<size_t>(cst->token.value.d)];
}
else if (args[0]->type && args[0]->type->isRange()) {
//todo: tuple range
}}
else if (cti && cti->type) {
if (any_of(args, args+nArgs, [&](auto& t){ return t->type && t->type->isRange(); })) return cti;
else return cti->subtypes.back();
}}
QToken subscriptToken = { T_NAME, "[]", 2, QV::UNDEFINED };
return resolveCallType(receiver, subscriptToken, nArgs, args, flags, fd);
}

void TypeAnalyzer::report (shared_ptr<Expression> expr) {
typeInfo(expr->nearestToken(), "Expr type = %s", expr->type? expr->type->toString() : "<null>");
}


