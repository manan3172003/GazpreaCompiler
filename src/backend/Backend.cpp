#include "run_time_errors.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"
#include "utils/ValidationUtils.h"

#include "ast/expressions/CharLiteralAst.h"
#include "ast/expressions/IntegerLiteralAst.h"
#include "ast/types/ArrayTypeAst.h"

#include <backend/Backend.h>
namespace gazprea::backend {
Backend::Backend(const std::shared_ptr<ast::Ast> &ast)
    : ast(ast), loc(mlir::UnknownLoc::get(&context)) {
  // Load Dialects.
  context.loadDialect<mlir::LLVM::LLVMDialect>();
  context.loadDialect<mlir::func::FuncDialect>();
  context.loadDialect<mlir::arith::ArithDialect>();
  context.loadDialect<mlir::scf::SCFDialect>();
  context.loadDialect<mlir::cf::ControlFlowDialect>();
  context.loadDialect<mlir::memref::MemRefDialect>();

  // Initialize the MLIR context
  builder = std::make_shared<mlir::OpBuilder>(&context);
  module = mlir::ModuleOp::create(builder->getUnknownLoc());
  builder->setInsertionPointToStart(module.getBody());

  // Some initial setup to get off the ground
  setupPrintf();
  setupScanf();
  setupIntPow();
  setupThrowDivisionByZeroError();
  setupThrowArraySizeError();
  setupThrowVectorSizeError();
  setupThrowArrayIndexError();
  setupThrowStrideError();
  setupPrintArray();
  createGlobalString("%c\0", "charFormat");
  createGlobalString("%c", "charInputFormat");
  createGlobalString("%d\0", "intFormat");
  createGlobalString("%d", "intInputFormat");
  createGlobalString("%g\0", "floatFormat");
  createGlobalString("%f", "floatInputFormat");
  createGlobalString(" %1[TF]", "boolInputFormat");
  createGlobalStreamState();
}

int Backend::emitModule() {

  visit(ast);

  // module.dump();

  if (mlir::failed(mlir::verify(module))) {
    module.emitError("module failed to verify");
    return 1;
  }
  return 0;
}

int Backend::lowerDialects() {
  // Set up the MLIR pass manager to iteratively lower all the Ops
  mlir::PassManager pm(&context);

  // Lower Func dialect to LLVM
  pm.addPass(mlir::createConvertFuncToLLVMPass());

  // Lower SCF to CF (ControlFlow)
  pm.addPass(mlir::createConvertSCFToCFPass());

  // Lower Arith to LLVM
  pm.addPass(mlir::createArithToLLVMConversionPass());

  // Lower MemRef to LLVM
  pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());

  // Lower CF to LLVM
  pm.addPass(mlir::createConvertControlFlowToLLVMPass());

  // Finalize the conversion to LLVM dialect
  pm.addPass(mlir::createReconcileUnrealizedCastsPass());

  // Run the passes
  if (mlir::failed(pm.run(module))) {
    llvm::errs() << "Pass pipeline failed\n";
    return 1;
  }
  return 0;
}

void Backend::dumpLLVM(std::ostream &os) {
  // The only remaining dialects in our module after the passes are builtin
  // and LLVM. Setup translation patterns to get them to LLVM IR.
  mlir::registerBuiltinDialectTranslation(context);
  mlir::registerLLVMDialectTranslation(context);
  llvm_module = mlir::translateModuleToLLVMIR(module, llvm_context);

  // Create llvm ostream and dump into the output file
  llvm::raw_os_ostream output(os);
  output << *llvm_module;
}

void Backend::setupPrintf() const {
  // Create a function declaration for printf, the signature is:
  //   * `i32 (ptr, ...)`
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(intTy(), ptrTy(),
                                                      /*isVarArg=*/true);

  // Insert the printf function into the body of the parent module.
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, kPrintfName, llvmFnType);
}

void Backend::setupScanf() const {
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(intTy(), ptrTy(),
                                                      /*isVarArg=*/true);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, kScanfName, llvmFnType);
}

void Backend::setupIntPow() const {
  // Signature: i32 ipow(i32 base, i32 exp)
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(intTy(), {intTy(), intTy()},
                                                      /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, kIpowName, llvmFnType);
}

void Backend::setupThrowDivisionByZeroError() const {
  // Signature: void throwDivisionByZeroError()
  auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {}, /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, kThrowDivByZeroErrorName, llvmFnType);
}

void Backend::setupThrowArraySizeError() const {
  // Signature: void throwArraySizeError()
  auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {}, /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, kThrowArraySizeErrorName, llvmFnType);
}

void Backend::setupThrowStrideError() const {
  // Signature: void throwStrideError()
  auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {}, /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, kThrowStrideErrorName, llvmFnType);
}

void Backend::setupThrowVectorSizeError() const {
  // Signature: void throwVectorSizeError()
  auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {}, /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, kThrowVectorSizeErrorName, llvmFnType);
}
void Backend::setupThrowArrayIndexError() const {
  // Signature: void throwArrayIndexError()
  auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {}, /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(
      loc, "throwArrayIndexError_019ae3a1_54f9_7452_b095_6faaebe8aa2e", llvmFnType);
}
void Backend::setupPrintArray() const {
  // Signature: void printArray(ptr arrayStructAddr, i32 elementType)
  auto voidType = mlir::LLVM::LLVMVoidType::get(builder->getContext());
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(voidType, {ptrTy(), intTy()},
                                                      /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, kPrintArrayName, llvmFnType);
}

void Backend::printFloat(mlir::Value floatValue) {
  mlir::LLVM::GlobalOp formatString;
  if (!(formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("floatFormat"))) {
    llvm::errs() << "missing format string!\n";
  }
  const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);

  // Convert float (f32) to double (f64) for printf variadic args
  auto doubleTy = mlir::Float64Type::get(builder->getContext());
  mlir::Value doubleValue = builder->create<mlir::LLVM::FPExtOp>(loc, doubleTy, floatValue);

  mlir::ValueRange args = {formatStringPtr, doubleValue};
  auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kPrintfName);
  builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}

void Backend::printInt(mlir::Value integer) {
  mlir::LLVM::GlobalOp formatString;
  if (!(formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("intFormat"))) {
    llvm::errs() << "missing format string!\n";
  }
  const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
  mlir::ValueRange args = {formatStringPtr, integer};
  auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kPrintfName);
  builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}

void Backend::printIntChar(mlir::Value integer) {
  mlir::LLVM::GlobalOp formatString;
  if (!(formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("charFormat"))) {
    llvm::errs() << "missing format string!\n";
  }
  const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
  mlir::ValueRange args = {formatStringPtr, integer};
  auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kPrintfName);
  builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}

void Backend::printBool(mlir::Value boolValue) {
  builder->create<mlir::scf::IfOp>(
      loc, boolValue,
      [&](mlir::OpBuilder &b, mlir::Location l) {
        printChar('T');
        b.create<mlir::scf::YieldOp>(l);
      },
      [&](mlir::OpBuilder &b, mlir::Location l) {
        printChar('F');
        b.create<mlir::scf::YieldOp>(l);
      });
}

void Backend::printChar(char c) {
  mlir::LLVM::GlobalOp formatString;
  if (!(formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("charFormat"))) {
    llvm::errs() << "missing format string!\n";
  }
  const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
  const mlir::Value charToPrint =
      builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI8Type(), c);
  mlir::ValueRange args = {formatStringPtr, charToPrint};
  auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kPrintfName);
  builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}

void Backend::printVector(mlir::Value vectorStructAddr,
                          std::shared_ptr<symTable::Type> vectorType) {
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(vectorType);
  if (!vectorTypeSym)
    return;

  auto elementType = vectorTypeSym->getType();
  while (auto arrayElement = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
    elementType = arrayElement->getType();
  }

  int elementTypeCode = 0;
  auto elementTypeName = elementType ? elementType->getName() : "";
  if (elementTypeName == "integer") {
    elementTypeCode = 0;
  } else if (elementTypeName == "real") {
    elementTypeCode = 1;
  } else if (elementTypeName == "character") {
    elementTypeCode = 2;
  } else if (elementTypeName == "boolean") {
    elementTypeCode = 3;
  }

  auto printArrayFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kPrintArrayName);
  auto elementTypeConst = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), elementTypeCode);
  mlir::ValueRange args = {vectorStructAddr, elementTypeConst};
  builder->create<mlir::LLVM::CallOp>(loc, printArrayFunc, args);
}

void Backend::createGlobalString(const char *str, const char *stringName) const {
  // create string and string type
  auto mlirString = mlir::StringRef(str, strlen(str) + 1);
  auto mlirStringType = mlir::LLVM::LLVMArrayType::get(charTy(), mlirString.size());

  builder->create<mlir::LLVM::GlobalOp>(loc, mlirStringType, /*isConstant=*/true,
                                        mlir::LLVM::Linkage::Internal, stringName,
                                        builder->getStringAttr(mlirString), /*alignment=*/0);
}

void Backend::createGlobalStreamState() const {
  builder->create<mlir::LLVM::GlobalOp>(loc, intTy(), false, mlir::LLVM::Linkage::Internal,
                                        kStreamStateGlobalName, builder->getI32IntegerAttr(0), 0);
}

mlir::Value Backend::constOne() const {
  return builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 1);
}
mlir::Value Backend::constZero() const {
  return builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);
}
mlir::Value Backend::constFalse() const {
  return builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
}
mlir::Value Backend::constTrue() const {
  return builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
}
mlir::Type Backend::structTy(const mlir::ArrayRef<mlir::Type> &memberTypes) {
  return mlir::LLVM::LLVMStructType::getLiteral(&context, memberTypes);
}
mlir::Type Backend::arrayTy() { return structTy({intTy(), ptrTy(), boolTy()}); }
mlir::Type Backend::vectorTy() { return structTy({intTy(), intTy(), ptrTy(), boolTy()}); }
mlir::Type Backend::floatTy() const { return mlir::Float32Type::get(builder->getContext()); }
mlir::Type Backend::charTy() const { return mlir::IntegerType::get(builder->getContext(), 8); }
mlir::Type Backend::boolTy() const { return mlir::IntegerType::get(builder->getContext(), 1); }
mlir::Type Backend::ptrTy() const {
  return mlir::LLVM::LLVMPointerType::get(builder->getContext());
}
mlir::Type Backend::intTy() const { return mlir::IntegerType::get(builder->getContext(), 32); }
mlir::Type Backend::getMLIRType(const std::shared_ptr<symTable::Type> &returnType) {
  if (!returnType) {
    return {};
  }
  if (returnType->getName() == "integer") {
    return intTy();
  }
  if (returnType->getName() == "real") {
    return floatTy();
  }
  if (returnType->getName() == "character") {
    return charTy();
  }
  if (returnType->getName() == "boolean") {
    return boolTy();
  }
  if (returnType->getName() == "tuple") {
    // Tuple type
    const auto tupleTypeSym = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(returnType);
    std::vector<mlir::Type> memberTypes;
    for (const auto &memberType : tupleTypeSym->getResolvedTypes()) {
      memberTypes.push_back(getMLIRType(memberType));
    }
    return structTy(memberTypes);
  }
  if (returnType->getName() == "struct") {
    // Struct type
    const auto structTypeSym = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(returnType);
    std::vector<mlir::Type> memberTypes;
    for (const auto &memberType : structTypeSym->getResolvedTypes()) {
      memberTypes.push_back(getMLIRType(memberType));
    }
    return structTy(memberTypes);
  }
  const auto typeName = returnType->getName();
  if (typeName.substr(0, 5) == "array" || typeName == "empty_array") {
    std::vector<mlir::Type> memberTypes;
    memberTypes.push_back(intTy());  // size
    memberTypes.push_back(ptrTy());  // data
    memberTypes.push_back(boolTy()); // is 2D?
    return structTy(memberTypes);
  }
  if (typeName.substr(0, 6) == "vector") {
    std::vector<mlir::Type> memberTypes;
    memberTypes.push_back(intTy());  // size
    memberTypes.push_back(intTy());  // capacity
    memberTypes.push_back(ptrTy());  // data
    memberTypes.push_back(boolTy()); // is 2D?
    return structTy(memberTypes);
  }
  if (typeName == "empty_array") {
    std::vector<mlir::Type> memberTypes;
    memberTypes.push_back(intTy());  // size
    memberTypes.push_back(ptrTy());  // data
    memberTypes.push_back(boolTy()); // is 2D?
    return structTy(memberTypes);
  }
  // TODO: Support other return types
  return {};
}

