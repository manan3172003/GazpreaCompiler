#include "Colors.h"
#include "symTable/BuiltInTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"

#include <sstream>
#include <symTable/TypealiasSymbol.h>

namespace gazprea::symTable {
std::string TypealiasSymbol::toString() {
  std::stringstream ss;
  ss << "<TypeAlias " << getName() << ": ";

  // If type is resolved, show it in green with details
  if (type) {
    // For TupleTypeSymbol, show the full tuple representation
    if (auto tupleType = std::dynamic_pointer_cast<TupleTypeSymbol>(type)) {
      ss << tupleType->toString();
    }
    // For other resolved types, show in green
    else {
      ss << KGRN << type->getName() << RST;
    }
  } else {
    // Unresolved type shown in red
    ss << KRED << "unresolved" << RST;
  }

  ss << ">";
  return ss.str();
}
} // namespace gazprea::symTable
