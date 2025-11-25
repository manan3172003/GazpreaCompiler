#include "ast/expressions/RangedIndexExprAst.h"

namespace gazprea::ast::expressions {

NodeType RangedIndexExprAst::getNodeType() const { return NodeType::RangedIndexExpr; }
std::string RangedIndexExprAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix + "[\n";
  if (leftIndexExpr)
    ss << leftIndexExpr->toStringTree(indent + prefix);
  ss << prefix + indent << "to\n";
  if (rightIndexExpr)
    ss << rightIndexExpr->toStringTree(indent + prefix);
  ss << prefix + "]\n";
  return ss.str();
}

} // namespace gazprea::ast::expressions