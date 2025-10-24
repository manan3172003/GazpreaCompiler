#include <ast/expressions/IntegerAst.h>

namespace gazprea::ast::expressions {

NodeType IntegerAst::getNodeType() const { return NodeType::Integer; }
std::string IntegerAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Integer: " << std::to_string(integerValue) << "\n";
  return ss.str();
}

} // namespace gazprea::ast::expressions