#include "../vm/Value.hpp"
#include "../vm/VM.hpp"
#include "../vm/OpCodeInfo.hpp"
#include "../../include/cpprintf.hpp"
#include<unordered_set>
#include<algorithm>
#include<sstream>
using namespace std;

static void writeQVBytecode (QV v, ostream& out, unordered_set<void*>& references);
static QV readQVBytecode (QVM& vm, istream& in, unordered_map<uintptr_t, QObject*>& references, unordered_map<int,int>& globalTable, unordered_map<int,int>& methodTable);

template<class T> static inline void write (ostream& out, const T& x) {
out.write(reinterpret_cast<const char*>(&x), sizeof(T));
}

template<class T> static inline T read (istream& in) {
T x;
in.read(reinterpret_cast<char*>(&x), sizeof(T));
return x;
}

template<class T> static inline void translateSymbol (T& symbol, unordered_map<int,int>& table) {
T orig = symbol;
auto it = table.find(symbol);
if (it!=table.end()) symbol = it->second;
}

static void writeVLN (ostream& out, size_t x) {
int i=0;
uint8_t buf[8] = {0};
do {
buf[7-i++] |= (x&0x7F);
buf[7-i] = 0x80;
x>>=7;
} while(x>0);
out.write(reinterpret_cast<const char*>(buf+8-i), i);
}

static size_t readVLN (istream& in) {
size_t x = 0;
uint8_t u = 0;
do {
in.read(reinterpret_cast<char*>(&u),1);
x = (x<<7) | (u&0x7F);
} while (u>=0x80);
return x;
}

static inline void writeString (ostream& out, const string& s) {
writeVLN(out, s.size());
out.write(s.data(), s.size());
}

static inline string readString (istream& in) {
size_t length = readVLN(in);
string s(length, '\0');
in.read(const_cast<char*>(s.data()), length);
return s;
}

static void writeSymbolTable (ostream& out, const vector<string>& table, int offset=0) {
writeVLN(out, offset);
writeVLN(out, table.size());
for (auto& s: table) writeString(out, s);
}

static void readSymbolTable (istream& in, unordered_map<int,int>& map, vector<string>& table) {
int offset = readVLN(in);
int length = readVLN(in);
for (int i=offset; i<length; i++) {
string s = readString(in);
auto it = find(table.begin(), table.end(), s);
if (it==table.end()) {
map[i] = table.size();
table.push_back(s);
}
else map[i] = it-table.begin();
}}

static void writeFunctionBytecode (QFunction& func, ostream& out, unordered_set<void*>& references) {
if (references.find(&func)!=references.end()) {
out << 'R';
writeVLN(out, reinterpret_cast<uintptr_t>(&func));
return;
}
references.insert(&func);
out << 'F';
writeVLN(out, reinterpret_cast<uintptr_t>(&func));
write(out, func.nArgs);
write<uint8_t>(out, func.vararg);
writeString(out, func.file);
writeString(out, func.name);
writeVLN(out, func.constants.size());
for (auto& cst: func.constants) writeQVBytecode(cst, out, references);
writeVLN(out, func.upvalues.size());
for (auto& upv: func.upvalues) {
writeVLN(out, upv.slot);
write<uint8_t>(out, upv.upperUpvalue);
}
writeString(out, func.bytecode);
//println("%s:%s: %d args, %d constants, %d upvalues, %d bytes BC length", func.file, func.name, static_cast<int>(func.nArgs), func.constants.size(), func.upvalues.size(), func.bytecode.size());
}

static QV readFunctionBytecode (QVM& vm, istream& in, unordered_map<uintptr_t, QObject*>& references, unordered_map<int,int>& globalTable, unordered_map<int,int>& methodTable) {
QFunction& func = *new QFunction(vm);
references[readVLN(in)] = &func;
func.nArgs = read<uint8_t>(in);
func.vararg = read<uint8_t>(in);
func.file = readString(in);
func.name = readString(in);
int nConsts = readVLN(in);
func.constants.reserve(nConsts);
for (int i=0; i<nConsts; i++) func.constants.push_back(readQVBytecode(vm, in, references, globalTable, methodTable));
int nUpvalues = readVLN(in);
func.upvalues.reserve(nUpvalues);
for (int i=0; i<nUpvalues; i++) {
int slot = readVLN(in);
bool upper = read<uint8_t>(in);
func.upvalues.push_back({ slot, upper });
}
func.bytecode = readString(in);
//println("%s:%s: %d args, %d constants, %d upvalues, %d bytes BC length", func.file, func.name, static_cast<int>(func.nArgs), func.constants.size(), func.upvalues.size(), func.bytecode.size());
for (const char *bc = func.bytecode.data(), *end = func.bytecode.data()+func.bytecode.length(); bc<end; ) {
uint8_t op = *bc++;
switch(op) {
case OP_LOAD_GLOBAL:
case OP_STORE_GLOBAL:
translateSymbol(*reinterpret_cast<uint_global_symbol_t*>(const_cast<char*>(bc)), globalTable);
break;
case OP_LOAD_METHOD:
case OP_STORE_METHOD:
case OP_STORE_STATIC_METHOD:
#define C(N) case OP_CALL_METHOD_##N: \
case OP_CALL_SUPER_##N: 
C(0) C(1) C(2) C(3) C(4) C(5) C(6) C(7) C(8)
C(9) C(10) C(11) C(12) C(13) C(14) C(15)
#undef C
case OP_CALL_METHOD:
case OP_CALL_SUPER:
case OP_CALL_METHOD_VARARG:
case OP_CALL_SUPER_VARARG:
translateSymbol(*reinterpret_cast<uint_method_symbol_t*>(const_cast<char*>(bc)), methodTable);
break;
}
bc += OPCODE_INFO[op].nArgs;
}
return QV(&func, QV_TAG_NORMAL_FUNCTION);
}

