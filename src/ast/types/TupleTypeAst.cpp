#include "ast/types/TupleTypeAst.h"

namespace gazprea::ast::types {

NodeType TupleTypeAst::getNodeType() const { return NodeType::TupleType; }

std::string TupleTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << "TupleType( ";
  for (auto type : types) {
    ss << type->toStringTree(prefix + indent) << ", ";
  }
  ss << ") ";
  return ss.str();
}
} // namespace gazprea::ast::types