std::vector<mlir::Type>
Backend::getMethodParamTypes(const std::vector<std::shared_ptr<ast::Ast>> &params) const {
  std::vector<mlir::Type> paramTypes;
  for (size_t i = 0; i < params.size(); ++i) {
    paramTypes.push_back(ptrTy());
  }
  return paramTypes;
}

mlir::Value Backend::binaryOperandToValue(std::shared_ptr<ast::Ast> ctx,
                                          ast::expressions::BinaryOpType op,
                                          std::shared_ptr<symTable::Type> opType,
                                          std::shared_ptr<symTable::Type> leftType,
                                          std::shared_ptr<symTable::Type> rightType,
                                          mlir::Value incomingLeftAddr,
                                          mlir::Value incomingRightAddr) {
  // copy values here so casting does not affect the incoming addresses
  mlir::Value leftAddr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(leftType), constOne());
  mlir::Value rightAddr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(rightType), constOne());
  copyValue(leftType, incomingLeftAddr, leftAddr);
  copyValue(rightType, incomingRightAddr, rightAddr);
  if (leftType->getName() == "tuple") { // we assert if one is tuple then the other is also a tuple
    auto leftTypeSym = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(leftType);
    auto rightTypeSym = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(rightType);
    if (leftTypeSym->getResolvedTypes().size() != rightTypeSym->getResolvedTypes().size()) {
      // return true/false based on the operation
      switch (op) {
      case ast::expressions::BinaryOpType::EQUAL: {
        auto newAddr =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
        auto falseValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
        builder->create<mlir::LLVM::StoreOp>(loc, falseValue, newAddr);
        return newAddr;
      }
      case ast::expressions::BinaryOpType::NOT_EQUAL: {
        auto newAddr =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
        auto trueValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
        builder->create<mlir::LLVM::StoreOp>(loc, trueValue, newAddr);
        return newAddr;
      }
      default:
        MathError("No other binary operations supported for tuples other than equality checks");
      }
    }
    auto newAddr = builder->create<mlir::LLVM::AllocaOp>(
        loc, ptrTy(), boolTy(), constOne()); // Can only have bool for tuples
    mlir::Value cumulativeResult;
    if (op == ast::expressions::BinaryOpType::EQUAL) {
      cumulativeResult = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
    } else if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
      cumulativeResult = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    }
    builder->create<mlir::LLVM::StoreOp>(loc, cumulativeResult, newAddr);

    // Check if types are the same
    auto leftResolvedTypes = leftTypeSym->getResolvedTypes();
    auto rightResolvedTypes = rightTypeSym->getResolvedTypes();
    for (size_t i = 0; i < leftResolvedTypes.size(); ++i) {
      auto leftTypeName = leftResolvedTypes[i]->getName();
      auto rightTypeName = rightResolvedTypes[i]->getName();
      if ((rightTypeName == "real" && leftTypeName == "integer") ||
          (rightTypeName == "integer" && leftTypeName == "real")) {
        continue;
      } else if (leftTypeName != rightTypeName) {
        if (op == ast::expressions::BinaryOpType::EQUAL) {
          auto falseValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
          builder->create<mlir::LLVM::StoreOp>(loc, falseValue, newAddr);
        } else if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
          auto trueValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
          builder->create<mlir::LLVM::StoreOp>(loc, trueValue, newAddr);
        }
        return newAddr;
      }
    }

    // Compare individual values
    for (size_t i = 0; i < leftTypeSym->getResolvedTypes().size(); ++i) {
      auto gepIndices =
          std::vector<mlir::Value>{builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0),
                                   builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), i)};
      auto leftElementPtr = builder->create<mlir::LLVM::GEPOp>(
          loc, ptrTy(), getMLIRType(leftTypeSym), leftAddr, gepIndices);
      auto rightElementPtr = builder->create<mlir::LLVM::GEPOp>(
          loc, ptrTy(), getMLIRType(rightTypeSym), rightAddr, gepIndices);
      auto comparationValueAddr = binaryOperandToValue(
          ctx, op, opType, leftTypeSym->getResolvedTypes()[i], rightTypeSym->getResolvedTypes()[i],
          leftElementPtr, rightElementPtr);
      builder->create<mlir::scf::IfOp>(
          loc, builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), comparationValueAddr),
          [&](mlir::OpBuilder &b, mlir::Location l) {
            auto curValue = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), newAddr);
            auto trueValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
            mlir::Value newCumulativeValue;
            if (op == ast::expressions::BinaryOpType::EQUAL) {
              newCumulativeValue = builder->create<mlir::LLVM::AndOp>(loc, curValue, trueValue);
            } else if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
              newCumulativeValue = builder->create<mlir::LLVM::OrOp>(loc, curValue, trueValue);
            }
            builder->create<mlir::LLVM::StoreOp>(loc, newCumulativeValue, newAddr);
            b.create<mlir::scf::YieldOp>(l);
          },
          [&](mlir::OpBuilder &b, mlir::Location l) {
            auto curValue = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), newAddr);
            auto falseValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
            mlir::Value newCumulativeValue;
            if (op == ast::expressions::BinaryOpType::EQUAL) {
              newCumulativeValue = builder->create<mlir::LLVM::AndOp>(loc, curValue, falseValue);
            } else if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
              newCumulativeValue = builder->create<mlir::LLVM::OrOp>(loc, curValue, falseValue);
            }
            builder->create<mlir::LLVM::StoreOp>(loc, newCumulativeValue, newAddr);
            b.create<mlir::scf::YieldOp>(l);
          });
    }
    return newAddr;
  }
  if (leftType->getName() ==
      "struct") { // we assert if one is struct then the other is also a struct
    auto leftTypeSym = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(leftType);
    auto rightTypeSym = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(rightType);

    if (leftTypeSym->getStructName() != rightTypeSym->getStructName()) {
      // return true/false based on the operation
      switch (op) {
      case ast::expressions::BinaryOpType::EQUAL: {
        auto newAddr =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
        auto falseValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
        builder->create<mlir::LLVM::StoreOp>(loc, falseValue, newAddr);
        return newAddr;
      }
      case ast::expressions::BinaryOpType::NOT_EQUAL: {
        auto newAddr =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
        auto trueValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
        builder->create<mlir::LLVM::StoreOp>(loc, trueValue, newAddr);
        return newAddr;
      }
      default:
        MathError("No other binary operations supported for tuples other than equality checks");
      }
    }

    auto newAddr = builder->create<mlir::LLVM::AllocaOp>(
        loc, ptrTy(), boolTy(), constOne()); // Can only have bool for tuples
    mlir::Value cumulativeResult;
    if (op == ast::expressions::BinaryOpType::EQUAL) {
      cumulativeResult = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
    } else if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
      cumulativeResult = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    }
    builder->create<mlir::LLVM::StoreOp>(loc, cumulativeResult, newAddr);

    // Check if types are the same
    auto leftResolvedTypes = leftTypeSym->getResolvedTypes();
    auto rightResolvedTypes = rightTypeSym->getResolvedTypes();
    for (size_t i = 0; i < leftResolvedTypes.size(); ++i) {
      auto leftTypeName = leftResolvedTypes[i]->getName();
      auto rightTypeName = rightResolvedTypes[i]->getName();
      if ((rightTypeName == "real" && leftTypeName == "integer") ||
          (rightTypeName == "integer" && leftTypeName == "real")) {
        continue;
      }
      if (leftTypeName != rightTypeName) {
        if (op == ast::expressions::BinaryOpType::EQUAL) {
          auto falseValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
          builder->create<mlir::LLVM::StoreOp>(loc, falseValue, newAddr);
        } else if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
          auto trueValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
          builder->create<mlir::LLVM::StoreOp>(loc, trueValue, newAddr);
        }
        return newAddr;
      }
    }

    // Compare individual values
    for (size_t i = 0; i < leftTypeSym->getResolvedTypes().size(); ++i) {
      auto gepIndices =
          std::vector<mlir::Value>{builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0),
                                   builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), i)};
      auto leftElementPtr = builder->create<mlir::LLVM::GEPOp>(
          loc, ptrTy(), getMLIRType(leftTypeSym), leftAddr, gepIndices);
      auto rightElementPtr = builder->create<mlir::LLVM::GEPOp>(
          loc, ptrTy(), getMLIRType(rightTypeSym), rightAddr, gepIndices);
      auto comparationValueAddr = binaryOperandToValue(
          ctx, op, opType, leftTypeSym->getResolvedTypes()[i], rightTypeSym->getResolvedTypes()[i],
          leftElementPtr, rightElementPtr);
      builder->create<mlir::scf::IfOp>(
          loc, builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), comparationValueAddr),
          [&](mlir::OpBuilder &b, mlir::Location l) {
            auto curValue = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), newAddr);
            auto trueValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
            mlir::Value newCumulativeValue;
            if (op == ast::expressions::BinaryOpType::EQUAL) {
              newCumulativeValue = builder->create<mlir::LLVM::AndOp>(loc, curValue, trueValue);
            } else if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
              newCumulativeValue = builder->create<mlir::LLVM::OrOp>(loc, curValue, trueValue);
            }
            builder->create<mlir::LLVM::StoreOp>(loc, newCumulativeValue, newAddr);
            b.create<mlir::scf::YieldOp>(l);
          },
          [&](mlir::OpBuilder &b, mlir::Location l) {
            auto curValue = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), newAddr);
            auto falseValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
            mlir::Value newCumulativeValue;
            if (op == ast::expressions::BinaryOpType::EQUAL) {
              newCumulativeValue = builder->create<mlir::LLVM::AndOp>(loc, curValue, falseValue);
            } else if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
              newCumulativeValue = builder->create<mlir::LLVM::OrOp>(loc, curValue, falseValue);
            }
            builder->create<mlir::LLVM::StoreOp>(loc, newCumulativeValue, newAddr);
            b.create<mlir::scf::YieldOp>(l);
          });
    }
    return newAddr;
  }
  if (isTypeVector(leftType) || isTypeVector(rightType)) {
    auto combinedType = leftType;
    if (op == ast::expressions::BinaryOpType::BY) {
      auto skipByIndex = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), rightAddr);
      auto isSkipByLessThanOne = builder->create<mlir::LLVM::ICmpOp>(
          loc, mlir::LLVM::ICmpPredicate::slt, skipByIndex, constOne());
      builder->create<mlir::scf::IfOp>(
          loc, isSkipByLessThanOne,
          [&](mlir::OpBuilder &b, mlir::Location l) {
            auto throwStrideErrorFunc =
                module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowStrideErrorName);
            b.create<mlir::LLVM::CallOp>(l, throwStrideErrorFunc, mlir::ValueRange{});
            b.create<mlir::scf::YieldOp>(l);
          },
          [&](mlir::OpBuilder &b, mlir::Location l) { b.create<mlir::scf::YieldOp>(l); });
      auto res = strideVectorByScalar(opType, leftAddr, skipByIndex);
      freeAllocatedMemory(leftType, leftAddr);
      freeAllocatedMemory(rightType, rightAddr);
      return res;
    }
    if (not isTypeVector(leftType)) {
      // should be a scalar
      // promote scalar to the same size as vector by making it a vector of same size
      auto scalarValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(leftType), leftAddr);
      leftAddr =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(rightType), constOne());
      fillVectorWithScalarValueWithVectorStruct(leftAddr, scalarValue, rightAddr, rightType);
      leftType = rightType;
      combinedType = rightType;
    }

    if (not isTypeVector(rightType)) {
      // should be a scalar
      // promote scalar to the same size as vector by making it a vector of same size
      auto scalarValue =
          builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(rightType), rightAddr);
      rightAddr =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(leftType), constOne());
      fillVectorWithScalarValueWithVectorStruct(rightAddr, scalarValue, leftAddr, leftType);
      rightType = leftType;
    }
    if (op != ast::expressions::BinaryOpType::DPIPE &&
        op != ast::expressions::BinaryOpType::EQUAL &&
        op != ast::expressions::BinaryOpType::NOT_EQUAL) {
      throwIfVectorSizeNotEqual(leftAddr, rightAddr, combinedType);
    }

    mlir::Value newAddr;
    if (op == ast::expressions::BinaryOpType::DPIPE) {
      auto res = concatVectors(opType, leftAddr, rightAddr);
      freeAllocatedMemory(leftType, leftAddr);
      freeAllocatedMemory(rightType, rightAddr);
      return res;
    } else if (op == ast::expressions::BinaryOpType::DMUL) {
      if (auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(opType)) {
        auto childType = vectorType->getType();
        auto leftChildTy =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftType)->getType();
        auto rightChildTy =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(rightType)->getType();
        auto is2D = false;
        auto is3D = false;

        auto elementTy = vectorType->getType();
        auto leftElementTy =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftType)->getType();
        auto rightElementTy =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(rightType)->getType();

        auto vectorStructTy = getMLIRType(opType);
        auto rightVectorStructTy = getMLIRType(rightType);

        auto makeIndexConst = [&](int idx) {
          return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
        };

        auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
          return builder->create<mlir::LLVM::GEPOp>(
              loc, ptrTy(), structTy, baseAddr,
              mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
        };

        auto vectorOuterSizeAddr = makeFieldPtr(rightVectorStructTy, rightAddr, VectorOffset::Size);
        auto vectorOuterSize =
            builder->create<mlir::LLVM::LoadOp>(loc, intTy(), vectorOuterSizeAddr);
        auto vectorInnerSize = constZero();

        if (auto subArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementTy)) {
          elementTy = subArrayType->getType();
          is2D = true;
          vectorInnerSize = maxSubVectorSize(rightAddr, rightType);
          leftElementTy =
              std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftElementTy)->getType();
          rightElementTy =
              std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightElementTy)->getType();

          // Check if we have nested arrays (3D case)
          if (auto subSubArrayType =
                  std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementTy)) {
            elementTy = subSubArrayType->getType();
            is3D = true;
          }
        }

        newAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), vectorStructTy, constOne());
        fillVectorWithScalar(newAddr, vectorType, getDefaultValue(elementTy), elementTy,
                             vectorOuterSize, vectorInnerSize);

        auto leftVectorStructTy = getMLIRType(leftType);
        auto leftDataPtrAddr = makeFieldPtr(leftVectorStructTy, leftAddr, VectorOffset::Data);
        mlir::Value leftDataPtr =
            builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataPtrAddr);
        auto rightDataPtrAddr = makeFieldPtr(rightVectorStructTy, rightAddr, VectorOffset::Data);
        mlir::Value rightDataPtr =
            builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataPtrAddr);
        auto newDataPtrAddr = makeFieldPtr(vectorStructTy, newAddr, VectorOffset::Data);
        mlir::Value newDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), newDataPtrAddr);

        builder->create<mlir::scf::ForOp>(
            loc, constZero(), vectorOuterSize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
              auto leftElementPtr = b.create<mlir::LLVM::GEPOp>(
                  l, ptrTy(), getMLIRType(leftChildTy), leftDataPtr, mlir::ValueRange{i});
              auto rightElementPtr = b.create<mlir::LLVM::GEPOp>(
                  l, ptrTy(), getMLIRType(rightChildTy), rightDataPtr, mlir::ValueRange{i});
              auto newValueAddr = binaryOperandToValue(
                  ctx, op, std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(opType)->getType(),
                  leftChildTy, rightChildTy, leftElementPtr, rightElementPtr);
              auto newElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), getMLIRType(childType),
                                                               newDataPtr, mlir::ValueRange{i});
              if (is2D) {
                auto newStructTy = getMLIRType(childType);
                auto newStructAddr =
                    b.create<mlir::LLVM::AllocaOp>(l, ptrTy(), newStructTy, constOne());
                copyArrayStruct(childType, newValueAddr, newStructAddr);
                auto newValue =
                    b.create<mlir::LLVM::LoadOp>(l, getMLIRType(childType), newStructAddr);
                freeAllocatedMemory(childType, newElementPtr);
                b.create<mlir::LLVM::StoreOp>(l, newValue, newElementPtr);
                freeAllocatedMemory(childType, newValueAddr);
              } else {
                auto newValue =
                    b.create<mlir::LLVM::LoadOp>(l, getMLIRType(childType), newValueAddr);
                b.create<mlir::LLVM::StoreOp>(l, newValue, newElementPtr);
              }
              b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
            });
        freeAllocatedMemory(leftType, leftAddr);
        freeAllocatedMemory(rightType, rightAddr);
        return newAddr;
      } else {
        // scalar result type (dot product)
        auto cumalativeValueAddr =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
        builder->create<mlir::LLVM::StoreOp>(loc, getDefaultValue(opType), cumalativeValueAddr);

        auto leftVectorStructTy = getMLIRType(leftType);
        auto rightVectorStructTy = getMLIRType(rightType);

        auto makeIndexConst = [&](int idx) {
          return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
        };

        auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
          return builder->create<mlir::LLVM::GEPOp>(
              loc, ptrTy(), structTy, baseAddr,
              mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
        };

        // Get vector size
        auto vectorSizeAddr = makeFieldPtr(leftVectorStructTy, leftAddr, VectorOffset::Size);
        auto vectorSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), vectorSizeAddr);

        // Get data pointers for both vectors
        auto leftDataPtrAddr = makeFieldPtr(leftVectorStructTy, leftAddr, VectorOffset::Data);
        mlir::Value leftDataPtr =
            builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataPtrAddr);
        auto rightDataPtrAddr = makeFieldPtr(rightVectorStructTy, rightAddr, VectorOffset::Data);
        mlir::Value rightDataPtr =
            builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataPtrAddr);

        // Get child types
        auto leftChildTy =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftType)->getType();
        auto rightChildTy =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(rightType)->getType();

        builder->create<mlir::scf::ForOp>(
            loc, constZero(), vectorSize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
              // Get pointer to left element
              auto leftElementPtr = b.create<mlir::LLVM::GEPOp>(
                  l, ptrTy(), getMLIRType(leftChildTy), leftDataPtr, mlir::ValueRange{i});

              // Get pointer to right element
              auto rightElementPtr = b.create<mlir::LLVM::GEPOp>(
                  l, ptrTy(), getMLIRType(rightChildTy), rightDataPtr, mlir::ValueRange{i});

              auto productValueAddr =
                  binaryOperandToValue(ctx, ast::expressions::BinaryOpType::MULTIPLY, opType,
                                       leftChildTy, rightChildTy, leftElementPtr, rightElementPtr);

              auto productValue =
                  b.create<mlir::LLVM::LoadOp>(l, getMLIRType(opType), productValueAddr);

              auto cumalativeValue =
                  b.create<mlir::LLVM::LoadOp>(loc, getMLIRType(opType), cumalativeValueAddr)
                      .getResult();
              if (opType->getName() == "real") {
                cumalativeValue = b.create<mlir::LLVM::FAddOp>(l, cumalativeValue, productValue);
              } else {
                cumalativeValue = b.create<mlir::LLVM::AddOp>(l, cumalativeValue, productValue);
              }
              b.create<mlir::LLVM::StoreOp>(l, cumalativeValue, cumalativeValueAddr);

              b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
            });
        freeAllocatedMemory(leftType, leftAddr);
        freeAllocatedMemory(rightType, rightAddr);
        return cumalativeValueAddr;
      }
    } else if (op == ast::expressions::BinaryOpType::EQUAL ||
               op == ast::expressions::BinaryOpType::NOT_EQUAL) {
      // Compare vectors for equality
      auto resultAddr = areVectorsEqual(*builder, loc, leftAddr, rightAddr, combinedType);

      // If operation is NOT_EQUAL, negate the result
      if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
        auto equalResult = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), resultAddr);
        auto notEqualResult = builder->create<mlir::LLVM::XOrOp>(loc, equalResult, constTrue());
        builder->create<mlir::LLVM::StoreOp>(loc, notEqualResult, resultAddr);
      }

      freeAllocatedMemory(leftType, leftAddr);
      freeAllocatedMemory(rightType, rightAddr);
      return resultAddr;
    } else {
      // Handle regular binary operations for vectors
      auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(opType);

      auto childType = vectorType->getType();
      auto leftChildTy = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftType)->getType();
      auto rightChildTy =
          std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(rightType)->getType();
      auto is2D = false;
      auto is3D = false;

      auto elementTy = vectorType->getType();
      auto leftElementTy =
          std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftType)->getType();
      auto rightElementTy =
          std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(rightType)->getType();

      auto vectorStructTy = getMLIRType(opType);

      auto makeIndexConst = [&](int idx) {
        return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
      };

      auto makeFieldPtr = [&](mlir::Type structTy, mlir::Value baseAddr, VectorOffset offset) {
        return builder->create<mlir::LLVM::GEPOp>(
            loc, ptrTy(), structTy, baseAddr,
            mlir::ValueRange{makeIndexConst(0), makeIndexConst(static_cast<int>(offset))});
      };

      auto rightVectorStructTy = getMLIRType(rightType);
      auto vectorOuterSizeAddr = makeFieldPtr(rightVectorStructTy, rightAddr, VectorOffset::Size);
      auto vectorOuterSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), vectorOuterSizeAddr);
      auto vectorInnerSize = constZero();

      // Check if element type is an array (2D case)
      if (auto subArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementTy)) {
        elementTy = subArrayType->getType();
        is2D = true;
        vectorInnerSize = maxSubVectorSize(rightAddr, rightType);
        leftElementTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftElementTy)->getType();
        rightElementTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightElementTy)->getType();

        // Check if we have nested arrays (3D case)
        if (auto subSubArrayType =
                std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementTy)) {
          elementTy = subSubArrayType->getType();
          is3D = true;
        }
      }

      newAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), vectorStructTy, constOne());
      fillVectorWithScalar(newAddr, vectorType, getDefaultValue(elementTy), elementTy,
                           vectorOuterSize, vectorInnerSize);

      auto leftVectorStructTy = getMLIRType(leftType);
      auto leftDataPtrAddr = makeFieldPtr(leftVectorStructTy, leftAddr, VectorOffset::Data);
      mlir::Value leftDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataPtrAddr);
      auto rightDataPtrAddr = makeFieldPtr(rightVectorStructTy, rightAddr, VectorOffset::Data);
      mlir::Value rightDataPtr =
          builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataPtrAddr);
      auto newDataPtrAddr = makeFieldPtr(vectorStructTy, newAddr, VectorOffset::Data);
      mlir::Value newDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), newDataPtrAddr);

      builder->create<mlir::scf::ForOp>(
          loc, constZero(), vectorOuterSize, constOne(), mlir::ValueRange{},
          [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
            auto leftElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), getMLIRType(leftChildTy),
                                                              leftDataPtr, mlir::ValueRange{i});
            auto rightElementPtr = b.create<mlir::LLVM::GEPOp>(
                l, ptrTy(), getMLIRType(rightChildTy), rightDataPtr, mlir::ValueRange{i});
            auto newValueAddr = binaryOperandToValue(
                ctx, op, std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(opType)->getType(),
                leftChildTy, rightChildTy, leftElementPtr, rightElementPtr);
            auto newElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), getMLIRType(childType),
                                                             newDataPtr, mlir::ValueRange{i});
            if (is2D) {
              auto newStructTy = getMLIRType(childType);
              auto newStructAddr =
                  b.create<mlir::LLVM::AllocaOp>(l, ptrTy(), newStructTy, constOne());
              copyArrayStruct(childType, newValueAddr, newStructAddr);
              auto newValue =
                  b.create<mlir::LLVM::LoadOp>(l, getMLIRType(childType), newStructAddr);
              freeAllocatedMemory(childType, newElementPtr);
              b.create<mlir::LLVM::StoreOp>(l, newValue, newElementPtr);
              freeAllocatedMemory(childType, newValueAddr);
            } else {
              auto newValue = b.create<mlir::LLVM::LoadOp>(l, getMLIRType(childType), newValueAddr);
              b.create<mlir::LLVM::StoreOp>(l, newValue, newElementPtr);
            }
            b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
          });

      freeAllocatedMemory(leftType, leftAddr);
      freeAllocatedMemory(rightType, rightAddr);
      return newAddr;
    }

    return leftAddr;
  }

  if (isTypeArray(leftType) || isTypeArray(rightType)) {
    mlir::Value newAddr;
    auto combinedType = leftType;
    if (op == ast::expressions::BinaryOpType::BY) {
      auto skipByIndex = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), rightAddr);
      auto isSkipByLessThanOne = builder->create<mlir::LLVM::ICmpOp>(
          loc, mlir::LLVM::ICmpPredicate::slt, skipByIndex, constOne());
      builder->create<mlir::scf::IfOp>(
          loc, isSkipByLessThanOne,
          [&](mlir::OpBuilder &b, mlir::Location l) {
            auto throwVectorSizeErrorFunc =
                module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowStrideErrorName);
            b.create<mlir::LLVM::CallOp>(l, throwVectorSizeErrorFunc, mlir::ValueRange{});
            b.create<mlir::scf::YieldOp>(l);
          },
          [&](mlir::OpBuilder &b, mlir::Location l) { b.create<mlir::scf::YieldOp>(l); });
      auto res = strideArrayByScalar(opType, leftAddr, skipByIndex);
      freeAllocatedMemory(leftType, leftAddr);
      freeAllocatedMemory(rightType, rightAddr);
      return res;
    }
    if (!isTypeArray(leftType) && !isEmptyArray(leftType)) {
      auto scalarValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(leftType), leftAddr);
      leftAddr =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(rightType), constOne());
      fillArrayWithScalarValueWithArrayStruct(leftAddr, scalarValue, rightAddr, rightType);
      leftType = rightType;
      combinedType = rightType;
    }
    if (!isTypeArray(rightType) && !isEmptyArray(rightType)) {
      auto scalarValue =
          builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(rightType), rightAddr);
      rightAddr =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(leftType), constOne());
      fillArrayWithScalarValueWithArrayStruct(rightAddr, scalarValue, leftAddr, leftType);
      rightType = leftType;
      combinedType = leftType;
    }

    // TODO: cast either arrayStructs if needed
    // Check if the size of the arrays are not the same (for operations that require equal sizes)
    if (op != ast::expressions::BinaryOpType::DPIPE &&
        op != ast::expressions::BinaryOpType::EQUAL &&
        op != ast::expressions::BinaryOpType::NOT_EQUAL) {
      throwIfNotEqualArrayStructs(leftAddr, rightAddr, combinedType);
    }
    if (op == ast::expressions::BinaryOpType::DPIPE) {
      auto result = concatArrays(combinedType, leftAddr, rightAddr);
      freeAllocatedMemory(leftType, leftAddr);
      freeAllocatedMemory(rightType, rightAddr);
      return result;
    } else if (op == ast::expressions::BinaryOpType::DMUL) {
      // array is multi-dimentional
      if (auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(opType)) {
        auto childType = arrayType->getType();
        auto leftChildTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftType)->getType();
        auto rightChildTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightType)->getType();
        auto is2D = false;

        auto elementTy = arrayType->getType();
        auto leftElementTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftType)->getType();
        auto rightElementTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightType)->getType();

        auto arrayOuterSizeAddr =
            getArraySizeAddr(*builder, loc, getMLIRType(arrayType), rightAddr);
        auto arrayOuterSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arrayOuterSizeAddr);
        auto arrayInnerSize = constZero();

        if (auto subArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementTy)) {
          elementTy = subArrayType->getType();
          is2D = true;
          arrayInnerSize = maxSubArraySize(rightAddr, rightType);
          leftElementTy =
              std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftElementTy)->getType();
          rightElementTy =
              std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightElementTy)->getType();
        }
        newAddr =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
        fillArrayWithScalar(newAddr, arrayType, getDefaultValue(elementTy), elementTy,
                            arrayOuterSize, arrayInnerSize);
        auto leftDataPtrAddr = getArrayDataAddr(*builder, loc, getMLIRType(leftType), leftAddr);
        mlir::Value leftDataPtr =
            builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataPtrAddr);
        auto rightDataPtrAddr = getArrayDataAddr(*builder, loc, getMLIRType(rightType), rightAddr);
        mlir::Value rightDataPtr =
            builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataPtrAddr);
        auto newDataPtrAddr = getArrayDataAddr(*builder, loc, getMLIRType(opType), newAddr);
        mlir::Value newDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), newDataPtrAddr);
        builder->create<mlir::scf::ForOp>(
            loc, constZero(), arrayOuterSize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
              auto leftElementPtr = b.create<mlir::LLVM::GEPOp>(
                  l, ptrTy(), getMLIRType(leftChildTy), leftDataPtr, mlir::ValueRange{i});
              auto rightElementPtr = b.create<mlir::LLVM::GEPOp>(
                  l, ptrTy(), getMLIRType(rightChildTy), rightDataPtr, mlir::ValueRange{i});
              auto newValueAddr = binaryOperandToValue(
                  ctx, op, std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(opType)->getType(),
                  leftChildTy, rightChildTy, leftElementPtr, rightElementPtr);
              auto newElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), getMLIRType(childType),
                                                               newDataPtr, mlir::ValueRange{i});
              if (is2D) {
                auto newStructTy = getMLIRType(childType);
                auto newStructAddr =
                    b.create<mlir::LLVM::AllocaOp>(l, ptrTy(), newStructTy, constOne());
                copyArrayStruct(childType, newValueAddr, newStructAddr);
                auto newValue =
                    b.create<mlir::LLVM::LoadOp>(l, getMLIRType(childType), newStructAddr);
                freeAllocatedMemory(childType, newElementPtr);
                b.create<mlir::LLVM::StoreOp>(l, newValue, newElementPtr);
                freeAllocatedMemory(childType, newValueAddr);
              } else {
                auto newValue =
                    b.create<mlir::LLVM::LoadOp>(l, getMLIRType(childType), newValueAddr);
                b.create<mlir::LLVM::StoreOp>(l, newValue, newElementPtr);
              }
              b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
            });
        freeAllocatedMemory(leftType, leftAddr);
        freeAllocatedMemory(rightType, rightAddr);
        return newAddr;
      } else {
        // scalar result type
        auto cumalativeValueAddr =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
        builder->create<mlir::LLVM::StoreOp>(loc, getDefaultValue(opType), cumalativeValueAddr);

        // Get array size
        auto arraySizeAddr = getArraySizeAddr(*builder, loc, getMLIRType(leftType), leftAddr);
        auto arraySize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arraySizeAddr);

        // Get data pointers for both arrays
        auto leftDataPtrAddr = getArrayDataAddr(*builder, loc, getMLIRType(leftType), leftAddr);
        mlir::Value leftDataPtr =
            builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataPtrAddr);
        auto rightDataPtrAddr = getArrayDataAddr(*builder, loc, getMLIRType(rightType), rightAddr);
        mlir::Value rightDataPtr =
            builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataPtrAddr);

        // Get child types
        auto leftChildTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftType)->getType();
        auto rightChildTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightType)->getType();

        builder->create<mlir::scf::ForOp>(
            loc, constZero(), arraySize, constOne(), mlir::ValueRange{},
            [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
              // Get pointer to left element
              auto leftElementPtr = b.create<mlir::LLVM::GEPOp>(
                  l, ptrTy(), getMLIRType(leftChildTy), leftDataPtr, mlir::ValueRange{i});
              // Load left element value
              auto leftValue =
                  b.create<mlir::LLVM::LoadOp>(l, getMLIRType(leftChildTy), leftElementPtr);

              // Get pointer to right element
              auto rightElementPtr = b.create<mlir::LLVM::GEPOp>(
                  l, ptrTy(), getMLIRType(rightChildTy), rightDataPtr, mlir::ValueRange{i});
              // Load right element value
              auto rightValue =
                  b.create<mlir::LLVM::LoadOp>(l, getMLIRType(rightChildTy), rightElementPtr);

              auto productValueAddr =
                  binaryOperandToValue(ctx, ast::expressions::BinaryOpType::MULTIPLY, opType,
                                       leftChildTy, rightChildTy, leftElementPtr, rightElementPtr);

              auto productValue =
                  b.create<mlir::LLVM::LoadOp>(l, getMLIRType(opType), productValueAddr);

              auto cumalativeValue =
                  b.create<mlir::LLVM::LoadOp>(loc, getMLIRType(opType), cumalativeValueAddr)
                      .getResult();
              if (opType->getName() == "real") {
                cumalativeValue = b.create<mlir::LLVM::FAddOp>(l, cumalativeValue, productValue);
              } else {
                cumalativeValue = b.create<mlir::LLVM::AddOp>(l, cumalativeValue, productValue);
              }
              b.create<mlir::LLVM::StoreOp>(l, cumalativeValue, cumalativeValueAddr);

              b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
            });
        freeAllocatedMemory(leftType, leftAddr);
        freeAllocatedMemory(rightType, rightAddr);
        return cumalativeValueAddr;
      }
    } else if (op == ast::expressions::BinaryOpType::EQUAL ||
               op == ast::expressions::BinaryOpType::NOT_EQUAL) {
      // Compare arrays for equality
      auto resultAddr = areArraysEqual(leftAddr, rightAddr, combinedType);

      // If operation is NOT_EQUAL, negate the result
      if (op == ast::expressions::BinaryOpType::NOT_EQUAL) {
        auto equalResult = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), resultAddr);
        auto notEqualResult = builder->create<mlir::LLVM::XOrOp>(loc, equalResult, constTrue());
        builder->create<mlir::LLVM::StoreOp>(loc, notEqualResult, resultAddr);
      }

      freeAllocatedMemory(leftType, leftAddr);
      freeAllocatedMemory(rightType, rightAddr);
      return resultAddr;
    }

    auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(opType);

    auto childType = arrayType->getType();
    auto leftChildTy = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftType)->getType();
    auto rightChildTy = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightType)->getType();
    auto is2D = false;

    auto elementTy = arrayType->getType();
    auto leftElementTy = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftType)->getType();
    auto rightElementTy =
        std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightType)->getType();

    auto arrayOuterSizeAddr = getArraySizeAddr(*builder, loc, getMLIRType(arrayType), rightAddr);
    auto arrayOuterSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), arrayOuterSizeAddr);
    auto arrayInnerSize = constZero();

    if (auto subArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementTy)) {
      elementTy = subArrayType->getType();
      is2D = true;
      arrayInnerSize = maxSubArraySize(rightAddr, rightType);
      leftElementTy =
          std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftElementTy)->getType();
      rightElementTy =
          std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rightElementTy)->getType();
    }
    newAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
    fillArrayWithScalar(newAddr, arrayType, getDefaultValue(elementTy), elementTy, arrayOuterSize,
                        arrayInnerSize);
    auto leftDataPtrAddr = getArrayDataAddr(*builder, loc, getMLIRType(leftType), leftAddr);
    mlir::Value leftDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), leftDataPtrAddr);
    auto rightDataPtrAddr = getArrayDataAddr(*builder, loc, getMLIRType(rightType), rightAddr);
    mlir::Value rightDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), rightDataPtrAddr);
    auto newDataPtrAddr = getArrayDataAddr(*builder, loc, getMLIRType(opType), newAddr);
    mlir::Value newDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), newDataPtrAddr);
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), arrayOuterSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
          auto leftElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), getMLIRType(leftChildTy),
                                                            leftDataPtr, mlir::ValueRange{i});
          auto rightElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), getMLIRType(rightChildTy),
                                                             rightDataPtr, mlir::ValueRange{i});
          auto newValueAddr = binaryOperandToValue(
              ctx, op, std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(opType)->getType(),
              leftChildTy, rightChildTy, leftElementPtr, rightElementPtr);
          auto newElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), getMLIRType(childType),
                                                           newDataPtr, mlir::ValueRange{i});
          if (is2D) {
            auto newStructTy = getMLIRType(childType);
            auto newStructAddr =
                b.create<mlir::LLVM::AllocaOp>(l, ptrTy(), newStructTy, constOne());
            copyArrayStruct(childType, newValueAddr, newStructAddr);
            auto newValue = b.create<mlir::LLVM::LoadOp>(l, getMLIRType(childType), newStructAddr);
            freeAllocatedMemory(childType, newElementPtr);
            b.create<mlir::LLVM::StoreOp>(l, newValue, newElementPtr);
            freeAllocatedMemory(childType, newValueAddr);
          } else {
            auto newValue = b.create<mlir::LLVM::LoadOp>(l, getMLIRType(childType), newValueAddr);
            b.create<mlir::LLVM::StoreOp>(l, newValue, newElementPtr);
          }
          b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{});
        });

    freeAllocatedMemory(leftType, leftAddr);
    freeAllocatedMemory(rightType, rightAddr);
    return newAddr;
  } else { // other primitive types
    leftAddr = castIfNeeded(ctx, leftAddr, leftType, rightType);
    rightAddr = castIfNeeded(ctx, rightAddr, rightType, leftType);

    bool isFloatType = (leftType->getName() == "real" || rightType->getName() == "real");
    if (isFloatType) {
      return floatBinaryOperandToValue(op, opType, leftType, rightType, leftAddr, rightAddr);
    }

    auto newAddr =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
    auto leftValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(leftType), leftAddr);
    auto rightValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(leftType), rightAddr);
    mlir::Value result;

    switch (op) {
    case ast::expressions::BinaryOpType::ADD:
      result = builder->create<mlir::LLVM::AddOp>(loc, leftValue, rightValue);
      break;
    case ast::expressions::BinaryOpType::SUBTRACT:
      result = builder->create<mlir::LLVM::SubOp>(loc, leftValue, rightValue);
      break;
    case ast::expressions::BinaryOpType::MULTIPLY:
      result = builder->create<mlir::LLVM::MulOp>(loc, leftValue, rightValue);
      break;
    case ast::expressions::BinaryOpType::DIVIDE: {
      auto isZeroCond = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq,
                                                            rightValue, constZero());
      builder->create<mlir::scf::IfOp>(loc, isZeroCond, [&](mlir::OpBuilder &b, mlir::Location l) {
        auto throwDivByZeroFunc =
            module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowDivByZeroErrorName);
        b.create<mlir::LLVM::CallOp>(l, throwDivByZeroFunc, mlir::ValueRange{});
        b.create<mlir::scf::YieldOp>(l);
      });
      result = builder->create<mlir::LLVM::SDivOp>(loc, leftValue, rightValue);
      break;
    }
    case ast::expressions::BinaryOpType::EQUAL:
      result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, leftValue,
                                                   rightValue);
      break;
    case ast::expressions::BinaryOpType::NOT_EQUAL:
      result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, leftValue,
                                                   rightValue);
      break;
    case ast::expressions::BinaryOpType::LESS_THAN:
      result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, leftValue,
                                                   rightValue);
      break;
    case ast::expressions::BinaryOpType::GREATER_THAN:
      result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sgt, leftValue,
                                                   rightValue);
      break;
    case ast::expressions::BinaryOpType::LESS_EQUAL:
      result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sle, leftValue,
                                                   rightValue);
      break;
    case ast::expressions::BinaryOpType::GREATER_EQUAL:
      result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sge, leftValue,
                                                   rightValue);
      break;
    case ast::expressions::BinaryOpType::REM:
      result = builder->create<mlir::LLVM::SRemOp>(loc, leftValue, rightValue);
      break;
    case ast::expressions::BinaryOpType::POWER: {
      auto ipowFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kIpowName);
      result =
          builder
              ->create<mlir::LLVM::CallOp>(loc, ipowFunc, mlir::ValueRange{leftValue, rightValue})
              .getResult();
      break;
    }
    case ast::expressions::BinaryOpType::AND:
      result = builder->create<mlir::LLVM::AndOp>(loc, leftValue, rightValue);
      break;
    case ast::expressions::BinaryOpType::OR:
      result = builder->create<mlir::LLVM::OrOp>(loc, leftValue, rightValue);
      break;
    case ast::expressions::BinaryOpType::XOR:
      result = builder->create<mlir::LLVM::XOrOp>(loc, leftValue, rightValue);
      break;
    default:
      MathError("Unmatched binary op type");
      break;
    }

    builder->create<mlir::LLVM::StoreOp>(loc, result, newAddr);
    return newAddr;
  }
};

