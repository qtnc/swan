#include "TypeChecker.hpp"
#include "TypeInfo.hpp"
#include "NativeFuncTypeInfo.hpp"
#include "../vm/VM.hpp"
using namespace std;

TypedVariable::TypedVariable (const QToken& n, int s):
name(n), scope(s), type(TypeInfo::ANY),
classDecl(nullptr), funcDecl(nullptr)
{}

ClassDeclaration* TypeChecker::getCurClass (int* atLevel) {
if (atLevel) ++(*atLevel);
if (curClass) return curClass;
else if (parent) return parent->getCurClass(atLevel);
else return nullptr;
}

FunctionDeclaration* TypeChecker::getCurMethod () {
if (curMethod) return curMethod;
else if (parent) return parent->getCurMethod();
else return nullptr;
}

void TypeChecker::pushScope () {
curScope++;
}

void TypeChecker::popScope () {
auto newEnd = remove_if(variables.begin(), variables.end(), [&](auto& x){ return x.scope>=curScope; });
int nVars = variables.end() -newEnd;
variables.erase(newEnd, variables.end());
curScope--;
}

TypedVariable* TypeChecker::findVariable (const QToken& name, int flags) {
if (flags&LV_NEW) {
variables.emplace_back(name, curScope);
return &variables.back();
}
auto rvar = find_if(variables.rbegin(), variables.rend(), [&](auto& x){
return x.name.length==name.length && strncmp(name.start, x.name.start, name.length)==0;
});
if (rvar!=variables.rend()) return &*rvar;
else if (parent) return parent->findVariable(name, flags);
int index = vm.findGlobalSymbol(string(name.start, name.length), LV_FOR_READ);
if (index>=0) {
findVariable(name, LV_FOR_WRITE | LV_NEW) ->type = getType(vm.globalVariables[index]);
}
return nullptr;
}

std::shared_ptr<TypeInfo> TypeChecker::getType (QV value) {
if (value.isClosure()) {
//todo
}
else if (value.isNativeFunction()) {
intptr_t ptr = (value.i &~QV_TAGMASK);
auto it = vm.nativeFuncTypeInfos.find(ptr);
if (it!=vm.nativeFuncTypeInfos.end()) return it->second->getFunctionType(vm);
}
return make_shared<ClassTypeInfo>(&(value.getClass(vm)));
}
