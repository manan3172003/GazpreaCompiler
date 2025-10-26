#include <ast/expressions/BoolLiteralAst.h>

namespace gazprea::ast::expressions {
NodeType BoolLiteralAst::getNodeType() const { return NodeType::BoolLiteral; }
std::string BoolLiteralAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Bool: " << value << "\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions