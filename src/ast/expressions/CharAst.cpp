#include <ast/expressions/CharAst.h>

namespace gazprea::ast::expressions {
NodeType CharAst::getNodeType() const { return NodeType::Char; }
std::string CharAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << prefix << "Char: '" << value << "'\n";
  return ss.str();
}
} // namespace gazprea::ast::expressions