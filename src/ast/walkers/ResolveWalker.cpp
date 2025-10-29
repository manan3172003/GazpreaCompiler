#include "ast/types/AliasTypeAst.h"
#include "symTable/MethodSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/TypealiasSymbol.h"
#include "symTable/VariableSymbol.h"

#include <ast/walkers/ResolveWalker.h>

namespace gazprea::ast::walkers {
std::shared_ptr<symTable::Type> ResolveWalker::resolvedType(
    const std::shared_ptr<types::DataTypeAst> &dataType) {
  auto globalScope = symTab->getGlobalScope();
  switch (dataType->getNodeType()) {
  case NodeType::IntegerType:
    return std::dynamic_pointer_cast<symTable::Type>(
        globalScope->resolveType("integer"));
  case NodeType::RealType:
    return std::dynamic_pointer_cast<symTable::Type>(
        globalScope->resolveType("real"));
  case NodeType::CharType:
    return std::dynamic_pointer_cast<symTable::Type>(
        globalScope->resolveType("character"));
  case NodeType::BoolType:
    return std::dynamic_pointer_cast<symTable::Type>(
        globalScope->resolveType("boolean"));
  case NodeType::AliasType: {
    const auto aliasTypeNode =
        std::dynamic_pointer_cast<types::AliasTypeAst>(dataType);
    const auto aliasSymType =
        globalScope->resolveType(aliasTypeNode->getAlias());
    return std::dynamic_pointer_cast<symTable::Type>(aliasSymType);
  }
  case NodeType::TupleType: {
    // visiting the tuple
    visit(dataType);
    return std::dynamic_pointer_cast<symTable::Type>(dataType->getSymbol());
  }
  default:
    return {};
  }
}
std::any ResolveWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}
std::any
ResolveWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  visit(ctx->getLVal());
  visit(ctx->getExpr());
  return {};
}
std::any ResolveWalker::visitDeclaration(
    std::shared_ptr<statements::DeclarationAst> ctx) {
  if (ctx->getExpr()) {
    visit(ctx->getExpr());
  }

  const auto varSymb =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  varSymb->setType(resolvedType(ctx->getType()));
  return {};
}
std::any
ResolveWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  visit(ctx->getLeft());
  visit(ctx->getRight());
  return {};
}
std::any ResolveWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  return {};
}
std::any ResolveWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  return AstWalker::visitBreak(ctx);
}
std::any
ResolveWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  return AstWalker::visitContinue(ctx);
}
std::any ResolveWalker::visitConditional(
    std::shared_ptr<statements::ConditionalAst> ctx) {
  visit(ctx->getCondition());
  visit(ctx->getThenBody());
  if (ctx->getElseBody()) {
    visit(ctx->getElseBody());
  }
  return {};
}

std::any ResolveWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  visit(ctx->getLVal());
  return {};
}
std::any
ResolveWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  visit(ctx->getExpression());
  return {};
}
std::any
ResolveWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  visit(ctx->getProto());
  visit(ctx->getBody());
  return {};
}
std::any ResolveWalker::visitProcedureParams(
    std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  const auto varSym =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  varSym->setType(resolvedType(ctx->getParamType()));
  return {};
}
std::any ResolveWalker::visitProcedureCall(
    std::shared_ptr<statements::ProcedureCallAst> ctx) {
  return AstWalker::visitProcedureCall(ctx);
}
std::any
ResolveWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  return AstWalker::visitReturn(ctx);
}
std::any ResolveWalker::visitTupleAssign(
    std::shared_ptr<statements::TupleAssignAst> ctx) {
  return AstWalker::visitTupleAssign(ctx);
}
std::any ResolveWalker::visitTupleAccess(
    std::shared_ptr<expressions::TupleAccessAst> ctx) {
  ctx->setSymbol(ctx->getScope()->resolveSymbol(ctx->getTupleName()));
  return {};
}
std::any
ResolveWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  return AstWalker::visitTuple(ctx);
}
std::any
ResolveWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(ctx->getSymbol());
  for (const auto &subType : ctx->getTypes()) {
    tupleTypeSymbol->addResolvedType(resolvedType(subType));
  }
  return {};
}
std::any
ResolveWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  auto typealiasSymbol =
      std::dynamic_pointer_cast<symTable::TypealiasSymbol>(ctx->getSymbol());
  typealiasSymbol->setType(resolvedType(ctx->getType()));
  return {};
}
std::any
ResolveWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  visit(ctx->getProto());
  visit(ctx->getBody());
  return {};
}
std::any ResolveWalker::visitFunctionParam(
    std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  const auto varSym =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  varSym->setType(resolvedType(ctx->getParamType()));
  return {};
}
std::any
ResolveWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  for (const auto &param : ctx->getParams()) {
    visit(param);
  }
  const auto methodSym =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (ctx->getReturnType() != nullptr) { // void procedures can have empty type
    methodSym->setReturnType(resolvedType(ctx->getReturnType()));
  }
  return {};
}
std::any ResolveWalker::visitFuncProcCall(
    std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  return AstWalker::visitFuncProcCall(ctx);
}
std::any ResolveWalker::visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
  return AstWalker::visitArg(ctx);
}
std::any
ResolveWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  return AstWalker::visitBool(ctx);
}
std::any ResolveWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  return AstWalker::visitCast(ctx);
}
std::any
ResolveWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  return AstWalker::visitChar(ctx);
}
std::any ResolveWalker::visitIdentifier(
    std::shared_ptr<expressions::IdentifierAst> ctx) {
  ctx->setSymbol(ctx->getScope()->resolveSymbol(ctx->getName()));
  return {};
}
std::any ResolveWalker::visitIdentifierLeft(
    std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  return AstWalker::visitIdentifierLeft(ctx);
}
std::any ResolveWalker::visitInteger(
    std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  return AstWalker::visitInteger(ctx);
}
std::any
ResolveWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  return AstWalker::visitReal(ctx);
}
std::any ResolveWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());
  return {};
}
std::any ResolveWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
  if (ctx->getCondition())
    visit(ctx->getCondition());
  visit(ctx->getBody());
  return {};
}
std::any ResolveWalker::visitIteratorLoop(
    std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers