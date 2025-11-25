#pragma once
#include "StatementAst.h"
#include "ast/expressions/ArgAst.h"

namespace gazprea::ast::statements {
enum class MemberFunctionType { Concat, Push, Append, Len };

class MemberFunctionAst final : public StatementAst {
  // Anything that's on the left of the dot operator
  std::shared_ptr<Ast> left;
  MemberFunctionType memberType;
  std::vector<std::shared_ptr<expressions::ArgAst>> args = {};

public:
  MemberFunctionAst(antlr4::Token *token, const MemberFunctionType method)
      : StatementAst(token), memberType(method) {}

  void setLeft(const std::shared_ptr<Ast> &_left) { left = _left; }
  const std::shared_ptr<Ast> &getLeft() const { return left; }

  void setMemberFunctionType(const MemberFunctionType method) { memberType = method; }
  MemberFunctionType getMemberFunctionType() const { return memberType; }

  void addArg(const std::shared_ptr<expressions::ArgAst> &arg) { args.push_back(arg); }
  void setArgs(std::vector<std::shared_ptr<expressions::ArgAst>> _args) { args = _args; }
  const std::vector<std::shared_ptr<expressions::ArgAst>> &getArgs() const { return args; }

  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;

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
} // namespace gazprea::ast::statements