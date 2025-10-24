#pragma once

#include <Token.h>
#include <string>

namespace gazprea::ast {
enum class NodeType {
  Assignment,
  Block,
  Declaration,
  Function,
  FunctionParam,
  UnaryExpression,
  Output,
  Identifier,
  Integer,
  Input,
  Prototype,
  Real,
  Return,
  Root,
  TupleAccess
};

enum class Qualifier {
  Var,
  Const,
};

class Ast {
protected:
  antlr4::Token *token;

public:
  std::string indent = ". . ";

  explicit Ast(antlr4::Token *token) : token(token) {};
  virtual NodeType getNodeType() const = 0;
  virtual std::string toStringTree(std::string prefix) const = 0;
  static std::string qualifierToString(Qualifier qualifier);
  virtual ~Ast() = default;
};
} // namespace gazprea::ast