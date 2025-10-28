#include <ast/prototypes/FunctionAst.h>

namespace gazprea::ast::prototypes {
NodeType FunctionAst::getNodeType() const { return NodeType::Function; }
std::string FunctionAst::toStringTree(std::string prefix) const {
  std::stringstream ss;

  ss << prefix << "Function" << scopeToString() << " " << proto->getName()
     << "(";

  const auto &args = proto->getParams();
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) {
      ss << ", ";
    }
    ss << args[i]->toStringTree("");
  }

  ss << ")";
  ss << " Returns: " << proto->getType();
  if (scope) {
    ss << " (Scope: " << scope->toString() << ")";
  }
  if (proto->getSymbol()) {
    ss << " (Symbol: " << proto->getSymbol()->toString() << " "
       << proto->scopeToString() << ")";
  }
  ss << "\n";
  if (body) {
    ss << body->toStringTree(prefix + indent);
  }

  return ss.str();
}
} // namespace gazprea::ast::prototypes