#include <ast/prototypes/FunctionParamAst.h>

namespace gazprea::ast::prototypes {
NodeType FunctionParamAst::getNodeType() const {
  return NodeType::FunctionParam;
}
std::string FunctionParamAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  if (paramType) {
    ss << paramType->toStringTree("");
  }
  if (!name.empty()) {
    ss << " " << name;
  }
  return ss.str();
}
} // namespace gazprea::ast::prototypes