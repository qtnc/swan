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
auto ati = fti.getArgTypeInfo(i, nPassedArgs, passedArgs);
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
case 'R': case 'r': return make_shared<ClassTypeInfo>(ta.vm.rangeClass);
case 'S': case 's': return make_shared<ClassTypeInfo>(ta.vm.stringClass);
case 'T': case 't': return make_shared<ClassTypeInfo>(ta.vm.tupleClass);
case 'U': case 'u': return make_shared<ClassTypeInfo>(ta.vm.undefinedClass);
case '@': return make_shared<SubindexTypeInfo>(0x100 + strtoul(str, const_cast<char**>(&str), 10));
case '%': return make_shared<SubindexTypeInfo>(strtoul(str, const_cast<char**>(&str), 10));
case '_':
fieldIndex = strtoul(str, const_cast<char**>(&str), 10);
continue;
case 'Q': case 'q': case '$': {
const char* b = str;
while(str&&*str&&*str!=';') ++str;
QToken tok = { T_NAME, b, static_cast<size_t>(str-b), QV::UNDEFINED };
return make_shared<NamedTypeInfo>(tok)->resolve(ta);
}
case 'C': case 'c': {
auto type = readNextTypeInfo(ta, str);
auto count = strtoul(str, const_cast<char**>(&str), 10);
vector<shared_ptr<TypeInfo>> subtypes;
subtypes.reserve(count);
for (int i=0; i<count; i++)  subtypes.push_back(readNextTypeInfo(ta, str));
return make_shared<ComposedTypeInfo>(type, subtypes);
}
default: return TypeInfo::MANY;
}}
return TypeInfo::MANY;
}

shared_ptr<TypeInfo> handleSubindex (shared_ptr<TypeInfo> type, int nPassedArgs, shared_ptr<TypeInfo>* passedArgs) {
if (auto itp = dynamic_pointer_cast<SubindexTypeInfo>(type)) {
if (itp->index >= 0x100 && nPassedArgs>itp->index -0x100) return passedArgs[itp->index -0x100];
else if (itp->index < 0x100 && nPassedArgs>0) {
if (auto cti = dynamic_pointer_cast<ComposedTypeInfo>(*passedArgs)) {
if (cti->countSubtypes()>itp->index) return cti->subtypes[itp->index];
}
}
return TypeInfo::MANY;
}
else if (auto cti = dynamic_pointer_cast<ComposedTypeInfo>(type)) {
cti->type = handleSubindex(cti->type, nPassedArgs, passedArgs);
for (auto& subtype: cti->subtypes) subtype = handleSubindex(subtype, nPassedArgs, passedArgs);
}
return type;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getReturnTypeInfo (int nPassedArgs,  std::shared_ptr<TypeInfo>* passedArgs) {
//print("Return type of: ");
//for (auto& t: types) print("%s, ", t?t->toString():"<null>");
//println("");
if (types.size()) return handleSubindex(types[nArgs], nPassedArgs, passedArgs);
else return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getArgTypeInfo (int n, int nPassedArgs, shared_ptr<TypeInfo>* passedArgs) {
if (types.size() && n>=0 && n<nArgs) return handleSubindex(types[n], nPassedArgs, passedArgs);
else return TypeInfo::MANY;
}

std::shared_ptr<TypeInfo> StringFunctionInfo::getFunctionTypeInfo (int nPassedArgs, std::shared_ptr<TypeInfo>* passedArgs) {
return ::getFunctionTypeInfo(*this, vm, nPassedArgs, passedArgs);
}
