#include "Colors.h"

#include <ast/prototypes/ProcedureAst.h>

namespace gazprea::ast::prototypes {
NodeType ProcedureAst::getNodeType() const { return NodeType::Procedure; }
std::string ProcedureAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Procedure" << scopeToString() << " " << proto->getName()
     << "(";
  const auto &args = proto->getParams();
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) {
      ss << ", ";
    }
    ss << args[i]->toStringTree("");
  }
  ss << ")";
  if (proto->getReturnType()) {
    ss << " Returns: " << proto->getReturnType()->toStringTree("");
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