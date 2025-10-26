#include "ast/types/AliasTypeAst.h"

namespace gazprea::ast::types {

NodeType AliasTypeAst::getNodeType() const { return NodeType::AliasType; }

std::string AliasTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << alias;
  return ss.str();
}
} // namespace gazprea::ast::types
