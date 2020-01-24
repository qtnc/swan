#ifndef _____SWAN_CLASS_HPP_____
#define _____SWAN_CLASS_HPP_____
#include "Object.hpp"
#include "Value.hpp"
#include "Allocator.hpp"
#include "Array.hpp"

struct QClass: QObject {
QVM& vm;
QClass* parent;
c_string name;
std::vector<QV, trace_allocator<QV>> methods;
int nFields;
QV staticFields[0];
QClass (QVM& vm, QClass* type, QClass* parent, const std::string& name, int nFields=0);
QClass* copyParentMethods ();
QClass* mergeMixinMethods (QClass* mixin);
QClass* bind (const std::string& methodName, QNativeFunction func);
QClass* bind (const std::string& methodName, QNativeFunction func, const char* typeInfo);
QClass* bind (int symbol, const QV& value);
inline bool isSubclassOf (QClass* cls) { return this==cls || (parent && parent->isSubclassOf(cls)); }
inline QV findMethod (int symbol) {
QV re = symbol>=methods.size()? QV::UNDEFINED : methods[symbol];
if (re.isNullOrUndefined() && parent) return parent->findMethod(symbol);
else return re;
}
static QClass* create (QVM& vm, QClass* type, QClass* parent, const std::string& name, int nStaticFields=0, int nFields=0);
virtual QObject* instantiate ();
virtual ~QClass () = default;
virtual bool gcVisit () final override;
virtual size_t getMemSize () override { return sizeof(*this) + sizeof(QV) * std::max(0, type->nFields); }
};

#endif
