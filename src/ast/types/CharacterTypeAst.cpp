#include "ast/types/CharacterTypeAst.h"

namespace gazprea::ast::types {

NodeType CharacterTypeAst::getNodeType() const { return NodeType::CharType; }

std::string CharacterTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "CharacterType";
  return ss.str();
}
} // namespace gazprea::ast::types
