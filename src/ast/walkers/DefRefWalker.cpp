#include "CompileTimeExceptions.h"
#include "ast/types/AliasTypeAst.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/TypealiasSymbol.h"
#include "symTable/VariableSymbol.h"

#include <ast/walkers/DefRefWalker.h>
#include <symTable/MethodSymbol.h>

namespace gazprea::ast::walkers {
void DefRefWalker::throwIfUndeclaredSymbol(int lineNumber, std::shared_ptr<symTable::Symbol> sym) {
  if (!sym)
    throw SymbolError(lineNumber, "Use of undeclared symbol");
}
void DefRefWalker::throwGlobalError(std::shared_ptr<Ast> ctx) {
  auto curScope = symTab->getCurrentScope();
  if (curScope->getScopeType() != symTable::ScopeType::Global)
    throw GlobalError(ctx->getLineNumber(), "Symbol can only be defined in Global");
}

void DefRefWalker::throwDuplicateSymbolError(std::shared_ptr<Ast> ctx, const std::string &name,
                                             std::shared_ptr<symTable::Scope> curScope,
                                             bool isType) {
  if (isType) {
    if (curScope->getTypeSymbol(name))
      throw SymbolError(ctx->getLineNumber(), "Duplicate symbol");
  } else {
    if (curScope->getSymbol(name))
      throw SymbolError(ctx->getLineNumber(), "Duplicate symbol");
  }
}

std::shared_ptr<symTable::Type>
DefRefWalker::resolvedType(int lineNumber, const std::shared_ptr<types::DataTypeAst> &dataType) {
  std::shared_ptr<symTable::Type> type;
  auto globalScope = symTab->getGlobalScope();
  switch (dataType->getNodeType()) {
  case NodeType::IntegerType:
    type = std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("integer"));
    break;
  case NodeType::RealType:
    type = std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("real"));
    break;
  case NodeType::CharType:
    type = std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("character"));
    break;
  case NodeType::BoolType:
    type = std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("boolean"));
    break;
  case NodeType::AliasType: {
    const auto aliasTypeNode = std::dynamic_pointer_cast<types::AliasTypeAst>(dataType);
    const auto aliasSymType = globalScope->resolveType(aliasTypeNode->getAlias());
    type = std::dynamic_pointer_cast<symTable::Type>(aliasSymType);
    break;
  }
  case NodeType::TupleType: {
    // visiting the tuple
    visit(dataType);
    type = std::dynamic_pointer_cast<symTable::Type>(dataType->getSymbol());
    break;
  }
  default:
    break;
  }
  if (!type)
    throw TypeError(lineNumber, "Undeclared type used");
  return type;
}

