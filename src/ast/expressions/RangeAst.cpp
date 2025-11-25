#include "ast/expressions/RangeAst.h"

namespace gazprea::ast::expressions {

NodeType RangeAst::getNodeType() const { return NodeType::Range; }

std::string RangeAst::toStringTree(std::string prefix) const {
  std::string result = prefix + "Range (..):\n";
  if (start) {
    result += start->toStringTree(prefix + indent);
  }
  if (end) {
    result += end->toStringTree(prefix + indent);
  }
  return result;
}

} // namespace gazprea::ast::expressions