#pragma once
#include "StatementAst.h"
#include "ast/types/IntegerTypeAst.h"

namespace gazprea::ast::statements {
class TypealiasAst : public StatementAst {
private:
  std::shared_ptr<types::DataTypeAst> type;
  std::string alias;

public:
  TypealiasAst(antlr4::Token *token) : Ast(token), StatementAst(token) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  std::shared_ptr<types::DataTypeAst> getType() const;
  void setType(std::shared_ptr<types::DataTypeAst> type);
  std::string getAlias() const;
  void setAlias(const std::string &alias);
};
} // namespace gazprea::ast::statements