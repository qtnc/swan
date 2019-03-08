#ifndef NO_RANDOM
#ifndef _____SWAN_RANDOM_HPP_____
#define _____SWAN_RANDOM_HPP_____
#include "Object.hpp"
#include "VM.hpp"
#include<random>

struct QRandom: QObject {
std::mt19937 rand;
QRandom (QVM& vm): QObject(vm.randomClass) {}
virtual size_t getMemSize () override { return sizeof(*this); }
};
#endif
#endif
