#pragma once
#include "BlockAst.h"
#include "StatementAst.h"
#include "ast/expressions/ExpressionAst.h"
#include <memory>

namespace gazprea::ast::statements {

class LoopAst : public StatementAst {
private:
  std::shared_ptr<BlockAst> body;
  std::shared_ptr<expressions::ExpressionAst> condition;
  bool isPostPredicated;
  bool isInfinite;

public:
  explicit LoopAst(antlr4::Token *token)
      : StatementAst(token), isPostPredicated(false), isInfinite(false) {}

  void setBody(std::shared_ptr<BlockAst> bodyBlock);
  void setCondition(std::shared_ptr<expressions::ExpressionAst> cond);
  void setIsPostPredicated(bool isPost);
  void setIsInfinite(const bool isInfinite_) { isInfinite = isInfinite_; };

  std::shared_ptr<BlockAst> getBody() const;
  std::shared_ptr<expressions::ExpressionAst> getCondition() const;
  bool getIsPostPredicated() const;
  bool getIsInfinite() const { return isInfinite; };

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  ~LoopAst() override = default;
};
} // namespace gazprea::ast::statements
