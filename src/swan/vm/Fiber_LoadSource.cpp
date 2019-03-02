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
QClosure* closure = new QClosure(vm, *func);
stack.push_back(QV(closure, QV_TAG_CLOSURE));
return 1;
}
