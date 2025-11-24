#include "ast/expressions/StringLiteralAst.h"

namespace gazprea::ast::expressions {
NodeType StringLiteralAst::getNodeType() const { return NodeType::StringLiteral; }

std::string StringLiteralAst::toStringTree(std::string prefix) const {
  return prefix + "StringLiteral: " + value + "\n";
}
} // namespace gazprea::ast::expressions