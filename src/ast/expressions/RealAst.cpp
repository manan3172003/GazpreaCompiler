#include <ast/expressions/RealAst.h>

namespace gazprea::ast::expressions {
NodeType RealAst::getNodeType() const { return NodeType::Real; }
std::string RealAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Real: " << std::to_string(realValue) << " "
     << token->toString() << "\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions