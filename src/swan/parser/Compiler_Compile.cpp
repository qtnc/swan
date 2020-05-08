#include "Compiler.hpp"
#include "Statement.hpp"
#include "Expression.hpp"
#include "ParserRules.hpp"
#include "TypeAnalyzer.hpp"
#include "../vm/VM.hpp"
using namespace std;

static shared_ptr<Statement> addReturnExports (shared_ptr<Statement> sta) {
if (!sta->isUsingExports()) return sta;
QToken exportsToken = { T_NAME, EXPORTS, 7, QV::UNDEFINED};
QToken exportMTToken = { T_LEFT_BRACE, EXPORTS, 7, QV::UNDEFINED};
auto exn = make_shared<NameExpression>(exportsToken);
auto exs = make_shared<ReturnStatement>(exportsToken, exn);
vector<shared_ptr<Variable>> exv = { make_shared<Variable>(exn, make_shared<LiteralMapExpression>(exportMTToken), VD_CONST) }; 
auto exvd = make_shared<VariableDeclaration>(exv);
auto bs = dynamic_pointer_cast<BlockStatement>(sta);
if (bs) {
bs->statements.insert(bs->statements.begin(), exvd);
bs->statements.push_back(exs);
}
else bs = make_shared<BlockStatement>(vector<shared_ptr<Statement>>({ exvd, sta, exs }));
return bs;
}

static void analyze (QCompiler& compiler, shared_ptr<Statement>& sta) {
if (!compiler.globalAnalyzer) compiler.globalAnalyzer = compiler.parent? compiler.parent->globalAnalyzer : make_shared<TypeAnalyzer>(compiler.parser);
if (!compiler.analyzer) compiler.analyzer = make_shared<TypeAnalyzer>(compiler.parser, compiler.parent? compiler.parent->analyzer.get() : compiler.globalAnalyzer.get());
int count = 0;
while (++count<6 && sta->analyze(*compiler.analyzer));
if (count>=3) println("Analyzer count = %d", count);
}

void QCompiler::compile () {
shared_ptr<Statement> sta = parser.parseStatements();
if (sta && !parser.result) {
sta = addReturnExports(sta);
sta=sta->optimizeStatement();
analyze(*this, sta);
sta=sta->optimizeStatement();
sta->compile(*this);
if (constants.size() >= std::numeric_limits<uint_constant_index_t>::max()) compileError(sta->nearestToken(), "Too many constant values");
if (vm.methodSymbols.size()  >= std::numeric_limits<uint_method_symbol_t>::max()) compileError(sta->nearestToken(), "Too many method symbols");
if (vm.globalVariables.size()  >= std::numeric_limits<uint_global_symbol_t>::max()) compileError(sta->nearestToken(), "Too many global variables");
// Implicit return last expression
if (sta->isExpression()) {
writeOp(OP_RETURN);
}
else if (lastOp==OP_POP) {
seek(-1);
writeOp(OP_RETURN);
}
else if (lastOp!=OP_RETURN) {
writeOp(OP_LOAD_UNDEFINED);
writeOp(OP_RETURN);
}
}
}

QFunction* QCompiler::getFunction (int nArgs) {
compile();

string bc = out.str();
size_t nConsts = constants.size(), nUpvalues = upvalues.size(), bcSize = bc.size(), dbgi = debugItems.size();

QFunction* function = QFunction::create(vm, nArgs, nConsts, nUpvalues, bcSize, dbgi);
copy(make_move_iterator(constants.begin()), make_move_iterator(constants.end()), function->constants);
copy(make_move_iterator(upvalues.begin()), make_move_iterator(upvalues.end()), function->upvalues);
copy(make_move_iterator(debugItems.begin()), make_move_iterator(debugItems.end()), function->debugItems);
memmove(function->bytecode, bc.data(), bcSize);
function->file = parser.filename;
result = result? result : parser.result;
return function;
}


