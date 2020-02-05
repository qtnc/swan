#ifndef _____SWAN_GRID_HPP_____
#define _____SWAN_GRID_HPP_____
#include "Iterable.hpp"
#include "Value.hpp"
#include "VM.hpp"
#include "Set.hpp"

struct QGrid: QSequence {
uint32_t width, height;
QV data[];
QGrid (QVM& vm, uint32_t w, uint32_t h): QSequence(vm.gridClass), width(w), height(h) {}
inline QV& at (int x, int y) {
if (x<0) x+=width;
if (y<0) y+=height;
return data[x + y*width];
}
static QGrid* create (QVM& vm, uint32_t width, uint32_t height, const QV* data);
bool join (QFiber& f, const std::string& delim, std::string& out);
~QGrid () = default;
bool gcVisit ();
inline size_t getMemSize () { return sizeof(*this) + sizeof(QV) * width * height; }
inline bool copyInto (QFiber& f, CopyVisitor& out) { std::for_each(data, data+width*height, std::ref(out)); return true; }
inline int getLength () { return width*height; }

inline void makeBounds (int& x, int& y) {
if (x<0) x+=width;
if (y<0) y+=height;
}

};

struct QGridIterator: QObject {
QGrid& grid;
const QV* iterator;
QGridIterator (QVM& vm, QGrid& m): QObject(vm.gridIteratorClass), grid(m), iterator(m.data)  {}
bool gcVisit ();
~QGridIterator() = default;
inline size_t getMemSize () { return sizeof(*this); }
};

#endif
