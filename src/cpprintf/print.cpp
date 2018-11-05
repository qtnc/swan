#include "../include/cpprintf.hpp"
#include <boost/io/ios_state.hpp>
#include <iomanip>

void print (std::ostream& out, const char* fmt, const any_ostreamable_vector& args) {
int index = 0;
char c;
next: while(c=*fmt++) {
if (c=='%') {
//boost::io::ios_flags_saver savef_outer(out, std::ios_base::dec);
boost::io::ios_flags_saver savef(out);
boost::io::ios_precision_saver savep(out);
boost::io::ios_fill_saver savefill(out);
bool first = true;
while(true) {
switch(c=*fmt++){
case '%': 
if (first) { out << '%';   goto next; }
else { --fmt; goto end; }
case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': index = strtoul(--fmt, const_cast<char**>(&fmt), 10) -1; break;
case '0': out << std::setfill('0') << std::internal; break;
case '+':  out << std::showpos;  break;
case '#': out << std::showpoint << std::showbase; break;
case '-': out << std::left; break;
case '<': index--; break;
case '$': case 'w':
if (*fmt>='0' && *fmt<='9') out << std::setw( strtoul(fmt, const_cast<char**>(&fmt), 10)); 
else if (*fmt=='*') { fmt++; out << std::setw(any_cast<int>(args.at(index++))); }
else { --fmt; goto end; }
break;
case '.': 
if (*fmt>='0' && *fmt<='9') out << std::setprecision( strtoul(fmt, const_cast<char**>(&fmt), 10)); 
else if (*fmt=='*') { fmt++; out << std::setprecision(any_cast<int>(args.at(index++))); }
else { --fmt; goto end; }
break;
case 'd': case 'i': case 'u': out << std::dec; goto end;
case 'o': out << std::oct; goto end;
case 'p': case 'x': out << std::hex; goto end;
case 'P': case 'X': out << std::hex << std::uppercase; goto end;
case 'f': out << std::fixed; goto end;
case 'e': out << std::scientific; goto end;
case 'E': out << std::uppercase << std::scientific; goto end;
case 'G': out << std::uppercase; goto end;
case 'h': case 'j': case 'l': case 'L': case 'q': break;
case 'c': case 'g': case 's': case 'z': goto end;
default: --fmt; goto end;
}
first=false;
} end: 
out << std::boolalpha << args.at(index++);
}
else if (c=='\n') out << std::endl;
else out << c;
}}
