#ifndef _____HASHER_AND_EQUALLER_HPP_____
#define _____HASHER_AND_EQUALLER_HPP_____
#include "Core.hpp"
#include "Value.hpp"
#include<utility>
#include<cstring>

size_t hashBytes (const uint8_t* start, const uint8_t* end);

struct StringCacheHasher {
inline size_t operator() (const std::pair<const char*, const char*>& p) const {
return hashBytes(reinterpret_cast<const uint8_t*>(p.first), reinterpret_cast<const uint8_t*>(p.second));
}};

struct StringCacheEqualler {
inline bool operator() (const std::pair<const char*, const char*>& p1, const std::pair<const char*, const char*>& p2) const {
return p1.second-p1.first == p2.second-p2.first && 0==memcmp(p1.first, p2.first, p1.second-p1.first);
}};

struct QVM;

struct QVHasher {
QVM& vm;
inline QVHasher (QVM& vm0): vm(vm0) {}
size_t operator() (const QV& qv) const;
};

struct QVEqualler {
QVM& vm;
inline QVEqualler  (QVM& vm0): vm(vm0) {}
bool operator() (const QV& a, const QV& b) const;
};

struct QVBinaryPredicate  {
QV func;
QVM& vm;
inline QVBinaryPredicate (QVM& vm0, const QV& f): vm(vm0), func(f) {}
bool operator() (const QV& a, const QV& b) const;
};

struct QVUnaryPredicate  {
QV func;
QVM& vm;
inline QVUnaryPredicate (QVM& vm0, const QV& f): vm(vm0), func(f) {}
bool operator() (const QV& a) const;
};

struct QVLess {
QVM& vm;
inline QVLess (QVM& vm0): vm(vm0) {}
bool operator() (const QV& a, const QV& b) const;
};

#endif
