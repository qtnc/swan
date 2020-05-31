#ifndef NO_RANDOM 
#include "SwanLib.hpp"
#include "../vm/Random.hpp"
#include "../vm/List.hpp"
using namespace std;

static void randomInstantiate (QFiber& f) {
QRandom* r = f.vm.construct<QRandom>(f.vm);
if (f.getArgCount()>=2 && f.isNum(1)) r->rand.seed(f.getNum(1));
f.returnValue(r);
}

static void randomCall (QFiber& f) {
QRandom& r = f.getObject<QRandom>(0);
int nargs = f.getArgCount();
if (nargs==1) {
uniform_real_distribution<double> dist(0.0, 1.0);
f.returnValue( dist(r.rand) );
}
else if (nargs==2) {
if (f.isNum(1)) {
double n = f.getNum(1);
if (n<0) error<invalid_argument>("ARgument to rand() must be positive");
else if (n<1) {
bernoulli_distribution dist(n);
f.returnValue( dist(r.rand) );
} else {
uniform_int_distribution<int64_t> dist(0, n -1);
f.returnValue(static_cast<double>( dist(r.rand) ));
}} else {//not num
vector<double> weights;
vector<QV, trace_allocator<QV>> qw(f.vm);
auto citr = copyVisitor(std::back_inserter(qw));
f.at(1).copyInto(f, citr);
for (QV& x: qw) weights.push_back(x.asNum());
discrete_distribution<size_t> dist(weights.begin(), weights.end());
f.returnValue( static_cast<double>( dist(r.rand) ) );
}}
else if (nargs==3) {
uniform_int_distribution<int> dist(f.getNum(1), f.getNum(2));
f.returnValue(static_cast<double>( dist(r.rand) ));
}}

static void randomNormal (QFiber& f) {
QRandom& r = f.getObject<QRandom>(0);
normal_distribution<double> dist( f.getOptionalNum(1, "mu", 0), f.getOptionalNum(2, "sigma", 1) );
f.returnValue( dist(r.rand) );
}

static void randomSeed (QFiber& f) {
QRandom& r = f.getObject<QRandom>(0);
r.rand.seed(f.getNum(1));
}

static inline QRandom& getDefaultRandom (QFiber& f) {
return * f.vm.globalVariables[f.vm.globalSymbols["rand"].index] .asObject<QRandom>();
}

static void listShuffle (QFiber& f) {
QList& l = f.getObject<QList>(0);
QRandom& r = f.getArgCount()>=2? f.getObject<QRandom>(1) : getDefaultRandom(f);
shuffle(l.data.begin(), l.data.end(), r.rand);
}

static void listDraw (QFiber& f) {
QList& l = f.getObject<QList>(0);
QRandom& r = f.getArgCount()>=2 && f.at(1).isInstanceOf(f.vm.randomClass)? f.getObject<QRandom>(1) : getDefaultRandom(f);
int count = f.getOptionalNum(-1, 1);
if (count>1) {
auto tmp = l.data;
shuffle(tmp.begin(), tmp.end(), r.rand);
QList* re = f.vm.construct<QList>(f.vm);
f.returnValue(re);
for (int i=0, n=tmp.size(); i<count && i<n; i++) re->data.push_back(tmp[i]);
} 
else if (f.getArgCount()>=2 && !f.at(-1).isInstanceOf(f.vm.randomClass)) {
vector<double> weights;
vector<QV, trace_allocator<QV>> qw(f.vm);
auto citr = copyVisitor(std::back_inserter(qw));
f.at(-1).copyInto(f, citr);
for (QV& x: qw) weights.push_back(x.asNum());
weights.resize(l.data.size());
discrete_distribution<size_t> dist(weights.begin(), weights.end());
f.returnValue( l.data[ dist(r.rand) ]);
}
else {
uniform_int_distribution<size_t> dist(0, l.data.size() -1);
f.returnValue( l.data[ dist(r.rand) ]);
}}

static void listPermute (QFiber& f) {
QList& list = f.getObject<QList>(0);
bool re;
if (f.getArgCount()>=2) re = next_permutation(list.data.begin(), list.data.end(), QVBinaryPredicate(f.vm, f.at(1)));
else re = next_permutation(list.data.begin(), list.data.end(), QVLess(f.vm));
f.returnValue(re);
}

void QVM::initRandomType () {
randomClass
->copyParentMethods()
->bind("()", randomCall)
->bind("reset", randomSeed, "ON@0")
->bind("normal", randomNormal, "OON?N")
->assoc<QRandom>();

randomClass ->type
->copyParentMethods()
->bind("()", randomInstantiate)
->assoc<QClass>();

listClass
->bind("shuffle", listShuffle)
->bind("draw", listDraw)
->bind("permute", listPermute)
;

bindGlobal("rand", QV(construct<QRandom>(*this)));
}
#endif
