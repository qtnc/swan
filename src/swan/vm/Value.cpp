#include "Value.hpp"
#include "Range.hpp"
using namespace std;

const QV QV::Null(QV_NULL), QV::UNDEFINED(QV_UNDEFINED), QV::FALSE(QV_FALSE), QV::TRUE(QV_TRUE);


const Swan::Range& QV::asRange () const { return *asObject<QRange>(); }

bool QV::isInstanceOf (QClass* tp) const {
if (!isObject()) return false;
QClass* type = asObject<QObject>()->type;
do {
if (type==tp) return true;
} while ((type=type->parent));
return false;
}
