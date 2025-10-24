#pragma once
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"

namespace gazprea::ast::statements {
class OutputAst: public StatementAst {
public:
  std::shared_ptr<expressions::ExpressionAst> expr;
  OutputAst(antlr4::Token *token): StatementAst(token) {};
  NodeType getNodeType() const override;
  std::string toStringTree() const override;
};
}