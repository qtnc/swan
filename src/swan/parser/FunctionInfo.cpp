#include "FunctionInfo.hpp"
#include "TypeInfo.hpp"
#include "TypeAnalyzer.hpp"
#include "../vm/VM.hpp"
using namespace std;

std::shared_ptr<TypeInfo> getFunctionTypeInfo (FunctionInfo& fti, struct QVM& vm, int nPassedArgs, std::shared_ptr<TypeInfo>* passedArgs) {
int nArgs = fti.getArgCount();
auto funcTI = make_shared<ClassTypeInfo>(vm.functionClass);
auto rt = fti.getReturnTypeInfo(nPassedArgs, passedArgs);
auto subtypes = make_unique<shared_ptr<TypeInfo>[]>(nArgs+1);
for (int i=0; i<nArgs; i++) {
auto ati = fti.getArgTypeInfo(i);
subtypes[i] = ati?ati:TypeInfo::MANY;
}
subtypes[nArgs] = rt?rt:TypeInfo::MANY;
return make_shared<ComposedTypeInfo>(funcTI, nArgs+1, std::move(subtypes));
}

StringFunctionInfo::StringFunctionInfo (TypeAnalyzer& ta, const char* typeInfoStr): 
vm(ta.vm), flags(0), fieldIndex(-1), types(nullptr), nArgs(0), retArg(-1)  
{ build(ta, typeInfoStr); }

void StringFunctionInfo::build (TypeAnalyzer&  ta, const char* str) {
vector<shared_ptr<TypeInfo>> argtypes;
while(str&&*str){
auto tp = readNextTypeInfo(ta, str);
if (tp) argtypes.push_back(tp);
}
types = make_unique<shared_ptr<TypeInfo>[]>(argtypes.size());
std::copy(argtypes.begin(), argtypes.end(), &types[0]);
nArgs = argtypes.size() -1;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::readNextTypeInfo (TypeAnalyzer& ta, const char*& str) {
while(str&&*str){
switch(*str++){
case ':': case ' ': case ',': case ';': continue;
case '+': flags |= FD_VARARG; continue;
case '=': flags |= FD_SETTER; continue;
case '.': flags |= FD_GETTER; continue;
case '>': flags |= FD_METHOD; continue;
case '*': return TypeInfo::ANY;
case '#': return TypeInfo::MANY;
case 'B': return make_shared<ClassTypeInfo>(ta.vm.boolClass);
case 'N': return make_shared<ClassTypeInfo>(ta.vm.numClass);
case 'S': return make_shared<ClassTypeInfo>(ta.vm.stringClass);
case 'U': return make_shared<ClassTypeInfo>(ta.vm.undefinedClass);
case 'L': return make_shared<ClassTypeInfo>(ta.vm.listClass);
case 'E': return make_shared<ClassTypeInfo>(ta.vm.setClass);
case 'T': return make_shared<ClassTypeInfo>(ta.vm.tupleClass);
case 'M': return make_shared<ClassTypeInfo>(ta.vm.mapClass);
case 'F': return make_shared<ClassTypeInfo>(ta.vm.functionClass);
case 'I': return make_shared<ClassTypeInfo>(ta.vm.iteratorClass);
case 'A': return make_shared<ClassTypeInfo>(ta.vm.iterableClass);
case '@': {
retArg = strtoul(str, const_cast<char**>(&str), 10);
return TypeInfo::ANY;
}
case '_':
fieldIndex = strtoul(str, const_cast<char**>(&str), 10);
continue;
case 'Q': {
const char* b = str;
while(str&&*str&&*str!=';') ++str;
QToken tok = { T_NAME, b, static_cast<size_t>(str-b), QV::UNDEFINED };
return make_shared<NamedTypeInfo>(tok)->resolve(ta);
}
case 'C': {
auto type = readNextTypeInfo(ta, str);
vector<shared_ptr<TypeInfo>> subtypes;
if (*str=='<') {
str++;
while(str&&*str&&*str!='>') {
subtypes.push_back(readNextTypeInfo(ta, str));
}
str++;
}
auto uptr = make_unique<shared_ptr<TypeInfo>[]>(subtypes.size());
std::copy(subtypes.begin(), subtypes.end(), &uptr[0]);
return make_shared<ComposedTypeInfo>(type, subtypes.size(), std::move(uptr));
}
default: return TypeInfo::MANY;
}}
return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getReturnTypeInfo (int na,  std::shared_ptr<TypeInfo>* ptr) {
if (retArg>=0 && ptr && na>=retArg) return ptr[retArg];
else if (types) return types[nArgs];
else return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getArgTypeInfo (int n) {
if (types && n>=0 && n<nArgs) return types[n];
else return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getFunctionTypeInfo (int na, std::shared_ptr<TypeInfo>* ptr) {
return ::getFunctionTypeInfo(*this, vm, na, ptr);
}
