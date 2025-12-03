#include <ast/expressions/BuiltinFuncAst.h>

namespace gazprea::ast::expressions {
std::string BuiltinFuncAst::funcTypeToString(BuiltinFuncType type) {
  switch (type) {
  case BuiltinFuncType::Length:
    return "Length";
  case BuiltinFuncType::Shape:
    return "Shape";
  case BuiltinFuncType::Reverse:
    return "Reverse";
  case BuiltinFuncType::Format:
    return "Format";
  case BuiltinFuncType::StreamState:
    return "StreamState";
  default:
    return "UnknownBuiltinFunc";
  }
}
std::string BuiltinFuncAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << funcTypeToString(funcType) << " " << scopeToString() << " ";
  if (sym) {
    ss << sym->toString();
  }
  ss << "\n";
  if (arg)
    ss << arg->toStringTree(prefix + indent);
  return ss.str();
}
} // namespace gazprea::ast::expressions