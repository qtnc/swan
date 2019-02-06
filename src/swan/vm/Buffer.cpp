#ifndef NO_BUFFER
#include "VLS.hpp"
#include "Buffer.hpp"
#include "Class.hpp"
#include "VM.hpp"

QBuffer::QBuffer (QVM& vm, uint32_t len): 
QSequence(vm.bufferClass), length(len) {}

QBuffer* QBuffer::create (QVM& vm, const void* str, int len) {
QBuffer* s = newVLS<QBuffer, uint8_t>(len+4, vm, len);
if (len>0) memcpy(s->data, str, len);
*reinterpret_cast<uint32_t*>(&s->data[len]) = 0;
return s;
}

QBuffer* QBuffer::create (QBuffer* s) { 
return create(s->type->vm, s->data, s->length); 
}

#endif

