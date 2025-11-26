#pragma once
#include "DataTypeAst.h"

namespace gazprea::ast::types {

class StructTypeAst final : public DataTypeAst {
private:
  std::string structName;
  std::unordered_map<std::string, size_t> nameToIdx;
  std::unordered_map<size_t, std::string> idxToName;
  std::vector<std::shared_ptr<DataTypeAst>> types;

public:
  explicit StructTypeAst(antlr4::Token *token) : DataTypeAst(token) {}
  void addElement(std::string elementName, std::shared_ptr<DataTypeAst> type);
  std::string getStructName() const;
  void setStructName(const std::string &name);
  std::string getElementName(size_t idx) const;
  size_t getElementIdx(std::string elementName) const;
  std::vector<std::shared_ptr<DataTypeAst>> getTypes();
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::types