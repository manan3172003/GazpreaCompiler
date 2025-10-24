#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::statements {

class AssignLeftAst : public Ast {
public:
  explicit AssignLeftAst(antlr4::Token *token) : Ast(token) {}
};

} // namespace gazprea::ast::statements