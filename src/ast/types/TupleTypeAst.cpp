#include "ast/types/TupleTypeAst.h"

namespace gazprea::ast::types {

NodeType TupleTypeAst::getNodeType() const { return NodeType::TupleType; }

std::string TupleTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "TupleType( ";
  for (size_t i = 0; i < types.size(); ++i) {
    ss << types[i]->toStringTree(prefix + indent);
    if (i + 1 < types.size()) {
      ss << ", ";
    }
  }
  ss << " )";
  return ss.str();
}
} // namespace gazprea::ast::types
