#pragma once

#include "ast/Ast.h"

namespace gazprea::ast::prototypes {
class PrototypeAst final : public Ast {
  std::string name;
  std::vector<std::shared_ptr<Ast>> args;
  std::string type;

public:
  explicit PrototypeAst(antlr4::Token *token) : Ast(token) {}

  std::string &getName() { return name; }
  void setName(const std::string &name_) { name = name_; }
  std::vector<std::shared_ptr<Ast>> &getArgs() { return args; }
  void setArgs(const std::vector<std::shared_ptr<Ast>> &args_) { args = args_; }
  std::string &getType() { return type; }
  void setType(const std::string &type_) { type = type_; }

  NodeType getNodeType() const override;
  std::string toStringTree() const override;
};
} // namespace gazprea::ast::prototypes