#ifndef _____SWAN_UPVALUE_HPP_____
#define _____SWAN_UPVALUE_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include "Fiber.hpp"

struct Upvalue: QObject {
struct QFiber* fiber;
QV value;
static inline int stackpos (const QFiber& f, int n) { return f.callFrames.back().stackBase+n; }
Upvalue (QFiber& f, int slot);
inline QV& get () {
if (value.isOpenUpvalue()) return *value.asPointer<QV>();
else return value;
}
inline void close () { value = get(); }
virtual bool gcVisit () override;
virtual ~Upvalue() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
