#include <ast/RootAst.h>

namespace gazprea::ast {
void RootAst::addChild(std::shared_ptr<Ast> child) {
  children.push_back(child);
}
NodeType RootAst::getNodeType() const { return NodeType::Root; }
std::string RootAst::toStringTree() const {
  std::stringstream ss;
  ss << "Root " << "\n";
  for (int i = 0; i < children.size(); ++i) {
    if (i == children.size() - 1) {
      ss << "└──" << children[i]->toStringTree();
    } else {
      ss << "├──" << children[i]->toStringTree();
    }
  }
  return ss.str();
}
} // namespace gazprea::ast