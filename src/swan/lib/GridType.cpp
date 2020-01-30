#ifndef NO_GRID
#include "SwanLib.hpp"
#include "../vm/Grid.hpp"
using namespace std;

#ifndef NO_GRID_PATHFIND
void initGridPathfind (QVM& vm);
#endif
#ifndef NO_GRID_MATRIX
void initGridMatrix (QVM& vm);
#endif


static void gridInstantiate (QFiber& f) {
int n = f.getArgCount() -3;
uint32_t width = f.getNum(1), height = f.getNum(2);
QGrid* grid = QGrid::create(f.vm, width, height, n? &f.at(3) : nullptr);
f.returnValue(grid);
}

static void gridIterator (QFiber& f) {
QGrid& grid = f.getObject<QGrid>(0);
f.returnValue(f.vm.construct<QGridIterator>(f.vm, grid));
}

static void gridIteratorNext (QFiber& f) {
QGridIterator& gi = f.getObject<QGridIterator>(0);
if (gi.iterator < gi.grid.data+gi.grid.width*gi.grid.height) f.returnValue(*gi.iterator++);
else f.returnValue(QV::UNDEFINED);
}

static void gridSubscript (QFiber& f) {
QGrid& grid = f.getObject<QGrid>(0);
if (f.isNum(1) && f.isNum(2)) {
int x = f.getNum(1), y = f.getNum(2);
if (x<0) x+=grid.width;
if (y<0) y+=grid.height;
f.returnValue(x>=0 && y>=0 && x<grid.width && y<grid.height? grid.at(x,y) : QV::UNDEFINED);
}
else if (f.isRange(1) || f.isRange(2)) {
int startX, startY, endX, endY;
if (f.isRange(1)) f.getRange(1).makeBounds(grid.width, startX, endX);
else Swan::Range(f.getNum(1), f.getNum(1), true).makeBounds(grid.width, startX, endX);
if (f.isRange(2)) f.getRange(2).makeBounds(grid.height, startY, endY);
else Swan::Range(f.getNum(2), f.getNum(2), true).makeBounds(grid.height, startY, endY);
QGrid* subgrid = QGrid::create(f.vm, (endX-startX), (endY-startY), nullptr);
for (int x=startX, i=0; x<endX; x++, i++) {
for (int y=startY, j=0; y<endY; y++, j++) {
subgrid->at(i, j) = grid.at(x, y);
}}
f.returnValue(subgrid);
}
else f.returnValue(QV::UNDEFINED);
}

static void gridSubscriptSetter (QFiber& f) {
QGrid& grid = f.getObject<QGrid>(0);
if (f.isNum(1) && f.isNum(2)) {
int x = f.getNum(1), y = f.getNum(2);
if (x<0) x+=grid.width;
if (y<0) y+=grid.height;
grid.at(x, y) = f.at(3);
}
else if (f.isRange(1) || f.isRange(2)) {
int startX, startY, endX, endY;
if (f.isRange(1)) f.getRange(1).makeBounds(grid.width, startX, endX);
else Swan::Range(f.getNum(1), f.getNum(1), true).makeBounds(grid.width, startX, endX);
if (f.isRange(2)) f.getRange(2).makeBounds(grid.height, startY, endY);
else Swan::Range(f.getNum(2), f.getNum(2), true).makeBounds(grid.height, startY, endY);
vector<QV, trace_allocator<QV>> v(f.vm);
f.getObject<QSequence>(3) .copyInto(f, v);
for (int y=startY, i=0, n=v.size(); y<endY && i<n; y++) {
for (int x=startX; x<endX && i<n; x++, i++) {
grid.at(x, y) = v[i];
}}
}
f.returnValue(f.at(3));
}

static void gridToString (QFiber& f) {
QGrid& grid = f.getObject<QGrid>(0);
string re = "";
for (uint32_t y=0; y<grid.height; y++) {
if (y>0) re += "\r\n";
re += "| ";
for (uint32_t x=0; x<grid.width; x++) {
if (x>0) re += ", ";
appendToString(f, grid.at(x,y), re);
}
re += " |";
}
f.returnValue(re);
}

static void gridEquals (QFiber& f) {
QGrid &g1 = f.getObject<QGrid>(0), &g2 = f.getObject<QGrid>(1);
if (g1.width!=g2.width || g1.height!=g2.height) { f.returnValue(false); return; }
int eqSymbol = f.vm.findMethodSymbol("==");
bool re = true;
for (uint32_t i=0, n=g1.width*g1.height; re && i<n; i++) {
f.pushCppCallFrame();
f.push(g1.data[i]);
f.push(g2.data[i]);
f.callSymbol(eqSymbol, 2);
re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
}
f.returnValue(re);
}

void QVM::initGridType () {
gridClass
->copyParentMethods()
->bind("[]", gridSubscript)
->bind("[]=", gridSubscriptSetter)
->bind("iterator", gridIterator)
BIND_L(width, { f.returnValue(static_cast<double>(f.getObject<QGrid>(0).width)); })
BIND_L(height, { f.returnValue(static_cast<double>(f.getObject<QGrid>(0).height)); })
BIND_L(length, { auto& g = f.getObject<QGrid>(0); f.returnValue(g.width*g.height); })
->bind("toString", gridToString)
->bind("==", gridEquals)
;

gridIteratorClass
->copyParentMethods()
->bind("next", gridIteratorNext)
;

gridClass ->type
->copyParentMethods()
->bind("()", gridInstantiate)
;

#ifndef NO_GRID_PATHFIND
initGridPathfind(*this);
#endif
#ifndef NO_GRID_MATRIX
initGridMatrix(*this);
#endif
}

#endif
