#include <symTable/BuiltInTypeSymbol.h>

namespace gazprea::symTable {
std::string BuiltInTypeSymbol::getName() { return Symbol::getName(); }
std::string BuiltInTypeSymbol::toString() { return Symbol::toString(); }
} // namespace gazprea::symTable