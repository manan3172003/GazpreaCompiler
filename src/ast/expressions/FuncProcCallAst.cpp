#include <ast/expressions/FuncProcCallAst.h>

namespace gazprea::ast::expressions {
NodeType FuncProcCallAst::getNodeType() const { return NodeType::FuncProcCall; }
std::string FuncProcCallAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "FuncProcCall: " << scopeToString() << getName() << " ";
  if (sym) {
    ss << sym->toString();
  }
  ss << "\n";
  if (!args.empty()) {
    for (size_t i = 0; i < args.size(); ++i) {
      ss << args[i]->toStringTree(prefix + indent);
    }
  }
  return ss.str();
}
} // namespace gazprea::ast::expressions