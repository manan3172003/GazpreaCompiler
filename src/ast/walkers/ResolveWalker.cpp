#include <ast/walkers/ResolveWalker.h>

namespace gazprea::ast::walkers {
std::shared_ptr<symTable::Type>
ResolveWalker::resolveType(std::string type) const {
  const auto symbol = symTab->getCurrentScope()->resolve(type);
  auto resolvedType = std::dynamic_pointer_cast<symTable::Type>(symbol);
  return resolvedType;
}
std::any ResolveWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  return AstWalker::visitRoot(ctx);
}
std::any
ResolveWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  return AstWalker::visitAssignment(ctx);
}
std::any ResolveWalker::visitDeclaration(
    std::shared_ptr<statements::DeclarationAst> ctx) {
  return AstWalker::visitDeclaration(ctx);
}
std::any ResolveWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  return AstWalker::visitBlock(ctx);
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
  return AstWalker::visitOutput(ctx);
}
std::any
ResolveWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  return AstWalker::visitProcedure(ctx);
}
std::any ResolveWalker::visitProcedureParams(
    std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  return AstWalker::visitProcedureParams(ctx);
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
ResolveWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  return AstWalker::visitTypealias(ctx);
}
std::any
ResolveWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  return AstWalker::visitFunction(ctx);
}
std::any ResolveWalker::visitFunctionParam(
    std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  return AstWalker::visitFunctionParam(ctx);
}
std::any
ResolveWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  return AstWalker::visitPrototype(ctx);
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
  return AstWalker::visitIdentifier(ctx);
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
  return AstWalker::visitLoop(ctx);
}
std::any ResolveWalker::visitIteratorLoop(
    std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers