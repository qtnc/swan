#ifdef DEBUG
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

const uint8_t* printOpCode (const uint8_t* bc, int offset, const QFunction& func, std::ostream& out) {
QVM& vm = func.type->vm;
int nArgs=0, argFormat = OPCODE_INFO[*bc].argFormat;
print(out, "%s", OPCODE_NAMES[*bc]);
bc++;
if (argFormat>=0x1000) nArgs=4;
else if (argFormat>=0x100) nArgs=3;
else if (argFormat>=0x10) nArgs=2;
else if (argFormat>=1) nArgs=1;
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
print(out, ", %d, %d", static_cast<int>(x.first), static_cast<int>(x.second));
bc++;
}break;
case 0xA: {
auto symbol = *reinterpret_cast<const uint_method_symbol_t*>(bc);
print(out, ", %d (%s)", symbol, vm.methodSymbols[symbol]);
bc+=sizeof(symbol);
}break;
case 0xB: {
auto index = *reinterpret_cast<const uint_global_symbol_t*>(bc);
auto gv = std::find_if(vm.globalSymbols.begin(), vm.globalSymbols.end(), [&](auto& p){ return p.second.index==index; });
if (gv!=vm.globalSymbols.end()) print(out, ", %d (%s=%s)", index, gv->first, vm.globalVariables[index].toString());
else print(out, ", %d (%s)", index, vm.globalVariables[index].toString());
bc+=sizeof(index);
}break;
case 0xC: {
auto index = *reinterpret_cast<const uint_constant_index_t*>(bc);
print(out, ", %d (%s)", index, func.constants[index].toString() );
bc+=sizeof(index);
}break;
}}
out << std::endl;
return bc;
}

void QFunction::disasm (std::ostream& out)  const {
auto bc = reinterpret_cast<const uint8_t*>( bytecode ), bcBeg=bc;
auto bcEnd = reinterpret_cast<const uint8_t*>( bytecodeEnd );
println(out, "Bytecode for %s defined in %s (%d bytes):", name, file, static_cast<size_t>(bcEnd-bcBeg) );
println(out, "flags=%0#$2X (vararg=%s, pure=%s, final=%s, overridden=%s); typeInfo=%s", flags.value, flags.vararg, flags.pure, flags.final, flags.overridden, typeInfo);
for (; bc<bcEnd; )  bc = printOpCode(bc, bc-bcBeg, *this, out);
}

static string funcToStr (const char* type, uint64_t i, const QFunction& func) {
ostringstream out;
print(out, "%s", type);
if (func.name) print(out, "\"%s\"", func.name);
if (func.typeInfo) print(out, "<%s>", func.typeInfo);
print(out, "@%#0$16llX", i);
if (func.file) print(out, " defined in %s", func.file);
return out.str();
}

string QV::toString () const {
if (isNull()) return ("null");
else if (isUndefined()) return "undefined";
else if (isTrue()) return ("true");
else if (isFalse()) return ("false");
else if (isNum()) return format("%.14G", d);
else if (isString()) return ("\"") + asString() + ("\"");
else if (isNativeFunction()) return format("%s@%#0$16llX", ("NativeFunction"), i);
else if (isNormalFunction()) return funcToStr("Function", i, *asObject<QFunction>());
else if (isClosure()) return funcToStr("Closure", i, asObject<QClosure>()->func);
else if (i==QV_VARARG_MARK) return "VarArgMark"; 
else {
QObject* obj = asObject<QObject>();
QClass* cls = isInstanceOf(obj->type->vm.classClass)? static_cast<QClass*>(obj) : nullptr;
QString* str = isInstanceOf(obj->type->vm.stringClass)? static_cast<QString*>(obj) : nullptr;
if (cls) return format("Class(%s)@%#0$16llX", cls->name, i);
else if (str) return ("\"") + asString() + ("\"");
else return format("%s@%#0$16llX", obj->type->name, i);
}}

#endif