std::any DefRefWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  ctx->setScope(symTab->getGlobalScope());
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}
std::any DefRefWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getExpr());
  visit(ctx->getLVal());
  return {};
}
std::any DefRefWalker::visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) {
  throwDuplicateSymbolError(ctx, ctx->getName(), symTab->getCurrentScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  if (ctx->getExpr()) {
    visit(ctx->getExpr());
  }
  const auto varSymbol =
      std::make_shared<symTable::VariableSymbol>(ctx->getName(), ctx->getQualifier());
  if (ctx->getType()) {
    if (ctx->getType()->getNodeType() == NodeType::TupleType) {
      visit(ctx->getType());
    }
    varSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getType()));
  }

  varSymbol->setDef(ctx);
  symTab->getCurrentScope()->defineSymbol(varSymbol);
  ctx->setSymbol(varSymbol);
  return {};
}
std::any DefRefWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  visit(ctx->getLeft());
  visit(ctx->getRight());
  return {};
}
std::any DefRefWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  auto newScope = std::make_shared<symTable::LocalScope>();
  ctx->setScope(newScope);
  symTab->pushScope(newScope);
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  symTab->popScope();
  return {};
}
std::any DefRefWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) { return {}; }
std::any DefRefWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) { return {}; }
std::any DefRefWalker::visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) {
  visit(ctx->getCondition());
  visit(ctx->getThenBody());
  if (ctx->getElseBody()) {
    visit(ctx->getElseBody());
  }
  return {};
}
std::any DefRefWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getLVal());
  return {};
}
std::any DefRefWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getExpression());
  return {};
}
std::any DefRefWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  throwGlobalError(ctx);
  throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  const auto methodSymbol = std::make_shared<symTable::MethodSymbol>(
      ctx->getProto()->getName(), ctx->getProto()->getProtoType());

  ctx->getProto()->setSymbol(methodSymbol);
  // Method Sym points to its prototype AST node which contains the method
  methodSymbol->setDef(ctx->getProto());

  symTab->getCurrentScope()->defineSymbol(methodSymbol);

  symTab->pushScope(methodSymbol);

  visit(ctx->getProto()); // visit the params
  visit(ctx->getBody());

  symTab->popScope();
  return {};
}
std::any DefRefWalker::visitProcedureParams(std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  throwDuplicateSymbolError(ctx, ctx->getName(), symTab->getCurrentScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  const auto varSymbol =
      std::make_shared<symTable::VariableSymbol>(ctx->getName(), ctx->getQualifier());

  if (ctx->getParamType()->getNodeType() == NodeType::TupleType) {
    visit(ctx->getParamType());
  }
  varSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getParamType()));
  varSymbol->setDef(ctx);

  symTab->getCurrentScope()->defineSymbol(varSymbol);
  ctx->setSymbol(varSymbol);
  return {};
}
std::any DefRefWalker::visitProcedureCall(std::shared_ptr<statements::ProcedureCallAst> ctx) {
  for (const auto &args : ctx->getArgs()) {
    visit(args);
  }
  ctx->setScope(symTab->getCurrentScope());

  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      ctx->getScope()->resolveSymbol(ctx->getName()));
  throwIfUndeclaredSymbol(ctx->getLineNumber(), methodSymbol);

  ctx->setSymbol(methodSymbol);
  return {};
}
std::any DefRefWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  visit(ctx->getExpr());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any
DefRefWalker::visitTupleElementAssign(std::shared_ptr<statements::TupleElementAssignAst> ctx) {
  auto symbol = ctx->getScope()->resolveSymbol(ctx->getTupleName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setScope(symTab->getCurrentScope());
  ctx->setSymbol(symbol);
  return {};
}
std::any
DefRefWalker::visitTupleUnpackAssign(std::shared_ptr<statements::TupleUnpackAssignAst> ctx) {
  for (const auto &lVal : ctx->getLVals()) {
    visit(lVal);
  }
  return {};
}
std::any DefRefWalker::visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) {
  auto symbol = ctx->getScope()->resolveSymbol(ctx->getTupleName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setScope(symTab->getCurrentScope());
  ctx->setSymbol(symbol);
  return {};
}
std::any DefRefWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  for (const auto &element : ctx->getElements()) {
    visit(element);
  }
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  auto tupleTypeSymbol = std::make_shared<symTable::TupleTypeSymbol>("");
  for (const auto &subType : ctx->getTypes()) {
    tupleTypeSymbol->addResolvedType(resolvedType(ctx->getLineNumber(), subType));
  }
  tupleTypeSymbol->setDef(ctx);
  ctx->setSymbol(tupleTypeSymbol);
  return {};
}
std::any DefRefWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  throwDuplicateSymbolError(ctx, ctx->getAlias(), symTab->getGlobalScope(), true);
  ctx->setScope(symTab->getCurrentScope());
  auto typealiasSymbol = std::make_shared<symTable::TypealiasSymbol>(ctx->getAlias());
  if (ctx->getType()->getNodeType() == NodeType::TupleType)
    visit(ctx->getType());
  typealiasSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getType()));
  symTab->getGlobalScope()->defineTypeSymbol(typealiasSymbol);
  typealiasSymbol->setDef(ctx);
  ctx->setSymbol(typealiasSymbol);
  return {};
}
std::any DefRefWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  throwGlobalError(ctx);
  throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  const auto methodSymbol = std::make_shared<symTable::MethodSymbol>(
      ctx->getProto()->getName(), ctx->getProto()->getProtoType());
  ctx->getProto()->setSymbol(methodSymbol);
  // Method Sym points to its prototype AST node which contains the method
  methodSymbol->setDef(ctx->getProto());
  symTab->getCurrentScope()->defineSymbol(methodSymbol);
  symTab->pushScope(methodSymbol);
  visit(ctx->getProto()); // visit the params
  visit(ctx->getBody());
  symTab->popScope();
  return {};
}
std::any DefRefWalker::visitFunctionParam(std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  throwDuplicateSymbolError(ctx, ctx->getName(), symTab->getCurrentScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  const auto varSymbol =
      std::make_shared<symTable::VariableSymbol>(ctx->getName(), ctx->getQualifier());

  if (ctx->getParamType()->getNodeType() == NodeType::TupleType) {
    visit(ctx->getParamType());
  }
  varSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getParamType()));
  varSymbol->setDef(ctx);

  ctx->setSymbol(varSymbol);
  symTab->getCurrentScope()->defineSymbol(varSymbol);
  return {};
}
std::any DefRefWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  for (const auto &param : ctx->getParams()) {
    visit(param);
  }
  if (ctx->getReturnType() && ctx->getReturnType()->getNodeType() == NodeType::TupleType) {
    visit(ctx->getReturnType());
  }

  const auto methodSym = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (ctx->getReturnType() != nullptr) { // void procedures can have empty type
    methodSym->setReturnType(resolvedType(ctx->getLineNumber(), ctx->getReturnType()));
  }

  return {};
}
std::any DefRefWalker::visitFuncProcCall(std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  for (const auto &args : ctx->getArgs()) {
    visit(args);
  }
  ctx->setScope(symTab->getCurrentScope());
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      ctx->getScope()->resolveSymbol(ctx->getName()));
  throwIfUndeclaredSymbol(ctx->getLineNumber(), methodSymbol);
  ctx->setSymbol(methodSymbol);
  return {};
}
std::any DefRefWalker::visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
  visit(ctx->getExpr());
  return {};
}
std::any DefRefWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  if (ctx->getTargetType() && ctx->getTargetType()->getNodeType() == NodeType::TupleType) {
    visit(ctx->getTargetType());
  }
  visit(ctx->getExpression());
  ctx->setResolvedTargetType(resolvedType(ctx->getLineNumber(), ctx->getTargetType()));
  return {};
}
std::any DefRefWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  auto symbol = symTab->getCurrentScope()->resolveSymbol(ctx->getName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setSymbol(symbol);
  return {};
}
std::any DefRefWalker::visitIdentifierLeft(std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  auto symbol = symTab->getCurrentScope()->resolveSymbol(ctx->getName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setSymbol(symbol);
  return {};
}
std::any DefRefWalker::visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
  if (ctx->getCondition())
    visit(ctx->getCondition());
  visit(ctx->getBody());
  return {};
}
std::any DefRefWalker::visitIteratorLoop(std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers