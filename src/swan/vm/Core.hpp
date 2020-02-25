#ifndef ____SWAN_CONSTANTS_HPP_____
#define ____SWAN_CONSTANTS_HPP_____
#include<cstdint>

#pragma GCC diagnostic ignored "-Wsign-compare"

#define QV_NAN 0x7FF8000000000000ULL
#define QV_PLUS_INF 0x7FF0000000000000ULL
#define QV_MINUS_INF 0xFFF0000000000000ULL
#define QV_NEG_NAN (QV_NAN | 0x8000000000000000ULL)
#define QV_TAGMASK 0xFFFF000000000000ULL

#define QV_TRUE 0x7FF8000000000001ULL
#define QV_FALSE 0x7FF8000000000002ULL
#define QV_UNDEFINED 0x7FF8000000000003ULL
#define QV_VARARG_MARK 0x7FF8000000000004ULL
#define QV_OPEN_UPVALUE_MARK 0x7FF8000000000004ULL
#define QV_NULL 0x7FF8000000000005ULL

#define QV_TAG_GENERIC_SYMBOL_FUNCTION 0x7FF9000000000000ULL
#define QV_TAG_NATIVE_FUNCTION 0x7FFA000000000000ULL
#define QV_TAG_UNUSED_1 0x7FFB000000000000ULL
#define QV_TAG_UNUSED_6 0x7FFC000000000000ULL
#define QV_TAG_UNUSED_2 0x7FFD000000000000ULL
#define QV_TAG_UNUSED_3 0x7FFE000000000000ULL
#define QV_TAG_UNUSED_4 0x7FFF000000000000ULL

#define QV_TAG_UNUSED_5 0xFFF8000000000000ULL
#define QV_TAG_STRING 0xFFF9000000000000ULL
#define QV_TAG_NORMAL_FUNCTION 0xFFFA000000000000ULL
#define QV_TAG_BOUND_FUNCTION 0xFFFB000000000000ULL
#define QV_TAG_CLOSURE 0xFFFC000000000000ULL
#define QV_TAG_DATA 0xFFFD000000000000ULL
#define QV_TAG_STD_FUNCTION  0xFFFE000000000000ULL
#define QV_TAG_FIBER  0xFFFF000000000000ULL

typedef uint16_t uint_jump_offset_t;
typedef uint16_t uint_method_symbol_t;
typedef uint16_t uint_global_symbol_t;
typedef uint16_t uint_constant_index_t;
typedef uint8_t uint_upvalue_index_t;
typedef uint8_t uint_local_index_t;
typedef uint8_t uint_field_index_t;

struct int2x4_t {
int8_t first :4;
int8_t second :4;
};

enum FiberState {
INITIAL,
RUNNING,
YIELDED,
FINISHED,
FAILED
};

struct QFiber;
typedef void(*QNativeFunction)(QFiber&);

struct QVM;
struct QClass;
struct QFiber;
union QV;

#endif
