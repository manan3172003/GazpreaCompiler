#include <symTable/VariableSymbol.h>

namespace gazprea::symTable {
std::string VariableSymbol::getName() { return Symbol::getName(); }
std::string VariableSymbol::toString() { return Symbol::toString(); }
} // namespace gazprea::symTable