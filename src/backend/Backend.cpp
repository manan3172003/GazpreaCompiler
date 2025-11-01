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

  // Constants
  constOne = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);
  constZero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);

  // Types
  intTy = mlir::IntegerType::get(&context, 32);
  floatTy = mlir::Float32Type::get(&context);
  charTy = mlir::IntegerType::get(&context, 8);
  ptrTy = mlir::LLVM::LLVMPointerType::get(&context);

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
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(intTy, ptrTy,
                                                      /*isVarArg=*/true);

  // Insert the printf function into the body of the parent module.
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, "printf", llvmFnType);
}

void Backend::createGlobalString(const char *str, const char *stringName) const {
  // create string and string type
  auto mlirString = mlir::StringRef(str, strlen(str) + 1);
  auto mlirStringType = mlir::LLVM::LLVMArrayType::get(charTy, mlirString.size());

  builder->create<mlir::LLVM::GlobalOp>(loc, mlirStringType, /*isConstant=*/true,
                                        mlir::LLVM::Linkage::Internal, stringName,
                                        builder->getStringAttr(mlirString), /*alignment=*/0);
  return;
}
} // namespace gazprea::backend