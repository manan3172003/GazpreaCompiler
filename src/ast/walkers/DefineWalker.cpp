#include "symTable/VariableSymbol.h"

#include <ast/walkers/DefineWalker.h>
#include <symTable/MethodSymbol.h>

namespace gazprea::ast::walkers {
std::any DefineWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}
std::any
DefineWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  return AstWalker::visitAssignment(ctx);
}
std::any DefineWalker::visitDeclaration(
    std::shared_ptr<statements::DeclarationAst> ctx) {
  if (ctx->getExpr()) {
    visit(ctx->getExpr());
  }
  const auto varSymbol = std::make_shared<symTable::VariableSymbol>(
      ctx->getName(), ctx->getQualifier());
  symTab->getCurrentScope()->define(varSymbol);
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
  symTab->pushScope(std::make_shared<symTable::LocalScope>());
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  symTab->popScope();
  return {};
}
std::any DefineWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  return AstWalker::visitBreak(ctx);
}
std::any
DefineWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  return AstWalker::visitContinue(ctx);
}
std::any DefineWalker::visitConditional(
    std::shared_ptr<statements::ConditionalAst> ctx) {
  return AstWalker::visitConditional(ctx);
}
std::any DefineWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  return AstWalker::visitInput(ctx);
}
std::any DefineWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  return AstWalker::visitOutput(ctx);
}
std::any
DefineWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  const auto methodSymbol = std::make_shared<symTable::MethodSymbol>(
      ctx->getProto()->getName(), ctx->getProto()->getProtoType());

  symTab->getCurrentScope()->define(methodSymbol);
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
  symTab->getCurrentScope()->define(varSymbol);
  ctx->setSymbol(varSymbol);
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
  return AstWalker::visitTupleAssign(ctx);
}
std::any DefineWalker::visitTupleAccess(
    std::shared_ptr<expressions::TupleAccessAst> ctx) {
  return AstWalker::visitTupleAccess(ctx);
}
std::any
DefineWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  return AstWalker::visitTuple(ctx);
}
std::any
DefineWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  return AstWalker::visitTupleType(ctx);
}
std::any
DefineWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  return AstWalker::visitTypealias(ctx);
}
std::any
DefineWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  return AstWalker::visitFunction(ctx);
}
std::any DefineWalker::visitFunctionParam(
    std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  return AstWalker::visitFunctionParam(ctx);
}
std::any
DefineWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  for (const auto &param : ctx->getParams()) {
    visit(param);
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
  return AstWalker::visitIdentifierLeft(ctx);
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
  return AstWalker::visitUnary(ctx);
}
std::any DefineWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
  return AstWalker::visitLoop(ctx);
}
std::any DefineWalker::visitIteratorLoop(
    std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers