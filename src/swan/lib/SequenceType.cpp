#include "SwanLib.hpp"
#include "../vm/Sequence.hpp"
using namespace std;

static void sequenceJoin (QFiber& f) {
QSequence& seq = f.getObject<QSequence>(0);
string out, delim = f.getOptionalString(1, "");
seq.join(f, delim, out);
f.returnValue(out);
}

static void sequencePlus (QFiber& f) {
QV s1 = f.at(0), s2 = f.at(1);
QClass& cls = s1.getClass(f.vm);
f.pushCppCallFrame();
f.push(cls.type->methods[f.vm.findMethodSymbol("of")]);
f.push(&cls);
f.push(s1);
f.push(s2);
f.call(3);
QV re = f.at(-1);
f.pop();
f.popCppCallFrame();
f.returnValue(re);
}


void QVM::initSequenceType () {
sequenceClass
BIND_F(join, sequenceJoin)
BIND_F(+, sequencePlus)
;
}
