#ifndef _____SWAN_GRID_HPP_____
#define _____SWAN_GRID_HPP_____
#include "Sequence.hpp"
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
virtual void copyInto (QFiber& f, std::vector<QV, trace_allocator<QV>>& list, int start) override { list.insert(list.begin()+start, data, data+(width*height)); }
virtual void join (QFiber& f, const std::string& delim, std::string& out) override;
virtual ~QGrid () = default;
virtual bool gcVisit () override;
virtual size_t getMemSize () override { return sizeof(*this) + sizeof(QV) * width * height; }

inline void makeBounds (int& x, int& y) {
if (x<0) x+=width;
if (y<0) y+=height;
}

};

#endif
