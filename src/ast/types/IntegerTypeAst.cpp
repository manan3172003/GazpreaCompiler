#include "ast/types/IntegerTypeAst.h"

namespace gazprea::ast::types {

NodeType IntegerTypeAst::getNodeType() const { return NodeType::IntegerType; }

std::string IntegerTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "IntegerType";
  return ss.str();
}
} // namespace gazprea::ast::types
