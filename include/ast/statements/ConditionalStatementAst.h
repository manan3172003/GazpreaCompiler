#pragma once
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"
#include <memory>

namespace gazprea::ast::statements {
class ConditionalStatementAst : public StatementAst {
public:
  std::shared_ptr<expressions::ExpressionAst> condition;
  std::shared_ptr<Ast> thenBody;
  std::shared_ptr<Ast> elseBody;

  ConditionalStatementAst(antlr4::Token *token) : StatementAst(token) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~ConditionalStatementAst() override = default;
};
} // namespace gazprea::ast::statements