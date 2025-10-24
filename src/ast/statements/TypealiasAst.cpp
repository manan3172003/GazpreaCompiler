#include <ast/statements/TypealiasAst.h>

namespace gazprea::ast::statements {

NodeType TypealiasAst::getNodeType() const { return NodeType::Typealias; }
std::string TypealiasAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Typealias " << type << " " << alias << "\n";
  return ss.str();
}
std::string TypealiasAst::getType() const { return type; }
void TypealiasAst::setType(const std::string &type) { this->type = type; }
std::string TypealiasAst::getAlias() const { return alias; }
void TypealiasAst::setAlias(const std::string &alias) { this->alias = alias; }

} // namespace gazprea::ast::statements