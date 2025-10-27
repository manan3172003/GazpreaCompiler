#include <ast/prototypes/ProcedureAst.h>

namespace gazprea::ast::prototypes {
NodeType ProcedureAst::getNodeType() const { return NodeType::Procedure; }
std::string ProcedureAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Procedure " << proto->getName() << "(";
  const auto &args = proto->getParams();
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) {
      ss << ", ";
    }
    ss << args[i]->toStringTree("");
  }
  ss << ")";
  if (!proto->getType().empty()) {
    ss << " Returns: " << proto->getType();
  }
  if (scope) {
    ss << " (Scope: " << scope->toString() << ")";
  }
  if (sym) {
    ss << " (Symbol: " << sym->toString() << ")";
  }
  ss << "\n";
  if (body) {
    ss << body->toStringTree(prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::prototypes