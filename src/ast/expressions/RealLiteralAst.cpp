#include <ast/expressions/RealLiteralAst.h>

namespace gazprea::ast::expressions {
NodeType RealLiteralAst::getNodeType() const { return NodeType::RealLiteral; }
std::string RealLiteralAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "RealLiteral: " << std::to_string(realValue) << "\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions