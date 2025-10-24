#include <ast/prototypes/FunctionAst.h>

namespace gazprea::ast::prototypes {
NodeType FunctionAst::getNodeType() const { return NodeType::Function; }
std::string FunctionAst::toStringTree(std::string prefix) const {
  std::stringstream ss;

  ss << prefix << "Function " << proto->getName() << "(";

  const auto &args = proto->getArgs();
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) {
      ss << ", ";
    }
    ss << args[i]->toStringTree("");
  }

  ss << ")";
  ss << " Returns: " << proto->getType() << " " << token->toString() << "\n";

  if (body) {
    ss << body->toStringTree(prefix + indent);
  }

  return ss.str();
}
} // namespace gazprea::ast::prototypes