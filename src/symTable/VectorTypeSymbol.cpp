#include <symTable/VectorTypeSymbol.h>

namespace gazprea::symTable {
std::string VectorTypeSymbol::getName() { return Symbol::getName() + "<" + type->getName() + ">"; }
std::string VectorTypeSymbol::toString() { return Symbol::toString(); }
} // namespace gazprea::symTable