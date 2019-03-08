#include "../../include/cpprintf.hpp"
#include "Fiber.hpp"
#include "VM.hpp"
#include "../parser/Compiler.hpp"
#include<boost/algorithm/string.hpp>
#include<utf8.h>
using namespace std;

int QFiber::loadFile (const string& filename) {
string source = vm.fileLoader(filename);
return loadString(source, filename);
}

int QFiber::loadString (const string& initialSource, const string& filename) {
LOCK_SCOPE(vm.gil)
GCLocker gcLocker(vm);
string displayName = "<string>";
if (!filename.empty()) {
int lastSlash = filename.rfind('/');
if (lastSlash<0 || lastSlash>=filename.length()) lastSlash=-1;
displayName = filename.substr(lastSlash+1);
}
if (boost::starts_with(initialSource, "\x1B\x01")) {
istringstream in(initialSource, ios::binary);
return loadBytecode(in);
}
string source = initialSource;
if (utf8::is_valid(source.begin(), source.end())) {
if (utf8::starts_with_bom(source.begin(), source.end())) source = source.substr(3);
}
else {
istringstream in(source, ios::binary);
ostringstream out(ios::binary);
QVM::bufferToStringConverters["native"](in, out, 0);
source = out.str();
}
QParser parser(vm, source, filename, displayName);
QCompiler compiler(parser);
QFunction* func = compiler.getFunction();
if (!func || CR_SUCCESS!=compiler.result) throw Swan::CompilationException(CR_INCOMPLETE==compiler.result);
QClosure* closure = vm.construct<QClosure>(vm, *func);
stack.push_back(QV(closure, QV_TAG_CLOSURE));
return 1;
}

void QFiber::importAndDumpBytecode (const string& baseFile, const string& requestedFile, ostream& out) {
GCLocker gcLocker(vm);
vector<string> importList;
unordered_map<string, QV> importMap;
string finalFile = vm.pathResolver(baseFile, requestedFile);
Swan::VM::ImportHookFn prevImportHook = vm.importHook;
bool finished = false;
vm.importHook = [&](Swan::Fiber& fb, const string& importedFile, Swan::VM::ImportHookState state, int count){
if (this != static_cast<QFiber*>(&fb)) throw std::runtime_error("Import in different fibers");
if (prevImportHook(fb, importedFile, state, count)) return true;
if (!finished) switch(state){
case Swan::VM::ImportHookState::IMPORT_REQUEST: {
auto it = find(importList.begin(), importList.end(), importedFile);
if (it!=importList.end()) importList.erase(it);
importList.push_back(importedFile);
}break;
case Swan::VM::ImportHookState::BEFORE_RUN:
if (count!=1) throw std::runtime_error("count!=1");
if (importedFile == finalFile) finished=true;
importMap[importedFile] = at(-1);
break;
}
return false;
};
import(baseFile, requestedFile);
pop();
vm.importHook = prevImportHook;
print("Import list = ");
for (auto& s: importList) print(", \"%s\"", s);
println(";");
for (auto& s: importList) push(importMap[s]);
dumpBytecode(out, importList.size());
for (int i=0; i<importList.size(); i++) pop();
}
