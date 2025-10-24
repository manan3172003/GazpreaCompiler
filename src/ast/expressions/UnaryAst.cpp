#pragma once
#include "ast/expressions/UnaryAst.h"

namespace gazprea::ast::expressions {

void UnaryAst::setUnaryOpType(UnaryOpType unaryOp) {
  this->unaryOpType = unaryOp;
}

void UnaryAst::setExpression(std::shared_ptr<ExpressionAst> expr) {
  this->expression = expr;
}

UnaryOpType UnaryAst::getUnaryOpType() const { return this->unaryOpType; }

std::shared_ptr<ExpressionAst> UnaryAst::getExpression() const {
  return this->expression;
}

NodeType UnaryAst::getNodeType() const { return NodeType::UnaryExpression; }

std::string UnaryAst::toStringTree(std::string prefix) const {
  std::stringstream ss;

  ss << prefix << "Unary(" << getOperator(unaryOpType) << "): \n";
  ss << expression->toStringTree(prefix + "----");

  return ss.str();
}

std::string UnaryAst::getOperator(UnaryOpType opType) {
  switch (opType) {
  case UnaryOpType::PLUS:
    return "+";
  case UnaryOpType::MINUS:
    return "-";
  case UnaryOpType::NOT:
    return "not";
  }
}

} // namespace gazprea::ast::expressions
