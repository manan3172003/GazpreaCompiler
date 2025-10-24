#include <ast/statements/ContinueAst.h>

namespace gazprea::ast::statements {
NodeType ContinueAst::getNodeType() const { return NodeType::Continue; }
std::string ContinueAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Continue" << "\n";
  return ss.str();
}

} // namespace gazprea::ast::statements