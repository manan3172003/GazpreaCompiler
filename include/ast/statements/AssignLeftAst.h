#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::types {
class DataTypeAst;
}

namespace gazprea::ast::statements {

class AssignLeftAst : public Ast {
private:
  std::shared_ptr<types::DataTypeAst> assignDataType;
  std::shared_ptr<symTable::Type> assignSymbolType;

public:
  explicit AssignLeftAst(antlr4::Token *token) : Ast(token) {}

  void setAssignDataType(const std::shared_ptr<types::DataTypeAst> &evaluatedType) {
    assignDataType = evaluatedType;
  }
  void setAssignSymbolType(const std::shared_ptr<symTable::Type> &evaluatedType) {
    assignSymbolType = evaluatedType;
  }

  std::shared_ptr<types::DataTypeAst> getAssignDataType() const { return assignDataType; }
  std::shared_ptr<symTable::Type> getAssignSymbolType() const { return assignSymbolType; }
};

} // namespace gazprea::ast::statements