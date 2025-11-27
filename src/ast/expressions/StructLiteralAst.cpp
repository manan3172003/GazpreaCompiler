#include "ast/expressions/StructLiteralAst.h"

namespace gazprea::ast::expressions {

void StructLiteralAst::addElement(const std::shared_ptr<ExpressionAst> &element) {
  elements.push_back(element);
}
void StructLiteralAst::setStructTypeName(const std::string &name) { structTypeName = name; }

std::string StructLiteralAst::getStructTypeName() const { return structTypeName; }
std::vector<std::shared_ptr<ExpressionAst>> StructLiteralAst::getElements() const {
  return elements;
}
NodeType StructLiteralAst::getNodeType() const { return NodeType::StructLiteral; }
std::string StructLiteralAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "StructLiteral: " << structTypeName << "(\n";
  for (const auto &element : elements) {
    ss << element->toStringTree(prefix + indent);
  }
  ss << prefix << ")\n";
  return ss.str();
}
bool StructLiteralAst::isLValue() { return false; }

} // namespace gazprea::ast::expressions