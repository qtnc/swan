#include "../include/Swan.hpp"
#include "../include/cpprintf.hpp"
#include<optional>
#include<fstream>
#include<sstream>
#include<typeinfo>
#include<boost/core/demangle.hpp>
#include<boost/algorithm/string.hpp>
using namespace std;
using boost::starts_with;

void printStackTrace (Swan::RuntimeException& e);
Swan::VM& createVM ();

static void printIntro () {
println("Swan version %s", Swan::VM::getVersionString());
println("Copyright (c) 2019, QuentinC ");
println("For more info, go to http://github.com/qtnc/swan");
}

static void printHelp (const std::string& argv0) {
printIntro();
println("Synopsis: %s [options] [script] [args...]", argv0);
println("Where [script] is the script to execute");
println("And where [options] can be: ");
println("-c:  compile script but don't run it");
println("-e:  execute the following expression given on the command line");
println("-g:  save debug information when compiling");
println("-g0: don't save debug information when compiling");
println("-h:  print this help message and exit");
println("-i:  run interactive REPL (default when no script file is specified)");
println("-m:  import the following given module");
println("-o:  output compiled bytecode to specified file");
println("-x=image: generate a standalone executable by using the given image");
}

static void printCLIHelp () {
println("Type any Swan code to see the result, or one of these commands: ");
println("clear:\t clear a possibly incomplete code input buffer");
println("exit:\t exits from the Swan CLI REPL");
println("quit:\t synonym for 'exit'");
}

static void replEval (Swan::Fiber& fiber, string& code, const string& line) {
code.append(line);
code.push_back('\n');
try {
Swan::ScopeLocker<Swan::VM>  locker(fiber.getVM());
fiber.loadString(code, "<REPL>");
fiber.call(0);
if (!fiber.isUndefined(-1)) {
fiber.callMethod("toString", 1);
cout << fiber.getCString(-1) << endl;
}
fiber.pop();
code.clear();
} 
catch (Swan::CompilationException& ce) {
if (!ce.isIncomplete()) code.clear();
}
catch (Swan::RuntimeException& e) {
printStackTrace(e);
code.clear();
}
}

static void repl (Swan::VM& vm, Swan::Fiber& fiber) {
Swan::ScopeUnlocker<Swan::VM> unlocker(vm);
string code, line;
printIntro();
println("Type 'exit', 'quit' or press Ctrl+Z to quit; type 'help' for other commands.");
print("?>>");
while(getline(cin, line)) {
if (line=="exit" || line=="quit") break;
else if (line=="clear") code.clear(); 
else if (line=="help") printCLIHelp(); 
else replEval(fiber, code, line);
print(code.empty()? "?>>" : " ?..");
line.clear();
}}

int main (int argc, char** argv) {
vector<string> args, importModules;
string inFile, outFile, expression, toExeImage;
bool runREPL=false, compileOnly=false;
optional<bool> compileDbgInfo;
int argIndex=1, exitCode=0;
while(argIndex<argc) {
string arg = argv[argIndex++];
if (arg=="-c") compileOnly=true;
else if (arg=="-e") expression = argv[argIndex++];
else if (arg=="-g") compileDbgInfo=true;
else if (arg=="-g0") compileDbgInfo=false;
else if (arg=="-h" || arg=="--help" || arg=="-?") { printHelp(argv[0]); return 0; }
else if (arg=="-i") runREPL=true;
else if (arg=="-m") importModules.push_back(argv[argIndex++]);
else if (arg=="-o") outFile = argv[argIndex++];
else if (starts_with(arg, "-x=")) toExeImage = arg.substr(3);
else if (starts_with(arg, "-")) println(std::cerr, "Warning: unknown option: %s", arg);
else { 
if (inFile.empty()) inFile=arg;
else if (compileOnly && outFile.empty()) outFile = arg;
else println(std::cerr, "Warning: unused extra argument: %s", arg);
if (!compileOnly) break; 
}}
while(argIndex<argc) args.push_back(argv[argIndex++]);

if (!compileOnly && inFile.empty() && expression.empty()) runREPL=true;

try {
Swan::VM& vm = createVM();
Swan::Fiber& fiber = vm.getActiveFiber();

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

if (compileDbgInfo.has_value()) {
fiber.getVM().setOption(Swan::VM::Option::COMPILATION_DEBUG_INFO, compileDbgInfo.value());
}

if (!expression.empty()) {
fiber.loadString(expression, "<cmdline>");
fiber.call(0);
if (!fiber.isUndefined(-1)) {
fiber.callMethod("toString", 1);
cout << fiber.getCString(-1) << endl;
}}

if (!compileOnly && !inFile.empty()) {
if (inFile=="-") {
ostringstream out(ios::binary);
out << std::cin.rdbuf();
fiber.loadString(out.str(), "<stdin>");
fiber.call(0);
}
else fiber.import("", inFile);
exitCode = fiber.getOptionalNum(-1, 0);
fiber.pop();
}

if (compileOnly && !inFile.empty()) {
if (outFile.empty()) outFile = inFile + ".sb";
ofstream out(outFile, ios::binary);
if (toExeImage.size()) {
ifstream eIn(toExeImage, ios::binary);
out << eIn.rdbuf();
}
auto pos = out.tellp();
fiber.importAndDumpBytecode("", inFile, out);
if (toExeImage.size()) {
uint32_t length = out.tellp() -pos;
out.write(reinterpret_cast<char*>(&length), 4);
out.write("Swan", 4);
}}

if (runREPL && !compileOnly) {
vm.setOption(Swan::VM::Option::VAR_DECL_MODE, Swan::VM::Option::VAR_IMPLICIT_GLOBAL);
repl(vm, fiber);
}

vm.destroy();
} 
catch (Swan::RuntimeException& e) {
printStackTrace(e);
exitCode = 3;
}
catch (std::exception& ex) {
boost::core::scoped_demangled_name exceptionType(typeid(ex).name());
println(std::cerr, "Exception caught: %s: %s", exceptionType.get(), ex.what());
exitCode = 3;
} catch (...) {
println(std::cerr, "Caught unknown exception !");
exitCode = 3;
}
return exitCode;
}
