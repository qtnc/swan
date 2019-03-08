#ifndef _____SWAN_FOREIGN_CLASS_HPP_____
#define _____SWAN_FOREIGN_CLASS_HPP_____
#include "Class.hpp"

struct QForeignClass: QClass {
typedef void(*DestructorFn)(void*);
DestructorFn destructor;
size_t id;
QForeignClass (QVM& vm, QClass* type, QClass* parent, const std::string& name, int nUserBytes=0, DestructorFn=nullptr);
virtual QObject* instantiate () override;
virtual ~QForeignClass () = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
