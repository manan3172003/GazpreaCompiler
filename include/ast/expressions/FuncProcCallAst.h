#pragma once
#include "ArgAst.h"
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
class FuncProcCallAst final : public ExpressionAst {
  // This class can represent both function and procedure calls in expressions
  std::string name;
  std::vector<std::shared_ptr<ArgAst>> args;

public:
  explicit FuncProcCallAst(antlr4::Token *token) : ExpressionAst(token) {}

  void setName(const std::string &name_) { name = name_; }
  std::string getName() const { return name; }
  void setArgs(const std::vector<std::shared_ptr<ArgAst>> &args_) { args = args_; }
  std::vector<std::shared_ptr<ArgAst>> getArgs() const { return args; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::expressions