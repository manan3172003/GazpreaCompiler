#include "Colors.h"
#include "symTable/BuiltInTypeSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/TypealiasSymbol.h"

#include <symTable/VariableSymbol.h>

namespace gazprea::symTable {
std::string VariableSymbol::getName() { return Symbol::getName(); }
std::string VariableSymbol::toString() {
  std::stringstream ss;
  ss << "<" << getName() << ":";

  if (type) {
    // Try to cast to different type symbols to use their custom toString
    // methods
    if (auto tupleType = std::dynamic_pointer_cast<TupleTypeSymbol>(type)) {
      ss << tupleType->toString();
    } else if (auto aliasType = std::dynamic_pointer_cast<TypealiasSymbol>(type)) {
      ss << KGRN << aliasType->getName() << RST;
    } else if (auto structType = std::dynamic_pointer_cast<StructTypeSymbol>(type)) {
      ss << structType->toString();
    } else if (auto builtInType = std::dynamic_pointer_cast<BuiltInTypeSymbol>(type)) {
      ss << KGRN << builtInType->getName() << RST;
    } else {
      // Fallback to getName for other types, shown as resolved
      ss << KGRN << type->getName() << RST;
    }
  } else {
    // Unresolved type
    ss << KRED << "unresolved" << RST;
  }
  ss << ">";
  ss << scopeToString();
  return ss.str();
}
} // namespace gazprea::symTable