mlir::Value Backend::floatBinaryOperandToValue(ast::expressions::BinaryOpType op,
                                               std::shared_ptr<symTable::Type> opType,
                                               std::shared_ptr<symTable::Type> leftType,
                                               std::shared_ptr<symTable::Type> rightType,
                                               mlir::Value leftAddr, mlir::Value rightAddr) {
  std::shared_ptr<symTable::Type> operandType;
  if (leftType->getName() == "real" || rightType->getName() == "real") {
    operandType = leftType->getName() == "real" ? leftType : rightType;
  } else {
    operandType = leftType;
  }

  auto newAddr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(opType), constOne());
  auto leftValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(operandType), leftAddr);
  auto rightValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(operandType), rightAddr);
  mlir::Value result;

  switch (op) {
  case ast::expressions::BinaryOpType::ADD:
    result = builder->create<mlir::LLVM::FAddOp>(loc, leftValue, rightValue);
    break;
  case ast::expressions::BinaryOpType::SUBTRACT:
    result = builder->create<mlir::LLVM::FSubOp>(loc, leftValue, rightValue);
    break;
  case ast::expressions::BinaryOpType::MULTIPLY:
    result = builder->create<mlir::LLVM::FMulOp>(loc, leftValue, rightValue);
    break;
  case ast::expressions::BinaryOpType::DIVIDE: {
    auto floatZero =
        builder->create<mlir::LLVM::ConstantOp>(loc, getMLIRType(operandType), llvm::APFloat(0.0f));
    auto isZeroCond = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::oeq,
                                                          rightValue, floatZero);
    builder->create<mlir::scf::IfOp>(loc, isZeroCond, [&](mlir::OpBuilder &b, mlir::Location l) {
      auto throwDivByZeroFunc =
          module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowDivByZeroErrorName);
      b.create<mlir::LLVM::CallOp>(l, throwDivByZeroFunc, mlir::ValueRange{});
      b.create<mlir::scf::YieldOp>(l);
    });
    result = builder->create<mlir::LLVM::FDivOp>(loc, leftValue, rightValue);
    break;
  }
  case ast::expressions::BinaryOpType::EQUAL:
    result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::oeq, leftValue,
                                                 rightValue);
    break;
  case ast::expressions::BinaryOpType::NOT_EQUAL:
    result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::one, leftValue,
                                                 rightValue);
    break;
  case ast::expressions::BinaryOpType::LESS_THAN:
    result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::olt, leftValue,
                                                 rightValue);
    break;
  case ast::expressions::BinaryOpType::GREATER_THAN:
    result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::ogt, leftValue,
                                                 rightValue);
    break;
  case ast::expressions::BinaryOpType::LESS_EQUAL:
    result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::ole, leftValue,
                                                 rightValue);
    break;
  case ast::expressions::BinaryOpType::GREATER_EQUAL:
    result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::oge, leftValue,
                                                 rightValue);
    break;
  case ast::expressions::BinaryOpType::POWER:
    result = builder->create<mlir::LLVM::PowOp>(loc, leftValue, rightValue);
    break;
  case ast::expressions::BinaryOpType::REM:
    result = builder->create<mlir::LLVM::FRemOp>(loc, leftValue, rightValue);
    break;
  default:
    MathError("Unmatched binary op type");
    break;
  }

  builder->create<mlir::LLVM::StoreOp>(loc, result, newAddr);
  return newAddr;
}

