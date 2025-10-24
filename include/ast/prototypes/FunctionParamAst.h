#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::prototypes {
class FunctionParamAst final : public Ast {
  std::string type;
  std::string name;

public:
  explicit FunctionParamAst(antlr4::Token *token) : Ast(token) {}

  std::string &getName() { return name; }
  void setName(const std::string &name_) { name = name_; }
  std::string &getType() { return type; }
  void setType(const std::string &type_) { type = type_; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::prototypes