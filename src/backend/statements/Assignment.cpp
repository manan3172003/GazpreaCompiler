#include "symTable/ArrayTypeSymbol.h"
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
      if (isTypeArray(lValSymbol->getType())) {
        freeArray(lValSymbol->getType(), destinationPtr);
      } else if (isTypeVector(lValSymbol->getType())) {
        freeVector(lValSymbol->getType(), destinationPtr);
      }
      copyValue(lValSymbol->getType(), elementPtr, destinationPtr);
      castIfNeeded(ctx, destinationPtr, tupleTypeSymbol->getResolvedTypes()[i],
                   lVal->getAssignSymbolType());
    }
  } else if (auto tupleElementAssign =
                 std::dynamic_pointer_cast<ast::statements::TupleElementAssignAst>(
                     ctx->getLVal())) { // check if tuple assign
    visit(tupleElementAssign);
    auto elementAddr = tupleElementAssign->getEvaluatedAddr();
    auto tupleTy = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(variableSymbol->getType());
    if (isTypeArray(variableSymbol->getType())) {
      freeArray(variableSymbol->getType(), variableSymbol->value);
    } else if (isTypeVector(variableSymbol->getType())) {
      freeVector(variableSymbol->getType(), variableSymbol->value);
    }
    valueAddr = castIfNeeded(ctx, valueAddr, ctx->getExpr()->getInferredSymbolType(),
                             tupleTy->getResolvedTypes()[tupleElementAssign->getFieldIndex() - 1]);
    copyValue(type, valueAddr, elementAddr);
    castIfNeeded(ctx, elementAddr, ctx->getExpr()->getInferredSymbolType(),
                 tupleTy->getResolvedTypes()[tupleElementAssign->getFieldIndex() - 1]);
  } else if (const auto structElementAssign =
                 std::dynamic_pointer_cast<ast::statements::StructElementAssignAst>(
                     ctx->getLVal())) { // check if struct assign
    visit(structElementAssign);
    const auto structTy =
        std::dynamic_pointer_cast<symTable::StructTypeSymbol>(variableSymbol->getType());
    auto elementAddr = structElementAssign->getEvaluatedAddr();
    if (isTypeArray(variableSymbol->getType())) {
      freeArray(variableSymbol->getType(), variableSymbol->value);
    } else if (isTypeVector(variableSymbol->getType())) {
      freeVector(variableSymbol->getType(), variableSymbol->value);
    }
    valueAddr = castIfNeeded(ctx, valueAddr, ctx->getExpr()->getInferredSymbolType(),
                             structTy->getResolvedType(structElementAssign->getElementName()));
    copyValue(type, valueAddr, elementAddr);
    castIfNeeded(ctx, elementAddr, ctx->getExpr()->getInferredSymbolType(),
                 structTy->getResolvedType(structElementAssign->getElementName()));
  } else if (const auto arrayElementAssign =
                 std::dynamic_pointer_cast<ast::statements::ArrayElementAssignAst>(
                     ctx->getLVal())) {
    visit(arrayElementAssign);
    if (arrayElementAssign->getElementIndex()->getNodeType() == ast::NodeType::SingularIndexExpr) {
      const auto elementAddr = arrayElementAssign->getEvaluatedAddr();
      // TODO: not sure if this is correct
      if (isTypeArray(arrayElementAssign->getAssignSymbolType())) {
        freeArray(arrayElementAssign->getAssignSymbolType(), elementAddr);
      } else if (isTypeVector(arrayElementAssign->getAssignSymbolType())) {
        freeVector(arrayElementAssign->getAssignSymbolType(), elementAddr);
      }
      copyValue(type, valueAddr, elementAddr);
      castIfNeeded(ctx, elementAddr, ctx->getExpr()->getInferredSymbolType(),
                   arrayElementAssign->getAssignSymbolType());
    } else if (arrayElementAssign->getElementIndex()->getNodeType() ==
               ast::NodeType::RangedIndexExpr) {
      // TODO: Dont assume RHS will always be array
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
      arraySizeValidationForArrayStructs(sliceStructPtr, lhsDeclaredType, newAddr, type);
      castIfNeeded(ctx, newAddr, type,
                   arrayElementAssign->getArrayInstance()->getAssignSymbolType());

      auto lhsArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lhsDeclaredType);
      auto elementType = lhsArrayType->getType();

      copyArrayElementsToSlice(newAddr, type, sliceDataPtr, elementType, sliceSize);
      freeArray(arrayElementAssign->getArrayInstance()->getAssignSymbolType(), newAddr);
    }
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
    variableSymbol->value =
        castIfNeeded(ctx, variableSymbol->value, ctx->getExpr()->getInferredSymbolType(),
                     variableSymbol->getType());
  }
  return {};
}

} // namespace gazprea::backend
