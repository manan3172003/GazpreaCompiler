#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::types {
class DataTypeAst : public Ast {
public:
  explicit DataTypeAst(antlr4::Token *token) : Ast(token) {}
};
} // namespace gazprea::ast::types