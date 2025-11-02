#pragma once
#include <utility>

#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"
#include "ast/types/DataTypeAst.h"

namespace gazprea::ast::statements {
class DeclarationAst final : public StatementAst {
private:
  std::string name;
  Qualifier qualifier;
  std::shared_ptr<types::DataTypeAst> type;
  std::shared_ptr<expressions::ExpressionAst> expr;
  std::shared_ptr<symTable::Type> inferredType;

public:
  explicit DeclarationAst(antlr4::Token *token) : StatementAst(token), qualifier(), type() {}

  std::string getName() const { return name; }
  std::shared_ptr<expressions::ExpressionAst> getExpr() const { return expr; }
  Qualifier getQualifier() const { return qualifier; }
  std::shared_ptr<types::DataTypeAst> getType() const { return type; }
  std::shared_ptr<symTable::Type> getInferredType() const { return inferredType; }

  void setName(std::string name_) { this->name = std::move(name_); }
  void setQualifier(Qualifier qualifier_) { this->qualifier = qualifier_; }
  void setType(std::shared_ptr<types::DataTypeAst> type_) { this->type = type_; }
  void setExpr(std::shared_ptr<expressions::ExpressionAst> expr_) { this->expr = expr_; }
  void setInferredType(std::shared_ptr<symTable::Type> inferdType_) {
    this->inferredType = inferdType_;
  }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~DeclarationAst() override = default;
};
} // namespace gazprea::ast::statements