#include <ast/RootAst.h>

namespace gazprea::ast {
void RootAst::addChild(std::shared_ptr<Ast> child) {
  children.push_back(child);
}
NodeType RootAst::getNodeType() const { return NodeType::Root; }
std::string RootAst::toStringTree() const {
  std::stringstream ss;
  ss << "Root " << "\n";
  return ss.str();
}
} // namespace gazprea::ast