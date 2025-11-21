#include <ast/expressions/ArrayLiteralAst.h>

namespace gazprea::ast::expressions {
void ArrayLiteralAst::addElement(std::shared_ptr<ExpressionAst> element) {
  elements.push_back(element);
}
NodeType ArrayLiteralAst::getNodeType() const { return NodeType::ArrayLiteral; }
std::string ArrayLiteralAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "ArrayLiteral: (\n";
  for (const auto &element : elements) {
    ss << element->toStringTree(prefix + indent);
  }
  ss << prefix << ")\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions