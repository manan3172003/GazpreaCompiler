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
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"

namespace gazprea::backend {
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
  std::any visitFuncProcCall(std::shared_ptr<ast::expressions::FuncProcCallAst> ctx) override;
  std::any visitArg(std::shared_ptr<ast::expressions::ArgAst> ctx) override;
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

protected:
  void setupPrintf() const;
  void printFloat(mlir::Value floatValue);
  void printInt(mlir::Value integer);
  void printIntChar(mlir::Value integer);
  void printBool(mlir::Value boolValue);
  void printChar(char c);
  void createGlobalString(const char *str, const char *stringName) const;
  void castIfNeeded(mlir::Value valueAddr, std::shared_ptr<symTable::Type> fromType,
                    std::shared_ptr<symTable::Type> toType);
  void copyValue(std::shared_ptr<symTable::Type> type, mlir::Value fromAddr, mlir::Value destAddr);
  void createGlobalDeclaration(const std::string &typeName, std::shared_ptr<ast::Ast> exprAst,
                               const std::string &variableName);

private:
  std::shared_ptr<ast::Ast> ast;
  std::unordered_map<std::string, mlir::Value> blockArg;

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

  mlir::Type structTy(const mlir::ArrayRef<mlir::Type> &memberTypes);
  mlir::Type floatTy() const;
  mlir::Type charTy() const;
  mlir::Type boolTy() const;
  mlir::Type ptrTy() const;
  mlir::Type intTy() const;

  // helpers
  mlir::Type getMLIRType(const std::shared_ptr<symTable::Type> &returnType);
  std::vector<mlir::Type>
  getMethodParamTypes(const std::vector<std::shared_ptr<ast::Ast>> &params) const;
};
} // namespace gazprea::backend
