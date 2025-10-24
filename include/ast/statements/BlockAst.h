#pragma once
#include "StatementAst.h"

namespace gazprea::ast::statements {
class BlockAst final : public StatementAst {
  std::vector<std::shared_ptr<Ast>> children;

public:
  explicit BlockAst(antlr4::Token *token) : StatementAst(token) {};

  std::vector<std::shared_ptr<Ast>> getChildren() const;
  void addChildren(std::shared_ptr<Ast> child_stats);

  NodeType getNodeType() const override;
  std::string toStringTree() const override;
};
} // namespace gazprea::ast::statements