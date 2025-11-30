#pragma once
#include "StatementAst.h"
#include "ast/expressions/ArgAst.h"

namespace gazprea::ast::statements {
enum class MemberFunctionType { Concat, Push, Append, Len };

class MemberFunctionAst : public StatementAst, public expressions::ExpressionAst {
  // Any expression that's on the left of the dot operator
  std::shared_ptr<expressions::ExpressionAst> left;
  MemberFunctionType memberType;
  std::vector<std::shared_ptr<expressions::ArgAst>> args = {};

public:
  MemberFunctionAst(antlr4::Token *token, const MemberFunctionType method)
      : Ast(token), StatementAst(token), ExpressionAst(token), memberType(method) {}

  void setLeft(const std::shared_ptr<expressions::ExpressionAst> &_left) { left = _left; }
  const std::shared_ptr<expressions::ExpressionAst> &getLeft() const { return left; }

  void setMemberFunctionType(const MemberFunctionType method) { memberType = method; }
  MemberFunctionType getMemberFunctionType() const { return memberType; }

  void addArg(const std::shared_ptr<expressions::ArgAst> &arg) { args.push_back(arg); }
  void setArgs(std::vector<std::shared_ptr<expressions::ArgAst>> _args) { args = _args; }
  const std::vector<std::shared_ptr<expressions::ArgAst>> &getArgs() const { return args; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;

  bool isLValue() override { return false; }

  static std::string MemberFunctionTypeToString(const MemberFunctionType method) {
    switch (method) {
    case MemberFunctionType::Concat:
      return "Concat";
    case MemberFunctionType::Push:
      return "Push";
    case MemberFunctionType::Append:
      return "Append";
    case MemberFunctionType::Len:
      return "Len";
    default:
      return "UnknownBuiltin";
    }
  }
};
class LenMemberFuncAst final : public MemberFunctionAst {
public:
  explicit LenMemberFuncAst(antlr4::Token *token)
      : Ast(token), MemberFunctionAst(token, MemberFunctionType::Len) {}

  NodeType getNodeType() const override { return NodeType::LenMemberFunc; };
};

class AppendMemberFuncAst final : public MemberFunctionAst {
public:
  explicit AppendMemberFuncAst(antlr4::Token *token)
      : Ast(token), MemberFunctionAst(token, MemberFunctionType::Append) {}

  NodeType getNodeType() const override { return NodeType::AppendMemberFunc; }
};
class PushMemberFuncAst final : public MemberFunctionAst {
public:
  explicit PushMemberFuncAst(antlr4::Token *token)
      : Ast(token), MemberFunctionAst(token, MemberFunctionType::Push) {}

  NodeType getNodeType() const override { return NodeType::PushMemberFunc; }
};
class ConcatMemberFuncAst final : public MemberFunctionAst {
public:
  explicit ConcatMemberFuncAst(antlr4::Token *token)
      : Ast(token), MemberFunctionAst(token, MemberFunctionType::Concat) {}

  NodeType getNodeType() const override { return NodeType::ConcatMemberFunc; }
};
} // namespace gazprea::ast::statements