#include "ast/expressions/BinaryAst.h"

namespace gazprea::ast::expressions {

void BinaryAst::setBinaryOptype(BinaryOpType binOp) {
  this->binaryOpType = binOp;
}

void BinaryAst::setLeft(std::shared_ptr<ExpressionAst> leftExpr) {
  this->left = leftExpr;
}

void BinaryAst::setRight(std::shared_ptr<ExpressionAst> rightExpr) {
  this->right = rightExpr;
}

BinaryOpType BinaryAst::getBinaryOpType() const { return this->binaryOpType;}

std::shared_ptr<ExpressionAst> BinaryAst::getLeft() const { return this->left;}

std::shared_ptr<ExpressionAst> BinaryAst::getRight() const { return this->right;}

NodeType BinaryAst::getNodeType() const { return NodeType::BinaryExpression; }

std::string BinaryAst::toStringTree(std::string prefix) const {
  std::stringstream ss;

  ss << prefix << getOperator(binaryOpType) << '\n';
  ss << left->toStringTree(prefix + indent);
  ss << right->toStringTree(prefix + indent);
  return ss.str();
}

std::string BinaryAst::getOperator(BinaryOpType opType) {
  switch (opType) {
  case BinaryOpType::POWER:
    return "^";
  case BinaryOpType::MULTIPLY:
    return "*";
  case BinaryOpType::DIVIDE:
    return "/";
  case BinaryOpType::MODULO:
    return "%";
  case BinaryOpType::DOT_PRODUCT:
    return "**";
  case BinaryOpType::ADD:
    return "+";
  case BinaryOpType::SUBTRACT:
    return "-";
  case BinaryOpType::LESS_THAN:
    return "<";
  case BinaryOpType::GREATER_THAN:
    return ">";
  case BinaryOpType::LESS_EQUAL:
    return "<=";
  case BinaryOpType::GREATER_EQUAL:
    return ">=";
  case BinaryOpType::EQUAL:
    return "==";
  case BinaryOpType::NOT_EQUAL:
    return "!=";
  case BinaryOpType::AND:
    return "and";
  case BinaryOpType::OR:
    return "or";
  case BinaryOpType::XOR:
    return "xor";
}
}

} // namespace gazprea::ast::expressions