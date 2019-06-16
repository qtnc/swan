#include "FiberVM.hpp"
#include "Value.hpp"
#include "Range.hpp"
#include "StdFunction.hpp"

QFiber& QVM::createFiber () {
auto f = construct<QFiber>(*this);
fibers.push_back(f);
return *f;
}

QFiber::QFiber (QVM& vm): 
QSequence(vm.fiberClass), 
vm(vm), 
state(FiberState::INITIAL), 
parentFiber(nullptr),
callFrames(trace_allocator<QCallFrame>(vm)),
catchPoints(trace_allocator<QCatchPoint>(vm)),
openUpvalues(trace_allocator<Upvalue*>(vm)),
stack([this](const QV* _old, const QV* _new){ adjustUpvaluePointers(_old, _new); }, 8, trace_allocator<QV>(vm))
 {
stack.reserve(8);
callFrames.reserve(4);
}

QFiber::~QFiber () { }

void QFiber::release () {
vm.fibers.erase(find(vm.fibers.begin(), vm.fibers.end(), this));
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
push(vm.construct<QRange>(vm, r));
}

void QFiber::setRange (int i, const Swan::Range& r) {
at(i) = vm.construct<QRange>(vm, r);
}

void QFiber::setStdFunction (int i, const std::function<void(Swan::Fiber&)>& f) {
at(i) = QV(vm.construct<StdFunction>(vm, f), QV_TAG_STD_FUNCTION);
}

void QFiber::pushStdFunction (const std::function<void(Swan::Fiber&)>& f) {
stack.push_back( QV(vm.construct<StdFunction>(vm, f), QV_TAG_STD_FUNCTION) );
}
