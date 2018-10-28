#include "../include/QScript.hpp"
#include "../include/cpprintf.hpp"
#include<iostream>
#include<sstream>
#include<typeinfo>
#include<exception>
#include<cstring>
using namespace std;

void registerCLIEnv (QS::Fiber& f);

void printIntro () {
println("QScript v0");
println("Copyright (c) 2018, QuentinC ");
println("For more info, go to http://github.com/qtnc/qscript");
}

void printHelp (const std::string& argv0) {
printIntro();
println("Synopsis: %s [options] [script]", argv0);
println("Where [script] is the QScript to execute");
println("And where [options] can be: ");
println("-c: compile script but don't run it; not yet supported");
println("-e: execute the following expression given on the command line");
println("-h: print this help message and exit");
println("-i: run interactive REPL (default when no script file is specified)");
}

void printStackTrace (QS::RuntimeException& e) {
println(std::cerr, "ERROR: %s", e.what());
println(std::cerr, "%s", e.getStackTraceAsString());
}

void repl (QS::VM& vm, QS::Fiber& fiber) {
vm.setOption(QS::VM::Option::VAR_DECL_MODE, QS::VM::Option::VAR_IMPLICIT_GLOBAL);
string code, line;
printIntro();
println("Type 'exit', 'quit' or press Ctrl+Z to quit");
print(">>>");
while(getline(cin, line)) {
if (line=="exit" || line=="quit") break;
code += line + "\n";
try {
fiber.loadString(code, "REPL");
fiber.call(0);
if (!fiber.isNull(-1)) {
fiber.callMethod("toString", 1);
cout << fiber.getCString(-1) << endl;
}
fiber.pop();
vm.garbageCollect();
code.clear();
} 
catch (QS::CompilationException& ce) {
if (!ce.isIncomplete()) code.clear();
}
catch (QS::RuntimeException& e) {
printStackTrace(e);
code.clear();
}
print(code.empty()? ">>>" : " ...");
}}


int main (int argc, char** argv) {
string inFile, outFile, expression;
bool runREPL=false, compileOnly=false;
int argIndex=1;
while(argIndex<argc) {
string arg = argv[argIndex++];
if (arg=="-c") compileOnly=true;
else if (arg=="-e") expression = argv[argIndex++];
else if (arg=="-h" || arg=="--help" || arg=="-?") { printHelp(argv[0]); return 0; }
else if (arg=="-i") runREPL=true;
else if (arg=="-o") outFile = argv[argIndex++];
else if (inFile.empty()) inFile = arg;
else break;
}
if (inFile.empty() && expression.empty()) runREPL=true;

try {
QS::VM& vm = *QS::VM::createVM();
QS::Fiber& fiber = *vm.createFiber();

registerCLIEnv(fiber);

if (!expression.empty()) {
fiber.loadString(expression, "<cmdLine>");
fiber.call(0);
vm.garbageCollect();
}

if (!inFile.empty()) {
fiber.loadFile(inFile);
if (compileOnly) cerr << "Compilation to bytecode file isn't yet supported." << endl;
else fiber.call(0);
vm.garbageCollect();
}

if (runREPL) repl(vm, fiber);

} 
catch (QS::RuntimeException& e) {
printStackTrace(e);
}
catch (std::exception& ex) {
println(std::cerr, "Exception caught: %s: %s", typeid(ex).name(), ex.what());
} catch (...) {
println(std::cerr, "Caught unknown exception !");
}
return 0;
}
