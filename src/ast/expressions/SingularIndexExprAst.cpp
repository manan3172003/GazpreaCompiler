#include "ast/expressions/SingularIndexExprAst.h"

namespace gazprea::ast::expressions {
NodeType SingularIndexExprAst::getNodeType() const { return NodeType::SingularIndexExpr; }
std::string SingularIndexExprAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix + "[\n";
  ss << singularIndexExpr->toStringTree(indent + prefix);
  ss << prefix + "]\n";
  return ss.str();
}

} // namespace gazprea::ast::expressions