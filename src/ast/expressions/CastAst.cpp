#include <ast/expressions/CastAst.h>

namespace gazprea::ast::expressions {
NodeType CastAst::getNodeType() const { return NodeType::Cast; }
std::string CastAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Cast (" << type << "): \n";
  if (expr) {
    ss << expr->toStringTree(prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::expressions