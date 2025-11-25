#include <ast/statements/MemberFunctionAst.h>

namespace gazprea::ast::statements {
NodeType MemberFunctionAst::getNodeType() const { return NodeType::Builtin; }
std::string MemberFunctionAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << left->toStringTree("");
  prefix += indent;
  ss << prefix << "MemberFunc: " << MemberFunctionTypeToString(memberType) << "(";
  if (!args.empty()) {
    ss << "\n";
    for (const auto &param : args) {
      ss << param->toStringTree(prefix + indent);
    }
    ss << prefix << ")\n";
  } else
    ss << ")\n";
  return ss.str();
}
} // namespace gazprea::ast::statements