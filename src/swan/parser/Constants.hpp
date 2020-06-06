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

// Call resolution flags
/*
#define CR_GETTER 1
#define CR_SETTER 2
#define CR_PURE 4
#define CR_FINAL 8
#define CR_OVERRIDDEN 0x10
*/

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
