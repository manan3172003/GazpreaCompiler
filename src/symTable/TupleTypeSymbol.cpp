#include "Colors.h"

#include <sstream>
#include <symTable/TupleTypeSymbol.h>

namespace gazprea::symTable {
std::string TupleTypeSymbol::getName() { return "tuple"; }
std::string TupleTypeSymbol::toString() {
  std::stringstream ss;
  ss << "TupleType( ";

  // Show resolved types in green
  for (size_t i = 0; i < resolvedTypes.size(); ++i) {
    ss << KGRN << resolvedTypes[i]->getName() << RST;
    if (i < resolvedTypes.size() - 1 || !unresolvedTypes.empty()) {
      ss << ", ";
    }
  }

  // Show unresolved types in red
  for (size_t i = 0; i < unresolvedTypes.size(); ++i) {
    ss << KRED << unresolvedTypes[i] << RST;
    if (i < unresolvedTypes.size() - 1) {
      ss << ", ";
    }
  }

  ss << " )";
  return ss.str();
}
} // namespace gazprea::symTable