static void writeClosureBytecode (QClosure& closure, ostream& out, unordered_set<void*>& references) {
if (closure.func.upvalues.size()>0) throw std::logic_error("Couldn't save closures with upvalues");
if (references.find(&closure)!=references.end()) {
out << 'Q';
writeVLN(out, reinterpret_cast<uintptr_t>(&closure));
return;
}
references.insert(&closure);
out << 'C';
writeVLN(out, reinterpret_cast<uintptr_t>(&closure));
writeQVBytecode(QV(&closure.func, QV_TAG_NORMAL_FUNCTION), out, references);
}

static QV readClosureBytecode (QVM& vm, istream& in, unordered_map<uintptr_t, QObject*>& references, unordered_map<int,int>& globalTable, unordered_map<int,int>& methodTable) {
size_t ref = readVLN(in);
QFunction& func = *readQVBytecode(vm, in, references, globalTable, methodTable).asObject<QFunction>();
QClosure* closure = newVLS<QClosure, Upvalue*>(func.upvalues.size(), vm, func);
references[ref] = closure;
return QV(closure, QV_TAG_CLOSURE);
}

static void writeQVBytecode (QV v, ostream& out, unordered_set<void*>& references) {
if (v.isNull()) out << 'E';
else if (v.isTrue()) out << '1';
else if (v.isFalse()) out << '0';
else if (v.isNum()) {
int x = static_cast<int>(v.d);
if (x==v.d) { out << (x<0? '-' : '+'); writeVLN(out, x<0? -x : x); }
else { out << 'D'; write(out, v.d); }
}
else if (v.isString()) {
QString& s = *v.asObject<QString>();
out << 'S';
writeVLN(out, s.length);
out.write(s.begin(), s.length);
}
else if (v.isGenericSymbolFunction()) {
uint_method_symbol_t x = v.asInt();
out << 'G';
writeVLN(out, x);
}
else if (v.isNormalFunction()) writeFunctionBytecode(*v.asObject<QFunction>(), out, references);
else if (v.isClosure()) writeClosureBytecode(*v.asObject<QClosure>(), out, references);
else if (v.isNativeFunction()) throw std::logic_error("Couldn't save native function");
else throw std::logic_error(format("Couldn't save an object %s@%p", v.asObject<QObject>()->type->name, v.i));
}

static QV readQVBytecode (QVM& vm, istream& in, unordered_map<uintptr_t, QObject*>& references, unordered_map<int,int>& globalTable, unordered_map<int,int>& methodTable) {
char c;
in.read(&c,1);
switch(c){
case 'E': return QV();
case '+': return QV(static_cast<double>(readVLN(in)));
case '-': return QV(static_cast<double>(-static_cast<int64_t>(readVLN(in))));
case 'D': return read<double>(in);
case '1': return true;
case '0': return false;
case 'S': {
string s = readString(in);
return QV(QString::create(vm, s), QV_TAG_STRING);
}
case 'G': {
uint_method_symbol_t symbol = readVLN(in);
translateSymbol(symbol, methodTable);
return QV(symbol  | QV_TAG_GENERIC_SYMBOL_FUNCTION);
}
case 'F': return readFunctionBytecode(vm, in, references, globalTable, methodTable);
case 'C': return readClosureBytecode(vm, in, references, globalTable, methodTable);
case 'R': return QV(references[readVLN(in)], QV_TAG_NORMAL_FUNCTION);
case 'Q': return QV(references[readVLN(in)], QV_TAG_CLOSURE);
default: throw std::logic_error(format("Unknown type specifier '%c'", c));
}}

void QFiber::saveBytecode (ostream& out, int count) {
unordered_set<void*> references;
out.write("\x1B\x01", 2);
writeSymbolTable(out, vm.globalSymbols);
writeSymbolTable(out, vm.methodSymbols);
writeVLN(out, count);
for (int i=0; i<count; i++) {
writeQVBytecode(at(i-count), out, references);
}}

int QFiber::loadBytecode (istream& in) {
char magic[2];
in.read(magic, 2);
unordered_map<int,int> globals, methods;
unordered_map<uintptr_t, QObject*> references;
readSymbolTable(in, globals, vm.globalSymbols);
readSymbolTable(in, methods, vm.methodSymbols);
size_t count = readVLN(in);
for (size_t i=0; i<count; i++) {
push(readQVBytecode(vm, in, references, globals, methods));
}
return count;
}

void QFiber::dumpBytecode (std::ostream& out, int count) {
saveBytecode(out, count);
}
