#pragma once
#include "ast/Ast.h"
#include "ast/types/DataTypeAst.h"

namespace gazprea::ast::prototypes {
class FunctionParamAst final : public Ast {
  Qualifier qualifier;
  std::shared_ptr<types::DataTypeAst> paramType;
  std::string name;

public:
  explicit FunctionParamAst(antlr4::Token *token) : Ast(token), qualifier() {}

  std::string &getName() { return name; }
  void setName(const std::string &name_) { name = name_; }
  std::shared_ptr<types::DataTypeAst> getParamType() { return paramType; }
  void setParamType(std::shared_ptr<types::DataTypeAst> type) {
    paramType = type;
  }
  Qualifier &getQualifier() { return qualifier; }
  void setQualifier(const Qualifier &qualifier_) { qualifier = qualifier_; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::prototypes