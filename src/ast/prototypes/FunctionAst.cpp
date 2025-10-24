#include <ast/prototypes/FunctionAst.h>

namespace gazprea::ast::prototypes {
NodeType FunctionAst::getNodeType() const { return NodeType::Function; }
std::string FunctionAst::toStringTree(std::string prefix) const {
  std::stringstream ss;

  ss << prefix << "Function " << proto->getName() << "(";

  bool first = true;
  for (const auto &arg : proto->getArgs()) {
    if (!first) {
      ss << ", ";
    }
    ss << arg->toStringTree("");
    first = false;
  }

  ss << ")";
  ss << " Returns: " << proto->getType() << " " << token->toString() << "\n";

  if (body) {
    ss << body->toStringTree(prefix + indent);
  }

  return ss.str();
}
} // namespace gazprea::ast::prototypes