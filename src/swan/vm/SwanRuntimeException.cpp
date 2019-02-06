#include "../../include/Swan.hpp"
#include "../../include/cpprintf.hpp"
#include<string>
#include<sstream>
using namespace std;

string Swan::RuntimeException::getStackTraceAsString () {
ostringstream out;
for (auto& e: stackTrace) {
if (e.line>=0 && !e.function.empty() && !e.file.empty()) println(out, "at %s in %s:%d", e.function, e.file, e.line);
else if (e.line<0 && !e.function.empty() && !e.file.empty()) println(out, "at %s in %s", e.function, e.file);
else if (e.line>=0 && e.function.empty() && !e.file.empty()) println(out, "in %s:%d", e.file, e.line);
else if (e.line<0 && e.function.empty() && !e.file.empty()) println(out, "in %s", e.file);
else if (e.line<0 && !e.function.empty() && e.file.empty()) println(out, "at %s", e.function);
}
return out.str();
}