bool Backend::typesEquivalent(const std::shared_ptr<symTable::Type> &lhs,
                              const std::shared_ptr<symTable::Type> &rhs) {
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs == rhs) {
    return true;
  }
  if (lhs->getName() != rhs->getName()) {
    return false;
  }
  if (lhs->getName() == "tuple") {
    const auto lhsTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(lhs);
    const auto rhsTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(rhs);
    if (!lhsTuple || !rhsTuple) {
      return false;
    }

    const auto &lhsMembers = lhsTuple->getResolvedTypes();
    const auto &rhsMembers = rhsTuple->getResolvedTypes();
    if (lhsMembers.size() != rhsMembers.size()) {
      return false;
    }
    for (size_t i = 0; i < lhsMembers.size(); ++i) {
      if (!typesEquivalent(lhsMembers[i], rhsMembers[i])) {
        return false;
      }
    }
    return true;
  }

  const auto lhsName = lhs->getName();
  const auto rhsName = rhs->getName();

  const bool lhsIsArray = lhsName.substr(0, 5) == "array";
  const bool rhsIsArray = rhsName.substr(0, 5) == "array";

  if (lhsIsArray && rhsIsArray) {
    auto lhsArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhs);
    auto rhsArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(rhs);
    if (!lhsArray || !rhsArray) {
      return false;
    }
    return typesEquivalent(lhsArray->getType(), rhsArray->getType());
  }
  return true;
}

