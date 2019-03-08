#ifndef NO_GRID
#include "Grid.hpp"
#include "FiberVM.hpp"
#include "String.hpp"
using namespace std;

QGrid* QGrid::create (QVM& vm, uint32_t width, uint32_t height, const QV* data) {
QGrid* grid = vm.constructVLS<QGrid, QV>(width*height, vm, width, height);
if (data) memcpy(grid->data, data, width*height*sizeof(QV));
else memset(grid->data, 0, width*height*sizeof(QV));
return grid;
}


void QGrid::join (QFiber& f, const string& delim, string& re) {
bool notFirst=false;
for (QV *x = data, *end=data+(width*height); x<end; x++) {
if (notFirst) re+=delim;
notFirst=true;
appendToString(f, *x, re);
}}

#endif
