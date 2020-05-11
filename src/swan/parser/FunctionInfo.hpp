#ifndef ___COMPILER_PARSER_FUNCTION_INFO
#define ___COMPILER_PARSER_FUNCTION_INFO
#include "StatementBase.hpp"
#include "TypeInfo.hpp"
#include<memory>
#include<string>

struct QCompiler;
struct TypeAnalyzer;

struct StringFunctionInfo: FunctionInfo {
struct QVM& vm;
std::vector<std::shared_ptr<TypeInfo>> types;
int nArgs, retArg, retCompArg, flags, fieldIndex;

StringFunctionInfo (TypeAnalyzer& ta, const char* typeInfoStr);
void build (TypeAnalyzer&  ta, const char* str);
std::shared_ptr<TypeInfo> readNextTypeInfo (TypeAnalyzer&  ta, const char*& str);
std::shared_ptr<TypeInfo> getReturnTypeInfo (int nArgs=0, std::shared_ptr<TypeInfo>* ptr = nullptr) override;
std::shared_ptr<TypeInfo> getArgTypeInfo (int n) override;
int getArgCount () override { return nArgs; }
std::shared_ptr<TypeInfo> getFunctionTypeInfo (int nArgs = 0, std::shared_ptr<TypeInfo>* ptr = nullptr) override;
int getFlags () override { return flags; }
int getFieldIndex () override { return fieldIndex; }
virtual ~StringFunctionInfo () = default;
};

#endif
