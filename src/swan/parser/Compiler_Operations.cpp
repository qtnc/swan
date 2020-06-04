#include "Compiler.hpp"
#include "TypeAnalyzer.hpp"
#include "TypeInfo.hpp"
#include "../vm/VM.hpp"
#include "../vm/Function.hpp"
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

int QCompiler::findLocalVariable (const QToken& name, int flags, LocalVariable** ptr) {
bool createNew = flags&LV_NEW, isConst = flags&LV_CONST;
auto rvar = find_if(localVariables.rbegin(), localVariables.rend(), [&](auto& x){
return x.name.length==name.length && strncmp(name.start, x.name.start, name.length)==0;
});
if (rvar==localVariables.rend() && !createNew && parser.vm.getOption(QVM::Option::VAR_DECL_MODE)!=QVM::Option::VAR_IMPLICIT) return -1;
else if (rvar==localVariables.rend() || (createNew && rvar->scope<curScope)) {
if (createNew && rvar!=localVariables.rend()) compileWarn(name, "Shadowig %s declared at line %d", string(rvar->name.start, rvar->name.length), parser.getPositionOf(rvar->name.start).first);
if (localVariables.size() >= std::numeric_limits<uint_local_index_t>::max()) compileError(name, "Too many local variables");
int n = localVariables.size();
localVariables.emplace_back(name, curScope, isConst);
if (ptr) *ptr = &localVariables[n];
return n;
}
else if (!createNew)  {
if (rvar->isConst && isConst) return LV_ERR_CONST;
int n = rvar.base() -localVariables.begin() -1;
if (ptr) *ptr = &localVariables[n];
return n;
}
else compileError(name, ("Variable already defined"));
return -1;
}

int QCompiler::findUpvalue (const QToken& token, int flags, LocalVariable** ptr) {
if (!parent) return -1;
int slot = parent->findLocalVariable(token, flags, ptr);
if (slot>=0) {
parent->localVariables[slot].hasUpvalues=true;
int upslot = addUpvalue(slot, false);
if (upslot >= std::numeric_limits<uint_upvalue_index_t>::max()) compileError(token, "Too many upvalues");
return upslot;
}
else if (slot==LV_ERR_CONST) return slot;
slot = parent->findUpvalue(token, flags, ptr);
if (slot>=0) return addUpvalue(slot, true);
if (slot >= std::numeric_limits<uint_upvalue_index_t>::max()) compileError(token, "Too many upvalues");
return slot;
}

int QCompiler::addUpvalue (int slot, bool upperUpvalue) {
auto it = find_if(upvalues.begin(), upvalues.end(), [&](auto& x){ return x.slot==slot && x.upperUpvalue==upperUpvalue; });
if (it!=upvalues.end()) return it - upvalues.begin();
int i = upvalues.size();
upvalues.push_back({ static_cast<uint_local_index_t>(slot), upperUpvalue });
return i;
}

int QCompiler::findGlobalVariable (const QToken& name, int flags, LocalVariable** ptr) {
bool isConst;
int slot = parser.vm.findGlobalSymbol(string(name.start, name.length), flags, &isConst);
if (ptr && slot>=0) {
QCompiler* p = this; while(p->parent) p=p->parent;
auto it = find_if(p->globalVariables.begin(), p->globalVariables.end(), [&](auto& x){ return x.name.length==name.length && strncmp(name.start, x.name.start, name.length)==0; });
if (it!=p->globalVariables.end()) *ptr = &*it;
else {
p->globalVariables.emplace_back(name, 0, isConst);
*ptr = &(p->globalVariables.back());
//(*ptr)->type = resolveValueType(vm.globalVariables[slot]);
}}
return slot;
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

int QVM::findGlobalSymbol (const string& name, int flags, bool* isConst) {
auto it = globalSymbols.find(name);
if (it!=globalSymbols.end()) {
auto& gv = it->second;
if (isConst) *isConst = gv.isConst;
if (flags&LV_NEW && varDeclMode!=Option::VAR_IMPLICIT_GLOBAL) return LV_ERR_ALREADY_EXIST;
else if ((flags&LV_FOR_WRITE) && gv.isConst) return LV_ERR_CONST;
return gv.index;
}
else if (!(flags&LV_NEW) && varDeclMode!=Option::VAR_IMPLICIT_GLOBAL) return -1;
else {
int n = globalSymbols.size();
globalSymbols[name] = { n, flags&LV_CONST };
globalVariables.push_back(QV::UNDEFINED);
return n;
}}

bool QCompiler::isCallInlinable (QFunction& func) {
return func.bytecodeEnd - func.bytecode <= 12
&& !func.flags.vararg
&& func.upvaluesEnd - func.upvalues == 0;
}

bool QCompiler::isCallInlinable (std::shared_ptr<TypeInfo> type, struct QFunction& func) {
return type && (type->isExact() || !func.flags.overridden) && isCallInlinable(func);
}

