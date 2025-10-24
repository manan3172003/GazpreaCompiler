#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::statements {

class AssignLeftAst : public Ast {
  // private:
  // In future might need mlir::Value as private method with getter and setter
  // So that we can get the memory location where the store needs to happen
public:
  explicit AssignLeftAst(antlr4::Token *token) : Ast(token) {}
};

} // namespace gazprea::ast::statements