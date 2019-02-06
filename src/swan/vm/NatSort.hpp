#ifndef _____NATSORT_HPP_____
#define _____NATSORT_HPP_____
#include<cstdlib>

inline bool isDigit (uint32_t c) {
return c>='0' && c<='9';
}

template<class T> int strnatcmp (const T* L, const T* R) {
bool alphaMode=true;
while(*L&&*R) {
if (alphaMode) {
while(*L&&*R) {
bool lDigit = isDigit(*L), rDigit = isDigit(*R);
if (lDigit&&rDigit) { alphaMode=false; break; }
else if (lDigit) return -1;
else if (rDigit) return 1;
else if (*L!=*R) return *L-*R;
++L; ++R;
}} else { // numeric mode
T *lEnd=0, *rEnd=0;
unsigned long long int l = strtoull(L, &lEnd, 0), r = strtoull(R, &rEnd, 0);
if (lEnd) L=lEnd; 
if (rEnd) R=rEnd;
if (l!=r) return l-r;
alphaMode=true;
}}
if (*R) return -1;
else if (*L) return 1;
return 0;
}

template<class T> struct nat_less {
inline bool operator() (const T* l, const T* r) { return strnatcmp(l,r)<0; }
inline bool operator() (const std::basic_string<T>& l, const std::basic_string<T>& r) {  return strnatcmp(l.data(), r.data() )<0;  }
};

#endif
