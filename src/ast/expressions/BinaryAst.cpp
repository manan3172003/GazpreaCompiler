#include "ast/expressions/BinaryAst.h"

namespace gazprea::ast::expressions {

void BinaryAst::setBinaryOpType(BinaryOpType binOp) {
  this->binaryOpType = binOp;
}

void BinaryAst::setLeft(std::shared_ptr<ExpressionAst> leftExpr) {
  this->left = leftExpr;
}

void BinaryAst::setRight(std::shared_ptr<ExpressionAst> rightExpr) {
  this->right = rightExpr;
}

BinaryOpType BinaryAst::getBinaryOpType() const { return this->binaryOpType; }

std::shared_ptr<ExpressionAst> BinaryAst::getLeft() const { return this->left; }

std::shared_ptr<ExpressionAst> BinaryAst::getRight() const {
  return this->right;
}

NodeType BinaryAst::getNodeType() const { return NodeType::BinaryExpression; }

std::string BinaryAst::toStringTree(std::string prefix) const {
  std::stringstream ss;

  ss << prefix << getOperator(binaryOpType) << ": \n";
  ss << left->toStringTree(prefix + indent);
  ss << right->toStringTree(prefix + indent);
  return ss.str();
}

std::string BinaryAst::getOperator(BinaryOpType opType) {
  switch (opType) {
  case BinaryOpType::POWER:
    return "POWER ^";
  case BinaryOpType::MULTIPLY:
    return "MUL *";
  case BinaryOpType::DIVIDE:
    return "DIV /";
  case BinaryOpType::REM:
    return "REM %";
  case BinaryOpType::ADD:
    return "ADD +";
  case BinaryOpType::SUBTRACT:
    return "SUB -";
  case BinaryOpType::LESS_THAN:
    return "LT <";
  case BinaryOpType::GREATER_THAN:
    return "GT >";
  case BinaryOpType::LESS_EQUAL:
    return "LE <=";
  case BinaryOpType::GREATER_EQUAL:
    return "GE >=";
  case BinaryOpType::EQUAL:
    return "EQ ==";
  case BinaryOpType::NOT_EQUAL:
    return "NE !=";
  case BinaryOpType::AND:
    return "AND and";
  case BinaryOpType::OR:
    return "OR or";
  case BinaryOpType::XOR:
    return "XOR xor";
  }
  return "";
}

} // namespace gazprea::ast::expressions