OP(LOAD_UNDEFINED, 1, 0, 0),
OP(LOAD_NULL, 1, 0, 0),
OP(LOAD_TRUE, 1, 0, 0),
OP(LOAD_FALSE, 1, 0, 0),
OP(LOAD_INT8, 1, sizeof(uint8_t), sizeof(uint8_t)),
OP(LOAD_CONSTANT, 1, sizeof(uint_constant_index_t), sizeof(uint_constant_index_t)),
OP(LOAD_CLOSURE, 1, sizeof(uint_constant_index_t), sizeof(uint_constant_index_t)),

OP(LOAD_THIS, 1, 0, 0),
OP(LOAD_LOCAL, 1, sizeof(uint_local_index_t), sizeof(uint_local_index_t)),
OP(LOAD_UPVALUE, 1, sizeof(uint_upvalue_index_t), sizeof(uint_upvalue_index_t)),
OP(LOAD_GLOBAL, 1, sizeof(uint_global_symbol_t), sizeof(uint_global_symbol_t)),
OP(LOAD_THIS_FIELD, 1, sizeof(uint_field_index_t), sizeof(uint_field_index_t)),
OP(LOAD_THIS_STATIC_FIELD, 1, sizeof(uint_field_index_t), sizeof(uint_field_index_t)),
OP(LOAD_FIELD, 0, sizeof(uint_field_index_t), sizeof(uint_field_index_t)),
OP(LOAD_STATIC_FIELD, 0, sizeof(uint_field_index_t), sizeof(uint_field_index_t)),
OP(LOAD_METHOD, 0, sizeof(uint_method_symbol_t), sizeof(uint_method_symbol_t)),

OP(STORE_LOCAL, 0, sizeof(uint_local_index_t), sizeof(uint_local_index_t)),
OP(STORE_UPVALUE, 0, sizeof(uint_upvalue_index_t), sizeof(uint_upvalue_index_t)),
OP(STORE_GLOBAL, 0, sizeof(uint_global_symbol_t), sizeof(uint_global_symbol_t)),
OP(STORE_THIS_FIELD, 0, sizeof(uint_field_index_t), sizeof(uint_field_index_t)),
OP(STORE_THIS_STATIC_FIELD, 0, sizeof(uint_field_index_t), sizeof(uint_field_index_t)),
OP(STORE_FIELD, -1, sizeof(uint_field_index_t), sizeof(uint_field_index_t)),
OP(STORE_STATIC_FIELD, -1, sizeof(uint_field_index_t), sizeof(uint_field_index_t)),
OP(STORE_METHOD, 0, sizeof(uint_method_symbol_t), sizeof(uint_method_symbol_t)),
OP(STORE_STATIC_METHOD, 0, sizeof(uint_method_symbol_t), sizeof(uint_method_symbol_t)),

OP(DUP, 1, 0, 0),
OP(SWAP, 0, 1, 1),
//OP(DUP_M2, 1, 0, 0),

OP(POP, -1, 0, 0),
//OP(POP_M2, -1, 0, 0),
OP(POP_SCOPE, 0, sizeof(uint_local_index_t), sizeof(uint_local_index_t)),

OP(RETURN, 0, 0, 0),
OP(YIELD, 0, 0, 0),

OP(AND, 0, sizeof(uint_jump_offset_t), sizeof(uint_jump_offset_t)),
OP(NULL_COALESCING, 0, sizeof(uint_jump_offset_t), sizeof(uint_jump_offset_t)),
OP(OR, 0, sizeof(uint_jump_offset_t), sizeof(uint_jump_offset_t)),

