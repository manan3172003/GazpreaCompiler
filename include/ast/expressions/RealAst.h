#pragma once

#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class RealAst : public ExpressionAst {
public:
  float realValue;
  RealAst(antlr4::Token *token, float realValue)
      : ExpressionAst(token), realValue(realValue) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions