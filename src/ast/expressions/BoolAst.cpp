#include <ast/expressions/BoolAst.h>

namespace gazprea::ast::expressions {
std::string BoolAst::boolToString() const {
  switch (value) {
  case BoolValue::TRUE:
    return "true";
  case BoolValue::FALSE:
    return "false";
  default:
    return "unknown";
  }
}
NodeType BoolAst::getNodeType() const { return NodeType::Bool; }
std::string BoolAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Bool: " << boolToString() << "\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions