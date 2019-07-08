#include "../include/Swan.hpp"
#include<iostream>
using namespace std;

void print (Swan::Fiber& f) {
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

