#include "../include/Swan.hpp"
#include "../include/cpprintf.hpp"
#include<fstream>
#include<string>
#include<cstring>
using namespace std;

Swan::VM& createVM ();

int main (int argc, char** argv) {
string buffer;

{
int32_t length = 0;
char magic[5] = {0};
ifstream in(argv[0], ios::binary);
in.seekg(-8, ios::end);
in.read(reinterpret_cast<char*>(&length), 4);
in.read(magic, 4);
if (strncmp(magic, "Swan", 4)) return 4;
buffer.reserve(length+1);
buffer.resize(length);
in.seekg(-length-8, ios::end);
in.read(const_cast<char*>(buffer.data()), length);
}
if (buffer.empty()) return 4;

Swan::VM& vm = createVM();
Swan::Fiber& fiber = vm.getActiveFiber();

fiber.loadGlobal("List");
for (int i=0; i<argc; i++) fiber.pushString(argv[i]);
fiber.call(argc);
fiber.storeGlobal("argv");

int count = fiber.loadString(buffer, "<main>");
while(count--) {
fiber.call(0);
if (count) fiber.pop();
}

int exitCode = 0;
if (fiber.isNum(-1)) exitCode = fiber.getNum(-1);

vm.destroy();
return exitCode;
}
