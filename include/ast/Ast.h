#pragma once

#include <Token.h>
#include <string>

namespace gazprea::ast {
enum class NodeType {
  Arg,
  Assignment,
  Block,
  Break,
  Bool,
  Continue,
  Conditional,
  Cast,
  Char,
  Declaration,
  Function,
  FunctionParam,
  FuncProcCall,
  Procedure,
  ProcedureParam,
  ProcedureCall,
  Prototype,
  Output,
  Identifier,
  IdentifierLeft,
  Integer,
  Input,
  Real,
  Return,
  Root,
  TupleAccess,
  Typealias,
  TupleAssign,
  UnaryExpression
};

enum class Qualifier {
  Var,
  Const,
};

enum class BoolValue { TRUE, FALSE };

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