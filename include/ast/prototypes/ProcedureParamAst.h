#pragma once
#include "ast/Ast.h"

namespace gazprea::ast::prototypes {
class ProcedureParamAst final : public Ast {
  Qualifier qualifier;
  std::string type;
  std::string name;

public:
  explicit ProcedureParamAst(antlr4::Token *token) : Ast(token), qualifier() {}
  std::string &getName() { return name; }
  void setName(const std::string &name_) { name = name_; }
  std::string &getType() { return type; }
  void setType(const std::string &type_) { type = type_; }
  Qualifier &getQualifier() { return qualifier; }
  void setQualifier(const Qualifier &qualifier_) { qualifier = qualifier_; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
}; // namespace gazprea::ast::prototypes