#pragma once
#include "AstWalker.h"
#include "ast/expressions/StructAccessAst.h"
#include "ast/statements/StructElementAssignAst.h"
#include "symTable/SymTable.h"
#include "symTable/TupleTypeSymbol.h"

namespace gazprea::ast::walkers {
class DefRefWalker final : public AstWalker {
  std::shared_ptr<symTable::SymbolTable> symTab;
  std::shared_ptr<symTable::Type> resolvedType(int lineNumber,
                                               const std::shared_ptr<types::DataTypeAst> &dataType);

public:
  explicit DefRefWalker(std::shared_ptr<symTable::SymbolTable> symTab) : symTab(symTab) {};
  ~DefRefWalker() override = default;
  static void throwIfUndeclaredSymbol(int lineNumber, std::shared_ptr<symTable::Symbol> sym);
  void throwGlobalError(std::shared_ptr<Ast> ctx) const;
  static void throwDuplicateSymbolError(std::shared_ptr<Ast> ctx, const std::string &name,
                                        std::shared_ptr<symTable::Scope> curScope, bool isType);
  bool isTupleTypeMatch(const std::shared_ptr<symTable::TupleTypeSymbol> &destination,
                        const std::shared_ptr<symTable::TupleTypeSymbol> &source);
  bool exactTypeMatch(const std::shared_ptr<symTable::Type> &destination,
                      const std::shared_ptr<symTable::Type> &source);
  void compareProtoTypes(std::shared_ptr<prototypes::PrototypeAst> prev,
                         std::shared_ptr<prototypes::PrototypeAst> cur,
                         symTable::ScopeType scopeType);
  static std::shared_ptr<expressions::ExpressionAst>
  createDefaultLiteral(const std::shared_ptr<symTable::Type> &type, antlr4::Token *token);

  std::any visitRoot(std::shared_ptr<RootAst> ctx) override;
  std::any visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) override;
  std::any visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) override;
  std::any visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) override;
  std::any visitBlock(std::shared_ptr<statements::BlockAst> ctx) override;
  std::any visitBreak(std::shared_ptr<statements::BreakAst> ctx) override;
  std::any visitContinue(std::shared_ptr<statements::ContinueAst> ctx) override;
  std::any visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) override;
  std::any visitInput(std::shared_ptr<statements::InputAst> ctx) override;
  std::any visitOutput(std::shared_ptr<statements::OutputAst> ctx) override;
  std::any visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) override;
  std::any visitProcedureParams(std::shared_ptr<prototypes::ProcedureParamAst> ctx) override;
  std::any visitProcedureCall(std::shared_ptr<statements::ProcedureCallAst> ctx) override;
  std::any visitReturn(std::shared_ptr<statements::ReturnAst> ctx) override;
  std::any visitTupleElementAssign(std::shared_ptr<statements::TupleElementAssignAst> ctx) override;
  std::any
  visitStructElementAssign(std::shared_ptr<statements::StructElementAssignAst> ctx) override;
  std::any visitTupleUnpackAssign(std::shared_ptr<statements::TupleUnpackAssignAst> ctx) override;
  std::any visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) override;
  std::any visitStructAccess(std::shared_ptr<expressions::StructAccessAst> ctx) override;
  std::any visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) override;
  std::any visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) override;
  std::any visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) override;
  std::any visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) override;
  std::any visitFunctionParam(std::shared_ptr<prototypes::FunctionParamAst> ctx) override;
  std::any visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) override;
  std::any
  visitStructFuncCallRouter(std::shared_ptr<expressions::StructFuncCallRouterAst> ctx) override;
  std::any visitFuncProcCall(std::shared_ptr<expressions::FuncProcCallAst> ctx) override;
  std::any visitArg(std::shared_ptr<expressions::ArgAst> ctx) override;
  std::any visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) override;
  std::any visitCast(std::shared_ptr<expressions::CastAst> ctx) override;
  std::any visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) override;
  std::any visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) override;
  std::any visitIdentifierLeft(std::shared_ptr<statements::IdentifierLeftAst> ctx) override;
  std::any visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) override;
  std::any visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) override;
  std::any visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) override;
  std::any visitLoop(std::shared_ptr<statements::LoopAst> ctx) override;
  std::any visitIteratorLoop(std::shared_ptr<statements::IteratorLoopAst> ctx) override;
  std::any visitAliasType(std::shared_ptr<types::AliasTypeAst> ctx) override;
  std::any visitIntegerType(std::shared_ptr<types::IntegerTypeAst> ctx) override;
  std::any visitRealType(std::shared_ptr<types::RealTypeAst> ctx) override;
  std::any visitCharacterType(std::shared_ptr<types::CharacterTypeAst> ctx) override;
  std::any visitBooleanType(std::shared_ptr<types::BooleanTypeAst> ctx) override;
  std::any visitArray(std::shared_ptr<expressions::ArrayLiteralAst> ctx) override;
  std::any visitArrayType(std::shared_ptr<types::ArrayTypeAst> ctx) override;
  std::any visitLengthBuiltinFunc(std::shared_ptr<expressions::LengthBuiltinFuncAst> ctx) override;
  std::any visitShapeBuiltinFunc(std::shared_ptr<expressions::ShapeBuiltinFuncAst> ctx) override;
  std::any
  visitReverseBuiltinFunc(std::shared_ptr<expressions::ReverseBuiltinFuncAst> ctx) override;
  std::any visitFormatBuiltinFunc(std::shared_ptr<expressions::FormatBuiltinFuncAst> ctx) override;
  std::any visitLenMemberFunc(std::shared_ptr<statements::LenMemberFuncAst> ctx) override;
  std::any visitAppendMemberFunc(std::shared_ptr<statements::AppendMemberFuncAst> ctx) override;
  std::any visitPushMemberFunc(std::shared_ptr<statements::PushMemberFuncAst> ctx) override;
  std::any visitConcatMemberFunc(std::shared_ptr<statements::ConcatMemberFuncAst> ctx) override;
  std::any visitVectorType(std::shared_ptr<types::VectorTypeAst> ctx) override;
  std::any visitStructDeclaration(std::shared_ptr<statements::StructDeclarationAst> ctx) override;
  std::any visitStructType(std::shared_ptr<types::StructTypeAst> ctx) override;
  std::any visitStruct(std::shared_ptr<expressions::StructLiteralAst> ctx) override;
  std::any visitArrayAccess(std::shared_ptr<expressions::ArrayAccessAst> ctx) override;
  std::any visitSingularIndex(std::shared_ptr<expressions::SingularIndexExprAst> ctx) override;
  std::any visitRangedIndexExpr(std::shared_ptr<expressions::RangedIndexExprAst> ctx) override;
  std::any visitArrayElementAssign(std::shared_ptr<statements::ArrayElementAssignAst> ctx) override;
};
} // namespace gazprea::ast::walkers