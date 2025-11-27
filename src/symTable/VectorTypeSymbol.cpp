#include <sstream>
#include <symTable/VectorTypeSymbol.h>

namespace gazprea::symTable {
std::string VectorTypeSymbol::getName() {
  std::stringstream name;
  name << Symbol::getName() << "<" << type->getName();
  name << ">";
  return name.str();
}

std::string VectorTypeSymbol::toString() {
  std::stringstream result;
  result << Symbol::toString();
  return result.str();
}
} // namespace gazprea::symTable
