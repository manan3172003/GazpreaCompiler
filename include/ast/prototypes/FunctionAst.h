#pragma once
#include <utility>

#include "PrototypeAst.h"

namespace gazprea::ast::prototypes {
class FunctionAst final : public Ast {
  std::shared_ptr<PrototypeAst> proto;
  std::shared_ptr<Ast> body;

public:
  explicit FunctionAst(antlr4::Token *token) : Ast(token) {};

  void setProto(std::shared_ptr<PrototypeAst> proto_) {
    proto = std::move(proto_);
  }
  std::shared_ptr<PrototypeAst> getProto() const { return proto; }
  void setBody(std::shared_ptr<Ast> body_) { body = std::move(body_); }
  std::shared_ptr<Ast> getBody() const { return body; }

  NodeType getNodeType() const override;
  std::string toStringTree() const override;
};
} // namespace gazprea::ast::prototypes