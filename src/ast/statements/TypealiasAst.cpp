#include <ast/statements/TypealiasAst.h>

namespace gazprea::ast::statements {

NodeType TypealiasAst::getNodeType() const { return NodeType::Typealias; }
std::string TypealiasAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Typealias " << type->toStringTree(prefix + indent) << " "
     << alias << "\n";
  return ss.str();
}
std::shared_ptr<types::DataTypeAst> TypealiasAst::getType() const {
  return type;
}
void TypealiasAst::setType(std::shared_ptr<types::DataTypeAst> type_) {
  this->type = type_;
}
std::string TypealiasAst::getAlias() const { return alias; }
void TypealiasAst::setAlias(const std::string &alias) { this->alias = alias; }

} // namespace gazprea::ast::statements