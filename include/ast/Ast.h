#pragma once

#include "symTable/Symbol.h"

#include <Token.h>
#include <TokenSource.h>
#include <string>

namespace gazprea::ast {
struct Location {
  int lineNumber;
  int columnNumber;
  std::string fileName;
};
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
  std::shared_ptr<symTable::Scope> scope;
  std::shared_ptr<symTable::Symbol> sym;
  Location location;

public:
  std::string indent = ". . ";
  antlr4::Token *token;

  explicit Ast(antlr4::Token *token) : token(token) {
    location.columnNumber = static_cast<int>(token->getStartIndex());
    location.lineNumber = static_cast<int>(token->getLine());
    location.fileName = token->getTokenSource()->getSourceName();
  };

  std::shared_ptr<symTable::Symbol> getSymbol() { return sym; }
  void setSymbol(std::shared_ptr<symTable::Symbol> symbol) { sym = symbol; }
  std::shared_ptr<symTable::Scope> getScope() { return scope; }
  void setScope(std::shared_ptr<symTable::Scope> scope_) { scope = scope_; }
  int getLineNumber() const { return location.lineNumber; }
  int getColumnNumber() const { return location.columnNumber; }
  const std::string &getFileName() const { return location.fileName; }
  std::string scopeToString() const;

  virtual NodeType getNodeType() const = 0;
  virtual std::string toStringTree(std::string prefix) const = 0;
  static std::string qualifierToString(Qualifier qualifier);
  virtual ~Ast() = default;
};
} // namespace gazprea::ast