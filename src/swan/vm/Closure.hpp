#ifndef _____SWAN_CLOSURE_HPP_____
#define _____SWAN_CLOSURE_HPP_____
#include "Object.hpp"

struct QClosure: QObject {
struct QFunction& func;
struct Upvalue* upvalues[];
QClosure (QVM& vm, QFunction& f);
virtual bool gcVisit () final override;
virtual ~QClosure () = default;
virtual size_t getMemSize () override ;
};

#endif
