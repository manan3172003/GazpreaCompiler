#pragma once
#include "AssignLeftAst.h"

namespace gazprea::ast::statements {

class IdentifierLeftAst final : public AssignLeftAst {
private:
  std::string name;

public:
  explicit IdentifierLeftAst(antlr4::Token *token) : AssignLeftAst(token){};
  std::string getName() const { return name; }
  void setName(std::string idName) { this->name = std::move(idName); }
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::statements