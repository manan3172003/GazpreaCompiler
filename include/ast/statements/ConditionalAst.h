#pragma once
#include "BlockAst.h"
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"
#include <memory>

namespace gazprea::ast::statements {
class ConditionalAst : public StatementAst {
  std::shared_ptr<expressions::ExpressionAst> condition;
  std::shared_ptr<BlockAst> thenBody;
  std::shared_ptr<BlockAst> elseBody;

public:
  explicit ConditionalAst(antlr4::Token *token) : Ast(token), StatementAst(token) {}

  // setters
  void setCondition(std::shared_ptr<expressions::ExpressionAst> condition);
  void setThenBody(std::shared_ptr<BlockAst> thenBody);
  void setElseBody(std::shared_ptr<BlockAst> elseBody);
  // getters
  std::shared_ptr<expressions::ExpressionAst> getCondition() const;
  std::shared_ptr<BlockAst> getThenBody() const;
  std::shared_ptr<BlockAst> getElseBody() const;

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~ConditionalAst() override = default;
};
} // namespace gazprea::ast::statements