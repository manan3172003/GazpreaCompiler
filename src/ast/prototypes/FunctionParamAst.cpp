#include <ast/prototypes/FunctionParamAst.h>

namespace gazprea::ast::prototypes {
NodeType FunctionParamAst::getNodeType() const {
  return NodeType::FunctionParam;
}
std::string FunctionParamAst::toStringTree() const {
  std::stringstream ss;
  ss << "FunctionParam " << type << " " << name;
  return ss.str();
}
} // namespace gazprea::ast::prototypes