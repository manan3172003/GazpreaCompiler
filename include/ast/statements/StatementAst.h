#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::statements {
class StatementAst : public virtual Ast {
public:
  explicit StatementAst(antlr4::Token *token) : Ast(token) {}
  ~StatementAst() override = default;
};
} // namespace gazprea::ast::statements