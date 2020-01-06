#include "NativeFuncTypeInfo.hpp"
#include "TypeInfo.hpp"
#include "../vm/VM.hpp"
using namespace std;

void nativeFuncTypeInfoDeleter (NativeFuncTypeInfo* nfti) {
delete nfti;
}

std::shared_ptr<struct TypeInfo> NativeFuncTypeInfo::getArgType (int n) { 
return std::make_shared<ClassTypeInfo>(argtypes[n]); 
}

std::shared_ptr<struct TypeInfo> NativeFuncTypeInfo::getReturnType () { 
return std::make_shared<ClassTypeInfo>(returnType); 
}

std::shared_ptr<struct TypeInfo> NativeFuncTypeInfo::getReturnType (QVM& vm, int nArgs, std::shared_ptr<TypeInfo>* args) {
if (rtFromArgs) return rtFromArgs(vm, nArgs, args);
else return getReturnType();
}

std::shared_ptr<TypeInfo> NativeFuncTypeInfo::getFunctionType (QVM& vm) {
auto funcTI = make_shared<ClassTypeInfo>(vm.functionClass);
auto argtypes = make_unique<shared_ptr<TypeInfo>[]>(nArgs+1);
for (int i=0; i<nArgs; i++) argtypes[i] = getArgType(i);
argtypes[nArgs] = getReturnType();
return make_shared<ComposedTypeInfo>(funcTI, nArgs+1, std::move(argtypes));
}


void QVM::bindGlobal (const std::string& name, QNativeFunction value, const std::initializer_list<QClass*>& argtypes, QClass* returnType) {
bindGlobal(name, value);
uintptr_t key = reinterpret_cast<uintptr_t>(value);
auto typeInfo = NativeFuncTypeInfo::create(argtypes, returnType);
nativeFuncTypeInfos[key] = unique_ptr<NativeFuncTypeInfo, NativeFuncTypeInfoDeleter>(typeInfo);
}
