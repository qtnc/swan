#include "../include/Swan.hpp"
#include "../include/cpprintf.hpp"
#include<sstream>
#include<fstream>
#include<unordered_map>
#include<functional>
using namespace std;

#define MODULE(NAME) void swanLoad##NAME (Swan::Fiber&);
#include "modules.hpp"
#undef MODULE

unordered_map<string,function<void(Swan::Fiber&)>> builtInModules = {
#define MODULE(NAME) { #NAME, swanLoad##NAME  },
#include "modules.hpp"
#undef MODULE
};

void print (Swan::Fiber&);

void printStackTrace (Swan::RuntimeException& e) {
println(std::cerr, "ERROR: %s", e.what());
println(std::cerr, "%s", e.getStackTraceAsString());
}

static bool importHook (Swan::Fiber& f, const string& name, Swan::VM::ImportHookState state, int unused) {
if (state!=Swan::VM::ImportHookState::BEFORE_IMPORT) return false;
auto it = builtInModules.find(name);
if (it!=builtInModules.end()) {
(it->second)(f);
return true;
}
return false;
}

static inline bool open (ifstream& in, const string& fn) {
in.open(fn, ios::binary);
return static_cast<bool>(in);
}

static string fileLoader (const string& filename) {
ifstream in;
if (!open(in, filename+".sb") && !open(in, filename+".swan") && !open(in, filename)) throw ios_base::failure(format("Couldn't load %s", filename));
ostringstream out;
out << in.rdbuf();
return out.str();
}

Swan::VM& createVM () {
Swan::VM& vm = Swan::VM::create();
vm.setFileLoader(fileLoader);
vm.setImportHook(importHook);
auto& f = vm.getActiveFiber();
f.registerFunction("print", print);
return vm;
}
