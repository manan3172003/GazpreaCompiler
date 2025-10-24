#pragma once
#include "StatementAst.h"

namespace gazprea::ast::statements {
class TypealiasAst : public StatementAst {
private:
  std::string type;
  std::string alias;

public:
  TypealiasAst(antlr4::Token *token) : StatementAst(token) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  std::string getType() const;
  void setType(const std::string &type);
  std::string getAlias() const;
  void setAlias(const std::string &alias);
};
} // namespace gazprea::ast::statements