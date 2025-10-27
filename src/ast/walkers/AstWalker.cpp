#include "ast/expressions/BinaryAst.h"
#include "ast/statements/ProcedureCallAst.h"

#include <ast/walkers/AstWalker.h>

namespace gazprea::ast::walkers {

AstWalker::AstWalker() {}

std::any AstWalker::visit(std::shared_ptr<Ast> ast) {
  switch (ast->getNodeType()) {
  case NodeType::Arg:
    return visitArg(std::static_pointer_cast<expressions::ArgAst>(ast));
  case NodeType::Assignment:
    return visitAssignment(
        std::static_pointer_cast<statements::AssignmentAst>(ast));
  case NodeType::BinaryExpression:
    return visitBinary(std::static_pointer_cast<expressions::BinaryAst>(ast));
  case NodeType::Block:
    return visitBlock(std::static_pointer_cast<statements::BlockAst>(ast));
  case NodeType::Break:
    return visitBreak(std::static_pointer_cast<statements::BreakAst>(ast));
  case NodeType::BoolLiteral:
    return visitBool(
        std::static_pointer_cast<expressions::BoolLiteralAst>(ast));
  case NodeType::Continue:
    return visitContinue(
        std::static_pointer_cast<statements::ContinueAst>(ast));
  case NodeType::Conditional:
    return visitConditional(
        std::static_pointer_cast<statements::ConditionalAst>(ast));
  case NodeType::Cast:
    return visitCast(std::static_pointer_cast<expressions::CastAst>(ast));
  case NodeType::CharLiteral:
    return visitChar(
        std::static_pointer_cast<expressions::CharLiteralAst>(ast));
  case NodeType::Declaration:
    return visitDeclaration(
        std::static_pointer_cast<statements::DeclarationAst>(ast));
  case NodeType::Function:
    return visitFunction(
        std::static_pointer_cast<prototypes::FunctionAst>(ast));
  case NodeType::FunctionParam:
    return visitFunctionParam(
        std::static_pointer_cast<prototypes::FunctionParamAst>(ast));
  case NodeType::FuncProcCall:
    return visitFuncProcCall(
        std::static_pointer_cast<expressions::FuncProcCallAst>(ast));
  case NodeType::Procedure:
    return visitProcedure(
        std::static_pointer_cast<prototypes::ProcedureAst>(ast));
  case NodeType::ProcedureParam:
    return visitProcedureParams(
        std::static_pointer_cast<prototypes::ProcedureParamAst>(ast));
  case NodeType::ProcedureCall:
    return visitProcedureCall(
        std::static_pointer_cast<statements::ProcedureCallAst>(ast));
  case NodeType::Prototype:
    return visitPrototype(
        std::static_pointer_cast<prototypes::PrototypeAst>(ast));
  case NodeType::Output:
    return visitOutput(std::static_pointer_cast<statements::OutputAst>(ast));
  case NodeType::Identifier:
    return visitIdentifier(
        std::static_pointer_cast<expressions::IdentifierAst>(ast));
  case NodeType::IdentifierLeft:
    return visitIdentifierLeft(
        std::static_pointer_cast<statements::IdentifierLeftAst>(ast));
  case NodeType::IntegerLiteral:
    return visitInteger(
        std::static_pointer_cast<expressions::IntegerLiteralAst>(ast));
  case NodeType::Input:
    return visitInput(std::static_pointer_cast<statements::InputAst>(ast));
  case NodeType::RealLiteral:
    return visitReal(
        std::static_pointer_cast<expressions::RealLiteralAst>(ast));
  case NodeType::Return:
    return visitReturn(std::static_pointer_cast<statements::ReturnAst>(ast));
  case NodeType::Root:
    return visitRoot(std::static_pointer_cast<RootAst>(ast));
  case NodeType::TupleAccess:
    return visitTupleAccess(
        std::static_pointer_cast<expressions::TupleAccessAst>(ast));
  case NodeType::Typealias:
    return visitTypealias(
        std::static_pointer_cast<statements::TypealiasAst>(ast));
  case NodeType::TupleAssign:
    return visitTupleAssign(
        std::static_pointer_cast<statements::TupleAssignAst>(ast));
  case NodeType::UnaryExpression:
    return visitUnary(std::static_pointer_cast<expressions::UnaryAst>(ast));
  case NodeType::Loop:
    return visitLoop(std::static_pointer_cast<statements::LoopAst>(ast));
  case NodeType::IteratorLoop:
    return visitIteratorLoop(
        std::static_pointer_cast<statements::IteratorLoopAst>(ast));
  case NodeType::TupleLiteral:
    return visitTuple(
        std::static_pointer_cast<expressions::TupleLiteralAst>(ast));
  case NodeType::TupleType:
    return visitTupleType(std::static_pointer_cast<types::TupleTypeAst>(ast));
  default:
    return {};
  }
}
} // namespace gazprea::ast::walkers