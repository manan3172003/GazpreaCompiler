// Pass manager
#pragma once
#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVMPass.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"

// Translation
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "llvm/Support/raw_os_ostream.h"

// MLIR IR
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/IR/Verifier.h"

// Dialects
#include "ast/walkers/AstWalker.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"

#include <symTable/VectorTypeSymbol.h>

namespace gazprea::backend {
enum class VectorOffset { Size = 0, Capacity = 1, Data = 2, Is2D = 3 };
class Backend final : public ast::walkers::AstWalker {
public:
  explicit Backend(const std::shared_ptr<ast::Ast> &ast);

  int emitModule();
  int lowerDialects();
  void dumpLLVM(std::ostream &os);
  std::any visitRoot(std::shared_ptr<ast::RootAst> ctx) override;
  std::any visitAssignment(std::shared_ptr<ast::statements::AssignmentAst> ctx) override;
  std::any visitDeclaration(std::shared_ptr<ast::statements::DeclarationAst> ctx) override;
  std::any visitBinary(std::shared_ptr<ast::expressions::BinaryAst> ctx) override;
  std::any visitBlock(std::shared_ptr<ast::statements::BlockAst> ctx) override;
  std::any visitBreak(std::shared_ptr<ast::statements::BreakAst> ctx) override;
  std::any visitContinue(std::shared_ptr<ast::statements::ContinueAst> ctx) override;
  std::any visitConditional(std::shared_ptr<ast::statements::ConditionalAst> ctx) override;
  std::any visitInput(std::shared_ptr<ast::statements::InputAst> ctx) override;
  std::any visitOutput(std::shared_ptr<ast::statements::OutputAst> ctx) override;
  std::any visitProcedure(std::shared_ptr<ast::prototypes::ProcedureAst> ctx) override;
  std::any visitProcedureParams(std::shared_ptr<ast::prototypes::ProcedureParamAst> ctx) override;
  std::any visitProcedureCall(std::shared_ptr<ast::statements::ProcedureCallAst> ctx) override;
  std::any visitReturn(std::shared_ptr<ast::statements::ReturnAst> ctx) override;
  std::any
  visitTupleElementAssign(std::shared_ptr<ast::statements::TupleElementAssignAst> ctx) override;
  std::any
  visitTupleUnpackAssign(std::shared_ptr<ast::statements::TupleUnpackAssignAst> ctx) override;
  std::any visitTupleAccess(std::shared_ptr<ast::expressions::TupleAccessAst> ctx) override;
  std::any visitTuple(std::shared_ptr<ast::expressions::TupleLiteralAst> ctx) override;
  std::any visitTupleType(std::shared_ptr<ast::types::TupleTypeAst> ctx) override;
  std::any visitTypealias(std::shared_ptr<ast::statements::TypealiasAst> ctx) override;
  std::any visitFunction(std::shared_ptr<ast::prototypes::FunctionAst> ctx) override;
  std::any visitFunctionParam(std::shared_ptr<ast::prototypes::FunctionParamAst> ctx) override;
  std::any visitPrototype(std::shared_ptr<ast::prototypes::PrototypeAst> ctx) override;
  std::any visitStructFuncCallRouter(
      std::shared_ptr<ast::expressions::StructFuncCallRouterAst> ctx) override;
  std::any
  visitStructDeclaration(std::shared_ptr<ast::statements::StructDeclarationAst> ctx) override;
  std::any
  visitStructElementAssign(std::shared_ptr<ast::statements::StructElementAssignAst> ctx) override;
  std::any visitStructAccess(std::shared_ptr<ast::expressions::StructAccessAst> ctx) override;
  std::any visitStructType(std::shared_ptr<ast::types::StructTypeAst> ctx) override;
  std::any visitStruct(std::shared_ptr<ast::expressions::StructLiteralAst> ctx) override;
  std::any visitFuncProcCall(std::shared_ptr<ast::expressions::FuncProcCallAst> ctx) override;
  std::any visitArg(std::shared_ptr<ast::expressions::ArgAst> ctx) override;
  std::any visitLenMemberFunc(std::shared_ptr<ast::statements::LenMemberFuncAst> ctx) override;
  std::any
  visitAppendMemberFunc(std::shared_ptr<ast::statements::AppendMemberFuncAst> ctx) override;
  std::any visitPushMemberFunc(std::shared_ptr<ast::statements::PushMemberFuncAst> ctx) override;
  std::any
  visitConcatMemberFunc(std::shared_ptr<ast::statements::ConcatMemberFuncAst> ctx) override;
  std::any visitBool(std::shared_ptr<ast::expressions::BoolLiteralAst> ctx) override;
  std::any visitCast(std::shared_ptr<ast::expressions::CastAst> ctx) override;
  std::any visitChar(std::shared_ptr<ast::expressions::CharLiteralAst> ctx) override;
  std::any visitIdentifier(std::shared_ptr<ast::expressions::IdentifierAst> ctx) override;
  std::any visitIdentifierLeft(std::shared_ptr<ast::statements::IdentifierLeftAst> ctx) override;
  std::any visitInteger(std::shared_ptr<ast::expressions::IntegerLiteralAst> ctx) override;
  std::any visitReal(std::shared_ptr<ast::expressions::RealLiteralAst> ctx) override;
  std::any visitUnary(std::shared_ptr<ast::expressions::UnaryAst> ctx) override;
  std::any visitLoop(std::shared_ptr<ast::statements::LoopAst> ctx) override;
  std::any visitIteratorLoop(std::shared_ptr<ast::statements::IteratorLoopAst> ctx) override;
  std::any visitArray(std::shared_ptr<ast::expressions::ArrayLiteralAst> ctx) override;
  std::any visitVectorType(std::shared_ptr<ast::types::VectorTypeAst> ctx) override;
  std::any
  visitLengthBuiltinFunc(std::shared_ptr<ast::expressions::LengthBuiltinFuncAst> ctx) override;
  std::any
  visitShapeBuiltinFunc(std::shared_ptr<ast::expressions::ShapeBuiltinFuncAst> ctx) override;
  std::any
  visitReverseBuiltinFunc(std::shared_ptr<ast::expressions::ReverseBuiltinFuncAst> ctx) override;
  std::any
  visitFormatBuiltinFunc(std::shared_ptr<ast::expressions::FormatBuiltinFuncAst> ctx) override;
  std::any visitArrayAccess(std::shared_ptr<ast::expressions::ArrayAccessAst> ctx) override;
  std::any
  visitArrayElementAssign(std::shared_ptr<ast::statements::ArrayElementAssignAst> ctx) override;
  std::any visitSingularIndex(std::shared_ptr<ast::expressions::SingularIndexExprAst> ctx) override;
  std::any visitRangedIndexExpr(std::shared_ptr<ast::expressions::RangedIndexExprAst> ctx) override;
  std::any visitRange(std::shared_ptr<ast::expressions::RangeAst> ctx) override;
  std::any visitDomainExpr(std::shared_ptr<ast::expressions::DomainExprAst> ctx) override;
  std::any visitGenerator(std::shared_ptr<ast::expressions::GeneratorAst> ctx) override;

protected:
  void setupPrintf() const;
  void setupIntPow() const;
  void setupPrintArray() const;
  void setupThrowDivisionByZeroError() const;
  void setupThrowArraySizeError() const;
  void setupThrowVectorSizeError() const;
  void setupThrowArrayIndexError() const;
  void printFloat(mlir::Value floatValue);
  void printInt(mlir::Value integer);
  void printIntChar(mlir::Value integer);
  void printBool(mlir::Value boolValue);
  void makeLengthBuiltin();
  void makeShapeBuiltin();
  void makeReverseBuiltin();
  void printChar(char c);
  void printArray(mlir::Value arrayStructAddr, std::shared_ptr<symTable::Type> arrayType);
  void computeArraySizeIfArray(std::shared_ptr<ast::statements::DeclarationAst> ctx,
                               std::shared_ptr<symTable::Type> type, mlir::Value arrayStruct);
  void arraySizeValidationForArrayStructs(mlir::Value lhsArrayStruct,
                                          std::shared_ptr<symTable::Type> lhsType,
                                          mlir::Value srcArrayStruct,
                                          std::shared_ptr<symTable::Type> srcType);
  void arraySizeValidation(std::shared_ptr<symTable::VariableSymbol> variableSymbol,
                           std::shared_ptr<symTable::Type> type, mlir::Value valueAddr);
  mlir::Value getArraySizeAddr(mlir::OpBuilder &b, mlir::Location l, mlir::Type arrayStructType,
                               mlir::Value arrayStruct) const;
  mlir::Value getArrayDataAddr(mlir::OpBuilder &b, mlir::Location l, mlir::Type arrayStructType,
                               mlir::Value arrayStruct) const;
  mlir::Value get2DArrayBoolAddr(mlir::OpBuilder &b, mlir::Location l, mlir::Type arrayStructType,
                                 mlir::Value arrayStruct) const;
  mlir::Value gepOpVector(mlir::Type vectorStructType, mlir::Value vectorStruct,
                          VectorOffset offset) const;
  mlir::LLVM::LLVMFuncOp getOrCreateMallocFunc();
  mlir::Value maxSubArraySize(mlir::Value arrayStruct, std::shared_ptr<symTable::Type> arrayType);
  mlir::Value mallocArray(mlir::Type elementMLIRType, mlir::Value elementCount);
  mlir::Value getTypeSizeInBytes(mlir::Type elementType);
  mlir::Value getDefaultValue(std::shared_ptr<symTable::Type> type);
  void padArrayWithValue(mlir::Value arrayStruct, std::shared_ptr<symTable::Type> arrayType,
                         mlir::Value currentSize, mlir::Value targetSize, mlir::Value defaultValue);
  void createEmptySubArray(mlir::Value subArrayPtr, std::shared_ptr<symTable::Type> subArrayType,
                           mlir::Value targetSize, mlir::Value defaultValue);
  void padArrayIfNeeded(mlir::Value arrayStruct, std::shared_ptr<symTable::Type> arrayType,
                        mlir::Value targetOuterSize, mlir::Value targetInnerSize);
  void copyArrayStruct(std::shared_ptr<symTable::Type> type, mlir::Value fromArrayStruct,
                       mlir::Value destArrayStruct);
  mlir::Value copyArray(std::shared_ptr<symTable::Type> elementType, mlir::Value srcDataPtr,
                        mlir::Value size);
  void cloneArrayStructure(std::shared_ptr<symTable::Type> type, mlir::Value sourceArrayStruct,
                           mlir::Value destArrayStruct);
  void freeArray(std::shared_ptr<symTable::Type> type, mlir::Value arrayStruct);
  void createArrayFromVector(std::vector<std::shared_ptr<ast::expressions::ExpressionAst>> elements,
                             mlir::Type elementMLIRType, mlir::Value dest);
  mlir::Value normalizeIndex(mlir::Value index, mlir::Value arraySize);
  void copyArrayElementsToSlice(mlir::Value srcArrayStruct,
                                std::shared_ptr<symTable::Type> srcArrayType,
                                mlir::Value dstDataPtr, std::shared_ptr<symTable::Type> elementType,
                                mlir::Value count);
  void printVector(mlir::Value vectorStructAddr, std::shared_ptr<symTable::Type> vectorType);
  void createGlobalString(const char *str, const char *stringName) const;
  void castIfNeeded(mlir::Value valueAddr, std::shared_ptr<symTable::Type> fromType,
                    std::shared_ptr<symTable::Type> toType);
  void copyValue(std::shared_ptr<symTable::Type> type, mlir::Value fromAddr, mlir::Value destAddr);
  bool isTypeArray(std::shared_ptr<symTable::Type> type);
  bool isTypeVector(std::shared_ptr<symTable::Type> type);
  void copyVectorStruct(std::shared_ptr<symTable::Type> type, mlir::Value fromVectorStruct,
                        mlir::Value destVectorStruct);
  bool isScalarType(std::shared_ptr<symTable::Type> type) const;
  void pushElementToScopeStack(std::shared_ptr<ast::Ast> ctx,
                               std::shared_ptr<symTable::Type> elementType, mlir::Value val);
  std::pair<std::shared_ptr<symTable::Type>, mlir::Value>
  popElementFromStack(std::shared_ptr<ast::Ast> ctx);
  void freeElementsFromMemory(std::shared_ptr<ast::Ast> ctx);
  void freeScopeResources(std::shared_ptr<symTable::Scope> scope, bool clear = true);
  void freeResourcesUntilFunction(std::shared_ptr<symTable::Scope> startScope);
  void createGlobalDeclaration(const std::string &typeName, std::shared_ptr<ast::Ast> exprAst,
                               std::shared_ptr<symTable::Symbol> symbol,
                               const std::string &variableName);
  mlir::Value createVectorValue(const std::shared_ptr<symTable::VectorTypeSymbol> &vectorType,
                                const std::shared_ptr<symTable::Type> &sourceType,
                                mlir::Value sourceAddr);
  void freeVector(std::shared_ptr<symTable::Type> type, mlir::Value vectorStruct);

private:
  std::shared_ptr<ast::Ast> ast;
  std::unordered_map<std::string, mlir::Value> blockArg;