OP(JUMP, 0, sizeof(uint_jump_offset_t), sizeof(uint_jump_offset_t)),
OP(JUMP_IF_FALSY, -1, sizeof(uint_jump_offset_t), sizeof(uint_jump_offset_t)),
OP(JUMP_IF_TRUTY, -1, sizeof(uint_jump_offset_t), sizeof(uint_jump_offset_t)),
OP(JUMP_IF_UNDEFINED, -1, sizeof(uint_jump_offset_t), sizeof(uint_jump_offset_t)),
OP(JUMP_BACK, 0, sizeof(uint_jump_offset_t), sizeof(uint_jump_offset_t)),

#define L(N) OP(LOAD_LOCAL_##N, 1, 0, 0)
L(0), L(1), L(2), L(3), L(4), L(5), L(6), L(7), 
#undef L

#define L(N) OP(STORE_LOCAL_##N, 0, 0, 0)
L(0), L(1), L(2), L(3), L(4), L(5), L(6), L(7), 
#undef L

#define C(N) OP(CALL_METHOD_##N, 1-N, sizeof(uint_method_symbol_t), sizeof(uint_method_symbol_t))
C(0), C(1), C(2), C(3), C(4), C(5), C(6), C(7), 
#undef C

#define C(N) OP(CALL_SUPER_##N, 1-N, sizeof(uint_method_symbol_t), sizeof(uint_method_symbol_t))
C(0), C(1), C(2), C(3), C(4), C(5), C(6), C(7),
#undef C

#define C(N) OP(CALL_FUNCTION_##N, -N, 0, 0)
C(0), C(1), C(2), C(3), C(4), C(5), C(6), C(7), 
#undef C

OP(CALL_METHOD, 255, sizeof(uint8_t) + sizeof(uint_method_symbol_t), (sizeof(uint8_t)<<4) | (sizeof(uint_method_symbol_t)<<0)),
OP(CALL_SUPER, 255, sizeof(uint8_t) + sizeof(uint_method_symbol_t), (sizeof(uint8_t)<<4) | (sizeof(uint_method_symbol_t)<<0)),
OP(CALL_FUNCTION, 255, sizeof(uint8_t), sizeof(uint8_t)),

OP(CALL_METHOD_VARARG, 255, sizeof(uint_method_symbol_t), sizeof(uint_method_symbol_t)),
OP(CALL_SUPER_VARARG, 255, sizeof(uint_method_symbol_t), sizeof(uint_method_symbol_t)),
OP(CALL_FUNCTION_VARARG, 255, 0, 0),

OP(PUSH_VARARG_MARK, 1, 0, 0),
OP(UNPACK_SEQUENCE, 255, 0, 0),

OP(TRY, 0, 8, 0x44),
OP(THROW, 0, 0, 0),
OP(END_FINALLY, 0, 0, 0),

OP(NEW_CLASS, -1, 3*sizeof(uint_field_index_t), (sizeof(uint_field_index_t)<<8)|(sizeof(uint_field_index_t)<<4)|sizeof(uint_field_index_t) ), // pop name, pop parent, push new class

/* Optimized instructions when we are sure to operate on numbers */
OP(ADD, -1, 0, 0),
OP(SUB, -1, 0, 0),
OP(MUL, -1, 0, 0),
OP(DIV, -1, 0, 0),
OP(MOD, -1, 0, 0),
OP(POW, -1, 0, 0),
OP(BINOR, -1, 0, 0),
OP(BINAND, -1, 0, 0),
OP(BINXOR, -1, 0, 0),
OP(LSH, -1, 0, 0),
OP(RSH, -1, 0, 0),
OP(INTDIV, -1, 0, 0),
OP(LT, -1, 0, 0),
OP(LTE, -1, 0, 0),
OP(GT, -1, 0, 0),
OP(GTE, -1, 0, 0),
OP(EQ, -1, 0, 0),
OP(NEQ, -1, 0, 0),
OP(BINNOT, 0, 0, 0),
OP(NEG, 0, 0, 0),
OP(NOT, 0, 0, 0),

/* Mark the greatest possible opcode */
OP(DEBUG, 0, 0, 0),
OP(END, 0, 0, 0)


