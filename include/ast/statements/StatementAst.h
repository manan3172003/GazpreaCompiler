#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::statements {
class StatementAst : public Ast {
public:
  StatementAst(antlr4::Token *token) : Ast(token) {}
  ~StatementAst() = default;
};
} // namespace gazprea::ast::statements