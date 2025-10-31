#pragma once

#include "AssignLeftAst.h"
#include <ast/statements/StatementAst.h>

namespace gazprea::ast::statements {
class InputAst : public StatementAst {
private:
  std::shared_ptr<AssignLeftAst> lVal;

public:
  InputAst(antlr4::Token *token) : StatementAst(token){};
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  std::shared_ptr<AssignLeftAst> getLVal() const;
  void setLVal(std::shared_ptr<AssignLeftAst> lVal_);
  ~InputAst() override = default;
};
} // namespace gazprea::ast::statements