#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::expressions {

class ExpressionAst : public Ast {

public:
  ExpressionAst(antlr4::Token *token) : Ast(token) {};
  virtual ~ExpressionAst() = default;
};

} // namespace gazprea::ast::expressions
