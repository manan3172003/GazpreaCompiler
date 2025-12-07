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
#include "ast/expressions/UnaryAst.h"
#include "ast/walkers/AstWalker.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "symTable/ArrayTypeSymbol.h"
#include <symTable/VariableSymbol.h>
#include <symTable/VectorTypeSymbol.h>

namespace gazprea::backend {
constexpr char kStreamStateGlobalName[] = "stream_state_019ae35e_4e0e_7d02_98f8_6e5abd8135e9";
constexpr char kScanfName[] = "scanf_019ae392_2fe0_72fc_ad1e_94bb9c5662c0";
constexpr char kPrintfName[] = "printf_019ae38d_3df3_74a3_b276_d9a9f7a8008b";
constexpr char kIpowName[] = "ipow_019addc8_6352_7de5_8629_b0688522175f";
constexpr char kThrowDivByZeroErrorName[] =
    "throwDivisionByZeroError_019addc8_a29b_740a_9b09_8a712296bc1a";
constexpr char kThrowArraySizeErrorName[] =
    "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c";
constexpr char kThrowVectorSizeErrorName[] =
    "throwVectorSizeError_019addc9_1a57_7674_b3dd_79d0624d2029";
constexpr char kPrintArrayName[] = "printArray_019addab_1674_72d4_aa4a_ac782e511e7a";
constexpr char kThrowStrideErrorName[] = "throwStrideError_a2beb751_ff3b_4d60_aefb_60f92ff9f4be";
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
  std::any visitArrayType(std::shared_ptr<ast::types::ArrayTypeAst> ctx) override;
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
  std::any visitStreamStateBuiltinFunc(
      std::shared_ptr<ast::expressions::StreamStateBuiltinFuncAst> ctx) override;
  mlir::Value concatArrays(std::shared_ptr<symTable::Type> type, mlir::Value leftArrayStruct,
                           mlir::Value rightArrayStruct);
  mlir::Value concatVectors(std::shared_ptr<symTable::Type> type, mlir::Value leftVectorStruct,
                            mlir::Value rightVectorStruct);
  void throwIfVectorSizeNotEqual(mlir::Value left, mlir::Value right,
                                 std::shared_ptr<symTable::Type> type);
  mlir::Value strideArrayByScalar(std::shared_ptr<symTable::Type> type, mlir::Value arrayStruct,
                                  mlir::Value scalarValue);
  mlir::Value strideVectorByScalar(std::shared_ptr<symTable::Type> type, mlir::Value vectorStruct,
                                   mlir::Value scalarValue);
  mlir::Value areArraysEqual(mlir::Value leftArrayStruct, mlir::Value rightArrayStruct,
                             std::shared_ptr<symTable::Type> arrayType);
  bool typeContainsReal(std::shared_ptr<symTable::Type> type);
  bool typeContainsInteger(std::shared_ptr<symTable::Type> type);
  mlir::Value castIntegerArrayToReal(mlir::Value fromArrayStruct,
                                     std::shared_ptr<symTable::Type> srcType, bool shouldCast);
  mlir::Value castIntegerVectorToReal(mlir::Value fromVectorStruct,
                                      std::shared_ptr<symTable::Type> srcType, bool shouldCast);
  std::shared_ptr<symTable::Type>
  convertIntegerTypeToRealType(std::shared_ptr<symTable::Type> type);
  bool isTypeReal(std::shared_ptr<symTable::Type> type);
  bool isTypeInteger(std::shared_ptr<symTable::Type> type);
  mlir::Value areVectorsEqual(mlir::OpBuilder &b, mlir::Location l, mlir::Value leftVectorStruct,
                              mlir::Value rightVectorStruct,
                              std::shared_ptr<symTable::Type> vectorType);

protected:
  void setupPrintf() const;
  void setupScanf() const;
  void setupIntPow() const;
  void setupPrintArray() const;
  void setupPrintString() const;
  void setupThrowDivisionByZeroError() const;
  void setupThrowArraySizeError() const;
  void setupThrowVectorSizeError() const;
  void setupThrowArrayIndexError() const;
  void setupThrowStrideError() const;
  void printFloat(mlir::Value floatValue);
  void printInt(mlir::Value integer);
  void printIntChar(mlir::Value integer);
  void printBool(mlir::Value boolValue);
  void makeLengthBuiltin();
  void makeShapeBuiltin();
  void makeReverseBuiltin();
  void printChar(char c);
  void printArray(mlir::Value arrayStructAddr, std::shared_ptr<symTable::Type> arrayType);
  mlir::Value applyUnaryToScalar(ast::expressions::UnaryOpType op,
                                 std::shared_ptr<symTable::Type> type, mlir::OpBuilder &b,
                                 mlir::Location l, mlir::Value value);
  void applyUnaryToArray(ast::expressions::UnaryOpType op, mlir::Value arrayStruct,
                         std::shared_ptr<symTable::Type> arrayType);
  void applyUnaryToVector(ast::expressions::UnaryOpType op, mlir::Value vectorStruct,
                          std::shared_ptr<symTable::Type> vectorType);

