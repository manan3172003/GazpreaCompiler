#pragma once

#include "symTable/Symbol.h"

#include <Token.h>
#include <string>

namespace gazprea::ast {
enum class NodeType {
  AliasType,
  Arg,
  Assignment,
  BinaryExpression,
  Block,
  Break,
  BoolLiteral,
  BoolType,
  Continue,
  Conditional,
  Cast,
  CharLiteral,
  CharType,
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
  IntegerLiteral,
  IntegerType,
  Input,
  RealLiteral,
  RealType,
  Loop,
  IteratorLoop,
  Return,
  Root,
  TupleAccess,
  TupleAssign,
  TupleLiteral,
  TupleType,
  Typealias,
  UnaryExpression
};

enum class Qualifier {
  Var,
  Const,
};

class Ast {
protected:
  antlr4::Token *token;
  std::shared_ptr<symTable::Scope> scope;
  std::shared_ptr<symTable::Symbol> sym;

public:
  std::string indent = ". . ";

  explicit Ast(antlr4::Token *token) : token(token) {};

  std::shared_ptr<symTable::Symbol> getSymbol() { return sym; }
  void setSymbol(std::shared_ptr<symTable::Symbol> symbol) { sym = symbol; }
  std::shared_ptr<symTable::Scope> getScope() { return scope; }
  void setScope(std::shared_ptr<symTable::Scope> scope_) { scope = scope_; }

  virtual NodeType getNodeType() const = 0;
  virtual std::string toStringTree(std::string prefix) const = 0;
  static std::string qualifierToString(Qualifier qualifier);
  virtual ~Ast() = default;
};
} // namespace gazprea::ast