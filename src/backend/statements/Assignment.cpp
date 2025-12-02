#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"

#include <backend/Backend.h>

namespace gazprea::backend {

std::any Backend::visitAssignment(std::shared_ptr<ast::statements::AssignmentAst> ctx) {
  visit(ctx->getExpr());
  auto [type, valueAddr] = popElementFromStack(ctx);
  auto variableSymbol =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getLVal()->getSymbol());
  if (not variableSymbol) { // Tuple Unpack Assignment
    auto tupleUnpackAssignAst =
        std::dynamic_pointer_cast<ast::statements::TupleUnpackAssignAst>(ctx->getLVal());
    auto tupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        ctx->getExpr()->getInferredSymbolType());
    for (size_t i = 0; i < tupleUnpackAssignAst->getLVals().size(); i++) {
      auto lVal = tupleUnpackAssignAst->getLVals()[i];
      auto lValSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(lVal->getSymbol());
      auto gepIndices = std::vector<mlir::Value>{
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};

      auto elementPtr = builder->create<mlir::LLVM::GEPOp>(
          loc, mlir::LLVM::LLVMPointerType::get(builder->getContext()),
          getMLIRType(tupleTypeSymbol), valueAddr, gepIndices);
      if (isTypeArray(lValSymbol->getType())) {
        freeArray(lValSymbol->getType(), lValSymbol->value);
      } else if (isTypeVector(lValSymbol->getType())) {
        freeVector(lValSymbol->getType(), lValSymbol->value);
      }
      copyValue(lValSymbol->getType(), elementPtr, lValSymbol->value);
      castIfNeeded(lValSymbol->value, tupleTypeSymbol->getResolvedTypes()[i],
                   lValSymbol->getType());
    }
    return {};
  }
  if (auto tupleElementAssign = std::dynamic_pointer_cast<ast::statements::TupleElementAssignAst>(
          ctx->getLVal())) { // check if tuple assign
    auto tupleTy = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(variableSymbol->getType());
    auto sTy = getMLIRType(tupleTy);
    auto gepIndices = std::vector<mlir::Value>{
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(),
                                                tupleElementAssign->getFieldIndex() - 1)};
    auto elementAddr = builder->create<mlir::LLVM::GEPOp>(
        loc, mlir::LLVM::LLVMPointerType::get(builder->getContext()), sTy, variableSymbol->value,
        gepIndices);
    if (isTypeArray(variableSymbol->getType())) {
      freeArray(variableSymbol->getType(), variableSymbol->value);
    } else if (isTypeVector(variableSymbol->getType())) {
      freeVector(variableSymbol->getType(), variableSymbol->value);
    }
    copyValue(type, valueAddr, elementAddr);
    castIfNeeded(elementAddr, ctx->getExpr()->getInferredSymbolType(),
                 tupleTy->getResolvedTypes()[tupleElementAssign->getFieldIndex() - 1]);
  }
  if (const auto structElementAssign =
          std::dynamic_pointer_cast<ast::statements::StructElementAssignAst>(
              ctx->getLVal())) { // check if tuple assign
    const auto structTy =
        std::dynamic_pointer_cast<symTable::StructTypeSymbol>(variableSymbol->getType());
    auto sTy = getMLIRType(structTy);
    auto gepIndices = std::vector<mlir::Value>{
        builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
        builder->create<mlir::LLVM::ConstantOp>(
            loc, builder->getI32Type(),
            structTy->getIdx(structElementAssign->getElementName()) - 1)};
    auto elementAddr = builder->create<mlir::LLVM::GEPOp>(
        loc, mlir::LLVM::LLVMPointerType::get(builder->getContext()), sTy, variableSymbol->value,
        gepIndices);
    if (isTypeArray(variableSymbol->getType())) {
      freeArray(variableSymbol->getType(), variableSymbol->value);
    } else if (isTypeVector(variableSymbol->getType())) {
      freeVector(variableSymbol->getType(), variableSymbol->value);
    }
    copyValue(type, valueAddr, elementAddr);
    castIfNeeded(elementAddr, ctx->getExpr()->getInferredSymbolType(),
                 structTy->getResolvedType(structElementAssign->getElementName()));
  } else {
    if (isTypeArray(variableSymbol->getType())) {
      freeArray(variableSymbol->getType(), variableSymbol->value);
    } else if (isTypeVector(variableSymbol->getType())) {
      freeVector(variableSymbol->getType(), variableSymbol->value);
    }
    if (auto vectorType =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(variableSymbol->getType())) {
      auto newVectorAddr = createVectorValue(vectorType, type, valueAddr);
      copyValue(vectorType, newVectorAddr, variableSymbol->value);
      freeVector(vectorType, newVectorAddr);
      return {};
    }
    copyValue(type, valueAddr, variableSymbol->value);
    arraySizeValidation(variableSymbol, type, variableSymbol->value);
    castIfNeeded(variableSymbol->value, ctx->getExpr()->getInferredSymbolType(),
                 variableSymbol->getType());
  }
  return {};
}

} // namespace gazprea::backend
