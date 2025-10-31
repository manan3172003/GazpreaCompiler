#pragma once
#include "ExpressionAst.h"
#include "ast/types/DataTypeAst.h"
#include "symTable/Type.h"

namespace gazprea::ast::expressions {
class CastAst final : public ExpressionAst {
  std::shared_ptr<types::DataTypeAst> targetType;
  std::shared_ptr<ExpressionAst> expr;
  // Resolved type of the cast target. Populated during the resolve pass
  // and used for type validation of the cast operation.
  std::shared_ptr<symTable::Type> resolvedTargetType;

public:
  explicit CastAst(antlr4::Token *token) : ExpressionAst(token){};

  std::shared_ptr<types::DataTypeAst> getTargetType() { return targetType; };
  void setType(std::shared_ptr<types::DataTypeAst> type_) { targetType = type_; }
  std::shared_ptr<ExpressionAst> getExpression() const { return expr; }
  void setExpression(std::shared_ptr<ExpressionAst> expr_) { expr = expr_; }
  std::shared_ptr<symTable::Type> getResolvedTargetType() const { return resolvedTargetType; }
  void setResolvedTargetType(std::shared_ptr<symTable::Type> resolvedType_) {
    resolvedTargetType = resolvedType_;
  }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions