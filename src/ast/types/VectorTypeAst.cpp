#include <ast/types/VectorTypeAst.h>

namespace gazprea::ast::types {
NodeType VectorTypeAst::getNodeType() const { return NodeType::VectorType; }
std::string VectorTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "VectorType( ";
  ss << elementType->toStringTree(prefix + indent);
  ss << " )";
  return ss.str();
}
} // namespace gazprea::ast::types