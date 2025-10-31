#pragma once
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"

namespace gazprea::ast::statements {
class OutputAst : public StatementAst {
  std::shared_ptr<expressions::ExpressionAst> expr;

public:
  OutputAst(antlr4::Token *token) : StatementAst(token){};
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;

  void setExpression(std::shared_ptr<expressions::ExpressionAst> expr);
  std::shared_ptr<expressions::ExpressionAst> getExpression() const;
};
} // namespace gazprea::ast::statements