  void arraySizeValidationForArrayStructs(mlir::Value lhsArrayStruct,
                                          std::shared_ptr<symTable::Type> lhsType,
                                          mlir::Value srcArrayStruct,
                                          std::shared_ptr<symTable::Type> srcType);
  void computeArraySizeIfArray(std::shared_ptr<ast::Ast> ctx, std::shared_ptr<symTable::Type> type,
                               mlir::Value arrayStruct);
  void castScalarToArrayIfNeeded(std::shared_ptr<symTable::Type> targetType, mlir::Value valueAddr,
                                 std::shared_ptr<symTable::Type> sourceType);
  void arraySizeValidation(std::shared_ptr<ast::Ast> ctx, std::shared_ptr<symTable::Type> type,
                           mlir::Value valueAddr);
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
  mlir::Value maxSubVectorSize(mlir::Value vectorStruct,
                               std::shared_ptr<symTable::Type> vectorType);
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
  bool isTypeTuple(const std::shared_ptr<symTable::Type> &value);
  bool isTypeStruct(const std::shared_ptr<symTable::Type> &value);
  void freeCompositeType(const std::vector<std::shared_ptr<symTable::Type>> &resolvedTypes,
                         mlir::Type compositeStructType, mlir::Value compositeStruct);
  mlir::Value copyArray(std::shared_ptr<symTable::Type> elementType, mlir::Value srcDataPtr,
                        mlir::Value size);
  void cloneArrayStructure(std::shared_ptr<symTable::Type> type, mlir::Value sourceArrayStruct,
                           mlir::Value destArrayStruct);
  void freeTuple(std::shared_ptr<symTable::Type> type, mlir::Value tupleStruct);
  void freeStruct(std::shared_ptr<symTable::Type> type, mlir::Value structStruct);
  void freeArray(std::shared_ptr<symTable::Type> type, mlir::Value arrayStruct);
  void createArrayFromVector(std::vector<std::shared_ptr<ast::expressions::ExpressionAst>> elements,
                             mlir::Type elementMLIRType, mlir::Value dest,
                             std::shared_ptr<symTable::Type> targetElementType);
  std::pair<mlir::Value, std::shared_ptr<symTable::ArrayTypeSymbol>>
  convertVectorToArrayStruct(mlir::Value vectorStruct,
                             std::shared_ptr<symTable::VectorTypeSymbol> vectorType);
  mlir::Value normalizeIndex(mlir::Value index, mlir::Value arraySize);
  void copyArrayElementsToSlice(mlir::Value srcArrayStruct,
                                std::shared_ptr<symTable::Type> srcArrayType,
                                mlir::Value dstDataPtr, std::shared_ptr<symTable::Type> elementType,
                                mlir::Value count);
  void printVector(int lineNumber, mlir::Value vectorStructAddr,
                   std::shared_ptr<symTable::Type> vectorType);
  void createGlobalString(const char *str, const char *stringName) const;
  mlir::Value castIfNeeded(std::shared_ptr<ast::Ast> ctx, mlir::Value valueAddr,
                           std::shared_ptr<symTable::Type> fromType,
                           std::shared_ptr<symTable::Type> toType);
  mlir::Value castScalarToArray(std::shared_ptr<ast::Ast> ctx, mlir::Value scalarValue,
                                std::shared_ptr<symTable::Type> scalarType,
                                std::shared_ptr<symTable::Type> arrayType);
  void fillArrayWithScalar(mlir::Value arrayStruct, std::shared_ptr<symTable::Type> arrayType,
                           mlir::Value scalarValue, std::shared_ptr<symTable::Type> scalarType,
                           mlir::Value targetOuterSize, mlir::Value targetInnerSize);
  std::pair<mlir::Value, bool> castStructIfNeeded(mlir::Value lhsArrayStruct,
                                                  std::shared_ptr<symTable::Type> lhsType,
                                                  mlir::Value srcArrayStruct,
                                                  std::shared_ptr<symTable::Type> srcType);
  void createGlobalStreamState() const;
  void fillArrayFromScalar(mlir::Value arrayStruct,
                           std::shared_ptr<symTable::ArrayTypeSymbol> arrayTypeSym,
                           mlir::Value scalarValue);
  void fillArrayWithScalarValueWithArrayStruct(mlir::Value arrayValueAddr, mlir::Value scalarValue,
                                               mlir::Value referenceArrayStruct,
                                               std::shared_ptr<symTable::Type> arrayType);
  void throwIfNotEqualArrayStructs(mlir::Value leftStuct, mlir::Value rightStruct,
                                   std::shared_ptr<symTable::Type> arrayType);
  void copyValue(std::shared_ptr<symTable::Type> type, mlir::Value fromAddr, mlir::Value destAddr);
  bool isTypeArray(std::shared_ptr<symTable::Type> type);
  bool isEmptyArray(std::shared_ptr<symTable::Type> type);
  bool isTypeVector(std::shared_ptr<symTable::Type> type);
  void copyVectorStruct(std::shared_ptr<symTable::Type> type, mlir::Value fromVectorStruct,
                        mlir::Value destVectorStruct);
  bool isScalarType(std::shared_ptr<symTable::Type> type) const;
  void pushElementToScopeStack(std::shared_ptr<ast::Ast> ctx,
                               std::shared_ptr<symTable::Type> elementType, mlir::Value val);
  std::pair<std::shared_ptr<symTable::Type>, mlir::Value>
  popElementFromStack(std::shared_ptr<ast::expressions::ExpressionAst> ctx);
  void freeElementsFromMemory(std::shared_ptr<ast::Ast> ctx);
  void freeScopeResources(std::shared_ptr<symTable::Scope> scope, bool clear = true);
  void freeResourcesUntilFunction(std::shared_ptr<symTable::Scope> startScope);
  void freeAllocatedMemory(const std::shared_ptr<symTable::Type> &type, mlir::Value ptrToMemory);
  void createGlobalDeclaration(const std::string &typeName, std::shared_ptr<ast::Ast> exprAst,
                               std::shared_ptr<symTable::Symbol> symbol,
                               const std::string &variableName);
  mlir::Value createVectorValue(const std::shared_ptr<symTable::VectorTypeSymbol> &vectorType,
                                const std::shared_ptr<symTable::Type> &sourceType,
                                mlir::Value sourceAddr);
  void freeVector(std::shared_ptr<symTable::Type> type, mlir::Value vectorStruct);
  void fillVectorWithScalarValueWithVectorStruct(mlir::Value vectorValueAddr,
                                                 mlir::Value scalarValue,
                                                 mlir::Value referenceVectorStruct,
                                                 std::shared_ptr<symTable::Type> vectorType);
  void fillVectorWithScalar(mlir::Value vectorStruct, std::shared_ptr<symTable::Type> vectorType,
                            mlir::Value scalarValue, std::shared_ptr<symTable::Type> scalarType,
                            mlir::Value targetOuterSize, mlir::Value targetInnerSize);

private:
  std::shared_ptr<ast::Ast> ast;
  std::unordered_map<std::string, mlir::Value> blockArg;
  std::shared_ptr<ast::prototypes::PrototypeAst> currentFunctionProto;

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
  mlir::Value binaryOperandToValue(std::shared_ptr<ast::Ast> ctx, ast::expressions::BinaryOpType op,
                                   std::shared_ptr<symTable::Type> opType,
                                   std::shared_ptr<symTable::Type> leftType,
                                   std::shared_ptr<symTable::Type> rightType,
                                   /*Address of the left value*/
                                   mlir::Value incomingLeftAddr,
                                   /*Address of the right value*/
                                   mlir::Value incomingRightAddr);
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
  mlir::Value loadCastSize(const std::vector<mlir::Value> &sizes, size_t idx, mlir::Value fallback);
  mlir::Value getCastTargetSize(const std::shared_ptr<symTable::ArrayTypeSymbol> &toArray,
                                size_t dimIndex, mlir::Value srcSize);
  void performArrayCast(mlir::Value srcPtr, std::shared_ptr<symTable::ArrayTypeSymbol> fromArray,
                        mlir::Value dstPtr, std::shared_ptr<symTable::ArrayTypeSymbol> toArray,
                        const std::vector<mlir::Value> &fromSizes,
                        const std::vector<mlir::Value> &toSizes, size_t dimIndex);
  void readInteger(mlir::Value destAddr);
  void readReal(mlir::Value destAddr);
  void readCharacter(mlir::Value destAddr);
  void readBoolean(mlir::Value destAddr);

