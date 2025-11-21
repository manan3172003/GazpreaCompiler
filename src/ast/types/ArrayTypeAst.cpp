#include <ast/types/ArrayTypeAst.h>

namespace gazprea::ast::types {

NodeType ArrayTypeAst::getNodeType() const { return NodeType::ArrayType; }
std::string ArrayTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "ArrayType(";
  if (type)
    ss << type->toStringTree(prefix + indent);
  ss << ")";
  return ss.str();
}
} // namespace gazprea::ast::types