  struct LoopContext {
    mlir::Block *exitBlock = nullptr;
    mlir::Block *continueBlock = nullptr;
  };
  std::vector<LoopContext> loopStack;

  // MLIR
  mlir::MLIRContext context;
  mlir::ModuleOp module;
  std::shared_ptr<mlir::OpBuilder> builder;
  mlir::Location loc;

  // LLVM
  llvm::LLVMContext llvm_context;
  std::unique_ptr<llvm::Module> llvm_module;

  mlir::Value constOne() const;
  mlir::Value constZero() const;
  mlir::Value constFalse() const;
  mlir::Value constTrue() const;

  mlir::Type structTy(const mlir::ArrayRef<mlir::Type> &memberTypes);
  mlir::Type arrayTy();
  mlir::Type vectorTy();
  mlir::Type floatTy() const;
  mlir::Type charTy() const;
  mlir::Type boolTy() const;
  mlir::Type ptrTy() const;
  mlir::Type intTy() const;

  // helpers
  mlir::Type getMLIRType(const std::shared_ptr<symTable::Type> &returnType);
  std::vector<mlir::Type>
  getMethodParamTypes(const std::vector<std::shared_ptr<ast::Ast>> &params) const;
  // Returns the address of mlir::Value of the result of applying the binary operation
  mlir::Value binaryOperandToValue(ast::expressions::BinaryOpType op,
                                   std::shared_ptr<symTable::Type> opType,
                                   std::shared_ptr<symTable::Type> leftType,
                                   std::shared_ptr<symTable::Type> rightType,
                                   /*Address of the left value*/
                                   mlir::Value leftAddr,
                                   /*Address of the right value*/
                                   mlir::Value rightAddr);
  mlir::Value floatBinaryOperandToValue(ast::expressions::BinaryOpType op,
                                        std::shared_ptr<symTable::Type> opType,
                                        std::shared_ptr<symTable::Type> leftType,
                                        std::shared_ptr<symTable::Type> rightType,
                                        mlir::Value leftAddr, mlir::Value rightAddr);
  static bool typesEquivalent(const std::shared_ptr<symTable::Type> &lhs,
                              const std::shared_ptr<symTable::Type> &rhs);
  mlir::Value promoteScalarValue(mlir::Value value, const std::shared_ptr<symTable::Type> &fromType,
                                 const std::shared_ptr<symTable::Type> &toType);
  void performExplicitCast(mlir::Value srcPtr, std::shared_ptr<symTable::Type> fromType,
                           mlir::Value dstPtr, std::shared_ptr<symTable::Type> toType);
};
} // namespace gazprea::backend
