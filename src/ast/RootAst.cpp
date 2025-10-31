#include <ast/RootAst.h>

namespace gazprea::ast {
void RootAst::addChild(std::shared_ptr<Ast> child) { children.push_back(child); }
NodeType RootAst::getNodeType() const { return NodeType::Root; }
std::string RootAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Root " << scopeToString() << "\n";
  for (const auto &child : children) {
    ss << child->toStringTree(prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast