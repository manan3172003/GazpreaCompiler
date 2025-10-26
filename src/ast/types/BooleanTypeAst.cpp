#include "ast/types/BooleanTypeAst.h"

namespace gazprea::ast::types {

NodeType BooleanTypeAst::getNodeType() const { return NodeType::BoolType; }

std::string BooleanTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "BooleanType";
  return ss.str();
}
} // namespace gazprea::ast::types
