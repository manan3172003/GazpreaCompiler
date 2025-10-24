#include <ast/statements/BreakAst.h>

namespace gazprea::ast::statements {
NodeType BreakAst::getNodeType() const { return NodeType::Break; }
std::string BreakAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Break" << '\n';
  return ss.str();
}
} // namespace gazprea::ast::statements