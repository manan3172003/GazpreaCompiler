#pragma once

#include <ast/statements/StatementAst.h>

namespace gazprea::ast::statements {
class InputAst : public StatementAst {
private:
  std::string identifier;

public:
  InputAst(antlr4::Token *token) : StatementAst(token) {};
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  std::string getIdentifier() const;
  void setIdentifier(const std::string &identifier);
  ~InputAst() override = default;
};
} // namespace gazprea::ast::statements