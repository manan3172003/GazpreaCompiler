#include "ast/types/AliasTypeAst.h"
#include "symTable/MethodSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/TypealiasSymbol.h"
#include "symTable/VariableSymbol.h"

#include <ast/walkers/ResolveWalker.h>

namespace gazprea::ast::walkers {
std::shared_ptr<symTable::Type> ResolveWalker::resolveType(std::string type) {
  if (type == "integer" || type == "real" || type == "character" ||
      type == "boolean") {
    return std::dynamic_pointer_cast<symTable::Type>(
        symTab->getGlobalScope()->resolve(type));
  }
  if (symTab->getGlobalScope()->getSymbols().find(type) ==
      symTab->getGlobalScope()->getSymbols().end()) {
    return nullptr;
  }
  auto typeSym = std::dynamic_pointer_cast<symTable::TypealiasSymbol>(
      symTab->getGlobalScope()->resolve(type));
  // TODO: do the same thing for structs
  if (typeSym->getType()->getName() == "tuple") {
    auto tupleTypeSym = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        typeSym->getType());
    return typeSym->getType();
  }
  return resolveType(typeSym->getType()->getName());
}
std::any ResolveWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}
std::any
ResolveWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  return AstWalker::visitAssignment(ctx);
}
std::any ResolveWalker::visitDeclaration(
    std::shared_ptr<statements::DeclarationAst> ctx) {
  if (ctx->getExpr()) {
    visit(ctx->getExpr());
  }
  const auto typeNode = ctx->getType()->getNodeType();
  const auto varSymb =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  switch (typeNode) {
  case NodeType::IntegerType:
    varSymb->setType(resolveType("integer"));
    break;
  case NodeType::RealType:
    varSymb->setType(resolveType("real"));
    break;
  case NodeType::CharType:
    varSymb->setType(resolveType("character"));
    break;
  case NodeType::BoolType:
    varSymb->setType(resolveType("boolean"));
    break;
  case NodeType::AliasType: {
    const auto aliasTypeNode =
        std::dynamic_pointer_cast<types::AliasTypeAst>(ctx->getType());
    const auto aliasSymType = resolveType(aliasTypeNode->getAlias());
    varSymb->setType(aliasSymType);
  } break;
  case NodeType::TupleType: {
    // visiting the tuple
    visit(ctx->getType());
    varSymb->setType(
        std::dynamic_pointer_cast<symTable::Type>(ctx->getType()->getSymbol()));
  } break;
  default:
    return {};
  }

  return {};
}
std::any
ResolveWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  return AstWalker::visitBinary(ctx);
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
  return AstWalker::visitConditional(ctx);
}
std::any ResolveWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  return AstWalker::visitInput(ctx);
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
  const auto resolvedType = resolveType(ctx->getType());
  varSym->setType(resolvedType);
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
  return AstWalker::visitTupleAccess(ctx);
}
std::any
ResolveWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  return AstWalker::visitTuple(ctx);
}
std::any
ResolveWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(ctx->getSymbol());
  for (const auto &subType : tupleTypeSymbol->getUnresolvedTypes()) {
    tupleTypeSymbol->addResolvedType(resolveType(subType));
  }
  return {};
}
std::any
ResolveWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  auto typealiasSymbol =
      std::dynamic_pointer_cast<symTable::TypealiasSymbol>(ctx->getSymbol());
  switch (ctx->getType()->getNodeType()) {
  case NodeType::TupleType: {
    visit(ctx->getType());
    auto resolvedType =
        std::dynamic_pointer_cast<symTable::Type>(ctx->getType()->getSymbol());
    typealiasSymbol->setType(resolvedType);
  } break;
  // TODO: Add struct type alias
  case NodeType::IntegerType: {
    auto resolvedType = resolveType("integer");
    typealiasSymbol->setType(resolvedType);
  } break;
  case NodeType::CharType: {
    auto resolvedType = resolveType("character");
    typealiasSymbol->setType(resolvedType);
  } break;
  case NodeType::BoolType: {
    auto resolvedType = resolveType("boolean");
    typealiasSymbol->setType(resolvedType);
  } break;
  case NodeType::RealType: {
    auto resolvedType = resolveType("real");
    typealiasSymbol->setType(resolvedType);
  } break;
  case NodeType::AliasType: {
    auto resolvedType = resolveType(
        std::dynamic_pointer_cast<types::AliasTypeAst>(ctx->getType())
            ->getAlias());
    typealiasSymbol->setType(resolvedType);
  } break;
  default: {
    throw std::runtime_error("Unknown node type");
  }
  }
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
  const auto resolvedType = resolveType(ctx->getType());
  varSym->setType(resolvedType);
  return {};
}
std::any
ResolveWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  for (const auto &param : ctx->getParams()) {
    visit(param);
  }
  const auto methodSym =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (ctx->getType() != "") { // void procedures can have empty type
    methodSym->setReturnType(resolveType(ctx->getType()));
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
  ctx->setSymbol(ctx->getScope()->resolve(ctx->getName()));
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
  return AstWalker::visitUnary(ctx);
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