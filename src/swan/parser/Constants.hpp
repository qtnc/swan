#ifndef _____PARSING_BASE_HPP_____
#define _____PARSING_BASE_HPP_____

#define LV_EXISTING 0
#define LV_NEW 1
#define LV_CONST 2
#define LV_FOR_READ 0
#define LV_FOR_WRITE 4
#define LV_ERR_CONST -127
#define LV_ERR_ALREADY_EXIST -126

#define VD_LOCAL 0
#define VD_VARARG 1
#define VD_CONST 2
#define VD_GLOBAL 4
#define VD_EXPORT 8
#define VD_SINGLE 0x100
#define VD_NODEFAULT 0x200

#define FD_VARARG 1
#define FD_FIBER 2
#define FD_METHOD 4
#define FD_STATIC 8
#define FD_ASYNC 16
#define FD_CONST 0x200

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