mlir::Value Backend::promoteScalarValue(mlir::Value value,
                                        const std::shared_ptr<symTable::Type> &fromType,
                                        const std::shared_ptr<symTable::Type> &toType) {
  const std::string fromName = fromType->getName();
  const std::string toName = toType->getName();
  auto targetMlirTy = getMLIRType(toType);

  if (fromName == toName) {
    return value;
  }

  switch (utils::getPromotionCode(fromName, toName)) {
  case utils::BOOL_TO_CHAR:
  case utils::BOOL_TO_INT:
    return builder->create<mlir::LLVM::ZExtOp>(loc, targetMlirTy, value);
  case utils::BOOL_TO_REAL:
    return builder->create<mlir::LLVM::UIToFPOp>(loc, targetMlirTy, value);
  case utils::CHAR_TO_BOOL: {
    auto extended = builder->create<mlir::LLVM::ZExtOp>(loc, intTy(), value);
    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);
    return builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, extended, zero);
  }
  case utils::CHAR_TO_INT:
    return builder->create<mlir::LLVM::ZExtOp>(loc, targetMlirTy, value);
  case utils::CHAR_TO_REAL:
    return builder->create<mlir::LLVM::UIToFPOp>(loc, targetMlirTy, value);
  case utils::INT_TO_BOOL: {
    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);
    return builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, value, zero);
  }
  case utils::INT_TO_CHAR:
    return builder->create<mlir::LLVM::TruncOp>(loc, targetMlirTy, value);
  case utils::INT_TO_REAL:
    return builder->create<mlir::LLVM::SIToFPOp>(loc, targetMlirTy, value);
  case utils::REAL_TO_INT:
    return builder->create<mlir::LLVM::FPToSIOp>(loc, targetMlirTy, value);
  case utils::IDENTITY:
    return value;
  default:
    return value;
  }
}

