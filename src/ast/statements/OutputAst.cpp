#include <ast/statements/OutputAst.h>

namespace gazprea::ast::statements {
NodeType OutputAst::getNodeType() const { return NodeType::Output; }
std::string OutputAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Output " << "\n";
  ss << expr->toStringTree(prefix + indent);
  return ss.str();
}
void OutputAst::setExpression(
    std::shared_ptr<expressions::ExpressionAst> expr) {
  this->expr = std::move(expr);
}
std::shared_ptr<expressions::ExpressionAst> OutputAst::getExpression() const {
  return this->expr;
}
} // namespace gazprea::ast::statements