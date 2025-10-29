#include "ast/types/AliasTypeAst.h"
#include "ast/types/BooleanTypeAst.h"
#include "ast/types/CharacterTypeAst.h"
#include "ast/types/RealTypeAst.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"

#include <ast/walkers/TypeWalker.h>

namespace gazprea::ast::walkers {

std::shared_ptr<symTable::Type> TypeWalker::resolvedInferredType(
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
    auto tupleTypeSymbol = std::make_shared<symTable::TupleTypeSymbol>("");
    auto tupleDataType =
        std::dynamic_pointer_cast<types::TupleTypeAst>(dataType);
    for (const auto &subType : tupleDataType->getTypes()) {
      tupleTypeSymbol->addResolvedType(resolvedInferredType(subType));
    }
    return std::dynamic_pointer_cast<symTable::Type>(tupleTypeSymbol);
  }
  default:
    return {};
  }
}

std::any TypeWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}
std::any
TypeWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  return AstWalker::visitAssignment(ctx);
}
std::any
TypeWalker::visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) {
  visit(ctx->getExpr());
  if (ctx->getType()) {
    // setting inferred type here
    // Promoting type here

  } else {
    std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol())
        ->setType(ctx->getExpr()->getInferredSymbolType());
    ctx->setType(ctx->getExpr()->getInferredDataType());
    // TODO: Check type
  }

  return {};
}
std::any TypeWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  return AstWalker::visitBlock(ctx);
}
std::any TypeWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  return AstWalker::visitBinary(ctx);
}
std::any TypeWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  return AstWalker::visitBreak(ctx);
}
std::any
TypeWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  return AstWalker::visitContinue(ctx);
}
std::any
TypeWalker::visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) {
  return AstWalker::visitConditional(ctx);
}
std::any TypeWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  return AstWalker::visitInput(ctx);
}
std::any TypeWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  return AstWalker::visitOutput(ctx);
}
std::any
TypeWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  return AstWalker::visitProcedure(ctx);
}
std::any TypeWalker::visitProcedureParams(
    std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  return AstWalker::visitProcedureParams(ctx);
}
std::any TypeWalker::visitProcedureCall(
    std::shared_ptr<statements::ProcedureCallAst> ctx) {
  return AstWalker::visitProcedureCall(ctx);
}
std::any TypeWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  return AstWalker::visitReturn(ctx);
}
std::any
TypeWalker::visitTupleAssign(std::shared_ptr<statements::TupleAssignAst> ctx) {
  return AstWalker::visitTupleAssign(ctx);
}
std::any
TypeWalker::visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) {
  return AstWalker::visitTupleAccess(ctx);
}
std::any
TypeWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  auto tupleType = std::make_shared<types::TupleTypeAst>(ctx->token);
  for (const auto &element : ctx->getElements()) {
    visit(element);
    tupleType->addType(element->getInferredDataType());
  }
  ctx->setInferredSymbolType(resolvedInferredType(tupleType));
  return {};
}
std::any TypeWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  return AstWalker::visitTupleType(ctx);
}
std::any
TypeWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  return AstWalker::visitTypealias(ctx);
}
std::any
TypeWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  return AstWalker::visitFunction(ctx);
}
std::any TypeWalker::visitFunctionParam(
    std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  return AstWalker::visitFunctionParam(ctx);
}
std::any
TypeWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  return AstWalker::visitPrototype(ctx);
}
std::any TypeWalker::visitFuncProcCall(
    std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  return AstWalker::visitFuncProcCall(ctx);
}
std::any TypeWalker::visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
  return AstWalker::visitArg(ctx);
}
std::any
TypeWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  auto boolType = std::make_shared<types::BooleanTypeAst>(ctx->token);
  ctx->setInferredDataType(boolType);
  ctx->setInferredSymbolType(resolvedInferredType(boolType));
  return {};
}
std::any TypeWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  return AstWalker::visitCast(ctx);
}
std::any
TypeWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  auto charType = std::make_shared<types::CharacterTypeAst>(ctx->token);
  ctx->setInferredDataType(charType);
  ctx->setInferredSymbolType(resolvedInferredType(charType));
  return {};
}
std::any
TypeWalker::visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) {
  auto dataTypeSymbol =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  auto dataType = std::dynamic_pointer_cast<statements::DeclarationAst>(
                      dataTypeSymbol->getDef())
                      ->getType();
  ctx->setInferredDataType(dataType);
  ctx->setInferredSymbolType(resolvedInferredType(dataType));
  return {};
}
std::any TypeWalker::visitIdentifierLeft(
    std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  return AstWalker::visitIdentifierLeft(ctx);
}
std::any
TypeWalker::visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  auto intType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  ctx->setInferredDataType(intType);
  ctx->setInferredSymbolType(resolvedInferredType(intType));
  return {};
}
std::any
TypeWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  auto realType = std::make_shared<types::RealTypeAst>(ctx->token);
  ctx->setInferredDataType(realType);
  ctx->setInferredSymbolType(resolvedInferredType(realType));
  return {};
}
std::any TypeWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  return AstWalker::visitUnary(ctx);
}
std::any TypeWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
  return AstWalker::visitLoop(ctx);
}
std::any TypeWalker::visitIteratorLoop(
    std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers