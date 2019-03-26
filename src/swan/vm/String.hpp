#ifndef _____SWAN_STRING_HPP_____
#define _____SWAN_STRING_HPP_____
#include "Iterable.hpp"
#include<string>

void appendToString (QFiber& f, QV x, std::string& out);

struct QString: QSequence {
size_t length;
char data[];
static QString* create (QVM& vm, const std::string& str);
static QString* create (QVM& vm, const char* str, int length = -1);
static inline QString* create (QVM& vm, const char* start, const char* end) { return create(vm, start, end-start); }
static QString* create (QString*);
QString (QVM& vm, size_t len);
inline std::string asString () { return std::string(data, length); }
inline char* begin () { return data; }
inline char* end () { return data+length; }
virtual ~QString ();
virtual size_t getMemSize () override { return sizeof(*this) + sizeof(char) * (length+1); }
};

struct QStringIterator: QObject {
QString& str;
int index;
QStringIterator (QVM& vm, QString& m);
virtual bool gcVisit () override;
virtual ~QStringIterator() = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif
