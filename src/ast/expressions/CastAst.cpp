#include "Colors.h"
#include "symTable/TupleTypeSymbol.h"

#include <ast/expressions/CastAst.h>

namespace gazprea::ast::expressions {
NodeType CastAst::getNodeType() const { return NodeType::Cast; }
std::string CastAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Cast (";
  if (targetType) {
    ss << KRED << targetType->toStringTree("") << RST;
  }
  if (resolvedTargetType) {
    const auto tupTypeSym =
        std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
            resolvedTargetType);
    if (tupTypeSym) {
      ss << ":" << KGRN << tupTypeSym->toString() << RST;
    } else {
      ss << ":" << KGRN << resolvedTargetType->getName() << RST;
    }
  }
  ss << "): \n";
  if (expr) {
    ss << expr->toStringTree(prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::expressions