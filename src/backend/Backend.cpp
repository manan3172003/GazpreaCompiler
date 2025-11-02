#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"

#include <assert.h>
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
  createGlobalString("%c\0", "charFormat");
  createGlobalString("%d\0", "intFormat");
}

int Backend::emitModule() {

  visit(ast);

  module.dump();

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
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, "printf", llvmFnType);
}

void Backend::printInt(mlir::Value integer) {
  mlir::LLVM::GlobalOp formatString;
  if (!(formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("intFormat"))) {
    llvm::errs() << "missing format string!\n";
  }
  const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
  mlir::ValueRange args = {formatStringPtr, integer};
  auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
  builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
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
  auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
  builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}

void Backend::createGlobalString(const char *str, const char *stringName) const {
  // create string and string type
  auto mlirString = mlir::StringRef(str, strlen(str) + 1);
  auto mlirStringType = mlir::LLVM::LLVMArrayType::get(charTy(), mlirString.size());

  builder->create<mlir::LLVM::GlobalOp>(loc, mlirStringType, /*isConstant=*/true,
                                        mlir::LLVM::Linkage::Internal, stringName,
                                        builder->getStringAttr(mlirString), /*alignment=*/0);
  return;
}
mlir::Value Backend::constOne() const {
  return builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 1);
}
mlir::Value Backend::constZero() const {
  return builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);
}
mlir::Type Backend::structTy(const mlir::ArrayRef<mlir::Type> &memberTypes) {
  return mlir::LLVM::LLVMStructType::getLiteral(&context, memberTypes);
}
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
} // namespace gazprea::backend