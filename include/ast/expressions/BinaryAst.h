#pragma once
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {

enum class BinaryOpType {
  POWER,
  MULTIPLY,
  DIVIDE,
  REM,
  ADD,
  SUBTRACT,
  LESS_THAN,
  GREATER_THAN,
  LESS_EQUAL,
  GREATER_EQUAL,
  EQUAL,
  NOT_EQUAL,
  AND,
  OR,
  XOR
};

class BinaryAst final : public ExpressionAst {
private:
  BinaryOpType binaryOpType;
  std::shared_ptr<ExpressionAst> left;
  std::shared_ptr<ExpressionAst> right;

  static std::string getOperator(BinaryOpType opType);

public:
  explicit BinaryAst(antlr4::Token *token) : ExpressionAst(token), binaryOpType() {}

  void setBinaryOpType(BinaryOpType binOp);
  void setLeft(std::shared_ptr<ExpressionAst> leftExpr);
  void setRight(std::shared_ptr<ExpressionAst> rightExpr);

  BinaryOpType getBinaryOpType() const;
  std::shared_ptr<ExpressionAst> getLeft() const;
  std::shared_ptr<ExpressionAst> getRight() const;

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};

} // namespace gazprea::ast::expressions