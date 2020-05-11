#include "FunctionInfo.hpp"
#include "TypeInfo.hpp"
#include "TypeAnalyzer.hpp"
#include "../vm/VM.hpp"
using namespace std;

shared_ptr<TypeInfo> getFunctionTypeInfo (FunctionInfo& fti, struct QVM& vm, int nPassedArgs, shared_ptr<TypeInfo>* passedArgs) {
int nArgs = fti.getArgCount();
auto funcTI = make_shared<ClassTypeInfo>(vm.functionClass);
auto rt = fti.getReturnTypeInfo(nPassedArgs, passedArgs);
vector<shared_ptr<TypeInfo>> subtypes;
subtypes.resize(nArgs+1);
for (int i=0; i<nArgs; i++) {
auto ati = fti.getArgTypeInfo(i);
subtypes[i] = ati?ati:TypeInfo::MANY;
}
subtypes[nArgs] = rt?rt:TypeInfo::MANY;
return make_shared<ComposedTypeInfo>(funcTI, subtypes);
}

StringFunctionInfo::StringFunctionInfo (TypeAnalyzer& ta, const char* typeInfoStr): 
vm(ta.vm), flags(0), fieldIndex(-1), types(), nArgs(0), retArg(-1), retCompArg(-1)  
{ build(ta, typeInfoStr); }

void StringFunctionInfo::build (TypeAnalyzer&  ta, const char* str) {
types.clear();
while(str&&*str){
auto tp = readNextTypeInfo(ta, str);
if (tp) types.push_back(tp);
}
nArgs = types.size() -1;
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
case 'B': case 'b': return make_shared<ClassTypeInfo>(ta.vm.boolClass);
case 'E': case 'e': return make_shared<ClassTypeInfo>(ta.vm.setClass);
case 'F': case 'f': return make_shared<ClassTypeInfo>(ta.vm.functionClass);
case 'I': case 'i': return make_shared<ClassTypeInfo>(ta.vm.iteratorClass);
case 'J': case 'j': return make_shared<ClassTypeInfo>(ta.vm.iterableClass);
case 'L': case 'l': return make_shared<ClassTypeInfo>(ta.vm.listClass);
case 'M': case 'm': return make_shared<ClassTypeInfo>(ta.vm.mapClass);
case 'N': case 'n': return make_shared<ClassTypeInfo>(ta.vm.numClass);
case 'O': case 'o': return make_shared<ClassTypeInfo>(ta.vm.objectClass);
case 'S': case 's': return make_shared<ClassTypeInfo>(ta.vm.stringClass);
case 'T': case 't': return make_shared<ClassTypeInfo>(ta.vm.tupleClass);
case 'U': case 'u': return make_shared<ClassTypeInfo>(ta.vm.undefinedClass);
case '@':
retArg = strtoul(str, const_cast<char**>(&str), 10);
continue;
case '_':
fieldIndex = strtoul(str, const_cast<char**>(&str), 10);
continue;
case '%':
retCompArg = strtoul(str, const_cast<char**>(&str), 10);
continue;
case 'Q': case '$': {
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
return make_shared<ComposedTypeInfo>(type, subtypes);
}
default: return TypeInfo::MANY;
}}
return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getReturnTypeInfo (int na,  std::shared_ptr<TypeInfo>* ptr) {
if (retCompArg>=0) println("retCompArg=%d, na=%d, ptr=%s(%s)", retCompArg, na, ptr&&na>0?(*ptr)->toString():"", ptr&&na>0?typeid(**ptr).name():"<null>");
if (retCompArg>=0 && ptr && na>0) {
auto cti = dynamic_pointer_cast<ComposedTypeInfo>(*ptr);
if (cti && cti->countSubtypes()>retCompArg) return cti->subtypes[retCompArg];
}
if (retArg>=0 && ptr && na>=retArg) return ptr[retArg];
else if (types.size()) return types[nArgs];
else return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getArgTypeInfo (int n) {
if (types.size() && n>=0 && n<nArgs) return types[n];
else return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getFunctionTypeInfo (int na, std::shared_ptr<TypeInfo>* ptr) {
return ::getFunctionTypeInfo(*this, vm, na, ptr);
}
