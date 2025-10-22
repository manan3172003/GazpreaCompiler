#pragma once
#include "types/Type.h"

#include <Token.h>
#include <memory>
#include <string>

namespace gazprea::ast {
enum class NodeType {};

class Ast {
public:
  Ast();
  explicit Ast(antlr4::Token *token);
  explicit Ast(size_t tokenType);
  bool isNil() const;
  static std::string typeToString(types::Type t);
  virtual NodeType getNodeType() const;
  void addChild(std::any t);
  template <class T> void addChild(std::any t);
  void addChild(std::shared_ptr<Ast> t);
  virtual std::string toString() const;
  std::string toStringTree() const;
  virtual ~Ast();
};
} // namespace gazprea::ast