  void handleSingularIndexAccess(std::shared_ptr<ast::expressions::ArrayAccessAst> ctx,
                                 std::shared_ptr<symTable::Type> instanceType, mlir::Value size,
                                 mlir::Value dataPtr);

  void handleRangedIndexAccess(std::shared_ptr<ast::expressions::ArrayAccessAst> ctx,
                               std::shared_ptr<symTable::Type> instanceType,
                               mlir::Value instanceAddr, mlir::Value size, mlir::Value dataPtr,
                               mlir::Type structType);
  enum class CastPolicy { AllowReallocate, InPlaceOnly };
  //
  // bool isArrayTypeName(const std::shared_ptr<symTable::Type> &t) const;
  // bool isScalarTypeName(const std::shared_ptr<symTable::Type> &t) const;
  // bool isEmptyArrayType(const std::shared_ptr<symTable::Type> &t) const;

  mlir::Value tryScalarToArrayCast(std::shared_ptr<ast::Ast> ctx, mlir::Value valueAddr,
                                   std::shared_ptr<symTable::Type> fromType,
                                   std::shared_ptr<symTable::Type> toType, CastPolicy policy);

  mlir::Value tryEmptyArrayToArrayInit(std::shared_ptr<ast::Ast> ctx, mlir::Value valueAddr,
                                       std::shared_ptr<symTable::Type> fromType,
                                       std::shared_ptr<symTable::Type> toType, CastPolicy policy);

