#pragma once
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"
#include <memory>
#include <string>

namespace gazprea::ast::statements {
class AssignmentAst : public StatementAst {
public:
  std::string name;
  std::shared_ptr<expressions::ExpressionAst> expr;
  AssignmentAst(antlr4::Token *token) : StatementAst(token) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~AssignmentAst() override = default;
};
} // namespace gazprea::ast::statements