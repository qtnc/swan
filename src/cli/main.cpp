#include "../include/Swan.hpp"
#include "../include/cpprintf.hpp"
#include<iostream>
#include<sstream>
#include<fstream>
#include<typeinfo>
#include<exception>
#include<cstring>
using namespace std;

void registerIO  (Swan::Fiber& f);
void registerDate (Swan::Fiber& f);

void printIntro () {
println("Swan version 0.1.2019.2");
println("Copyright (c) 2019, QuentinC ");
println("For more info, go to http://github.com/qtnc/qscript");
}

void printHelp (const std::string& argv0) {
printIntro();
println("Synopsis: %s [options] [script] [args...]", argv0);
println("Where [script] is the script to execute");
println("And where [options] can be: ");
println("-c: compile script but don't run it");
println("-e: execute the following expression given on the command line");
println("-h: print this help message and exit");
println("-i: run interactive REPL (default when no script file is specified)");
println("-m: import the following given module");
println("-o: output compiled bytecode to specified file");
}

void printStackTrace (Swan::RuntimeException& e) {
println(std::cerr, "ERROR: %s", e.what());
println(std::cerr, "%s", e.getStackTraceAsString());
}

void repl (Swan::VM& vm, Swan::Fiber& fiber) {
vm.setOption(Swan::VM::Option::VAR_DECL_MODE, Swan::VM::Option::VAR_IMPLICIT_GLOBAL);
string code, line;
printIntro();
println("Type 'exit', 'quit' or press Ctrl+Z to quit");
print("?>>");
while(getline(cin, line)) {
if (line=="exit" || line=="quit") break;
code.append(line);
code.push_back('\n');
try {
fiber.loadString(code, "REPL");
fiber.call(0);
if (!fiber.isNull(-1)) {
fiber.callMethod("toString", 1);
cout << fiber.getCString(-1) << endl;
}
fiber.pop();
//vm.garbageCollect();
code.clear();
} 
catch (Swan::CompilationException& ce) {
if (!ce.isIncomplete()) code.clear();
}
catch (Swan::RuntimeException& e) {
printStackTrace(e);
code.clear();
}
print(code.empty()? "?>>" : " ?..");
line.clear();
}}

int main (int argc, char** argv) {
vector<string> args, importModules;
string inFile, outFile, expression, outDir;
bool runREPL=false, compileOnly=false;
int argIndex=1, exitCode=0;
while(argIndex<argc) {
string arg = argv[argIndex++];
if (arg=="-c") compileOnly=true;
else if (arg=="-d") outDir = argv[argIndex++];
else if (arg=="-e") expression = argv[argIndex++];
else if (arg=="-h" || arg=="--help" || arg=="-?") { printHelp(argv[0]); return 0; }
else if (arg=="-i") runREPL=true;
else if (arg=="-m") importModules.push_back(argv[argIndex++]);
else if (arg=="-o") outFile = argv[argIndex++];
else if (arg=="--") break;
else if (!compileOnly && inFile.empty()) inFile = arg;
else {
argIndex -= compileOnly;
break;
}}
while(argIndex<argc) args.push_back(argv[argIndex++]);

if (!compileOnly && inFile.empty() && expression.empty()) runREPL=true;
if (compileOnly && !outFile.empty() && args.size()>1) {
println(std::cerr, "Warning: output file ignored when compliling multiple files at once");
outFile.clear();
}

try {
Swan::VM& vm = Swan::VM::getVM();
Swan::Fiber& fiber = vm.getActiveFiber();

registerIO(fiber);
registerDate(fiber);

for (auto& mod: importModules) {
fiber.import("", mod);
fiber.pop();
}

if (!compileOnly) {
fiber.loadGlobal("List");
for (auto& arg: args) fiber.pushString(arg);
fiber.call(args.size());
fiber.storeGlobal("argv");
}

if (!expression.empty()) {
fiber.loadString(expression, "<inline>");
fiber.call(0);
//vm.garbageCollect();
}

if (!inFile.empty()) {
if (inFile=="-") {
ostringstream out;
out << std::cin.rdbuf();
fiber.loadString(out.str(), "<stdin>");
}
else fiber.loadFile(inFile);
fiber.call(0);
exitCode = fiber.getOptionalNum(-1, 0);
fiber.pop();
//vm.garbageCollect();
}

if (compileOnly) for (auto& file: args) {
if (!outFile.empty())  file=outFile;
else file += ".qb";
ofstream out(file, ios::binary);
fiber.dumpBytecode(out);
}

if (runREPL) repl(vm, fiber);

} 
catch (Swan::RuntimeException& e) {
printStackTrace(e);
exitCode = 3;
}
catch (std::exception& ex) {
println(std::cerr, "Exception caught: %s: %s", typeid(ex).name(), ex.what());
exitCode = 3;
} catch (...) {
println(std::cerr, "Caught unknown exception !");
exitCode = 3;
}
return exitCode;
}
