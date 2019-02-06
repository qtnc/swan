#ifndef _____SWAN_INSTANCE_HPP_____
#define _____SWAN_INSTANCE_HPP_____
#include "VLS.hpp"
#include "Sequence.hpp"

struct QInstance: QSequence {
QV fields[];
QInstance (QClass* type): QSequence(type) {}
static inline QInstance* create (QClass* type, int nFields) { return newVLS<QInstance, QV>(nFields, type); }
virtual ~QInstance () = default;
virtual bool gcVisit () final override;
};

#endif
