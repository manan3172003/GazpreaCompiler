#include <ast/statements/InputAst.h>

namespace gazprea::ast::statements {
NodeType InputAst::getNodeType() const { return NodeType::Input; }
std::string InputAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Input\n";
  ss << lVal->toStringTree(prefix + indent);
  if (sym) {
    ss << " (Symbol: " << sym->toString() << ")";
  }
  ss << "\n";
  return ss.str();
}
std::shared_ptr<AssignLeftAst> InputAst::getLVal() const { return lVal; }
void InputAst::setLVal(std::shared_ptr<AssignLeftAst> lVal_) {
  this->lVal = lVal_;
}
} // namespace gazprea::ast::statements