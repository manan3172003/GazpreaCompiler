#include "ast/statements/TupleUnpackAssignAst.h"
namespace gazprea::ast::statements {

NodeType TupleUnpackAssignAst::getNodeType() const {
  return NodeType::TupleUnpackAssign;
}
void TupleUnpackAssignAst::addSubLVal(std::shared_ptr<AssignLeftAst> lVal) {
  lVals.push_back(lVal);
}

std::string TupleUnpackAssignAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "L-values:\n";
  for (const auto &lVal : lVals) {
    ss << lVal->toStringTree(prefix + indent);
  }
  return ss.str();
}

} // namespace gazprea::ast::statements