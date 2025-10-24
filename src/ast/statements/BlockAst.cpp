#include <ast/statements/BlockAst.h>

namespace gazprea::ast::statements {
NodeType BlockAst::getNodeType() const { return NodeType::Block; }
std::string BlockAst::toStringTree() const {
  std::stringstream ss;
  ss << "Block " << token->toString() << "\n";
  for (int i = 0; i < children.size(); ++i) {
    if (i == children.size() - 1) {
      ss << "└──" << children[i]->toStringTree();
    } else {
      ss << "├──" << children[i]->toStringTree();
    }
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