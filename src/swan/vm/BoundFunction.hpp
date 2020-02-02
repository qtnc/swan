#ifndef _____SWAN_BOUND_FUNCTION_HPP_____
#define _____SWAN_BOUND_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"

struct BoundFunction: QObject {
QV method;
size_t count;
QV args[];
BoundFunction (QVM& vm, const QV& m, size_t c);
static BoundFunction* create (QVM& vm, const QV& m, size_t c, const QV* a);
bool gcVisit ();
~BoundFunction () = default;
inline size_t getMemSize () { return sizeof(*this)+count*sizeof(QV); }
};

#endif
