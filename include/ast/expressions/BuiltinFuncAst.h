#pragma once
#include "ArgAst.h"
#include "ExpressionAst.h"

namespace gazprea::ast::expressions {
enum class BuiltinFuncType { Length, Shape, Reverse, Format, StreamState };

class BuiltinFuncAst : public ExpressionAst {
  BuiltinFuncType funcType;

public:
  std::shared_ptr<ExpressionAst> arg;
  explicit BuiltinFuncAst(antlr4::Token *token, const BuiltinFuncType type)
      : Ast(token), ExpressionAst(token), funcType(type) {}

  void setBuiltinFuncType(const BuiltinFuncType type) { funcType = type; }
  BuiltinFuncType getBuiltinFuncType() const { return funcType; }

  NodeType getNodeType() const override { return NodeType::BuiltinFunc; };
  std::string toStringTree(std::string prefix) const override;
  bool isLValue() override { return false; }

  static std::string funcTypeToString(const BuiltinFuncType type);
};

class LengthBuiltinFuncAst final : public BuiltinFuncAst {
public:
  explicit LengthBuiltinFuncAst(antlr4::Token *token)
      : Ast(token), BuiltinFuncAst(token, BuiltinFuncType::Length) {}
  NodeType getNodeType() const override { return NodeType::LengthBuiltin; }
};
class ShapeBuiltinFuncAst final : public BuiltinFuncAst {
public:
  explicit ShapeBuiltinFuncAst(antlr4::Token *token)
      : Ast(token), BuiltinFuncAst(token, BuiltinFuncType::Shape) {}
  NodeType getNodeType() const override { return NodeType::ShapeBuiltin; }
};
class ReverseBuiltinFuncAst final : public BuiltinFuncAst {
public:
  explicit ReverseBuiltinFuncAst(antlr4::Token *token)
      : Ast(token), BuiltinFuncAst(token, BuiltinFuncType::Reverse) {}
  NodeType getNodeType() const override { return NodeType::ReverseBuiltin; }
};
class FormatBuiltinFuncAst final : public BuiltinFuncAst {
public:
  explicit FormatBuiltinFuncAst(antlr4::Token *token)
      : Ast(token), BuiltinFuncAst(token, BuiltinFuncType::Format) {}
  NodeType getNodeType() const override { return NodeType::FormatBuiltin; }
};
class StreamStateBuiltinFuncAst final : public BuiltinFuncAst {
public:
  explicit StreamStateBuiltinFuncAst(antlr4::Token *token)
      : Ast(token), BuiltinFuncAst(token, BuiltinFuncType::StreamState) {}
  NodeType getNodeType() const override { return NodeType::StreamStateBuiltin; }
};

} // namespace gazprea::ast::expressions