#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {

enum class UnaryOpType { MINUS, NOT, PLUS };

class UnaryAst : public ExpressionAst {
private:
  UnaryOpType unaryOpType;
  std::shared_ptr<ExpressionAst> expression;

  static std::string getOperator(UnaryOpType opType);

public:
  UnaryAst(antlr4::Token *token) : ExpressionAst(token) {}

  void setUnaryOpType(UnaryOpType unaryOp);
  void setExpression(std::shared_ptr<ExpressionAst> expr);

  UnaryOpType getUnaryOpType() const;
  std::shared_ptr<ExpressionAst> getExpression() const;

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::expressions
