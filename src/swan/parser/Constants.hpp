#ifndef _____PARSING_BASE_HPP_____
#define _____PARSING_BASE_HPP_____

// Local variable flags
#define LV_EXISTING 0
#define LV_NEW 1
#define LV_CONST 2
#define LV_FOR_READ 0
#define LV_FOR_WRITE 4
#define LV_ERR_CONST -127
#define LV_ERR_ALREADY_EXIST -126

// Variable declaration flags
#define VD_LOCAL 0
#define VD_VARARG 1
#define VD_CONST 2
#define VD_GLOBAL 4
#define VD_EXPORT 8
#define VD_SINGLE 0x100
#define VD_NODEFAULT 0x200
#define VD_FINAL 0x400
#define VD_HOISTED 0x1000
#define VD_OPTIMFLAG 0x4000000

// Function declaration flags
/*
#define FD_VARARG 1
#define FD_FIBER 2
#define FD_METHOD 4
#define FD_STATIC 8
#define FD_ASYNC 0x10
#define FD_PURE 0x100
#define FD_READ_ONLY 0x200
#define FD_FINAL 0x400
#define FD_ACCESSOR 0x800
*/

// Call resolution flags
#define CR_GETTER 1
#define CR_SETTER 2
#define CR_PURE 4
#define CR_FINAL 8
#define CR_OVERRIDDEN 0x10

// Operator flags
#define P_LEFT 0
#define P_RIGHT 1
#define P_SWAP_OPERANDS 2

enum QOperatorPriority {
P_LOWEST,
P_ASSIGNMENT,
P_COMPREHENSION,
P_CONDITIONAL,
P_LOGICAL,
P_COMPARISON,
P_RANGE,
P_BITWISE,
P_TERM,
P_FACTOR,
P_EXPONENT,
P_PREFIX,
P_POSTFIX,
P_SUBSCRIPT,
P_MEMBER,
P_CALL,
P_HIGHEST
};

enum CompilationResult {
CR_SUCCESS,
CR_FAILED,
CR_INCOMPLETE
};

#endif
