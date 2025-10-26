#include <ast/expressions/CharLiteralAst.h>

namespace gazprea::ast::expressions {
NodeType CharLiteralAst::getNodeType() const { return NodeType::CharLiteral; }
std::string CharLiteralAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "CharLiteral: " << value << "\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions