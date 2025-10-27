#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::types {
class DataTypeAst : public Ast {
  std::string typeName;

public:
  explicit DataTypeAst(antlr4::Token *token) : Ast(token) {}
  DataTypeAst(antlr4::Token *token, std::string typeName_)
      : Ast(token), typeName(std::move(typeName_)) {}
};
} // namespace gazprea::ast::types