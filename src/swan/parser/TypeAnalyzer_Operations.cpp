#include "TypeAnalyzer.hpp"
#include "TypeInfo.hpp"
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
if (t1) return t1->merge(t2, *this);
else if (t2) return t2->merge(t1, *this);
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
auto oldType = e.type;
e.type = type;
if (isSameType(type, oldType)) return false;
//typeInfo(e.nearestToken(), "Type changed from %s to %s in %s", oldType?oldType->toString():"<null>", type?type->toString():"<null>", typeid(e).name());
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

