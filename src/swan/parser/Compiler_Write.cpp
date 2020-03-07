#include "Compiler.hpp"
#include "../vm/VM.hpp"
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

