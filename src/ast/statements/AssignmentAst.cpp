#include <ast/statements/AssignmentAst.h>

namespace gazprea::ast::statements {

NodeType AssignmentAst::getNodeType() const { return NodeType::Assignment; }
std::string AssignmentAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Assignment " << name << '\n';
  if (expr) {
    ss << expr->toStringTree(prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::statements