mlir::Value Backend::loadCastSize(const std::vector<mlir::Value> &sizes, size_t idx,
                                  mlir::Value fallback) {
  if (idx < sizes.size() && sizes[idx]) {
    return builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizes[idx]);
  }
  return fallback;
}

mlir::Value Backend::getCastTargetSize(const std::shared_ptr<symTable::ArrayTypeSymbol> &toArray,
                                       size_t dimIndex, mlir::Value srcSize) {
  if (!toArray) {
    return {};
  }
  auto defAst = toArray->getDef();
  auto arrayTypeAst = std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(defAst);
  if (!arrayTypeAst) {
    return {};
  }
  const auto &sizes = arrayTypeAst->getSizes();
  if (dimIndex >= sizes.size()) {
    return {};
  }
  const auto &sizeExpr = sizes[dimIndex];
  if (!sizeExpr) {
    return {};
  }

  if (auto charLit = std::dynamic_pointer_cast<ast::expressions::CharLiteralAst>(sizeExpr)) {
    if (charLit->getValue() == '*') {
      return srcSize;
    }
    return {};
  }

  if (auto intLit = std::dynamic_pointer_cast<ast::expressions::IntegerLiteralAst>(sizeExpr)) {
    return builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), intLit->integerValue);
  }

  return {};
}

void Backend::performArrayCast(mlir::Value srcPtr,
                               std::shared_ptr<symTable::ArrayTypeSymbol> fromArray,
                               mlir::Value dstPtr,
                               std::shared_ptr<symTable::ArrayTypeSymbol> toArray,
                               const std::vector<mlir::Value> &fromSizes,
                               const std::vector<mlir::Value> &toSizes, size_t dimIndex) {
  if (!toArray) {
    return;
  }

  auto dstElemType = toArray->getType();
  auto dstElemMlirType = getMLIRType(dstElemType);
  auto dstStructType = getMLIRType(toArray);

  const bool hasSrc = srcPtr && fromArray;
  mlir::Value srcSize = constZero();
  mlir::Value srcDataPtr;
  std::shared_ptr<symTable::Type> srcElemType;
  mlir::Type srcElemMlirType;

  if (hasSrc) {
    auto srcStructType = getMLIRType(fromArray);
    auto srcSizeAddr = getArraySizeAddr(*builder, loc, srcStructType, srcPtr);
    srcSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeAddr);
    auto srcDataAddr = getArrayDataAddr(*builder, loc, srcStructType, srcPtr);
    srcDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), srcDataAddr);
    srcElemType = fromArray->getType();
    srcElemMlirType = getMLIRType(srcElemType);
  }

  auto explicitTargetSize = getCastTargetSize(toArray, dimIndex, srcSize);
  auto fallbackSize = hasSrc ? srcSize : constZero();
  auto targetSize =
      explicitTargetSize
          ? explicitTargetSize
          : loadCastSize(toSizes, dimIndex, loadCastSize(fromSizes, dimIndex, fallbackSize));

  auto destDataPtr = mallocArray(dstElemMlirType, targetSize);
  auto destSizeAddr = getArraySizeAddr(*builder, loc, dstStructType, dstPtr);
  builder->create<mlir::LLVM::StoreOp>(loc, targetSize, destSizeAddr);

  auto destDataAddr = getArrayDataAddr(*builder, loc, dstStructType, dstPtr);
  builder->create<mlir::LLVM::StoreOp>(loc, destDataPtr, destDataAddr);

  auto is2dConst =
      builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), isTypeArray(dstElemType) ? 1 : 0);
  auto destIs2dAddr = get2DArrayBoolAddr(*builder, loc, dstStructType, dstPtr);
  builder->create<mlir::LLVM::StoreOp>(loc, is2dConst, destIs2dAddr);

  mlir::Value copyLimit = constZero();
  if (hasSrc) {
    auto smaller = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, srcSize,
                                                       targetSize);
    copyLimit = builder->create<mlir::LLVM::SelectOp>(loc, smaller, srcSize, targetSize);

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), copyLimit, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange /*iterArgs*/) {
          auto srcElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), srcElemMlirType, srcDataPtr,
                                                        mlir::ValueRange{i});
          auto dstElemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), dstElemMlirType, destDataPtr,
                                                        mlir::ValueRange{i});
          if (isTypeArray(dstElemType)) {
            auto srcElemArrayType =
                std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(srcElemType);
            auto dstElemArrayType =
                std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(dstElemType);
            mlir::OpBuilder::InsertionGuard guard(*builder);
            builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
            performArrayCast(srcElemPtr, srcElemArrayType, dstElemPtr, dstElemArrayType, fromSizes,
                             toSizes, dimIndex + 1);
          } else {
            auto loadedVal = b.create<mlir::LLVM::LoadOp>(l, getMLIRType(srcElemType), srcElemPtr);
            mlir::OpBuilder::InsertionGuard guard(*builder);
            builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
            auto castedVal = promoteScalarValue(loadedVal, srcElemType, dstElemType);
            b.create<mlir::LLVM::StoreOp>(l, castedVal, dstElemPtr);
          }
          b.create<mlir::scf::YieldOp>(l);
        });
  }

  auto needsPadding = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt,
                                                          copyLimit, targetSize);

  builder->create<mlir::scf::IfOp>(loc, needsPadding, [&](mlir::OpBuilder &b, mlir::Location l) {
    mlir::Value defaultVal;
    if (!isTypeArray(dstElemType)) {
      mlir::OpBuilder::InsertionGuard guard(*builder);
      builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
      defaultVal = getDefaultValue(dstElemType);
    }

    b.create<mlir::scf::ForOp>(
        l, copyLimit, targetSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value i, mlir::ValueRange /*iterArgs2*/) {
          auto dstElemPtr = b2.create<mlir::LLVM::GEPOp>(l2, ptrTy(), dstElemMlirType, destDataPtr,
                                                         mlir::ValueRange{i});

          if (isTypeArray(dstElemType)) {
            auto dstElemArrayType =
                std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(dstElemType);
            auto srcElemArrayType =
                std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(srcElemType);
            mlir::OpBuilder::InsertionGuard guard(*builder);
            builder->setInsertionPoint(b2.getInsertionBlock(), b2.getInsertionPoint());
            performArrayCast(mlir::Value(), srcElemArrayType, dstElemPtr, dstElemArrayType,
                             fromSizes, toSizes, dimIndex + 1);
          } else {
            b2.create<mlir::LLVM::StoreOp>(l2, defaultVal, dstElemPtr);
          }

          b2.create<mlir::scf::YieldOp>(l2);
        });

    b.create<mlir::scf::YieldOp>(l);
  });
}

void Backend::performExplicitCast(mlir::Value srcPtr, std::shared_ptr<symTable::Type> fromType,
                                  mlir::Value dstPtr, std::shared_ptr<symTable::Type> toType) {
  if (fromType->getName() == "tuple") {
    const auto fromTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(fromType);
    const auto toTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(toType);
    auto fromStructTy = getMLIRType(fromType);
    auto toStructTy = getMLIRType(toType);
    const auto &fromMembers = fromTuple->getResolvedTypes();
    const auto &toMembers = toTuple->getResolvedTypes();

    for (size_t i = 0; i < fromMembers.size(); ++i) {
      auto idxZero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
      auto idxPos = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i);
      std::vector<mlir::Value> indices{idxZero, idxPos};
      auto srcElemPtr =
          builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), fromStructTy, srcPtr, indices);
      auto dstElemPtr =
          builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), toStructTy, dstPtr, indices);
      performExplicitCast(srcElemPtr, fromMembers[i], dstElemPtr, toMembers[i]);
    }
    return;
  }
  if (isTypeArray(fromType) && isTypeArray(toType)) {
    const auto fromArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(fromType);
    const auto toArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(toType);

    const auto fromSizes = fromArray ? fromArray->getSizes() : std::vector<mlir::Value>{};
    const auto toSizes = toArray ? toArray->getSizes() : std::vector<mlir::Value>{};

    performArrayCast(srcPtr, fromArray, dstPtr, toArray, fromSizes, toSizes, 0);
    return;
  }

  if (isScalarType(fromType) && isTypeArray(toType)) {
    auto toArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(toType);
    if (!toArray)
      return;

    auto dstStructType = getMLIRType(toType);
    auto dstElemType = toArray->getType();
    auto dstElemMlirType = getMLIRType(dstElemType);

    auto sizes = toArray->getSizes();
    if (sizes.empty()) {
      if (auto arrayTypeAst =
              std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(toArray->getDef())) {
        for (const auto &sizeExpr : arrayTypeAst->getSizes()) {
          visit(sizeExpr);
          auto [_, sizePtr] = popElementFromStack(sizeExpr);
          auto sizeVal = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizePtr);
          sizes.push_back(sizeVal);
        }
      }
    }
    mlir::Value size = sizes.empty() ? constOne() : sizes[0];

    auto dataPtr = mallocArray(dstElemMlirType, size);

    auto sizeFieldPtr = getArraySizeAddr(*builder, loc, dstStructType, dstPtr);
    builder->create<mlir::LLVM::StoreOp>(loc, size, sizeFieldPtr);

    auto dataFieldPtr = getArrayDataAddr(*builder, loc, dstStructType, dstPtr);
    builder->create<mlir::LLVM::StoreOp>(loc, dataPtr, dataFieldPtr);

    bool is2d = isTypeArray(dstElemType);
    auto is2dVal = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), is2d);
    auto is2dFieldPtr = get2DArrayBoolAddr(*builder, loc, dstStructType, dstPtr);
    builder->create<mlir::LLVM::StoreOp>(loc, is2dVal, is2dFieldPtr);

    auto lowerBound = constZero();
    auto upperBound = size;
    auto step = constOne();

    builder->create<mlir::scf::ForOp>(
        loc, lowerBound, upperBound, step, mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange args) {
          auto elemPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), dstElemMlirType, dataPtr,
                                                     mlir::ValueRange{i});
          performExplicitCast(srcPtr, fromType, elemPtr, dstElemType);
          b.create<mlir::scf::YieldOp>(l);
        });
    return;
  }

  // Scalar type casting
  auto fromMlirType = getMLIRType(fromType);
  auto loadedValue = builder->create<mlir::LLVM::LoadOp>(loc, fromMlirType, srcPtr);
  auto castedValue = promoteScalarValue(loadedValue, fromType, toType);
  builder->create<mlir::LLVM::StoreOp>(loc, castedValue, dstPtr);
}

