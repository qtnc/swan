#ifndef NO_RANDOM
#ifndef _____SWAN_RANDOM_HPP_____
#define _____SWAN_RANDOM_HPP_____
#include "Object.hpp"
#include "VM.hpp"
#include<random>
#include<ctime>

struct QRandom: QObject {
std::mt19937 rand;
QRandom (QVM& vm): QObject(vm.randomClass), rand(time(nullptr)) {}
~QRandom () = default;
inline size_t getMemSize () { return sizeof(*this); }
};
#endif
#endif
