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

extern const char* OPCODE_NAMES[];

OpCodeInfo OPCODE_INFO[] = {
#define OP(name, stackEffect, nArgs, argFormat) { stackEffect, nArgs, argFormat }
#include "../vm/OpCodes.hpp"
#undef OP
};

const uint8_t* printOpCode (const uint8_t* bc) {
if (!OPCODE_NAMES[*bc]) {
println(std::cerr, "Unknown opcode: %d(%<#0$2X)", *bc);
println(std::cerr, "Following bytes: %0$2X %0$2X %0$2X %0$2X %0$2X %0$2X %0$2X %0$2X", bc[0], bc[1], bc[2], bc[3], bc[4], bc[5], bc[6], bc[7]);
return bc+8;
}
int nArgs = OPCODE_INFO[*bc].nArgs, argFormat = OPCODE_INFO[*bc].argFormat;
print("%s", OPCODE_NAMES[*bc]);
bc++;
for (int i=0; i<nArgs; i++) {
int arglen = argFormat&0x0F;
argFormat>>=4;
switch(arglen){
case 1:
print(", %d", static_cast<int>(*bc));
bc++;
break;
case 2:
print(", %d", *reinterpret_cast<const uint16_t*>(bc));
bc+=2;
break;
case 4:
print(", %d", *reinterpret_cast<const uint32_t*>(bc));
bc+=4;
break;
case 8:
print(", %d", *reinterpret_cast<const uint64_t*>(bc));
bc+=8;
break;
}}
println("");
return bc;
}

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

static string printFuncInfo (const QFunction& func) {
return format("%s (arity=%d, consts=%d, upvalues=%d, bc=%d, file=%s)", func.name, static_cast<int>(func.nArgs), func.constantsEnd-func.constants, func.upvaluesEnd-func.upvalues, func.bytecodeEnd-func.bytecode, func.file);
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
else if (isOpenUpvalue()) return format("%s@%#0$16llX=>%s", ("Upvalue"), i, asPointer<Upvalue>()->get().print() );
else if (i==QV_VARARG_MARK) return "<VarArgMark>"; 
else {
QObject* obj = asObject<QObject>();
QClass* cls = dynamic_cast<QClass*>(obj);
QString* str = dynamic_cast<QString*>(obj);
if (cls) return format("Class:%s@%#0$16llX", cls->name, i);
else if (str) return format("String:'%s'@%#0$16llX", str->data, i);
else return format("%s@%#0$16llX", obj->type->name, i);
}}


