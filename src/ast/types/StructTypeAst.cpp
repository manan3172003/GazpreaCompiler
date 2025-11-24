#include "ast/types/StructTypeAst.h"

namespace gazprea::ast::types {

NodeType StructTypeAst::getNodeType() const { return NodeType::StructType; }

void StructTypeAst::addElement(std::string elementName, std::shared_ptr<DataTypeAst> type) {
  types.push_back(type);
  nameToIdx[elementName] = types.size(); // 1-indexed like tuples
  idxToName[types.size()] = elementName; // 1-indexed like tuples
}

std::string StructTypeAst::getStructName() const { return structName; }
void StructTypeAst::setStructName(const std::string &name) { structName = name; }

std::string StructTypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  ss << structName << " ";
  ss << "StructType( ";
  for (size_t i = 0; i < types.size(); ++i) {
    ss << idxToName.at(i + 1) << ":";
    ss << types[i]->toStringTree(prefix + indent);
    if (i + 1 < types.size()) {
      ss << ", ";
    }
  }
  ss << " )";
  return ss.str();
}

} // namespace gazprea::ast::types