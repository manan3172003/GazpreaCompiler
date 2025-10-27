#include "symTable/TupleTypeSymbol.h"

#include <symTable/VariableSymbol.h>

namespace gazprea::symTable {
std::string VariableSymbol::getName() { return Symbol::getName(); }
std::string VariableSymbol::toString() {
  std::stringstream ss;
  if (type) {
    const auto castedType = std::dynamic_pointer_cast<TupleTypeSymbol>(type);
    ss << "<" << getName() << ": " << castedType->toString() << ">";
  } else {
    ss << "<" << getName() << ": unknown>";
  }
  return ss.str();
}
} // namespace gazprea::symTable