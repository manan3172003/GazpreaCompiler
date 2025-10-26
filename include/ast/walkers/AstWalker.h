#include "ast/Ast.h"
#include "ast/RootAst.h"
#include "ast/expressions/BoolLiteralAst.h"
#include "ast/expressions/CastAst.h"
#include "ast/expressions/CharLiteralAst.h"
#include "ast/expressions/FuncProcCallAst.h"
#include "ast/expressions/IdentifierAst.h"
#include "ast/expressions/IntegerLiteralAst.h"
#include "ast/expressions/RealLiteralAst.h"
#include "ast/expressions/TupleAccessAst.h"
#include "ast/expressions/TupleLiteralAst.h"
#include "ast/expressions/UnaryAst.h"
#include "ast/prototypes/FunctionAst.h"
#include "ast/prototypes/FunctionParamAst.h"
#include "ast/prototypes/ProcedureAst.h"
#include "ast/prototypes/ProcedureParamAst.h"
#include "ast/statements/AssignmentAst.h"
#include "ast/statements/BlockAst.h"
#include "ast/statements/BreakAst.h"
#include "ast/statements/ConditionalAst.h"
#include "ast/statements/ContinueAst.h"
#include "ast/statements/DeclarationAst.h"
#include "ast/statements/IdentifierLeftAst.h"
#include "ast/statements/InputAst.h"
#include "ast/statements/IteratorLoopAst.h"
#include "ast/statements/LoopAst.h"
#include "ast/statements/OutputAst.h"
#include "ast/statements/ReturnAst.h"
#include "ast/statements/TupleAssignAst.h"
#include "ast/statements/TypealiasAst.h"

#include <any>
namespace gazprea::ast::walkers {
class AstWalker {
public:
  AstWalker();
  ~AstWalker() = default;

  std::any visit(std::shared_ptr<Ast> ast);
  virtual std::any visitRoot(std::shared_ptr<RootAst> ctx) { return {}; }
  virtual std::any
  visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
    return {};
  }
  virtual std::any
  visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) {
    return {};
  }
  virtual std::any visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
    return {};
  }
  virtual std::any visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
    return {};
  }
  virtual std::any visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
    return {};
  }
  virtual std::any
  visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) {
    return {};
  }
  virtual std::any visitInput(std::shared_ptr<statements::InputAst> ctx) {
    return {};
  }
  virtual std::any visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
    return {};
  }
  virtual std::any
  visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
    return {};
  }
  virtual std::any
  visitProcedureParams(std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
    return {};
  }
  virtual std::any
  visitProcedureCall(std::shared_ptr<statements::ProcedureCallAst> ctx) {
    return {};
  }
  virtual std::any visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
    return {};
  }
  virtual std::any
  visitTupleAssign(std::shared_ptr<statements::TupleAssignAst> ctx) {
    return {};
  }
  virtual std::any
  visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) {
    return {};
  }
  virtual std::any
  visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
    return {};
  }
  virtual std::any
  visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
    return {};
  }
  virtual std::any visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
    return {};
  }
  virtual std::any
  visitFunctionParam(std::shared_ptr<prototypes::FunctionParamAst> ctx) {
    return {};
  }
  virtual std::any
  visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
    return {};
  }
  virtual std::any
  visitFuncProcCall(std::shared_ptr<expressions::FuncProcCallAst> ctx) {
    return {};
  }
  virtual std::any visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
    return {};
  }
  virtual std::any visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
    return {};
  }
  virtual std::any visitCast(std::shared_ptr<expressions::CastAst> ctx) {
    return {};
  }
  virtual std::any visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
    return {};
  }
  virtual std::any
  visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) {
    return {};
  }
  virtual std::any
  visitIdentifierLeft(std::shared_ptr<statements::IdentifierLeftAst> ctx) {
    return {};
  }
  virtual std::any
  visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
    return {};
  }
  virtual std::any visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
    return {};
  }
  virtual std::any visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
    return {};
  }
  virtual std::any visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
    return {};
  }
  virtual std::any
  visitIteratorLoop(std::shared_ptr<statements::IteratorLoopAst> ctx) {
    return {};
  }
};
} // namespace gazprea::ast::walkers