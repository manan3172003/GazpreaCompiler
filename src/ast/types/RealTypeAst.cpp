#include "ast/types/RealTypeAst.h"

namespace gazprea::ast::types {

NodeType RealTypeAst::getNodeType() const { return NodeType::RealType; }

std::string RealTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "RealType";
  return ss.str();
}
} // namespace gazprea::ast::types
