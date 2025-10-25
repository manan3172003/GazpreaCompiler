#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class CastAst final : public ExpressionAst {
  std::string type;
  std::shared_ptr<ExpressionAst> expr;

public:
  explicit CastAst(antlr4::Token *token) : ExpressionAst(token) {};

  std::string &getType() { return type; }
  void setType(const std::string &type_) { type = type_; }
  std::shared_ptr<ExpressionAst> getExpression() const { return expr; }
  void setExpression(std::shared_ptr<ExpressionAst> expr_) { expr = expr_; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions