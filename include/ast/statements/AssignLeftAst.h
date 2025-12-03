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
  mlir::Value evaluatedAddr;

public:
  explicit AssignLeftAst(antlr4::Token *token) : Ast(token) {}

  void setAssignDataType(const std::shared_ptr<types::DataTypeAst> &evaluatedType) {
    assignDataType = evaluatedType;
  }
  void setAssignSymbolType(const std::shared_ptr<symTable::Type> &evaluatedType) {
    assignSymbolType = evaluatedType;
  }
  void setEvaluatedAddr(mlir::Value addr) { evaluatedAddr = addr; }

  std::shared_ptr<types::DataTypeAst> getAssignDataType() const { return assignDataType; }
  std::shared_ptr<symTable::Type> getAssignSymbolType() const { return assignSymbolType; }
  mlir::Value getEvaluatedAddr() const { return evaluatedAddr; }
};

} // namespace gazprea::ast::statements