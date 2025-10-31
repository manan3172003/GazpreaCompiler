#include <ast/statements/ProcedureCallAst.h>

namespace gazprea::ast::statements {
NodeType ProcedureCallAst::getNodeType() const { return NodeType::ProcedureCall; }
std::string ProcedureCallAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "ProcedureCall: " << getName() << "\n";
  if (!args.empty()) {
    for (size_t i = 0; i < args.size(); ++i) {
      ss << args[i]->toStringTree(prefix + indent);
    }
  }
  return ss.str();
}
} // namespace gazprea::ast::statements