  mlir::Value tryArrayReturnPaddingCast(std::shared_ptr<ast::Ast> ctx, mlir::Value valueAddr,
                                        std::shared_ptr<symTable::Type> fromType,
                                        std::shared_ptr<symTable::Type> toType, CastPolicy policy);

  void runInPlaceArrayPrepAndValidation(std::shared_ptr<ast::Ast> ctx, mlir::Value valueAddr,
                                        std::shared_ptr<symTable::Type> fromType,
                                        std::shared_ptr<symTable::Type> toType);

  void castTupleElementsInPlace(std::shared_ptr<ast::Ast> ctx, mlir::Value valueAddr,
                                std::shared_ptr<symTable::Type> fromType,
                                std::shared_ptr<symTable::Type> toType);

  void castIntegerToRealInPlace(mlir::Value valueAddr);

  void castArrayToArrayInPlace(std::shared_ptr<symTable::Type> fromType,
                               std::shared_ptr<symTable::Type> toType, mlir::Value valueAddr);

  mlir::Value castIfNeededImpl(std::shared_ptr<ast::Ast> ctx, mlir::Value valueAddr,
                               std::shared_ptr<symTable::Type> fromType,
                               std::shared_ptr<symTable::Type> toType, CastPolicy policy);
};
} // namespace gazprea::backend
