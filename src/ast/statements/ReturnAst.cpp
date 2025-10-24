#include <ast/statements/ReturnAst.h>

namespace gazprea::ast::statements {
NodeType ReturnAst::getNodeType() const { return NodeType::Return; }
std::string ReturnAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Return\n";
  if (expr) {
    ss << expr->toStringTree(prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::statements