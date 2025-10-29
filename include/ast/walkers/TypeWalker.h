#pragma once
#include "AstWalker.h"
#include "symTable/SymTable.h"

namespace gazprea::ast::walkers {
class TypeWalker final : public AstWalker {
  std::shared_ptr<symTable::SymbolTable> symTab;
public:
  explicit TypeWalker(std::shared_ptr<symTable::SymbolTable> symTab) : symTab(symTab) {};
  ~TypeWalker() override = default;
  std::any visitRoot(std::shared_ptr<RootAst> ctx) override;
  std::any
  visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) override;
  std::any
  visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) override;
  std::any visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) override;
  std::any visitBlock(std::shared_ptr<statements::BlockAst> ctx) override;
  std::any visitBreak(std::shared_ptr<statements::BreakAst> ctx) override;
  std::any visitContinue(std::shared_ptr<statements::ContinueAst> ctx) override;
  std::any
  visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) override;
  std::any visitInput(std::shared_ptr<statements::InputAst> ctx) override;
  std::any visitOutput(std::shared_ptr<statements::OutputAst> ctx) override;
  std::any
  visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) override;
  std::any visitProcedureParams(
      std::shared_ptr<prototypes::ProcedureParamAst> ctx) override;
  std::any visitProcedureCall(
      std::shared_ptr<statements::ProcedureCallAst> ctx) override;
  std::any visitReturn(std::shared_ptr<statements::ReturnAst> ctx) override;
  std::any
  visitTupleAssign(std::shared_ptr<statements::TupleAssignAst> ctx) override;
  std::any
  visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) override;
  std::any
  visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) override;
  std::any visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) override;
  std::any
  visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) override;
  std::any visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) override;
  std::any visitFunctionParam(
      std::shared_ptr<prototypes::FunctionParamAst> ctx) override;
  std::any
  visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) override;
  std::any
  visitFuncProcCall(std::shared_ptr<expressions::FuncProcCallAst> ctx) override;
  std::any visitArg(std::shared_ptr<expressions::ArgAst> ctx) override;
  std::any visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) override;
  std::any visitCast(std::shared_ptr<expressions::CastAst> ctx) override;
  std::any visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) override;
  std::any
  visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) override;
  std::any visitIdentifierLeft(
      std::shared_ptr<statements::IdentifierLeftAst> ctx) override;
  std::any
  visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) override;
  std::any visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) override;
  std::any visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) override;
  std::any visitLoop(std::shared_ptr<statements::LoopAst> ctx) override;
  std::any
  visitIteratorLoop(std::shared_ptr<statements::IteratorLoopAst> ctx) override;
};
} // namespace gazprea::ast::walkers