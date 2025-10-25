#include <ast/expressions/BoolAst.h>

namespace gazprea::ast::expressions {
NodeType BoolAst::getNodeType() const { return NodeType::Bool; }
std::string BoolAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Bool: " << value << "\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions