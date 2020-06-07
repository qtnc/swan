#ifndef ___COMPILER_PARSER_FUNCTION_INFO
#define ___COMPILER_PARSER_FUNCTION_INFO
#include "StatementBase.hpp"
//#include "TypeInfo.hpp"
#include "../../include/bitfield.hpp"
#include<memory>
#include<string>

struct QCompiler;
struct TypeAnalyzer;
struct TypeInfo;

struct StringFunctionInfo: FunctionInfo {
struct QVM& vm;
std::vector<std::shared_ptr<TypeInfo>> types;
bitmask<VarFlag> flags;
int nArgs, retArg, retCompArg, fieldIndex;

StringFunctionInfo (TypeAnalyzer& ta, const char* typeInfoStr);
void build (TypeAnalyzer&  ta, const char* str);
std::shared_ptr<TypeInfo> readNextTypeInfo (TypeAnalyzer&  ta, const char*& str);
std::shared_ptr<TypeInfo> getReturnTypeInfo (int nPassedArgs=0, std::shared_ptr<TypeInfo>* passedArgs = nullptr) override;
std::shared_ptr<TypeInfo> getArgTypeInfo (int n, int nPassedArgs = 0, std::shared_ptr<TypeInfo>* passedArgs = nullptr) override;
int getArgCount () override { return nArgs; }
std::shared_ptr<TypeInfo> getFunctionTypeInfo (int nPassedArgs = 0, std::shared_ptr<TypeInfo>* passedArgs = nullptr) override;
int getFlags () override { return flags.value; }
int getFieldIndex () override { return fieldIndex; }
virtual ~StringFunctionInfo () = default;
};

#endif
