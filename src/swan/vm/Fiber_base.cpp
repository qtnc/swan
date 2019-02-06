#include "FiberVM.hpp"
#include "Range.hpp"

thread_local QFiber* QFiber::curFiber = nullptr;

QFiber::QFiber (QVM& vm0): QSequence(vm0.fiberClass), vm(vm0), state(FiberState::INITIAL)
, stack([this](const QV* _old, const QV* _new){ adjustUpvaluePointers(_old, _new); }, 8) 
{
stack.reserve(8);
callFrames.reserve(4);
}

bool QFiber::isRange (int i) { return at(i).isInstanceOf(vm.rangeClass); }

#ifndef NO_BUFFER
#include "Buffer.hpp"

bool QFiber::isBuffer (int i) { return at(i).isInstanceOf(vm.bufferClass); }

const void* QFiber::getBufferV (int i, int* length) {
QBuffer& b = getObject<QBuffer>(i);
if (length) *length = b.length;
return b.data;
}

void QFiber::pushBuffer  (const void* data, int length) {
push(QBuffer::create(vm, data, length));
}

void QFiber::setBuffer  (int i, const void* data, int length) {
at(i) = QBuffer::create(vm, data, length);
}

#else
bool QFiber::isBuffer (int i) { return false; }
const void* QFiber::getBufferV (int i, int* length) { return nullptr; }
void QFiber::setBuffer  (int i, const void* data, int length) { }
#endif

void QFiber::pushRange (const Swan::Range& r) {
push(new QRange(vm, r));
}

void QFiber::setRange (int i, const Swan::Range& r) {
at(i) = new QRange(vm, r);
}

