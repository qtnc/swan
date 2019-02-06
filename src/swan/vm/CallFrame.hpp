#ifndef ____SWAN_CALL_FRAME_HPP_____
#define ____SWAN_CALL_FRAME_HPP_____

struct QCallFrame {
struct QClosure* closure;
const char* bcp;
size_t stackBase;
template<class T> inline T read () { return *reinterpret_cast<const T*&>(bcp)++; }
inline bool isCppCallFrame () { return !closure || !bcp; }
};

#endif
