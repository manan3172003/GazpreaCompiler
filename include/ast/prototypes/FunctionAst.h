#pragma once
#include <utility>

#include "PrototypeAst.h"
#include "ast/statements/StatementAst.h"

namespace gazprea::ast::prototypes {
class FunctionAst final : public Ast {
  std::shared_ptr<PrototypeAst> proto;
  std::shared_ptr<statements::StatementAst> body;

public:
  explicit FunctionAst(antlr4::Token *token) : Ast(token) {};

  void setProto(std::shared_ptr<PrototypeAst> proto_) { proto = proto_; }
  std::shared_ptr<PrototypeAst> getProto() const { return proto; }
  void setBody(std::shared_ptr<statements::StatementAst> body_) {
    body = body_;
  }
  std::shared_ptr<statements::StatementAst> getBody() const { return body; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::prototypes