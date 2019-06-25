#ifndef NO_GRID
#ifndef NO_GRID_MATRIX
#include "SwanLib.hpp"
#include "../vm/Grid.hpp"
#include "../vm/Fiber_inlines.hpp"
using namespace std;

template<class F>
static void matrixOp (QGrid& m1, QGrid& m2, QGrid& m3, const F& func) {
for (uint32_t i=0; i<m1.width; i++) {
for (uint32_t j=0; j<m1.height; j++) {
m3.at(i, j).d = func(m1.at(i, j).d, m2.at(i, j).d);
}}
}

template<class F>
static void matrixOp (QGrid& m1, QGrid& m3, const F& func) {
for (uint32_t i=0; i<m1.width; i++) {
for (uint32_t j=0; j<m1.height; j++) {
m3.at(i, j).d = func(m1.at(i, j).d);
}}
}

static void matrixMul (QGrid& m1, QGrid& m2, QGrid& m3) {
//todo: replace with a less naive matrix multiplication algorithm
for (uint32_t i=0; i<m1.width; i++) {
for (uint32_t j=0; j<m2.height; j++) {
for (uint32_t k=0; k<m1.height; k++) {
m3.at(i, j).d += m1.at(i, k).d * m2.at(k, j).d;
}}}
}

static void matrixAdd (QFiber& f) {
QGrid &m1 = f.getObject<QGrid>(0);
QGrid &m3 = *QGrid::create(f.vm, m1.width, m1.height, nullptr);
f.returnValue(&m3);
if (f.isNum(1)) {
double d = f.getNum(1);
matrixOp(m1, m3, [&](double x){ return x+d; });
}
else {
QGrid &m2 = f.getObject<QGrid>(1);
if (m1.width!=m2.width || m1.height!=m2.height) error<invalid_argument>("Can't operate on matrices of different sizes (%dx%d and %dx%d)", m1.width, m1.height, m2.width, m2.height);
matrixOp(m1, m2, m3, [](double a, double b){ return a+b; });
}
}

static void matrixSubtract (QFiber& f) {
QGrid &m1 = f.getObject<QGrid>(0);
QGrid &m3 = *QGrid::create(f.vm, m1.width, m1.height, nullptr);
f.returnValue(&m3);
if (f.isNum(1)) {
double d = f.getNum(1);
matrixOp(m1, m3, [&](double x){ return x-d; });
}
else {
QGrid &m2 = f.getObject<QGrid>(1);
if (m1.width!=m2.width || m1.height!=m2.height) error<invalid_argument>("Can't operate on matrices of different sizes (%dx%d and %dx%d)", m1.width, m1.height, m2.width, m2.height);
matrixOp(m1, m2, m3, [](double a, double b){ return a-b; });
}
}

static void matrixNegate (QFiber& f) {
QGrid& m1 = f.getObject<QGrid>(0);
QGrid &m3 = *QGrid::create(f.vm, m1.width, m1.height, nullptr);
f.returnValue(&m3);
matrixOp(m1, m3, [](double x){ return -x; });
}

static void matrixTranspose (QFiber& f) {
QGrid& grid = f.getObject<QGrid>(0);
QGrid* newGrid = QGrid::create(f.vm, grid.height, grid.width, nullptr);
f.returnValue(newGrid);
for (uint32_t y=0; y<grid.height; y++) {
for (uint32_t x=0; x<grid.width; x++) {
newGrid->at(y, x) = grid.at(x, y);
}}
}

static void matrixMultiply (QFiber& f) {
QGrid &m1 = f.getObject<QGrid>(0);
if (f.isNum(1)) {
QGrid& m3 = *QGrid::create(f.vm, m1.width, m1.height, nullptr);
f.returnValue(&m3);
double d = f.getNum(1);
matrixOp(m1, m3, [&](double x){ return x*d; });
}
else {
QGrid &m2 = f.getObject<QGrid>(1);
if (m1.height != m2.width) error<invalid_argument>("Matrices ahve incompatible sizes (%dx%d and %dx%d)", m1.width, m1.height, m2.width, m2.height);
QGrid &m3 = *QGrid::create(f.vm, m1.width, m2.height, nullptr);
f.returnValue(&m3);
matrixMul(m1, m2, m3);
}}

static void matrixDivide (QFiber& f) {
QGrid &m1 = f.getObject<QGrid>(0);
if (f.isNum(1)) {
QGrid& m3 = *QGrid::create(f.vm, m1.width, m1.height, nullptr);
f.returnValue(&m3);
double d = f.getNum(1);
matrixOp(m1, m3, [&](double x){ return x/d; });
}
else {
QGrid &m2 = f.getObject<QGrid>(1);
if (m1.width!=m2.width || m1.height!=m2.height) error<invalid_argument>("Can't operate on matrices of different sizes (%dx%d and %dx%d)", m1.width, m1.height, m2.width, m2.height);
if (m1.width!=m1.height) error<invalid_argument>("Can't operate on non-square matrix (%dx%d)", m1.width, m1.height);
error<domain_error>("Matrix inversion isn't yet supported");
QGrid& m3 = *QGrid::create(f.vm, m1.width, m1.height, nullptr);
f.returnValue(&m3);
//todo: matrix inversion
}}

static void matrixExpone (QFiber& f) {
QGrid &m1 = f.getObject<QGrid>(0);
int exponent = f.getNum(1);
if (m1.width!=m1.height) error<invalid_argument>("Can't operate on non-squared matrix (%dx%d)", m1.width, m1.height);
if (exponent>=1) {
QGrid* m3 = &m1;
for (int i=1; i<exponent; i++) {
QGrid* m2 = QGrid::create(f.vm, m1.width, m1.height, nullptr);
matrixMul(m1, *m3, *m2);
m3 = m2;
}
f.returnValue(m3);
}
else if (exponent==0) {
QGrid* m3 = QGrid::create(f.vm, m1.width, m1.height, nullptr);
for (uint32_t i=0; i<m1.width; i++) m3->at(i, i).d = 1;
f.returnValue(m3);
}
else if (exponent<0) {
//todo: matrix inversion
error<domain_error>("Matrix inversion isn't yet supported");
}
}

void initGridMatrix (QVM& vm) {
vm.gridClass
BIND_F(+, matrixAdd)
BIND_F(-, matrixSubtract)
BIND_F(unm, matrixNegate)
BIND_F(*, matrixMultiply)
BIND_F(/, matrixDivide)
BIND_F(**, matrixExpone)
BIND_F(transpose, matrixTranspose)
;
}



#endif
#endif
