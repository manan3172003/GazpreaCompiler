#pragma once
#include "AstWalker.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/SymTable.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VectorTypeSymbol.h"

namespace gazprea::ast::walkers {
class ValidationWalker final : public AstWalker {
  std::shared_ptr<symTable::SymbolTable> symTab;
  bool inBinaryOp = false;
  bool inAssignment = false;
  static int opTable[6][15];
  void visitExpression(const std::shared_ptr<Ast> &exprAst) {
    inBinaryOp = true;
    visit(exprAst);
    inBinaryOp = false;
  }

public:
  explicit ValidationWalker(std::shared_ptr<symTable::SymbolTable> symTab) : symTab(symTab) {};
  ~ValidationWalker() override = default;
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
  std::any visitTupleUnpackAssign(std::shared_ptr<statements::TupleUnpackAssignAst> ctx) override;
  std::any visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) override;
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
  std::any visitArray(std::shared_ptr<expressions::ArrayLiteralAst> ctx) override;
  std::any visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) override;
  std::any visitLoop(std::shared_ptr<statements::LoopAst> ctx) override;
  std::any visitIteratorLoop(std::shared_ptr<statements::IteratorLoopAst> ctx) override;
  std::any visitStruct(std::shared_ptr<expressions::StructLiteralAst> ctx) override;
  std::any visitStructDeclaration(std::shared_ptr<statements::StructDeclarationAst> ctx) override;
  std::any
  visitStructElementAssign(std::shared_ptr<statements::StructElementAssignAst> ctx) override;
  std::any visitStructAccess(std::shared_ptr<expressions::StructAccessAst> ctx) override;
  std::any visitStructType(std::shared_ptr<types::StructTypeAst> ctx) override;
  std::any visitArrayType(std::shared_ptr<types::ArrayTypeAst> ctx) override;
  std::any visitVectorType(std::shared_ptr<types::VectorTypeAst> ctx) override;
  std::any visitLenMemberFunc(std::shared_ptr<statements::LenMemberFuncAst> ctx) override;
  std::any visitAppendMemberFunc(std::shared_ptr<statements::AppendMemberFuncAst> ctx) override;
  std::any visitPushMemberFunc(std::shared_ptr<statements::PushMemberFuncAst> ctx) override;
  std::any visitConcatMemberFunc(std::shared_ptr<statements::ConcatMemberFuncAst> ctx) override;
  std::any visitLengthBuiltinFunc(std::shared_ptr<expressions::LengthBuiltinFuncAst> ctx) override;
  std::any visitShapeBuiltinFunc(std::shared_ptr<expressions::ShapeBuiltinFuncAst> ctx) override;
  std::any
  visitReverseBuiltinFunc(std::shared_ptr<expressions::ReverseBuiltinFuncAst> ctx) override;
  std::any visitFormatBuiltinFunc(std::shared_ptr<expressions::FormatBuiltinFuncAst> ctx) override;
  std::any visitArrayAccess(std::shared_ptr<expressions::ArrayAccessAst> ctx) override;
  std::any visitSingularIndex(std::shared_ptr<expressions::SingularIndexExprAst> ctx) override;
  std::any visitRangedIndexExpr(std::shared_ptr<expressions::RangedIndexExprAst> ctx) override;
  std::any visitArrayElementAssign(std::shared_ptr<statements::ArrayElementAssignAst> ctx) override;
  std::any visitRange(std::shared_ptr<expressions::RangeAst> ctx) override;
  std::any visitDomainExpr(std::shared_ptr<expressions::DomainExprAst> ctx) override;
  std::any visitGenerator(std::shared_ptr<expressions::GeneratorAst> ctx) override;

  // Helpers
  std::shared_ptr<symTable::Type>
  resolvedInferredType(const std::shared_ptr<types::DataTypeAst> &dataType);
  static bool isScalar(const std::shared_ptr<symTable::Type> &type);
  static bool isCollection(const std::shared_ptr<symTable::Type> &type);
  static bool isTuple(const std::shared_ptr<symTable::Type> &type);
  static void validateVariableAssignmentTypes(std::shared_ptr<statements::IdentifierLeftAst> ctx,
                                              std::shared_ptr<symTable::Type> exprTypeSymbol);
  static void
  validateTupleElementAssignmentTypes(std::shared_ptr<statements::TupleElementAssignAst> ctx,
                                      std::shared_ptr<symTable::Type> exprTypeSymbol);
  static void
  validateStructElementAssignmentTypes(std::shared_ptr<statements::StructElementAssignAst> ctx,
                                       std::shared_ptr<symTable::Type> exprTypeSymbol);
  static void
  validateArrayElementAssignmentTypes(std::shared_ptr<statements::ArrayElementAssignAst> ctx,
                                      std::shared_ptr<symTable::Type> exprTypeSymbol);
  static void
  validateTupleUnpackAssignmentTypes(std::shared_ptr<statements::TupleUnpackAssignAst> ctx,
                                     std::shared_ptr<symTable::Type> exprTypeSymbol);
  static bool typesMatch(const std::shared_ptr<symTable::Type> &destination,
                         const std::shared_ptr<symTable::Type> &source);
  static bool isTupleTypeMatch(const std::shared_ptr<symTable::TupleTypeSymbol> &destination,
                               const std::shared_ptr<symTable::TupleTypeSymbol> &source);
  static bool isOfSymbolType(const std::shared_ptr<symTable::Type> &symbolType,
                             const std::string &typeName);
  static std::shared_ptr<symTable::Scope>
  getEnclosingFuncProcScope(std::shared_ptr<symTable::Scope> currentScope);
  static bool isValidOp(const std::string &typeName, expressions::BinaryOpType opType);
  static void promoteIfNeeded(std::shared_ptr<expressions::ExpressionAst> ctx,
                              std::shared_ptr<symTable::Type> promoteFrom,
                              std::shared_ptr<symTable::Type> promoteTo,
                              std::shared_ptr<types::DataTypeAst> promoteToDataType);
  static int nodeTypeToIndex(const std::string &typeName);
  static bool hasReturnInMethod(const std::shared_ptr<statements::BlockAst> &block);
  static bool isNumericType(const std::shared_ptr<symTable::Type> &type);
  static bool isComparisonOperator(expressions::BinaryOpType opType);
  static bool areBothNumeric(const std::shared_ptr<expressions::ExpressionAst> &left,
                             const std::shared_ptr<expressions::ExpressionAst> &right);
  void validateArgs(const std::vector<std::shared_ptr<Ast>> &params,
                    const std::vector<std::shared_ptr<expressions::ArgAst>> &args,
                    symTable::ScopeType scopeType, int lineNumber);
  static void identifierAliasingCheck(
      std::unordered_map<std::string,
                         std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
          &seenVarMap,
      std::shared_ptr<expressions::ExpressionAst> expr, bool isVar);
  static void tupleAccessAliasingCheck(
      std::unordered_map<std::string,
                         std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
          &seenVarMap,
      std::shared_ptr<expressions::ExpressionAst> expr, bool isVar);
  static void structAccessAliasingCheck(
      std::unordered_map<std::string,
                         std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
          &seenVarMap,
      std::shared_ptr<expressions::ExpressionAst> expr, bool isVar);
  static void arrayAccessAliasingCheck(
      std::unordered_map<std::string,
                         std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
          &seenVarMap,
      std::shared_ptr<expressions::ExpressionAst> expr, bool isVar);
  static void checkVarArgs(const std::vector<std::shared_ptr<Ast>> &params,
                           const std::vector<std::shared_ptr<expressions::ArgAst>> &args);
  static void validateTupleAccessInferredTypes(std::shared_ptr<expressions::TupleAccessAst> ctx);
  static bool isLiteralExpression(const std::shared_ptr<expressions::ExpressionAst> &expr);
  static void ensureArrayLiteralType(const std::shared_ptr<expressions::ExpressionAst> &expr,
                                     const std::shared_ptr<symTable::Type> &targetType);
  static void inferVectorSize(const std::shared_ptr<symTable::VectorTypeSymbol> &vectorType,
                              const std::shared_ptr<expressions::ExpressionAst> &expr);
};
} // namespace gazprea::ast::walkers
