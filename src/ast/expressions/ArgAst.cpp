#include <ast/expressions/ArgAst.h>

namespace gazprea::ast::expressions {
NodeType ArgAst::getNodeType() const { return NodeType::Arg; }
std::string ArgAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  if (expr) {
    ss << expr->toStringTree(prefix);
  }
  return ss.str();
}
} // namespace gazprea::ast::expressions