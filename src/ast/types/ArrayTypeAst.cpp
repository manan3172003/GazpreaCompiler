#include "ast/expressions/CharLiteralAst.h"

#include <ast/types/ArrayTypeAst.h>

namespace gazprea::ast::types {

NodeType ArrayTypeAst::getNodeType() const { return NodeType::ArrayType; }
std::vector<bool> ArrayTypeAst::isSizeInferred() const {
  std::vector<bool> result;
  for (const auto &sizeExpr : static_sizes) {
    if (sizeExpr->getNodeType() == NodeType::CharLiteral &&
        std::dynamic_pointer_cast<expressions::CharLiteralAst>(sizeExpr)->getValue() == '*') {
      result.push_back(true);
    } else
      result.push_back(false);
  }
  return result;
}
std::string ArrayTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "ArrayType(";
  if (type)
    ss << type->toStringTree(prefix + indent);
  ss << ")";
  return ss.str();
}
} // namespace gazprea::ast::types