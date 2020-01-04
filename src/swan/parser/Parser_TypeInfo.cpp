#include "Parser.hpp"
#include "TypeInfo.hpp"
using namespace std;
shared_ptr<TypeInfo> QParser::parseTypeInfo () {
consume(T_NAME, "Expected type name");
return make_shared<NamedTypeInfo>(cur);
}
