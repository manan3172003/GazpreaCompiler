#pragma once

#include <Token.h>
#include <memory>
#include <string>

namespace gazprea::ast {
enum class NodeType { Assignment, Integer, Declaration, Real, Root };

enum class Qualifier {
  Var,
  Const,
};

class Ast {
protected:
  antlr4::Token *token;

public:
  Ast(antlr4::Token *token) : token(token) {};
  virtual NodeType getNodeType() const = 0;
  virtual std::string toStringTree() const = 0;
  static std::string qualifierToString(Qualifier qualifier);
  virtual ~Ast() = default;
};
} // namespace gazprea::ast