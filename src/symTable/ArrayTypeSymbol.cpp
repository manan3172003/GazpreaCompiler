#include <symTable/ArrayTypeSymbol.h>

namespace gazprea::symTable {

std::string ArrayTypeSymbol::getName() { return Symbol::getName() + "<" + type->getName() + ">"; }
std::string ArrayTypeSymbol::toString() { return Symbol::toString(); }
} // namespace gazprea::symTable