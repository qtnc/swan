#include "Parser.hpp"
#include "Compiler.hpp"
#include "../vm/ExtraAlgorithms.hpp"
#include "../vm/VM.hpp"
#include "../vm/Upvalue.hpp"
#include "../vm/NatSort.hpp"
#include "../../include/cpprintf.hpp"
#include<cmath>
#include<cstdlib>
#include<limits>
#include<memory>
#include<algorithm>
#include<unordered_map>
#include<unordered_set>
using namespace std;

static const char* OPCODE_NAMES[] = {
#define OP(name, stackEffect, nArgs, argFormat) #name
#include "../vm/OpCodes.hpp"
#undef OP
, nullptr
};

OpCodeInfo OPCODE_INFO[] = {
#define OP(name, stackEffect, szArgs, argFormat) { stackEffect, szArgs, argFormat }
#include "../vm/OpCodes.hpp"
#undef OP
};

const uint8_t* printOpCode (const uint8_t* bc, int offset, const QFunction& func, std::ostream& out) {
QVM& vm = func.type->vm;
int nArgs=0, argFormat = OPCODE_INFO[*bc].argFormat;
print(out, "%s", OPCODE_NAMES[*bc]);
bc++;
if (argFormat>=0x1000) nArgs=4;
else if (argFormat>=0x100) nArgs=3;
else if (argFormat>=0x10) nArgs=2;
else if (argFormat>1) nArgs=1;
for (int i=0; i<nArgs; i++) {
int arglen = argFormat&0x0F;
argFormat>>=4;
switch(arglen){
case 1:
print(out, ", %d", static_cast<int>(*bc));
bc++;
break;
case 2:
print(out, ", %d", *reinterpret_cast<const uint16_t*>(bc));
bc+=2;
break;
case 4:
print(out, ", %d", *reinterpret_cast<const uint32_t*>(bc));
bc+=4;
break;
case 8:
print(out, ", %d", *reinterpret_cast<const uint64_t*>(bc));
bc+=8;
break;
case 0x7: {
auto x = *reinterpret_cast<const int2x4_t*>(bc);
print(out, ", %d, %d", x.first, x.second);
bc++;
}break;
case 0xA: {
auto symbol = *reinterpret_cast<const uint_method_symbol_t*>(bc);
print(out, ", %d (%s)", symbol, vm.methodSymbols[symbol]);
bc+=sizeof(symbol);
}break;
case 0xB: {
auto index = *reinterpret_cast<const uint_global_symbol_t*>(bc);
auto& name = std::find_if(vm.globalSymbols.begin(), vm.globalSymbols.end(), [&](auto& p){ return p.second.index==index; }) ->first;
print(out, ", %d (%s)", index, name);
bc+=sizeof(index);
}break;
case 0xC: {
auto index = *reinterpret_cast<const uint_constant_index_t*>(bc);
print(out, ", %d (%s)", index, func.constants[index].print() );
bc+=sizeof(index);
}break;
}}
println(out, "\t\t[%+d]", offset);
return bc;
}

/*
void QCompiler::dump () {
if (!parent) {
println("\nOpcode list: ");
for (int i=0; OPCODE_NAMES[i]; i++) {
println("%d (%#0$2X). %s", i, i, OPCODE_NAMES[i]);
}
println("");

println("\n%d method symbols:", vm.methodSymbols.size());
for (int i=0, n=vm.methodSymbols.size(); i<n; i++) println("%d (%<#0$4X). %s", i, vm.methodSymbols[i]);
println("\n%d global symbols:", vm.globalSymbols.size());
//for (int i=0, n=vm.globalSymbols.size(); i<n; i++) println("%d (%<#0$4X). %s", i, vm.globalSymbols[i]);
println("\n%d constants:", constants.size());
for (int i=0, n=constants.size(); i<n; i++) println("%d (%<#0$4X). %s", i, constants[i].print());
}
else print("\nBytecode of method:");

string bcs = out.str();
println("\nBytecode length: %d bytes", bcs.length());
for (const uint8_t *bc = reinterpret_cast<const uint8_t*>( bcs.data() ), *end = bc + bcs.length(); bc<end; ) bc = printOpCode(bc);
}
*/

static string printFuncInfo (const QFunction& func) {
return format("%s (arity=%d, consts=%d, upvalues=%d, bc=%d, file=%s)", func.name, static_cast<int>(func.nArgs), func.constantsEnd-func.constants, func.upvaluesEnd-func.upvalues, func.bytecodeEnd-func.bytecode, func.file);
}


void QFunction::disasm (std::ostream& out)  const {
auto bc = reinterpret_cast<const uint8_t*>( bytecode ), bcBeg=bc;
auto bcEnd = reinterpret_cast<const uint8_t*>( bytecodeEnd );
for (; bc<bcEnd; )  bc = printOpCode(bc, bc-bcBeg, *this, out);
}

string QV::print () const {
if (isNull()) return ("null");
else if (isUndefined()) return "undefined";
else if (isTrue()) return ("true");
else if (isFalse()) return ("false");
else if (isNum()) return format("%.14G", d);
else if (isString()) return ("\"") + asString() + ("\"");
else if (isNativeFunction()) return format("%s@%#0$16llX", ("NativeFunction"), i);
else if (isNormalFunction()) return format("%s@%#0$16llX: %s", ("NormalFunction"), i, printFuncInfo(*asObject<QFunction>()));
else if (isClosure()) return format("%s@%#0$16llX: %s", ("Closure"), i, printFuncInfo(asObject<QClosure>()->func));
else if (i==QV_VARARG_MARK) return "<VarArgMark>"; 
else {
QObject* obj = asObject<QObject>();
QClass* cls = isInstanceOf(obj->type->vm.classClass)? static_cast<QClass*>(obj) : nullptr;
QString* str = isInstanceOf(obj->type->vm.stringClass)? static_cast<QString*>(obj) : nullptr;
if (cls) return format("Class:%s@%#0$16llX", cls->name, i);
else if (str) return format("String:'%s'@%#0$16llX", str->data, i);
else return format("%s@%#0$16llX", obj->type->name, i);
}}


