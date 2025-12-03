#include <ast/walkers/AstWalker.h>

namespace gazprea::ast::walkers {

AstWalker::AstWalker() = default;

std::any AstWalker::visit(std::shared_ptr<Ast> ast) {
  switch (ast->getNodeType()) {
  case NodeType::Arg:
    return visitArg(std::dynamic_pointer_cast<expressions::ArgAst>(ast));
  case NodeType::Assignment:
    return visitAssignment(std::dynamic_pointer_cast<statements::AssignmentAst>(ast));
  case NodeType::BinaryExpression:
    return visitBinary(std::dynamic_pointer_cast<expressions::BinaryAst>(ast));
  case NodeType::Block:
    return visitBlock(std::dynamic_pointer_cast<statements::BlockAst>(ast));
  case NodeType::Break:
    return visitBreak(std::dynamic_pointer_cast<statements::BreakAst>(ast));
  case NodeType::BoolLiteral:
    return visitBool(std::dynamic_pointer_cast<expressions::BoolLiteralAst>(ast));
  case NodeType::Continue:
    return visitContinue(std::dynamic_pointer_cast<statements::ContinueAst>(ast));
  case NodeType::Conditional:
    return visitConditional(std::dynamic_pointer_cast<statements::ConditionalAst>(ast));
  case NodeType::Cast:
    return visitCast(std::dynamic_pointer_cast<expressions::CastAst>(ast));
  case NodeType::CharLiteral:
    return visitChar(std::dynamic_pointer_cast<expressions::CharLiteralAst>(ast));
  case NodeType::Declaration:
    return visitDeclaration(std::dynamic_pointer_cast<statements::DeclarationAst>(ast));
  case NodeType::Function:
    return visitFunction(std::dynamic_pointer_cast<prototypes::FunctionAst>(ast));
  case NodeType::FunctionParam:
    return visitFunctionParam(std::dynamic_pointer_cast<prototypes::FunctionParamAst>(ast));
  case NodeType::FuncProcCall:
    return visitFuncProcCall(std::dynamic_pointer_cast<expressions::FuncProcCallAst>(ast));
  case NodeType::Procedure:
    return visitProcedure(std::dynamic_pointer_cast<prototypes::ProcedureAst>(ast));
  case NodeType::ProcedureParam:
    return visitProcedureParams(std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(ast));
  case NodeType::ProcedureCall:
    return visitProcedureCall(std::dynamic_pointer_cast<statements::ProcedureCallAst>(ast));
  case NodeType::Prototype:
    return visitPrototype(std::dynamic_pointer_cast<prototypes::PrototypeAst>(ast));
  case NodeType::Output:
    return visitOutput(std::dynamic_pointer_cast<statements::OutputAst>(ast));
  case NodeType::Identifier:
    return visitIdentifier(std::dynamic_pointer_cast<expressions::IdentifierAst>(ast));
  case NodeType::IdentifierLeft:
    return visitIdentifierLeft(std::dynamic_pointer_cast<statements::IdentifierLeftAst>(ast));
  case NodeType::IntegerLiteral:
    return visitInteger(std::dynamic_pointer_cast<expressions::IntegerLiteralAst>(ast));
  case NodeType::Input:
    return visitInput(std::dynamic_pointer_cast<statements::InputAst>(ast));
  case NodeType::RealLiteral:
    return visitReal(std::dynamic_pointer_cast<expressions::RealLiteralAst>(ast));
  case NodeType::Return:
    return visitReturn(std::dynamic_pointer_cast<statements::ReturnAst>(ast));
  case NodeType::Root:
    return visitRoot(std::dynamic_pointer_cast<RootAst>(ast));
  case NodeType::StructAccess:
    return visitStructAccess(std::dynamic_pointer_cast<expressions::StructAccessAst>(ast));
  case NodeType::StructDeclaration:
    return visitStructDeclaration(std::dynamic_pointer_cast<statements::StructDeclarationAst>(ast));
  case NodeType::StructElementAssign:
    return visitStructElementAssign(
        std::dynamic_pointer_cast<statements::StructElementAssignAst>(ast));
  case NodeType::StructFuncCallRouter:
    return visitStructFuncCallRouter(
        std::dynamic_pointer_cast<expressions::StructFuncCallRouterAst>(ast));
  case NodeType::StructLiteral:
    return visitStruct(std::dynamic_pointer_cast<expressions::StructLiteralAst>(ast));
  case NodeType::TupleAccess:
    return visitTupleAccess(std::dynamic_pointer_cast<expressions::TupleAccessAst>(ast));
  case NodeType::Typealias:
    return visitTypealias(std::dynamic_pointer_cast<statements::TypealiasAst>(ast));
  case NodeType::TupleElementAssign:
    return visitTupleElementAssign(
        std::dynamic_pointer_cast<statements::TupleElementAssignAst>(ast));
  case NodeType::TupleUnpackAssign:
    return visitTupleUnpackAssign(std::dynamic_pointer_cast<statements::TupleUnpackAssignAst>(ast));
  case NodeType::UnaryExpression:
    return visitUnary(std::dynamic_pointer_cast<expressions::UnaryAst>(ast));
  case NodeType::Loop:
    return visitLoop(std::dynamic_pointer_cast<statements::LoopAst>(ast));
  case NodeType::IteratorLoop:
    return visitIteratorLoop(std::dynamic_pointer_cast<statements::IteratorLoopAst>(ast));
  case NodeType::TupleLiteral:
    return visitTuple(std::dynamic_pointer_cast<expressions::TupleLiteralAst>(ast));
  case NodeType::StructType:
    return visitStructType(std::dynamic_pointer_cast<types::StructTypeAst>(ast));
  case NodeType::TupleType:
    return visitTupleType(std::dynamic_pointer_cast<types::TupleTypeAst>(ast));
  case NodeType::AliasType:
    return visitAliasType(std::dynamic_pointer_cast<types::AliasTypeAst>(ast));
  case NodeType::IntegerType:
    return visitIntegerType(std::dynamic_pointer_cast<types::IntegerTypeAst>(ast));
  case NodeType::RealType:
    return visitRealType(std::dynamic_pointer_cast<types::RealTypeAst>(ast));
  case NodeType::CharType:
    return visitCharacterType(std::dynamic_pointer_cast<types::CharacterTypeAst>(ast));
  case NodeType::BoolType:
    return visitBooleanType(std::dynamic_pointer_cast<types::BooleanTypeAst>(ast));
  case NodeType::ArrayType:
    return visitArrayType(std::dynamic_pointer_cast<types::ArrayTypeAst>(ast));
  case NodeType::ArrayLiteral:
    return visitArray(std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(ast));
  case NodeType::VectorType:
    return visitVectorType(std::dynamic_pointer_cast<types::VectorTypeAst>(ast));
  case NodeType::LenMemberFunc:
    return visitLenMemberFunc(std::dynamic_pointer_cast<statements::LenMemberFuncAst>(ast));
  case NodeType::AppendMemberFunc:
    return visitAppendMemberFunc(std::dynamic_pointer_cast<statements::AppendMemberFuncAst>(ast));
  case NodeType::PushMemberFunc:
    return visitPushMemberFunc(std::dynamic_pointer_cast<statements::PushMemberFuncAst>(ast));
  case NodeType::ConcatMemberFunc:
    return visitConcatMemberFunc(std::dynamic_pointer_cast<statements::ConcatMemberFuncAst>(ast));
  case NodeType::LengthBuiltin:
    return visitLengthBuiltinFunc(
        std::dynamic_pointer_cast<expressions::LengthBuiltinFuncAst>(ast));
  case NodeType::ShapeBuiltin:
    return visitShapeBuiltinFunc(std::dynamic_pointer_cast<expressions::ShapeBuiltinFuncAst>(ast));
  case NodeType::ReverseBuiltin:
    return visitReverseBuiltinFunc(
        std::dynamic_pointer_cast<expressions::ReverseBuiltinFuncAst>(ast));
  case NodeType::FormatBuiltin:
    return visitFormatBuiltinFunc(
        std::dynamic_pointer_cast<expressions::FormatBuiltinFuncAst>(ast));
  case NodeType::ArrayAccess:
    return visitArrayAccess(std::dynamic_pointer_cast<expressions::ArrayAccessAst>(ast));
  case NodeType::SingularIndexExpr:
    return visitSingularIndex(std::dynamic_pointer_cast<expressions::SingularIndexExprAst>(ast));
  case NodeType::RangedIndexExpr:
    return visitRangedIndexExpr(std::dynamic_pointer_cast<expressions::RangedIndexExprAst>(ast));
  case NodeType::ArrayElementAssign:
    return visitArrayElementAssign(
        std::static_pointer_cast<statements::ArrayElementAssignAst>(ast));
  default:
    return {};
  }
}
} // namespace gazprea::ast::walkers