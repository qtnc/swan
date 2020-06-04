#include "Compiler.hpp"
#include "../vm/VM.hpp"
#include "../vm/Function.hpp"
#include "../vm/OpCodeInfo.hpp"
using namespace std;

void QCompiler::writeDebugLine (const QToken& tk) {
int16_t line = parser.getPositionOf(tk.start).first;
if (debugItems.size()>0 && debugItems.back().line==line) return;
debugItems.push_back({ writePosition(), line });
}

int QCompiler::writeOpJumpBackTo  (QOpCode op, int pos) { 
int distance = writePosition() -pos + sizeof(uint_jump_offset_t) +1;
if (distance >= std::numeric_limits<uint_jump_offset_t>::max()) compileError(parser.cur, "Jump too long");
return writeOpJump(op, distance); 
}

void QCompiler::patchJump (int pos, int reach) {
int curpos = out.tellp();
out.seekp(pos);
if (reach<0) reach = curpos;
int distance = reach -pos  - sizeof(uint_jump_offset_t);
if (distance >= std::numeric_limits<uint_jump_offset_t>::max()) compileError(parser.cur, "Jump too long");
write<uint_jump_offset_t>(distance);
out.seekp(curpos);
}

void QCompiler::writeOpCallFunction (uint8_t nArgs) {
if (nArgs<8) writeOp(static_cast<QOpCode>(OP_CALL_FUNCTION_0 + nArgs));
else writeOpArg<uint_local_index_t>(OP_CALL_FUNCTION, nArgs);
}

void QCompiler::writeOpCallMethod (uint8_t nArgs, uint_method_symbol_t symbol) {
if (nArgs<8) writeOpArg<uint_method_symbol_t>(static_cast<QOpCode>(OP_CALL_METHOD_1 + nArgs), symbol);
else writeOpArgs<uint_method_symbol_t, uint_local_index_t>(OP_CALL_METHOD, symbol, nArgs+1);
}

void QCompiler::writeOpCallSuper  (uint8_t nArgs, uint_method_symbol_t symbol) {
if (nArgs<8) writeOpArg<uint_method_symbol_t>(static_cast<QOpCode>(OP_CALL_SUPER_1 + nArgs), symbol);
else writeOpArgs<uint_method_symbol_t, uint_local_index_t>(OP_CALL_SUPER, symbol, nArgs+1);
}

void QCompiler::writeOpLoadLocal (uint_local_index_t slot) {
if (slot<8) writeOp(static_cast<QOpCode>(OP_LOAD_LOCAL_0 + slot));
else writeOpArg<uint_local_index_t>(OP_LOAD_LOCAL, slot);
}

void QCompiler::writeOpStoreLocal (uint_local_index_t slot) {
if (slot<8) writeOp(static_cast<QOpCode>(OP_STORE_LOCAL_0 + slot));
else writeOpArg<uint_local_index_t>(OP_STORE_LOCAL, slot);
}

void QCompiler::writeInlineCall (QFunction& func, bool sameThis) {
int localShift = countLocalVariablesInScope(0);
vector<int> returns;
for (auto bc = func.bytecode, bcEnd = func.bytecodeEnd; bc<bcEnd; ) {
auto op = *bc;
auto sz = OPCODE_INFO[op].szArgs +1;
switch(op){
case OP_LOAD_THIS:
writeOpLoadLocal(localShift);
break;

case OP_LOAD_LOCAL:
case OP_STORE_LOCAL:
writeOpArg<uint_local_index_t>(static_cast<QOpCode>(op), *reinterpret_cast<uint_local_index_t*>(bc+1) + localShift);
break;

#define C(N) case OP_LOAD_LOCAL_##N:
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7)
#undef C
writeOpLoadLocal(op - OP_LOAD_LOCAL_0 + localShift);
break;

#define C(N) case OP_STORE_LOCAL_##N:
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7)
#undef C
writeOpStoreLocal(op - OP_STORE_LOCAL_0 + localShift);
break;

case OP_LOAD_THIS_FIELD:
case OP_LOAD_THIS_STATIC_FIELD:
if (sameThis) goto normal;
writeOpLoadLocal(localShift);
writeOpArg<uint_field_index_t>(static_cast<QOpCode>(op+2), *reinterpret_cast<uint_field_index_t*>(bc+1));
break;

case OP_STORE_THIS_FIELD:
case OP_STORE_THIS_STATIC_FIELD:
if (sameThis) goto normal;
writeOpLoadLocal(localShift);
writeOpArg<uint8_t>(OP_SWAP, 0xFE);
writeOpArg<uint_field_index_t>(static_cast<QOpCode>(op+2), *reinterpret_cast<uint_field_index_t*>(bc+1));
break;

case OP_RETURN:
returns.push_back(writeOpJump(OP_JUMP));
break;


default: normal:
write(bc, sz);
break;
}
bc += sz;
}
for (auto pos: returns) patchJump(pos);
writeOpStoreLocal(localShift);
for (int i=0; i<func.nArgs; i++) writeOp(OP_POP);
}
