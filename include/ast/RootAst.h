#pragma once
#include "Ast.h"

namespace gazprea::ast {
class RootAst : public Ast {
public:
  std::vector<std::shared_ptr<Ast>> children;
  explicit RootAst(antlr4::Token *token) : Ast(token) {};
  void addChild(std::shared_ptr<Ast> child);
  NodeType getNodeType() const override;
  std::string toStringTree() const override;
  ~RootAst() override = default;
};
} // namespace gazprea::ast