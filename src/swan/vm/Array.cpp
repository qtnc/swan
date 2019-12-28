#include "Array.hpp"
#include<sstream>
using namespace std;

/*
std::ostream& operator<< (std::ostream& out, const simple_string& s) {
out << s.c_str();
return out;
}
*/

std::ostream& operator<< (std::ostream& out, const c_string& s) {
out << s.c_str();
return out;
}
