#pragma once
#include "StatementAst.h"

namespace gazprea::ast::statements {
class ContinueAst : public StatementAst {
public:
  ContinueAst(antlr4::Token *token) : Ast(token), StatementAst(token) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::statements