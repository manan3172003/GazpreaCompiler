#include "Colors.h"
#include "symTable/BuiltInTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/TypealiasSymbol.h"

#include <sstream>
#include <symTable/MethodSymbol.h>

namespace gazprea::symTable {
std::string MethodSymbol::toString() {
  std::stringstream ss;
  ss << scTypeToString() << " (";
  for (size_t i = 0; i < getSymbols().size(); ++i) {
    const auto &[name, symbol] = getSymbols()[i];
    ss << symbol->toString();
    if (i < getSymbols().size() - 1) {
      ss << ", ";
    }
  }
  ss << ") -> ";

  if (returnType) {
    if (const auto tupleType =
            std::dynamic_pointer_cast<TupleTypeSymbol>(returnType)) {
      ss << tupleType->toString();
    } else if (const auto aliasType =
                   std::dynamic_pointer_cast<TypealiasSymbol>(returnType)) {
      ss << KGRN << aliasType->getName() << RST;
    } else if (const auto builtInType =
                   std::dynamic_pointer_cast<BuiltInTypeSymbol>(returnType)) {
      ss << KGRN << builtInType->getName() << RST;
    } else {
      ss << KGRN << returnType->getName() << RST;
    }
  } else {
    if (getScopeType() == ScopeType::Procedure) {
      ss << KGRN << "void" << RST;
      return ss.str();
    }
    ss << KRED << "unresolved" << RST;
  }
  return ss.str();
}
} // namespace gazprea::symTable