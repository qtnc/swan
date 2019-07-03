#include<optional>
#include "../include/Swan.hpp"
#include "../include/SwanBinding.hpp"
#include "../include/cpprintf.hpp"
#include<iostream>
#include<sstream>
#include<fstream>
#include<exception>
#include<memory>
#include<cstdlib>
#include<cstdio>
using namespace std;



static void print (Swan::Fiber& f) {
auto& p = std::cout;
for (int i=0, n=f.getArgCount(); i<n; i++) {
if (i>0) p<<' ';
if (f.isString(i)) p << f.getString(i);
else {
f.pushCopy(i);
f.callMethod("toString", 1);
p << f.getString(-1);
f.pop();
}}
p << endl;
f.setUndefined(0);
}


void registerIO (Swan::Fiber& f) {
f.registerFunction("print", print);
}
