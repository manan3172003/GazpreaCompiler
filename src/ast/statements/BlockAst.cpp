#include <ast/statements/BlockAst.h>

namespace gazprea::ast::statements {
NodeType BlockAst::getNodeType() const { return NodeType::Block; }
std::string BlockAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Block ";
  ss << scopeToString();
  ss << "\n";
  for (size_t i = 0; i < children.size(); ++i) {
    ss << children[i]->toStringTree(prefix + indent);
  }
  return ss.str();
}
std::vector<std::shared_ptr<Ast>> BlockAst::getChildren() const {
  return children;
}
void BlockAst::addChildren(std::shared_ptr<Ast> child) {
  children.push_back(child);
}
} // namespace gazprea::ast::statements