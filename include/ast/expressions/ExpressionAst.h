#pragma once
#include "ast/Ast.h"
#include "ast/types/DataTypeAst.h"

namespace gazprea::ast::expressions {

class ExpressionAst : public Ast {
  std::shared_ptr<symTable::Type> inferredSymbolType;
  std::shared_ptr<types::DataTypeAst> inferredDataType;

public:
  ExpressionAst(antlr4::Token *token) : Ast(token) {};
  virtual ~ExpressionAst() = default;
  void setInferredSymbolType(std::shared_ptr<symTable::Type> type_) { inferredSymbolType = type_; };
  std::shared_ptr<symTable::Type> getInferredSymbolType() { return inferredSymbolType; }
  void setInferredDataType(std::shared_ptr<types::DataTypeAst> type_) { inferredDataType = type_; };
  std::shared_ptr<types::DataTypeAst> getInferredDataType() { return inferredDataType; }
};

} // namespace gazprea::ast::expressions
