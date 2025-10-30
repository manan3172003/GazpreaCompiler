#include "ast/expressions/TupleLiteralAst.h"
namespace gazprea::ast::expressions {

void TupleLiteralAst::addElement(std::shared_ptr<ExpressionAst> element) {
  elements.push_back(element);
}

NodeType TupleLiteralAst::getNodeType() const { return NodeType::TupleLiteral; }

std::string TupleLiteralAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "TupleLiteral: (\n";
  for (const auto &element : elements) {
    ss << element->toStringTree(prefix + indent);
  }
  ss << prefix << ")\n";
  return ss.str();
}

} // namespace gazprea::ast::expressions