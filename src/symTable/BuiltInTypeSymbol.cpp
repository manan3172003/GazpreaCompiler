#include "Colors.h"

#include <sstream>
#include <symTable/BuiltInTypeSymbol.h>

namespace gazprea::symTable {
std::string BuiltInTypeSymbol::getName() { return Symbol::getName(); }
std::string BuiltInTypeSymbol::toString() {
  std::stringstream ss;
  const std::string sname =
      (!getScope().expired()) ? getScope().lock()->getScopeName() + "::" : "";
  ss << '<' << sname << KGRN << getName() << RST << '>';
  return ss.str();
}
} // namespace gazprea::symTable