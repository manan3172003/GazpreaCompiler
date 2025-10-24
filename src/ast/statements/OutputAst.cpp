#include <ast/statements/OutputAst.h>

namespace gazprea::ast::statements {
NodeType OutputAst::getNodeType() const {
  return NodeType::Output;
}
std::string OutputAst::toStringTree() const {
  std::stringstream ss;
  ss << "Output " << "\n";
  ss << "└──" << expr->toStringTree();
}
} // namespace gazprea::ast::statements