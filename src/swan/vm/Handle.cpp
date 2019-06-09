#include "../../include/Swan.hpp"
#include "Value.hpp"
#include "VM.hpp"

export Swan::Handle::Handle (): value(QV_NULL) {}
export Swan::Handle::Handle (Swan::Handle&& h): value(h.value) { h.value=QV_NULL; }
Swan::Handle& export Swan::Handle::operator= (Handle&& h) { value=h.value; h.value=QV_NULL; return *this; }

Swan::Handle QV::asHandle () {
if (isObject()) {
QObject* obj = asObject<QObject>();
obj->type->vm.keptHandles.push_back(*this);
}
Swan::Handle h;
h.value = i;
return h;
}

export Swan::Handle::~Handle () {
QV qv(value);
if (qv.isObject()) {
QObject* obj = qv.asObject<QObject>();
auto& keptHandles = obj->type->vm.keptHandles;
auto it = find_if(keptHandles.begin(), keptHandles.end(), [&](const auto& x){ return x.i==qv.i; });
if (it!=keptHandles.end()) keptHandles.erase(it);
}}

