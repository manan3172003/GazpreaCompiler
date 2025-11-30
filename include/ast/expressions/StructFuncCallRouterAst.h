#pragma once
#include "ExpressionAst.h"
#include "FuncProcCallAst.h"
#include "StructLiteralAst.h"

namespace gazprea::ast::expressions {

class StructFuncCallRouterAst final : public ExpressionAst {
private:
  std::string callName; // name of the func/procedure/struct that is being called
  bool isStruct;
  std::shared_ptr<FuncProcCallAst> funcProcCallAst;
  std::shared_ptr<StructLiteralAst> structLiteralAst;

public:
  explicit StructFuncCallRouterAst(antlr4::Token *token)
      : Ast(token), ExpressionAst(token), isStruct(false) {}

  void setIsStruct(bool isStructCall);
  void setCallName(const std::string &callId);
  void setFuncProcCallAst(const std::shared_ptr<FuncProcCallAst> &createdFuncProcCallAst);
  void setStructLiteralAst(const std::shared_ptr<StructLiteralAst> &createdStructLiteralAst);

  bool getIsStruct() const;
  std::string getCallName() const;
  std::shared_ptr<FuncProcCallAst> getFuncProcCallAst() const;
  std::shared_ptr<StructLiteralAst> getStructLiteralAst() const;

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override;
};

} // namespace gazprea::ast::expressions