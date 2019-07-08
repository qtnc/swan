#include "../../include/Swan.hpp"
#include "Value.hpp"
#include "VM.hpp"

export Swan::Handle::Handle (): value(QV_UNDEFINED) {}

export void Swan::Handle::assign (uint64_t newValue) {
release();
QV qv(value=newValue);
if (qv.isObject()) {
QObject* obj = qv.asObject<QObject>();
auto& vm = obj->type->vm;
vm.lock();
vm.keptHandles[qv.i]++;
vm.unlock();
}}

Swan::Handle QV::asHandle () {
return Swan::Handle(i);
}

export void Swan::Handle::release () {
QV qv(value);
if (qv.isObject()) {
QObject* obj = qv.asObject<QObject>();
auto& vm = obj->type->vm;
vm.lock();
auto it = vm.keptHandles.find(qv.i);
if (it!=vm.keptHandles.end() && --it->second<=0) vm.keptHandles.erase(it);
vm.unlock();
}
value = QV_UNDEFINED;
}