mlir::Value Backend::castIfNeeded(std::shared_ptr<ast::Ast> ctx, mlir::Value valueAddr,
                                  std::shared_ptr<symTable::Type> fromType,
                                  std::shared_ptr<symTable::Type> toType) {
  // Check if we need scalar-to-array conversion
  bool needsScalarToArrayCast = false;
  if (toType->getName().substr(0, 5) == "array" &&
      (fromType->getName() == "integer" || fromType->getName() == "real" ||
       fromType->getName() == "character" || fromType->getName() == "boolean")) {
    needsScalarToArrayCast = true;
  }

  if (needsScalarToArrayCast) {
    // Load the scalar value and create a new array
    auto scalarValue = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(fromType), valueAddr);
    return castScalarToArray(ctx, scalarValue, fromType, toType);
  }

  if (fromType->getName() == "empty_array" && toType->getName().substr(0, 5) == "array") {
    auto toArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(toType);
    if (toArrayType) {
      if (toArrayType->getSizes().empty()) {
        if (!toArrayType->declaredElementSize.empty()) {
          for (auto size : toArrayType->declaredElementSize) {
            toArrayType->addSize(size);
          }
        }
      }

      auto newArrayAddr =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(toType), constOne());

      // Get the innermost element type to determine the default value
      auto elementType = toArrayType->getType();
      while (auto nestedArrayType =
                 std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType)) {
        elementType = nestedArrayType->getType();
      }

      auto defaultValue = getDefaultValue(elementType);
      fillArrayFromScalar(newArrayAddr, toArrayType, defaultValue);
      return newArrayAddr;
    }
  }

  // Check if we need array-to-array padding (different sizes)
  if (fromType->getName().substr(0, 5) == "array" && toType->getName().substr(0, 5) == "array") {
    auto fromArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(fromType);
    auto toArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(toType);

    // Only create a new array if the target type has explicit sizes (like for function returns)
    // AND they're different type objects (prevents affecting normal declarations)
    if (toArrayType && !toArrayType->getSizes().empty() && fromType.get() != toType.get()) {
      // Allocate new array with target type
      auto newArrayAddr =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(toType), constOne());

      // Copy source array to new array
      copyValue(fromType, valueAddr, newArrayAddr);

      // Get target sizes for validation and padding
      mlir::Value targetOuterSize =
          builder->create<mlir::LLVM::LoadOp>(loc, intTy(), toArrayType->getSizes()[0]);
      mlir::Value targetInnerSize = constZero();
      if (toArrayType->getSizes().size() > 1) {
        targetInnerSize =
            builder->create<mlir::LLVM::LoadOp>(loc, intTy(), toArrayType->getSizes()[1]);
      }

      // Get current size from the copied array
      auto currentStructType = getMLIRType(fromType);
      auto currentSizeAddr = getArraySizeAddr(*builder, loc, currentStructType, newArrayAddr);
      mlir::Value currentSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), currentSizeAddr);

      // Validation: check if current size is larger than target size
      auto isSizeTooBig = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sgt,
                                                              currentSize, targetOuterSize);
      builder->create<mlir::scf::IfOp>(
          loc, isSizeTooBig, [&](mlir::OpBuilder &b, mlir::Location l) {
            auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
                "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
            b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
            b.create<mlir::scf::YieldOp>(l);
          });

      // 2D validation
      if (toArrayType->getSizes().size() > 1) {
        mlir::Value maxSubSize = maxSubArraySize(newArrayAddr, fromType);
        auto isInnerSizeTooBig = builder->create<mlir::LLVM::ICmpOp>(
            loc, mlir::LLVM::ICmpPredicate::sgt, maxSubSize, targetInnerSize);
        builder->create<mlir::scf::IfOp>(
            loc, isInnerSizeTooBig, [&](mlir::OpBuilder &b, mlir::Location l) {
              auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
                  "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
              b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
              b.create<mlir::scf::YieldOp>(l);
            });
      }

      // Pad the new array to target size
      padArrayIfNeeded(newArrayAddr, toType, targetOuterSize, targetInnerSize);

      return newArrayAddr;
    }
  }

  // Normal casting - operates in place
  computeArraySizeIfArray(ctx, toType, valueAddr);
  castScalarToArrayIfNeeded(toType, valueAddr, fromType);
  arraySizeValidation(ctx, toType, valueAddr);
  if (auto fromTupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(fromType)) {
    auto toTupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(toType);
    auto sTy = getMLIRType(fromType);
    for (size_t i = 0; i < fromTupleTypeSymbol->getResolvedTypes().size(); i++) {
      auto fromSubType = fromTupleTypeSymbol->getResolvedTypes()[i];
      auto toSubType = toTupleTypeSymbol->getResolvedTypes()[i];
      auto gepIndices = std::vector<mlir::Value>{
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};
      auto elementPtr =
          builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, valueAddr, gepIndices);
      // Note: For tuple elements, in-place casting is expected (no scalar-to-array in tuples)
      castIfNeeded(ctx, elementPtr, fromSubType, toSubType);
    }
  } else if (fromType->getName() == "integer" && toType->getName() == "real") {
    auto value = builder->create<mlir::LLVM::LoadOp>(loc, builder->getI32Type(), valueAddr);
    auto castedValue = builder->create<mlir::LLVM::SIToFPOp>(
        loc, mlir::Float32Type::get(builder->getContext()), value);
    builder->create<mlir::LLVM::StoreOp>(loc, castedValue, valueAddr);
  } else if (isTypeArray(fromType) && isTypeArray(toType)) {
    auto fromArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(fromType);
    auto toArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(toType);
    if (!fromArray || !toArray) {
      return mlir::Value{};
    }
    auto dstStructType = getMLIRType(toType);
    auto tmpDst =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), dstStructType, constOne()).getResult();

    const auto fromSizes = fromArray->getSizes();
    const auto toSizes = toArray->getSizes();
    performArrayCast(valueAddr, fromArray, tmpDst, toArray, fromSizes, toSizes, 0);
    freeArray(fromType, valueAddr);
    auto srcSizeAddr = getArraySizeAddr(*builder, loc, dstStructType, tmpDst);
    auto srcDataAddr = getArrayDataAddr(*builder, loc, dstStructType, tmpDst);
    auto srcIs2dAddr = get2DArrayBoolAddr(*builder, loc, dstStructType, tmpDst);

    auto dstSizeAddr = getArraySizeAddr(*builder, loc, dstStructType, valueAddr);
    auto dstDataAddr = getArrayDataAddr(*builder, loc, dstStructType, valueAddr);
    auto dstIs2dAddr = get2DArrayBoolAddr(*builder, loc, dstStructType, valueAddr);

    auto sizeVal = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), srcSizeAddr);
    auto dataVal = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), srcDataAddr);
    auto is2dVal = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), srcIs2dAddr);

    builder->create<mlir::LLVM::StoreOp>(loc, sizeVal, dstSizeAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, dataVal, dstDataAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, is2dVal, dstIs2dAddr);
  }

  return valueAddr;
}

void Backend::copyValue(std::shared_ptr<symTable::Type> type, mlir::Value fromAddr,
                        mlir::Value destAddr) {
  if (type->getName() == "tuple") {
    auto sTy = getMLIRType(type);
    const auto fromTupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(type);
    for (size_t i = 0; i < fromTupleTypeSymbol->getResolvedTypes().size(); i++) {
      auto fromSubType = fromTupleTypeSymbol->getResolvedTypes()[i];
      auto gepIndices = std::vector<mlir::Value>{
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};
      auto elementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, fromAddr, gepIndices);
      auto newElementPtr =
          builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, destAddr, gepIndices);
      auto newAddrForElement =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(fromSubType), constOne());
      copyValue(fromSubType, elementPtr, newAddrForElement);
      auto loadedValue =
          builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(fromSubType), newAddrForElement);
      builder->create<mlir::LLVM::StoreOp>(loc, loadedValue, newElementPtr);
    }
  } else if (type->getName() == "struct") {
    auto sTy = getMLIRType(type);
    const auto fromStructTypeSymbol = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(type);
    for (size_t i = 0; i < fromStructTypeSymbol->getResolvedTypes().size(); i++) {
      auto fromSubType = fromStructTypeSymbol->getResolvedTypes()[i];
      auto gepIndices = std::vector<mlir::Value>{
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};
      auto elementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, fromAddr, gepIndices);
      auto newElementPtr =
          builder->create<mlir::LLVM::GEPOp>(loc, ptrTy(), sTy, destAddr, gepIndices);
      auto newAddrForElement =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(fromSubType), constOne());
      copyValue(fromSubType, elementPtr, newAddrForElement);
      auto loadedValue =
          builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(fromSubType), newAddrForElement);
      builder->create<mlir::LLVM::StoreOp>(loc, loadedValue, newElementPtr);
    }
  } else if (isTypeArray(type)) {
    copyArrayStruct(type, fromAddr, destAddr);
  } else if (isTypeVector(type)) {
    copyVectorStruct(type, fromAddr, destAddr);
  } else {
    auto value = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(type), fromAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, value, destAddr);
  }
}

bool Backend::isTypeArray(std::shared_ptr<symTable::Type> type) {
  return (type->getName().substr(0, 5) == "array") || isEmptyArray(type);
}

bool Backend::isEmptyArray(std::shared_ptr<symTable::Type> type) {
  return (type->getName() == "empty_array") ? true : false;
}

bool Backend::isTypeVector(std::shared_ptr<symTable::Type> type) {
  return (type->getName().substr(0, 6) == "vector") ? true : false;
}
bool Backend::isScalarType(std::shared_ptr<symTable::Type> type) const {
  if (!type) {
    return false;
  }
  const auto &name = type->getName();
  return name == "integer" || name == "real" || name == "character" || name == "boolean";
}

void Backend::readInteger(mlir::Value destAddr) {
  if (!destAddr) {
    return;
  }
  auto scanfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kScanfName);
  if (!scanfFunc) {
    return;
  }
  auto zeroValue = constZero();
  builder->create<mlir::LLVM::StoreOp>(loc, zeroValue, destAddr);
  auto formatGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>("intInputFormat");
  auto formatPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatGlobal);
  auto call =
      builder->create<mlir::LLVM::CallOp>(loc, scanfFunc, mlir::ValueRange{formatPtr, destAddr});
  auto result = call.getResult();

  auto streamGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>(kStreamStateGlobalName);
  auto streamStatePtr = builder->create<mlir::LLVM::AddressOfOp>(loc, streamGlobal);

  auto minusOne = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), -1);
  auto twoConst = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 2);
  auto zeroConst = constZero();
  auto oneConst = constOne();
  auto isEOF =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, result, minusOne);
  auto isSuccess =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, result, oneConst);
  auto stateWhenNotEOF = builder->create<mlir::LLVM::SelectOp>(loc, isSuccess, zeroConst, oneConst);
  auto finalState = builder->create<mlir::LLVM::SelectOp>(loc, isEOF, twoConst, stateWhenNotEOF);
  builder->create<mlir::LLVM::StoreOp>(loc, finalState, streamStatePtr);
}

void Backend::readReal(mlir::Value destAddr) {
  if (!destAddr) {
    return;
  }
  auto scanfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kScanfName);
  if (!scanfFunc) {
    return;
  }
  auto zeroFloat = builder->create<mlir::LLVM::ConstantOp>(loc, floatTy(),
                                                           builder->getFloatAttr(floatTy(), 0.0));
  builder->create<mlir::LLVM::StoreOp>(loc, zeroFloat, destAddr);
  auto formatGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>("floatInputFormat");
  auto formatPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatGlobal);
  auto call =
      builder->create<mlir::LLVM::CallOp>(loc, scanfFunc, mlir::ValueRange{formatPtr, destAddr});
  auto result = call.getResult();

  auto streamGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>(kStreamStateGlobalName);
  auto streamStatePtr = builder->create<mlir::LLVM::AddressOfOp>(loc, streamGlobal);

  auto minusOne = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), -1);
  auto twoConst = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 2);
  auto zeroConst = constZero();
  auto oneConst = constOne();
  auto isEOF =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, result, minusOne);
  auto isSuccess =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, result, oneConst);
  auto stateWhenNotEOF = builder->create<mlir::LLVM::SelectOp>(loc, isSuccess, zeroConst, oneConst);
  auto finalState = builder->create<mlir::LLVM::SelectOp>(loc, isEOF, twoConst, stateWhenNotEOF);
  builder->create<mlir::LLVM::StoreOp>(loc, finalState, streamStatePtr);
}

