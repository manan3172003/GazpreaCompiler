#pragma once
#include <utility>

#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"

namespace gazprea::ast::statements {
class DeclarationAst : public StatementAst {
public:
  std::string name;
  Qualifier qualifier;
  std::string type;
  std::shared_ptr<Ast> expr;
  DeclarationAst(antlr4::Token *token) : StatementAst(token) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~DeclarationAst() override = default;
};
} // namespace gazprea::ast::statements