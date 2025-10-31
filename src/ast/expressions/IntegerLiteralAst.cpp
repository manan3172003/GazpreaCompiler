#include <ast/expressions/IntegerLiteralAst.h>

namespace gazprea::ast::expressions {

NodeType IntegerLiteralAst::getNodeType() const { return NodeType::IntegerLiteral; }
std::string IntegerLiteralAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Integer: " << std::to_string(integerValue) << "\n";
  return ss.str();
}

} // namespace gazprea::ast::expressions