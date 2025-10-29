#include "ast/types/AliasTypeAst.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/TypealiasSymbol.h"
#include "symTable/VariableSymbol.h"

#include <ast/walkers/DefineWalker.h>
#include <symTable/MethodSymbol.h>

namespace gazprea::ast::walkers {
std::any DefineWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  ctx->setScope(symTab->getGlobalScope());
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}
std::any
DefineWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  visit(ctx->getExpr());
  visit(ctx->getLVal());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefineWalker::visitDeclaration(
    std::shared_ptr<statements::DeclarationAst> ctx) {
  if (ctx->getExpr()) {
    visit(ctx->getExpr());
  }
  const auto varSymbol = std::make_shared<symTable::VariableSymbol>(
      ctx->getName(), ctx->getQualifier());
  varSymbol->setDef(ctx);

  if (ctx->getType()->getNodeType() == NodeType::TupleType) {
    visit(ctx->getType());
  }

  symTab->getCurrentScope()->defineSymbol(varSymbol);
  ctx->setScope(symTab->getCurrentScope());
  ctx->setSymbol(varSymbol);
  return {};
}
std::any
DefineWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  visit(ctx->getLeft());
  visit(ctx->getRight());
  return {};
}
std::any DefineWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  auto newScope = std::make_shared<symTable::LocalScope>();
  ctx->setScope(newScope);
  symTab->pushScope(newScope);
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  symTab->popScope();
  return {};
}
std::any DefineWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  return {};
}
std::any
DefineWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  return {};
}
std::any DefineWalker::visitConditional(
    std::shared_ptr<statements::ConditionalAst> ctx) {
  visit(ctx->getCondition());
  visit(ctx->getThenBody());
  if (ctx->getElseBody()) {
    visit(ctx->getElseBody());
  }
  return {};
}
std::any DefineWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  visit(ctx->getLVal());
  return {};
}
std::any DefineWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  visit(ctx->getExpression());
  return {};
}
std::any
DefineWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  const auto methodSymbol = std::make_shared<symTable::MethodSymbol>(
      ctx->getProto()->getName(), ctx->getProto()->getProtoType());

  ctx->getProto()->setSymbol(methodSymbol);
  // Method Sym points to its prototype AST node which contains the method
  methodSymbol->setDef(ctx->getProto());

  symTab->getCurrentScope()->defineSymbol(methodSymbol);
  ctx->setScope(symTab->getCurrentScope());

  symTab->pushScope(methodSymbol);

  visit(ctx->getProto()); // visit the params
  visit(ctx->getBody());

  symTab->popScope();
  return {};
}
std::any DefineWalker::visitProcedureParams(
    std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  const auto varSymbol = std::make_shared<symTable::VariableSymbol>(
      ctx->getName(), ctx->getQualifier());
  ctx->setScope(symTab->getCurrentScope());
  symTab->getCurrentScope()->defineSymbol(varSymbol);
  if (ctx->getParamType()->getNodeType() == NodeType::TupleType) {
    visit(ctx->getParamType());
  }
  ctx->setSymbol(varSymbol);
  varSymbol->setDef(ctx);
  return {};
}
std::any DefineWalker::visitProcedureCall(
    std::shared_ptr<statements::ProcedureCallAst> ctx) {
  return AstWalker::visitProcedureCall(ctx);
}
std::any DefineWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  visit(ctx->getExpr());
  return {};
}
std::any DefineWalker::visitTupleAssign(
    std::shared_ptr<statements::TupleAssignAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefineWalker::visitTupleAccess(
    std::shared_ptr<expressions::TupleAccessAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any
DefineWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  for (const auto &element : ctx->getElements()) {
    visit(element);
  }
  return {};
}
std::any
DefineWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  auto tupTypeSymbol = std::make_shared<symTable::TupleTypeSymbol>("");
  tupTypeSymbol->setDef(ctx);
  for (const auto &typeAst : ctx->getTypes()) {
    switch (typeAst->getNodeType()) {
    case NodeType::IntegerType:
      tupTypeSymbol->addUnresolvedType("integer");
      break;
    case NodeType::RealType:
      tupTypeSymbol->addUnresolvedType("real");
      break;
    case NodeType::CharType:
      tupTypeSymbol->addUnresolvedType("character");
      break;
    case NodeType::BoolType:
      tupTypeSymbol->addUnresolvedType("boolean");
      break;
    case NodeType::AliasType: {
      const auto aliasTypeNode =
          std::dynamic_pointer_cast<types::AliasTypeAst>(typeAst);
      tupTypeSymbol->addUnresolvedType(aliasTypeNode->getAlias());
      break;
    }
    default:
      throw std::runtime_error("Unhandled node type");
    }
  }
  ctx->setSymbol(tupTypeSymbol);
  return {};
}
std::any
DefineWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  auto typealiasSymbol =
      std::make_shared<symTable::TypealiasSymbol>(ctx->getAlias());
  typealiasSymbol->setDef(ctx);
  symTab->getGlobalScope()->defineTypeSymbol(typealiasSymbol);
  if (ctx->getType()->getNodeType() == NodeType::TupleType)
    visit(ctx->getType());
  ctx->setSymbol(typealiasSymbol);
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any
DefineWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  const auto methodSymbol = std::make_shared<symTable::MethodSymbol>(
      ctx->getProto()->getName(), ctx->getProto()->getProtoType());
  ctx->getProto()->setSymbol(methodSymbol);
  // Method Sym points to its prototype AST node which contains the method
  methodSymbol->setDef(ctx->getProto());
  symTab->getCurrentScope()->defineSymbol(methodSymbol);
  ctx->setScope(symTab->getCurrentScope());
  symTab->pushScope(methodSymbol);
  visit(ctx->getProto()); // visit the params
  visit(ctx->getBody());
  symTab->popScope();
  return {};
}
std::any DefineWalker::visitFunctionParam(
    std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  const auto varSymbol = std::make_shared<symTable::VariableSymbol>(
      ctx->getName(), ctx->getQualifier());
  ctx->setScope(symTab->getCurrentScope());
  symTab->getCurrentScope()->defineSymbol(varSymbol);

  if (ctx->getParamType()->getNodeType() == NodeType::TupleType) {
    visit(ctx->getParamType());
  }

  ctx->setSymbol(varSymbol);
  varSymbol->setDef(ctx);
  return {};
}
std::any
DefineWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  for (const auto &param : ctx->getParams()) {
    visit(param);
  }
  if (ctx->getReturnType() &&
      ctx->getReturnType()->getNodeType() == NodeType::TupleType) {
    visit(ctx->getReturnType());
  }
  return {};
}
std::any DefineWalker::visitFuncProcCall(
    std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  return AstWalker::visitFuncProcCall(ctx);
}
std::any DefineWalker::visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
  return AstWalker::visitArg(ctx);
}
std::any
DefineWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  return AstWalker::visitBool(ctx);
}
std::any DefineWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  return AstWalker::visitCast(ctx);
}
std::any
DefineWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  return AstWalker::visitChar(ctx);
}
std::any
DefineWalker::visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefineWalker::visitIdentifierLeft(
    std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefineWalker::visitInteger(
    std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  return AstWalker::visitInteger(ctx);
}
std::any
DefineWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  return AstWalker::visitReal(ctx);
}
std::any DefineWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());
  return {};
}
std::any DefineWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
  if (ctx->getCondition())
    visit(ctx->getCondition());
  visit(ctx->getBody());
  return {};
}
std::any DefineWalker::visitIteratorLoop(
    std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers