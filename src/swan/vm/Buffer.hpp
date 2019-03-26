#ifndef NO_BUFFER 
#ifndef _____SWAN_BUFFER_HPP_____
#define _____SWAN_BUFFER_HPP_____
#include "Iterable.hpp"

struct QBuffer: QSequence {
uint32_t length;
uint8_t data[];
static QBuffer* create (QVM& vm, const void* buf, int length);
template <class T> static inline QBuffer* create (QVM& vm, const T* start, const T* end) { return create(vm, start, (end-start)*sizeof(T)); }
static QBuffer* create (QBuffer*);
QBuffer (QVM& vm, uint32_t len);
inline uint8_t* begin () { return data; }
inline uint8_t* end () { return data+length; }
virtual ~QBuffer () = default;
virtual size_t getMemSize () override { return sizeof(*this) + sizeof(char) * (length+4); }
};

struct QBufferIterator: QObject {
QBuffer& buf;
int index;
QBufferIterator (QVM& vm, QBuffer& m);
virtual bool gcVisit () override;
virtual ~QBufferIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif
#endif
