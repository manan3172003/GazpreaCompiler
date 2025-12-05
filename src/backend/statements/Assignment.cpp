#include "symTable/ArrayTypeSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"

#include <backend/Backend.h>

namespace gazprea::backend {

std::any Backend::visitAssignment(std::shared_ptr<ast::statements::AssignmentAst> ctx) {
  visit(ctx->getExpr());
  auto [type, valueAddr] = popElementFromStack(ctx->getExpr());
  auto variableSymbol =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getLVal()->getSymbol());
  if (auto tupleUnpackAssignAst = std::dynamic_pointer_cast<ast::statements::TupleUnpackAssignAst>(
          ctx->getLVal())) { // Tuple Unpack Assignment
    auto tupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        ctx->getExpr()->getInferredSymbolType());
    for (size_t i = 0; i < tupleUnpackAssignAst->getLVals().size(); i++) {
      auto lVal = tupleUnpackAssignAst->getLVals()[i];
      visit(lVal);
      auto lValSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(lVal->getSymbol());
      auto gepIndices = std::vector<mlir::Value>{
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
          builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), i)};

      auto elementPtr = builder->create<mlir::LLVM::GEPOp>(
          loc, mlir::LLVM::LLVMPointerType::get(builder->getContext()),
          getMLIRType(tupleTypeSymbol), valueAddr, gepIndices);
      const auto destinationPtr = lVal->getEvaluatedAddr();
      freeAllocatedMemory(lVal->getAssignSymbolType(), destinationPtr);

      const auto fromType = tupleTypeSymbol->getResolvedTypes()[i];
      const auto castedPtr = castIfNeeded(ctx, elementPtr, fromType, lVal->getAssignSymbolType());
      copyValue(lVal->getAssignSymbolType(), castedPtr, destinationPtr);
      freeAllocatedMemory(lVal->getAssignSymbolType(), castedPtr);
    }
  } else if (auto tupleElementAssign =
                 std::dynamic_pointer_cast<ast::statements::TupleElementAssignAst>(
                     ctx->getLVal())) { // check if tuple assign
    visit(tupleElementAssign);
    const auto elementAddr = tupleElementAssign->getEvaluatedAddr();
    freeAllocatedMemory(tupleElementAssign->getAssignSymbolType(), elementAddr);

    const auto fromType = ctx->getExpr()->getInferredSymbolType();
    valueAddr = castIfNeeded(ctx, valueAddr, fromType, tupleElementAssign->getAssignSymbolType());
    copyValue(tupleElementAssign->getAssignSymbolType(), valueAddr, elementAddr);
    freeAllocatedMemory(tupleElementAssign->getAssignSymbolType(), valueAddr);

  } else if (const auto structElementAssign =
                 std::dynamic_pointer_cast<ast::statements::StructElementAssignAst>(
                     ctx->getLVal())) { // check if struct assign
    visit(structElementAssign);
    const auto elementAddr = structElementAssign->getEvaluatedAddr();
    freeAllocatedMemory(structElementAssign->getAssignSymbolType(), elementAddr);

    const auto fromType = ctx->getExpr()->getInferredSymbolType();
    valueAddr = castIfNeeded(ctx, valueAddr, fromType, structElementAssign->getAssignSymbolType());
    copyValue(structElementAssign->getAssignSymbolType(), valueAddr, elementAddr);
    freeAllocatedMemory(structElementAssign->getAssignSymbolType(), valueAddr);

  } else if (const auto arrayElementAssign =
                 std::dynamic_pointer_cast<ast::statements::ArrayElementAssignAst>(
                     ctx->getLVal())) {
    visit(arrayElementAssign);
    if (arrayElementAssign->getElementIndex()->getNodeType() == ast::NodeType::SingularIndexExpr) {
      const auto elementAddr = arrayElementAssign->getEvaluatedAddr();
      freeAllocatedMemory(arrayElementAssign->getAssignSymbolType(), elementAddr);

      const auto fromType = ctx->getExpr()->getInferredSymbolType();
      valueAddr = castIfNeeded(ctx, valueAddr, fromType, arrayElementAssign->getAssignSymbolType());
      copyValue(arrayElementAssign->getAssignSymbolType(), valueAddr, elementAddr);
      freeAllocatedMemory(arrayElementAssign->getAssignSymbolType(), valueAddr);

    } else if (arrayElementAssign->getElementIndex()->getNodeType() ==
               ast::NodeType::RangedIndexExpr) {
      auto sliceStructPtr = arrayElementAssign->getEvaluatedAddr();
      auto sliceMlirType =
          getMLIRType(arrayElementAssign->getArrayInstance()->getAssignSymbolType());
      auto sliceSizeAddr = getArraySizeAddr(*builder, loc, sliceMlirType, sliceStructPtr);
      auto sliceDataAddr = getArrayDataAddr(*builder, loc, sliceMlirType, sliceStructPtr);

      mlir::Value sliceSize =
          builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sliceSizeAddr).getResult();
      mlir::Value sliceDataPtr =
          builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), sliceDataAddr).getResult();

      auto rhsArrayMlirType = getMLIRType(type);
      auto newAddr =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), rhsArrayMlirType, constOne());
      copyValue(type, valueAddr, newAddr);

      auto lhsDeclaredType = arrayElementAssign->getArrayInstance()->getAssignSymbolType();
      auto [finalAddr, needToFree] =
          castStructIfNeeded(sliceStructPtr, lhsDeclaredType, newAddr, type);
      auto lhsArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhsDeclaredType);
      auto elementType = lhsArrayType->getType();
      copyArrayElementsToSlice(finalAddr, lhsArrayType, sliceDataPtr, elementType, sliceSize);
      freeArray(arrayElementAssign->getArrayInstance()->getAssignSymbolType(), finalAddr);
    }
  } else if (const auto identifierLeft =
                 std::dynamic_pointer_cast<ast::statements::IdentifierLeftAst>(ctx->getLVal())) {
    visit(identifierLeft);
    freeAllocatedMemory(identifierLeft->getAssignSymbolType(), identifierLeft->getEvaluatedAddr());
    if (auto vectorType =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(variableSymbol->getType())) {
      auto newVectorAddr = createVectorValue(vectorType, type, valueAddr);
      copyValue(vectorType, newVectorAddr, variableSymbol->value);
      freeVector(vectorType, newVectorAddr);
      return {};
    }
    const auto fromType = ctx->getExpr()->getInferredSymbolType();
    valueAddr = castIfNeeded(ctx, valueAddr, fromType, identifierLeft->getAssignSymbolType());
    copyValue(identifierLeft->getAssignSymbolType(), valueAddr, identifierLeft->getEvaluatedAddr());
    freeAllocatedMemory(identifierLeft->getAssignSymbolType(), valueAddr);
  }
  return {};
}

} // namespace gazprea::backend
