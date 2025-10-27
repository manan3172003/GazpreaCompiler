#include "Colors.h"

#include <sstream>
#include <symTable/TupleTypeSymbol.h>

namespace gazprea::symTable {
std::string TupleTypeSymbol::getName() { return Symbol::getName(); }
std::string TupleTypeSymbol::toString() {
  std::stringstream ss;
  ss << "TupleType( ";
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