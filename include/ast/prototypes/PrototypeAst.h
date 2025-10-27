#pragma once

#include "ast/Ast.h"
#include "symTable/Scope.h"

namespace gazprea::ast::prototypes {
class PrototypeAst final : public Ast {
  symTable::ScopeType protoType;
  std::string name;
  std::vector<std::shared_ptr<Ast>> params;
  std::string type;

public:
  explicit PrototypeAst(antlr4::Token *token) : Ast(token), protoType() {}

  std::string &getName() { return name; }
  void setName(const std::string &name_) { name = name_; }
  std::vector<std::shared_ptr<Ast>> &getParams() { return params; }
  void setParams(const std::vector<std::shared_ptr<Ast>> &params_) {
    params = params_;
  }
  std::string &getType() { return type; }
  void setType(const std::string &type_) { type = type_; }
  symTable::ScopeType getProtoType() const { return protoType; }
  void setProtoType(const symTable::ScopeType protoType_) {
    protoType = protoType_;
  }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::prototypes