#include "Compiler.hpp"
#include "TypeAnalyzer.hpp"
#include "TypeInfo.hpp"
#include "../vm/VM.hpp"
#include "../vm/Function.hpp"
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


LocalVariable::LocalVariable (const QToken& n, int s, bool ic): 
name(n), scope(s), type(TypeInfo::ANY), value(nullptr), hasUpvalues(false), isConst(ic) {}

ClassDeclaration* QCompiler::getCurClass (int* atLevel) {
if (atLevel) ++(*atLevel);
if (curClass) return curClass;
else if (parent) return parent->getCurClass(atLevel);
else return nullptr;
}

FunctionDeclaration* QCompiler::getCurMethod () {
if (curMethod) return curMethod;
else if (parent) return parent->getCurMethod();
else return nullptr;
}
void QCompiler::pushLoop () {
pushScope();
loops.emplace_back( curScope, writePosition() );
}

void QCompiler::popLoop () {
popScope();
Loop& loop = loops.back();
for (auto p: loop.jumpsToPatch) {
switch(p.first){
case Loop::CONDITION: patchJump(p.second, loop.condPos); break;
case Loop::END: patchJump(p.second, loop.endPos); break;
}}
while (curScope>loop.scope) popScope();
loops.pop_back();
}

void QCompiler::pushScope () {
curScope++;
}

void QCompiler::popScope () {
auto newEnd = remove_if(localVariables.begin(), localVariables.end(), [&](auto& x){ return x.scope>=curScope; });
int nVars = localVariables.end() -newEnd;
localVariables.erase(newEnd, localVariables.end());
curScope--;
if (nVars>0) writeOpArg<uint_local_index_t>(OP_POP_SCOPE, nVars);
}

int QCompiler::countLocalVariablesInScope (int scope) {
if (scope<0) scope = curScope;
return count_if(localVariables.begin(), localVariables.end(), [&](auto& x){ return x.scope>=scope; });
}

inline auto findLV (vector<LocalVariable>& localVariables, const QToken& name) {
return find_if_backward(localVariables.begin(), localVariables.end(), [&](auto& x){
return x.name.length==name.length && strncmp(name.start, x.name.start, name.length)==0;
});
}

int QCompiler::createLocalVariable (const QToken& name, bool isConst) {
auto it = findLV(localVariables, name);
if (it!=localVariables.end()) {
if (it->scope>=curScope) return ERR_ALREADY_EXIST;
else compileWarn(name, "Shadowig %s declared at line %d", it->name.str(), parser.getPositionOf(it->name.start).first);
}
if (localVariables.size() >= std::numeric_limits<uint_local_index_t>::max()) return ERR_TOO_MANY;
int n = localVariables.size();
localVariables.emplace_back(name, curScope, isConst);
return n;
}

int QCompiler::findLocalVariable (const QToken& name, bool forWrite) {
auto it = findLV(localVariables, name);
if (it==localVariables.end()) return ERR_NOT_FOUND;
else if (forWrite && it->isConst) return ERR_CONSTANT;
else return it - localVariables.begin();
}

int QCompiler::findUpvalue (const QToken& name, bool forWrite) {
if (!parent) return ERR_NOT_FOUND;
int slot = parent->findLocalVariable(name, forWrite);
if (slot>=0) {
parent->localVariables[slot].hasUpvalues=true;
int upslot = addUpvalue(slot, false);
if (upslot >= std::numeric_limits<uint_upvalue_index_t>::max()) return ERR_TOO_MANY;
return upslot;
}
else if (slot!=ERR_NOT_FOUND) return slot;
slot = parent->findUpvalue(name, forWrite);
if (slot>=0) {
int upslot = addUpvalue(slot, true);
if (upslot >= std::numeric_limits<uint_upvalue_index_t>::max()) return ERR_TOO_MANY;
return upslot;
}
return slot;
}

int QCompiler::createGlobalVariable (const QToken& name, bool isConst) {
int slot = findGlobalVariable(name, true);
if (slot==ERR_NOT_FOUND) slot = vm.bindGlobal(name.str(), QV::UNDEFINED, isConst);
return slot;
}

int QCompiler::findGlobalVariable (const QToken& name, bool forWrite) {
return vm.findGlobalSymbol(name.str(), forWrite);
}

int QCompiler::findGlobalVariable (const string& name, bool forWrite) {
return vm.findGlobalSymbol(name, forWrite);
}

FindVarResult QCompiler::findVariable (const QToken& name, bool forWrite) {
int slot = findLocalVariable(name, forWrite);
if (slot!=ERR_NOT_FOUND) return { slot, VarKind::Local };
slot = findUpvalue(name, forWrite);
if (slot!=ERR_NOT_FOUND) return { slot, VarKind::Upvalue };
slot = findGlobalVariable(name, forWrite);
if (slot==ERR_NOT_FOUND) {
if (parser.vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT) return { createLocalVariable(name), VarKind::Local };
else if (parser.vm.getOption(QVM::Option::VAR_DECL_MODE)==QVM::Option::VAR_IMPLICIT_GLOBAL) slot = createGlobalVariable(name);
}
return { slot, VarKind::Global };
}

FindVarResult QCompiler::createVariable (const QToken& name, bitmask<VarFlag> flags) {
bool isConst = static_cast<bool>(flags & VarFlag::Const), isGlobal = static_cast<bool>(flags & VarFlag::Global);
int slot = ERR_NOT_FOUND;
if (isGlobal) slot = createGlobalVariable(name, isConst);
else slot = createLocalVariable(name, isConst);
if (slot==ERR_ALREADY_EXIST) compileError(name, "Variable already exist");
else if (slot==ERR_TOO_MANY) compileError(name, "Too many variables");
return { slot, isGlobal? VarKind::Global : VarKind::Local };
}

int QCompiler::addUpvalue (int slot, bool upperUpvalue) {
auto it = find_if(upvalues.begin(), upvalues.end(), [&](auto& x){ return x.slot==slot && x.upperUpvalue==upperUpvalue; });
if (it!=upvalues.end()) return it - upvalues.begin();
int i = upvalues.size();
upvalues.push_back({ static_cast<uint_local_index_t>(slot), upperUpvalue });
return i;
}

int QCompiler::findConstant (const QV& value) {
auto it = find_if(constants.begin(), constants.end(), [&](const auto& v){
return value.i == v.i;
});
if (it!=constants.end()) return it - constants.begin();
else {
int n = constants.size();
constants.push_back(value);
return n;
}}

int QVM::findMethodSymbol (const string& name) {
auto it = find(methodSymbols.begin(), methodSymbols.end(), name);
if (it!=methodSymbols.end()) return it - methodSymbols.begin();
else {
int n = methodSymbols.size();
methodSymbols.push_back(name);
return n;
}}
