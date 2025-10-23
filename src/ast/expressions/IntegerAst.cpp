#include <ast/expressions/IntegerAst.h>

namespace gazprea::ast::expressions {

NodeType IntegerAst::getNodeType() const { return NodeType::Integer; }
std::string IntegerAst::toStringTree() const {
  std::stringstream ss;
  ss << "Integer: " << std::to_string(integerValue) << " " << token->toString()
     << "\n";
  return ss.str();
}

} // namespace gazprea::ast::expressions