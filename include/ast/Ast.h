#pragma once

#include <Token.h>
#include <string>

namespace gazprea::ast {
enum class NodeType {
  Real,
  Assignment,
  Integer,
  Declaration,
  Root,
  Block,
  Function,
  Prototype,
  FunctionParam
};

enum class Qualifier {
  Var,
  Const,
};

class Ast {
protected:
  antlr4::Token *token;

public:
  explicit Ast(antlr4::Token *token) : token(token) {};
  virtual NodeType getNodeType() const = 0;
  virtual std::string toStringTree(std::string prefix) const = 0;
  static std::string qualifierToString(Qualifier qualifier);
  virtual ~Ast() = default;
};
} // namespace gazprea::ast