#pragma once
#include "AssignmentAst.h"

namespace gazprea::ast::statements {

class StructElementAssignAst final : public AssignLeftAst {
private:
  std::string structName;
  std::string elementName;

public:
  explicit StructElementAssignAst(antlr4::Token *token)
      : AssignLeftAst(token), structName(), elementName() {}

  void setStructName(std::string name) { structName = std::move(name); }
  const std::string &getStructName() const { return structName; }

  void setElementName(const std::string &name) { elementName = name; }
  std::string getElementName() const { return elementName; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::statements