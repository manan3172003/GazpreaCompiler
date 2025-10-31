#pragma once
#include "StatementAst.h"
#include "ast/expressions/ArgAst.h"

namespace gazprea::ast::statements {
class ProcedureCallAst final : public StatementAst {
  std::string name;
  std::vector<std::shared_ptr<expressions::ArgAst>> args;

public:
  explicit ProcedureCallAst(antlr4::Token *token) : StatementAst(token) {}

  void setName(const std::string &name_) { name = name_; }
  std::string getName() const { return name; }
  void setArgs(const std::vector<std::shared_ptr<expressions::ArgAst>> &args_) { args = args_; }
  std::vector<std::shared_ptr<expressions::ArgAst>> getArgs() const { return args; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::statements