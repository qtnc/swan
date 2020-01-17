#ifndef _____SWAN_UPVALUE_HPP_____
#define _____SWAN_UPVALUE_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include "Fiber.hpp"

struct Upvalue: QObject {
struct QFiber* fiber;
QV* value;
QV closedValue;
static inline int stackpos (const QFiber& f, int n) { return f.callFrames.back().stackBase+n; }
Upvalue (QFiber& f, int slot);
explicit Upvalue (QFiber& f, const QV& value);
inline QV& get () { return *value; }
inline void close () { closedValue = *value; value = &closedValue; }
virtual bool gcVisit () override;
virtual ~Upvalue() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
