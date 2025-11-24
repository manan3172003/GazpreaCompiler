#pragma once

#include "StatementAst.h"
#include "ast/types/DataTypeAst.h"
#include "ast/types/StructTypeAst.h"

namespace gazprea::ast::statements {
class StructDeclarationAst final : public StatementAst {
private:
  std::shared_ptr<types::StructTypeAst> type;

public:
  explicit StructDeclarationAst(antlr4::Token *token) : StatementAst(token) {}

  std::shared_ptr<types::StructTypeAst> getType() const { return type; }
  void setType(std::shared_ptr<types::StructTypeAst> type_) { this->type = type_; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~StructDeclarationAst() override = default;
};
} // namespace gazprea::ast::statements