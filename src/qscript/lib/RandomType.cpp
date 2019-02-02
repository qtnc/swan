#ifndef NO_RANDOM 
#include "SwanLib.hpp"
using namespace std;

static void randomInstantiate (QFiber& f) {
QRandom* r = new QRandom(f.vm);
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
if (n<1) {
bernoulli_distribution dist(n);
f.returnValue( dist(r.rand) );
} else {
uniform_int_distribution<int64_t> dist(0, n -1);
f.returnValue(static_cast<double>( dist(r.rand) ));
}} else {
vector<double> weights;
vector<QV> qw;
f.getObject<QSequence>(1) .insertIntoVector(f, qw, 0);
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
normal_distribution<double> dist( f.getOptionalNum(1, 0), f.getOptionalNum(2, 1) );
f.returnValue( dist(r.rand) );
}

static inline QRandom& getDefaultRandom (QFiber& f) {
return * f.vm.globalVariables[find(f.vm.globalSymbols.begin(), f.vm.globalSymbols.end(), "rand") -f.vm.globalSymbols.begin()] .asObject<QRandom>();
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
vector<QV> tmp = l.data;
shuffle(tmp.begin(), tmp.end(), r.rand);
QList* re = new QList(f.vm);
for (int i=0, n=tmp.size(); i<count && i<n; i++) re->data.push_back(tmp[i]);
f.returnValue(re);
} 
else if (f.getArgCount()>=2 && !f.at(-1).isInstanceOf(f.vm.randomClass)) {
vector<double> weights;
vector<QV> qw;
f.getObject<QSequence>(-1) .insertIntoVector(f, qw, 0);
for (QV& x: qw) weights.push_back(x.asNum());
weights.resize(l.data.size());
discrete_distribution<size_t> dist(weights.begin(), weights.end());
f.returnValue( l.data[ dist(r.rand) ]);
}
else {
uniform_int_distribution<size_t> dist(0, l.data.size() -1);
f.returnValue( l.data[ dist(r.rand) ]);
}}

void QVM::initRandomType () {
randomClass
->copyParentMethods()
BIND_F( (), randomCall )
BIND_F( normal, randomNormal)
;

randomMetaClass
->copyParentMethods()
BIND_F( (), randomInstantiate)
;

listClass
BIND_F(shuffle, listShuffle)
BIND_F(draw, listDraw)
;

bindGlobal("rand", QV(new QRandom(*this)));
}
#endif
