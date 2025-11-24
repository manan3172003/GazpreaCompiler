#include <ast/statements/StructDeclarationAst.h>

namespace gazprea::ast::statements {

NodeType StructDeclarationAst::getNodeType() const { return NodeType::StructDeclaration; }
std::string StructDeclarationAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Declaration ";
  ss << type->toStringTree(prefix + indent);
  if (sym) {
    ss << sym->toString();
  }
  ss << '\n';
  return ss.str();
}

} // namespace gazprea::ast::statements