#include <ast/prototypes/ProcedureAst.h>

namespace gazprea::ast::prototypes {
NodeType ProcedureAst::getNodeType() const { return NodeType::Procedure; }
std::string ProcedureAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Procedure " << proto->getName() << "(";
  const auto &args = proto->getArgs();
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
  ss << "\n";
  if (body) {
    ss << body->toStringTree(prefix + indent);
  }
  return ss.str();
}
} // namespace gazprea::ast::prototypes