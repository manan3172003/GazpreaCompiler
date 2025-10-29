#pragma once
#include "ast/Ast.h"
#include "ast/types/DataTypeAst.h"

namespace gazprea::ast::expressions {

class ExpressionAst : public Ast {
  std::shared_ptr<types::DataTypeAst> inferredType;
public:
  ExpressionAst(antlr4::Token *token) : Ast(token) {};
  virtual ~ExpressionAst() = default;
  void setInferredType(std::shared_ptr<types::DataTypeAst> type_) {inferredType = type_;};
  std::shared_ptr<types::DataTypeAst> getInferredType() {return inferredType;}
};

} // namespace gazprea::ast::expressions
