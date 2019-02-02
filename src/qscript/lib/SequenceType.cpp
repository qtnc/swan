#include "SwanLib.hpp"
using namespace std;

static void sequenceJoin (QFiber& f) {
QSequence& seq = f.getObject<QSequence>(0);
string out, delim = f.getOptionalString(1, "");
seq.join(f, delim, out);
f.returnValue(out);
}


void QVM::initSequenceType () {
sequenceClass
BIND_F(join, sequenceJoin)
;
}