void Backend::readCharacter(mlir::Value destAddr) {
  if (!destAddr) {
    return;
  }
  auto scanfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kScanfName);
  if (!scanfFunc) {
    return;
  }
  auto formatGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>("charInputFormat");
  auto formatPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatGlobal);
  auto call =
      builder->create<mlir::LLVM::CallOp>(loc, scanfFunc, mlir::ValueRange{formatPtr, destAddr});
  auto result = call.getResult();

  auto streamGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>(kStreamStateGlobalName);
  auto streamStatePtr = builder->create<mlir::LLVM::AddressOfOp>(loc, streamGlobal);

  auto minusOne = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), -1);
  auto twoConst = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 2);
  auto zeroConst = constZero();
  auto isEOF =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, result, minusOne);
  auto charMinusOne = builder->create<mlir::LLVM::ConstantOp>(loc, charTy(), -1);
  auto currentValue = builder->create<mlir::LLVM::LoadOp>(loc, charTy(), destAddr);
  auto finalChar = builder->create<mlir::LLVM::SelectOp>(loc, isEOF, charMinusOne, currentValue);
  builder->create<mlir::LLVM::StoreOp>(loc, finalChar, destAddr);

  auto finalState = builder->create<mlir::LLVM::SelectOp>(loc, isEOF, twoConst, zeroConst);
  builder->create<mlir::LLVM::StoreOp>(loc, finalState, streamStatePtr);
}

void Backend::readBoolean(mlir::Value destAddr) {
  if (!destAddr) {
    return;
  }
  auto scanfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kScanfName);
  if (!scanfFunc) {
    return;
  }
  auto defaultBoolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
  builder->create<mlir::LLVM::StoreOp>(loc, defaultBoolFalse, destAddr);
  auto formatGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>("boolInputFormat");
  auto formatPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatGlobal);
  auto tempChar = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), charTy(), constOne());
  auto defaultChar = builder->create<mlir::LLVM::ConstantOp>(loc, charTy(), 'F');
  builder->create<mlir::LLVM::StoreOp>(loc, defaultChar, tempChar);
  auto call =
      builder->create<mlir::LLVM::CallOp>(loc, scanfFunc, mlir::ValueRange{formatPtr, tempChar});
  auto result = call.getResult();

  auto streamGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>(kStreamStateGlobalName);
  auto streamStatePtr = builder->create<mlir::LLVM::AddressOfOp>(loc, streamGlobal);

  auto minusOne = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), -1);
  auto twoConst = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 2);
  auto zeroConst = constZero();
  auto oneConst = constOne();
  auto isEOF =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, result, minusOne);
  auto isSuccess =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, result, oneConst);

  auto readValue = builder->create<mlir::LLVM::LoadOp>(loc, charTy(), tempChar);
  auto tConst = builder->create<mlir::LLVM::ConstantOp>(loc, charTy(), 'T');
  auto fConst = builder->create<mlir::LLVM::ConstantOp>(loc, charTy(), 'F');
  auto isTrueChar =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, readValue, tConst);
  auto isFalseChar =
      builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, readValue, fConst);
  auto isValid = builder->create<mlir::LLVM::OrOp>(loc, isTrueChar, isFalseChar);

  auto boolTrue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1);
  auto boolFalse = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
  auto boolFromChar = builder->create<mlir::LLVM::SelectOp>(loc, isTrueChar, boolTrue, boolFalse);
  auto boolWhenValid = builder->create<mlir::LLVM::SelectOp>(loc, isValid, boolFromChar, boolFalse);
  auto boolWhenSuccess =
      builder->create<mlir::LLVM::SelectOp>(loc, isSuccess, boolWhenValid, boolFalse);
  auto finalBool = builder->create<mlir::LLVM::SelectOp>(loc, isEOF, boolFalse, boolWhenSuccess);
  builder->create<mlir::LLVM::StoreOp>(loc, finalBool, destAddr);

  auto stateOnSuccess = builder->create<mlir::LLVM::SelectOp>(loc, isValid, zeroConst, oneConst);
  auto stateWhenNotEOF =
      builder->create<mlir::LLVM::SelectOp>(loc, isSuccess, stateOnSuccess, oneConst);
  auto finalState = builder->create<mlir::LLVM::SelectOp>(loc, isEOF, twoConst, stateWhenNotEOF);
  builder->create<mlir::LLVM::StoreOp>(loc, finalState, streamStatePtr);
}

void Backend::pushElementToScopeStack(std::shared_ptr<ast::Ast> ctx,
                                      std::shared_ptr<symTable::Type> elementType,
                                      mlir::Value val) {
  ctx->getScope()->pushElementToScopeStack(elementType, val);
}

std::pair<std::shared_ptr<symTable::Type>, mlir::Value>
Backend::popElementFromStack(std::shared_ptr<ast::expressions::ExpressionAst> ctx) {
  auto [type, mlirStruct] = ctx->getScope()->getTopElementInStack();
  if (not ctx->isLValue())
    ctx->getScope()->pushElementToFree({type, mlirStruct});
  ctx->getScope()->popElementFromScopeStack();
  return {type, mlirStruct};
}

void Backend::freeScopeResources(std::shared_ptr<symTable::Scope> scope, bool clear) {
  const auto elements = scope->getElementsToFree();
  const auto stackElements = scope->getScopeStack();
  for (auto element : stackElements) {
    freeAllocatedMemory(element.first, element.second);
  }
  for (auto element : elements) {
    freeAllocatedMemory(element.first, element.second);
  }

  if (auto baseScope = std::dynamic_pointer_cast<symTable::BaseScope>(scope)) {
    for (const auto &[name, symbol] : baseScope->getSymbols()) {
      if (auto varSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(symbol)) {
        auto type = varSymbol->getType();
        if (varSymbol->value)
          freeAllocatedMemory(type, varSymbol->value);
      }
    }
  }

  if (clear) {
    scope->getScopeStack().clear();
    scope->clearElementsToFree();
  }
}

void Backend::freeElementsFromMemory(std::shared_ptr<ast::Ast> ctx) {
  freeScopeResources(ctx->getScope(), true);
}

void Backend::freeResourcesUntilFunction(std::shared_ptr<symTable::Scope> startScope) {
  auto currentScope = startScope;
  while (currentScope) {
    freeScopeResources(currentScope, false);

    auto type = currentScope->getScopeType();
    if (type == symTable::ScopeType::Function || type == symTable::ScopeType::Procedure ||
        type == symTable::ScopeType::Global) {
      break;
    }

    currentScope = currentScope->getEnclosingScope();
  }
}

void Backend::freeAllocatedMemory(const std::shared_ptr<symTable::Type> &type,
                                  mlir::Value ptrToMemory) {
  if (isTypeArray(type)) {
    freeArray(type, ptrToMemory);
  } else if (isTypeVector(type)) {
    freeVector(type, ptrToMemory);
  } else if (isTypeTuple(type)) {
    freeTuple(type, ptrToMemory);
  } else if (isTypeStruct(type)) {
    freeStruct(type, ptrToMemory);
  }
}

void Backend::createGlobalDeclaration(const std::string &typeName,
                                      std::shared_ptr<ast::Ast> exprAst,
                                      std::shared_ptr<symTable::Symbol> symbol,
                                      const std::string &variableName) {
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(symbol);
  if (typeName == "integer") {
    auto intAst = std::dynamic_pointer_cast<ast::expressions::IntegerLiteralAst>(exprAst);
    // Do implicit type conversion here
    if (variableSymbol->getType()->getName() == "real") {
      builder->create<mlir::LLVM::GlobalOp>(
          loc, floatTy(),
          /*isConstant=*/true, mlir::LLVM::Linkage::Internal, variableName,
          builder->getFloatAttr(floatTy(), intAst->integerValue), 0);
    } else {
      builder->create<mlir::LLVM::GlobalOp>(
          loc, intTy(),
          /*isConstant=*/true, mlir::LLVM::Linkage::Internal, variableName,
          builder->getIntegerAttr(intTy(), intAst->integerValue), 0);
    }
  } else if (typeName == "real") {
    auto realAst = std::dynamic_pointer_cast<ast::expressions::RealLiteralAst>(exprAst);
    builder->create<mlir::LLVM::GlobalOp>(loc, floatTy(),
                                          /*isConstant=*/true, mlir::LLVM::Linkage::Internal,
                                          variableName,
                                          builder->getFloatAttr(floatTy(), realAst->realValue), 0);
  } else if (typeName == "character") {
    auto charAst = std::dynamic_pointer_cast<ast::expressions::CharLiteralAst>(exprAst);
    builder->create<mlir::LLVM::GlobalOp>(
        loc, charTy(), true, mlir::LLVM::Linkage::Internal, variableName,
        builder->getIntegerAttr(charTy(), charAst->getValue()), 0);
  } else if (typeName == "boolean") {
    auto boolAst = std::dynamic_pointer_cast<ast::expressions::BoolLiteralAst>(exprAst);
    builder->create<mlir::LLVM::GlobalOp>(
        loc, boolTy(), true, mlir::LLVM::Linkage::Internal, variableName,
        builder->getIntegerAttr(boolTy(), boolAst->getValue()), 0);
  } else if (typeName == "tuple") {
    auto tupleAst = std::dynamic_pointer_cast<ast::expressions::TupleLiteralAst>(exprAst);
    auto tupleTypeSym =
        std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(variableSymbol->getType());
    auto structType = getMLIRType(tupleTypeSym);

    // GlobalOp with an initializer region
    auto globalOp = builder->create<mlir::LLVM::GlobalOp>(
        loc, structType,
        /*isConstant=*/true, mlir::LLVM::Linkage::Internal, variableName, mlir::Attribute(),
        /*alignment=*/0);

    auto &region = globalOp.getInitializerRegion();
    auto *block = builder->createBlock(&region);
    builder->setInsertionPointToStart(block);

    mlir::Value structValue = builder->create<mlir::LLVM::UndefOp>(loc, structType);

    for (size_t i = 0; i < tupleAst->getElements().size(); i++) {
      auto element = tupleAst->getElements()[i];
      mlir::Value elementValue;
      if (auto intLit = std::dynamic_pointer_cast<ast::expressions::IntegerLiteralAst>(element)) {
        if (tupleTypeSym->getResolvedTypes()[i]->getName() == "real") {
          elementValue = builder->create<mlir::LLVM::ConstantOp>(
              loc, floatTy(), builder->getFloatAttr(floatTy(), intLit->integerValue));
        } else {
          elementValue = builder->create<mlir::LLVM::ConstantOp>(
              loc, intTy(), builder->getIntegerAttr(intTy(), intLit->integerValue));
        }
      } else if (auto realLit =
                     std::dynamic_pointer_cast<ast::expressions::RealLiteralAst>(element)) {
        elementValue = builder->create<mlir::LLVM::ConstantOp>(
            loc, floatTy(), builder->getFloatAttr(floatTy(), realLit->realValue));
      } else if (auto charLit =
                     std::dynamic_pointer_cast<ast::expressions::CharLiteralAst>(element)) {
        elementValue = builder->create<mlir::LLVM::ConstantOp>(
            loc, charTy(), builder->getIntegerAttr(charTy(), charLit->getValue()));
      } else if (auto boolLit =
                     std::dynamic_pointer_cast<ast::expressions::BoolLiteralAst>(element)) {
        elementValue = builder->create<mlir::LLVM::ConstantOp>(
            loc, boolTy(), builder->getIntegerAttr(boolTy(), boolLit->getValue()));
      }
      structValue = builder->create<mlir::LLVM::InsertValueOp>(loc, structValue, elementValue, i);
    }
    builder->create<mlir::LLVM::ReturnOp>(loc, structValue);
    builder->setInsertionPointToEnd(module.getBody());
  }
}
} // namespace gazprea::backend
