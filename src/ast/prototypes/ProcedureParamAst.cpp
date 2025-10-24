#include <ast/prototypes/ProcedureParamAst.h>

namespace gazprea::ast::prototypes {
NodeType ProcedureParamAst::getNodeType() const {
  return NodeType::ProcedureParam;
}
std::string ProcedureParamAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << qualifierToString(qualifier) << " " << type;
  if (!name.empty()) {
    ss << " " << name;
  }
  return ss.str();
}
} // namespace gazprea::ast::prototypes