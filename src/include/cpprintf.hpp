#ifndef ___QCL_PRINTF___1
#define ___QCL_PRINTF___1
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/mpl/vector.hpp>
#include <vector>
#include <string>
#include <iostream>
#include<sstream>
using namespace boost::type_erasure;

typedef any<boost::mpl::vector<boost::type_erasure::typeid_<>, ostreamable<>, copy_constructible<>>, _self> any_ostreamable;
typedef std::vector<any_ostreamable> any_ostreamable_vector;

void print (std::ostream& out, const char* fmt, const any_ostreamable_vector& args);

template<class... A> inline void print (std::ostream& out, const char* fmt, const A&... args) {
any_ostreamable_vector v = { any_ostreamable(args)... };
print(out, fmt, v);
}

template<class... A> inline void println (std::ostream& out, const char* fmt, const A&... args) {
print(out, fmt, args...);
out << std::endl;
}

template<class... A> inline void print (const char* fmt, const A&... args) {
print(std::cout, fmt, args...);
}

template<class... A> inline void println (const char* fmt, const A&... args) {
println(std::cout, fmt, args...);
}

template<class... A> inline std::string format (const char* fmt, const A&... args) {
std::ostringstream out;
print(out, fmt, args...);
return out.str();
}

#endif