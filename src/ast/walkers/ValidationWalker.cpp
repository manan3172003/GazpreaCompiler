#include "CompileTimeExceptions.h"
#include "ast/types/ArrayTypeAst.h"
#include "ast/types/BooleanTypeAst.h"
#include "ast/types/CharacterTypeAst.h"
#include "ast/types/RealTypeAst.h"
#include "ast/types/VectorTypeAst.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/EmptyArrayTypeSymbol.h"
#include "symTable/MethodSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"
#include "utils/ValidationUtils.h"
#include <ast/walkers/ValidationWalker.h>

namespace gazprea::ast::walkers {

std::any ValidationWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  bool visitedMain = false;
  for (const auto &child : ctx->children) {
    if (child->getNodeType() == NodeType::Procedure &&
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(child)->getProto()->getName() == "main")
      visitedMain = true;
    visit(child);
  }
  if (not visitedMain)
    throw MainError(ctx->getLineNumber(), "Main procedure not found");
  return {};
}
std::any ValidationWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  visit(ctx->getLVal());
  ensureArrayLiteralType(ctx->getExpr(), ctx->getLVal()->getAssignSymbolType());
  inAssignment = true;
  visit(ctx->getExpr());
  inAssignment = false;
  if (!ctx->getExpr()->getInferredSymbolType()) {
    ensureArrayLiteralType(ctx->getExpr(), ctx->getLVal()->getAssignSymbolType());
    inAssignment = true;
    visit(ctx->getExpr());
    inAssignment = false;
  }

  const auto exprTypeSymbol = ctx->getExpr()->getInferredSymbolType();
  if (ctx->getLVal()->getNodeType() == NodeType::IdentifierLeft) {
    const auto idAssignStat =
        std::dynamic_pointer_cast<statements::IdentifierLeftAst>(ctx->getLVal());
    validateVariableAssignmentTypes(idAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::TupleElementAssign) {
    const auto tupleElementAssignStat =
        std::dynamic_pointer_cast<statements::TupleElementAssignAst>(ctx->getLVal());
    validateTupleElementAssignmentTypes(tupleElementAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::StructElementAssign) {
    const auto structElementAssignStat =
        std::dynamic_pointer_cast<statements::StructElementAssignAst>(ctx->getLVal());
    validateStructElementAssignmentTypes(structElementAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::TupleUnpackAssign) {
    const auto tupleUnpackAssignStat =
        std::dynamic_pointer_cast<statements::TupleUnpackAssignAst>(ctx->getLVal());
    validateTupleUnpackAssignmentTypes(tupleUnpackAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::ArrayElementAssign) {
    const auto arrayElementAssignStat =
        std::dynamic_pointer_cast<statements::ArrayElementAssignAst>(ctx->getLVal());
    validateArrayElementAssignmentTypes(arrayElementAssignStat, exprTypeSymbol);
  }
  return {};
}
std::any ValidationWalker::visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) {
  std::shared_ptr<symTable::Type> declarationType = nullptr;
  if (ctx->getType()) {
    visit(ctx->getType());
    declarationType = std::dynamic_pointer_cast<symTable::Type>(ctx->getType()->getSymbol());
    ensureArrayLiteralType(ctx->getExpr(), declarationType);
  }
  const auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());

  // We're going to have an expression since we'll set defaults in AstBuilder
  inAssignment = true;
  visit(ctx->getExpr());
  inAssignment = false;
  if (not ctx->getType()) {
    // setting inferred type here
    variableSymbol->setType(ctx->getExpr()->getInferredSymbolType());
    ctx->setType(ctx->getExpr()->getInferredDataType());
    ctx->getType()->setSymbol(
        std::dynamic_pointer_cast<symTable::Symbol>(ctx->getExpr()->getInferredSymbolType()));
    declarationType = ctx->getExpr()->getInferredSymbolType();
  } else if (!declarationType && ctx->getType()) {
    declarationType = std::dynamic_pointer_cast<symTable::Type>(ctx->getType()->getSymbol());
  }
  // type check
  ensureArrayLiteralType(ctx->getExpr(), declarationType);
  if (!ctx->getExpr()->getInferredSymbolType()) {
    ensureArrayLiteralType(ctx->getExpr(), declarationType);
    inAssignment = true;
    visit(ctx->getExpr());
    inAssignment = false;
  }
  const auto expressionType = ctx->getExpr()->getInferredSymbolType();
  if (not typesMatch(declarationType, expressionType))
    throw TypeError(ctx->getLineNumber(), "Type mismatch");

  // do not need to check qualifier
  // var can be assigned to const, const can be assigned to var

  // Infer vector sizes from RHS expression
  if (isOfSymbolType(declarationType, "vector")) {
    auto vectorDeclarationType =
        std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(declarationType);
    if (vectorDeclarationType) {
      if (const auto literal =
              std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(ctx->getExpr())) {
      }
      inferVectorSize(vectorDeclarationType, ctx->getExpr());
      variableSymbol->setType(vectorDeclarationType);
    }
  }

  if (isOfSymbolType(declarationType, "array") || isOfSymbolType(declarationType, "vector")) {
    const auto rhsType = ctx->getExpr()->getInferredSymbolType();
    if (std::dynamic_pointer_cast<symTable::EmptyArrayTypeSymbol>(rhsType)) {
      auto throwIfInferredArray = [&](const std::shared_ptr<types::ArrayTypeAst> &arrayTypeAst) {
        if (!arrayTypeAst)
          return;
      };

      if (ctx->getType()) {
        if (ctx->getType()->getNodeType() == NodeType::ArrayType) {
          auto arrayTypeAst = std::dynamic_pointer_cast<types::ArrayTypeAst>(ctx->getType());
          throwIfInferredArray(arrayTypeAst);
        } else if (ctx->getType()->getNodeType() == NodeType::VectorType) {
          const auto vectorTypeAst =
              std::dynamic_pointer_cast<types::VectorTypeAst>(ctx->getType());
          if (vectorTypeAst) {
            auto elementArrayType =
                std::dynamic_pointer_cast<types::ArrayTypeAst>(vectorTypeAst->getElementType());
            throwIfInferredArray(elementArrayType);
          }
        }
      }

      if (auto targetArrayType =
              std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(declarationType)) {
        ctx->getExpr()->setInferredSymbolType(targetArrayType);
      } else if (auto targetVectorType =
                     std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(declarationType)) {
        auto newArrayType = std::make_shared<symTable::ArrayTypeSymbol>("array");
        newArrayType->setType(targetVectorType->getType());
        ctx->getExpr()->setInferredSymbolType(newArrayType);
      }
    }
  }

  // Check if global declaration - must have only literals
  if (ctx->getScope() && ctx->getScope()->getScopeType() == symTable::ScopeType::Global) {
    if (!isLiteralExpression(ctx->getExpr())) {
      throw GlobalError(ctx->getLineNumber(), "Global declarations can only have literal values");
    }
    if (ctx->getQualifier() == Qualifier::Var)
      throw TypeError(ctx->getLineNumber(), "Cannot declare a variable as var in Global Scope");
  }
  return {};
}
std::any ValidationWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  bool visitedAllDeclarations = false;
  for (const auto &child : ctx->getChildren()) {
    if (not(std::dynamic_pointer_cast<statements::DeclarationAst>(child) ||
            std::dynamic_pointer_cast<statements::StructDeclarationAst>(child)))
      visitedAllDeclarations = true;
    else {
      if (visitedAllDeclarations)
        throw StatementError(child->getLineNumber(),
                             "Declarations are only allowed on the top of the block");
    }
    visit(child);
  }
  return {};
}
std::any ValidationWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  auto leftExpr = ctx->getLeft();
  auto rightExpr = ctx->getRight();
  visitExpression(leftExpr);
  visitExpression(rightExpr);

  auto leftType = leftExpr->getInferredSymbolType();
  auto rightType = rightExpr->getInferredSymbolType();
  auto leftDataType = leftExpr->getInferredDataType();
  auto rightDataType = rightExpr->getInferredDataType();
  auto op = ctx->getBinaryOpType();

  bool is_arithmetic =
      op == expressions::BinaryOpType::ADD || op == expressions::BinaryOpType::SUBTRACT ||
      op == expressions::BinaryOpType::MULTIPLY || op == expressions::BinaryOpType::DIVIDE;

  // Type promote Integer to Real if either of the operands is a real
  if (isOfSymbolType(leftType, "integer") && isOfSymbolType(rightType, "integer")) {
    ctx->setInferredSymbolType(leftType);
    ctx->setInferredDataType(leftDataType);
  } else if (isOfSymbolType(leftType, "integer") && isOfSymbolType(rightType, "real")) {
    promoteIfNeeded(ctx, leftType, rightType, rightDataType);
  } else if (isOfSymbolType(leftType, "real") && isOfSymbolType(rightType, "integer")) {
    promoteIfNeeded(ctx, rightType, leftType, leftDataType);
  } else if (isOfSymbolType(leftType, "real") && isOfSymbolType(rightType, "real")) {
    ctx->setInferredSymbolType(leftType);
    ctx->setInferredDataType(leftDataType);
  }
  // stride by operation
  else if (op == expressions::BinaryOpType::BY) {
    if (not isCollection(leftType) || not isOfSymbolType(rightType, "integer")) {
      throw TypeError(ctx->getLineNumber(), "Invalid types for stride by operation");
    }
    ctx->setInferredDataType(leftDataType);
    ctx->setInferredSymbolType(leftType);
    return {};
  }
  // handle arrays and vectors for arithmetic operations
  else if (is_arithmetic && isOfSymbolType(leftType, "array") && typesMatch(leftType, rightType)) {
    // Promote to real if either operand is real type
    if (isArrayRealType(leftType) || isArrayRealType(rightType)) {
      auto arrayDataType = std::dynamic_pointer_cast<types::ArrayTypeAst>(leftDataType);

      // Collect array nesting levels
      std::vector<std::shared_ptr<types::ArrayTypeAst>> arrayLevels;
      auto currentType = arrayDataType;
      while (currentType) {
        arrayLevels.push_back(currentType);
        currentType = std::dynamic_pointer_cast<types::ArrayTypeAst>(currentType->getType());
      }

      // Build array type with real as base, preserving nesting structure and sizes
      auto realDataType = std::make_shared<types::RealTypeAst>(ctx->token);
      std::shared_ptr<types::DataTypeAst> resultType = realDataType;

      // Wrap in array types from innermost to outermost, preserving sizes
      for (auto it = arrayLevels.rbegin(); it != arrayLevels.rend(); ++it) {
        auto newArrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
        newArrayType->setType(resultType);
        for (const auto &size : (*it)->getSizes()) {
          newArrayType->pushSize(size);
        }
        resultType = newArrayType;
      }

      ctx->setInferredDataType(resultType);
      ctx->setInferredSymbolType(resolvedInferredType(resultType));
    } else {
      ctx->setInferredDataType(leftDataType);
      ctx->setInferredSymbolType(leftType);
    }
  } else if (is_arithmetic && isOfSymbolType(rightType, "array") &&
             typesMatch(rightType, leftType)) {
    // Promote to real if either operand is real type
    if (isArrayRealType(leftType) || isArrayRealType(rightType)) {
      auto arrayDataType = std::dynamic_pointer_cast<types::ArrayTypeAst>(rightDataType);

      // Collect array nesting levels
      std::vector<std::shared_ptr<types::ArrayTypeAst>> arrayLevels;
      auto currentType = arrayDataType;
      while (currentType) {
        arrayLevels.push_back(currentType);
        currentType = std::dynamic_pointer_cast<types::ArrayTypeAst>(currentType->getType());
      }

      // Build array type with real as base, preserving nesting structure and sizes
      auto realDataType = std::make_shared<types::RealTypeAst>(ctx->token);
      std::shared_ptr<types::DataTypeAst> resultType = realDataType;

      // Wrap in array types from innermost to outermost, preserving sizes
      for (auto it = arrayLevels.rbegin(); it != arrayLevels.rend(); ++it) {
        auto newArrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
        newArrayType->setType(resultType);
        for (const auto &size : (*it)->getSizes()) {
          newArrayType->pushSize(size);
        }
        resultType = newArrayType;
      }

      ctx->setInferredDataType(resultType);
      ctx->setInferredSymbolType(resolvedInferredType(resultType));
    } else {
      ctx->setInferredDataType(rightDataType);
      ctx->setInferredSymbolType(rightType);
    }
  }
  // handle vectors for arithmetic operations
  else if (is_arithmetic && isOfSymbolType(leftType, "vector") && typesMatch(leftType, rightType)) {
    // Promote to real if either operand is real type
    if (isVectorRealType(leftType) || isVectorRealType(rightType)) {
      auto vectorDataType = std::dynamic_pointer_cast<types::VectorTypeAst>(leftDataType);

      // Build vector type with real as base
      auto realDataType = std::make_shared<types::RealTypeAst>(ctx->token);
      std::shared_ptr<types::DataTypeAst> elementType = realDataType;

      // Check if the element type is an array (vector of arrays)
      if (vectorDataType && vectorDataType->getElementType()) {
        if (auto arrayElementType =
                std::dynamic_pointer_cast<types::ArrayTypeAst>(vectorDataType->getElementType())) {
          // Collect array nesting levels
          std::vector<std::shared_ptr<types::ArrayTypeAst>> arrayLevels;
          auto currentType = arrayElementType;
          while (currentType) {
            arrayLevels.push_back(currentType);
            currentType = std::dynamic_pointer_cast<types::ArrayTypeAst>(currentType->getType());
          }

          // Wrap in array types from innermost to outermost, preserving sizes
          for (auto it = arrayLevels.rbegin(); it != arrayLevels.rend(); ++it) {
            auto newArrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
            newArrayType->setType(elementType);
            for (const auto &size : (*it)->getSizes()) {
              newArrayType->pushSize(size);
            }
            elementType = newArrayType;
          }
        }
      }

      auto resultVectorType = std::make_shared<types::VectorTypeAst>(ctx->token);
      resultVectorType->setElementType(elementType);

      ctx->setInferredDataType(resultVectorType);
      ctx->setInferredSymbolType(resolvedInferredType(resultVectorType));
    } else {
      ctx->setInferredDataType(leftDataType);
      ctx->setInferredSymbolType(leftType);
    }
  } else if (is_arithmetic && isOfSymbolType(rightType, "vector") &&
             typesMatch(rightType, leftType)) {
    // Promote to real if either operand is real type
    if (isVectorRealType(leftType) || isVectorRealType(rightType)) {
      auto vectorDataType = std::dynamic_pointer_cast<types::VectorTypeAst>(rightDataType);

      // Build vector type with real as base
      auto realDataType = std::make_shared<types::RealTypeAst>(ctx->token);
      std::shared_ptr<types::DataTypeAst> elementType = realDataType;

      // Check if the element type is an array (vector of arrays)
      if (vectorDataType && vectorDataType->getElementType()) {
        if (auto arrayElementType =
                std::dynamic_pointer_cast<types::ArrayTypeAst>(vectorDataType->getElementType())) {
          // Collect array nesting levels
          std::vector<std::shared_ptr<types::ArrayTypeAst>> arrayLevels;
          auto currentType = arrayElementType;
          while (currentType) {
            arrayLevels.push_back(currentType);
            currentType = std::dynamic_pointer_cast<types::ArrayTypeAst>(currentType->getType());
          }

          // Wrap in array types from innermost to outermost, preserving sizes
          for (auto it = arrayLevels.rbegin(); it != arrayLevels.rend(); ++it) {
            auto newArrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
            newArrayType->setType(elementType);
            for (const auto &size : (*it)->getSizes()) {
              newArrayType->pushSize(size);
            }
            elementType = newArrayType;
          }
        }
      }

      auto resultVectorType = std::make_shared<types::VectorTypeAst>(ctx->token);
      resultVectorType->setElementType(elementType);

      ctx->setInferredDataType(resultVectorType);
      ctx->setInferredSymbolType(resolvedInferredType(resultVectorType));
    } else {
      ctx->setInferredDataType(rightDataType);
      ctx->setInferredSymbolType(rightType);
    }
  }
  // Handle scalar + vector and vector + scalar for arithmetic operations
  else if (is_arithmetic && ((isScalar(leftType) && isOfSymbolType(rightType, "vector")) ||
                             (isOfSymbolType(leftType, "vector") && isScalar(rightType)))) {
    std::shared_ptr<symTable::VectorTypeSymbol> vectorTypeSym;
    std::shared_ptr<symTable::Type> scalarType;
    std::shared_ptr<types::DataTypeAst> scalarDataType;

    if (isOfSymbolType(rightType, "vector")) {
      vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(rightType);
      scalarType = leftType;
      scalarDataType = leftDataType;
    } else {
      vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftType);
      scalarType = rightType;
      scalarDataType = rightDataType;
    }

    if (!isOfSymbolType(scalarType, "integer") && !isOfSymbolType(scalarType, "real")) {
      throw TypeError(ctx->getLineNumber(), "Scalar operand must be numeric for vector operation");
    }

    auto vectorElementType = vectorTypeSym->getType();
    auto vectorElementDataType =
        std::dynamic_pointer_cast<types::VectorTypeAst>(
            (isOfSymbolType(rightType, "vector") ? rightDataType : leftDataType))
            ->getElementType();

    if (!typesMatch(scalarType, vectorElementType) &&
        !(isOfSymbolType(scalarType, "integer") && isOfSymbolType(vectorElementType, "real")) &&
        !(isOfSymbolType(scalarType, "real") && isOfSymbolType(vectorElementType, "integer"))) {
      throw TypeError(ctx->getLineNumber(), "Scalar type incompatible with vector element type");
    }

    // Promote to real if either scalar or vector element is real
    if ((isOfSymbolType(scalarType, "integer") && isOfSymbolType(vectorElementType, "real")) ||
        (isOfSymbolType(scalarType, "real") && isOfSymbolType(vectorElementType, "integer")) ||
        (isOfSymbolType(scalarType, "real") && isOfSymbolType(vectorElementType, "real"))) {
      auto realDataType = std::make_shared<types::RealTypeAst>(ctx->token);
      std::shared_ptr<types::DataTypeAst> newVectorElementType = realDataType;

      // If vector element was an array, rebuild the array type with real base
      if (auto arrayElementType =
              std::dynamic_pointer_cast<types::ArrayTypeAst>(vectorElementDataType)) {
        std::vector<std::shared_ptr<types::ArrayTypeAst>> arrayLevels;
        auto currentType = arrayElementType;
        while (currentType) {
          arrayLevels.push_back(currentType);
          currentType = std::dynamic_pointer_cast<types::ArrayTypeAst>(currentType->getType());
        }

        std::shared_ptr<types::DataTypeAst> resultType = realDataType;
        for (auto it = arrayLevels.rbegin(); it != arrayLevels.rend(); ++it) {
          auto newArrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
          newArrayType->setType(resultType);
          for (const auto &size : (*it)->getSizes()) {
            newArrayType->pushSize(size);
          }
          resultType = newArrayType;
        }
        newVectorElementType = resultType;
      }

      auto resultVectorTypeAst = std::make_shared<types::VectorTypeAst>(ctx->token);
      resultVectorTypeAst->setElementType(newVectorElementType);
      ctx->setInferredDataType(resultVectorTypeAst);
      ctx->setInferredSymbolType(resolvedInferredType(resultVectorTypeAst));
    } else {
      // No promotion needed, keep original vector type
      ctx->setInferredDataType(isOfSymbolType(rightType, "vector") ? rightDataType : leftDataType);
      ctx->setInferredSymbolType(vectorTypeSym);
    }
  } else {
    if (op != expressions::BinaryOpType::DPIPE) {
      if (not typesMatch(leftType, rightType)) {
        throw TypeError(ctx->getLineNumber(), "Binary operation: Type mismatch");
      }
    }
  }

  if (op != expressions::BinaryOpType::DPIPE && op != expressions::BinaryOpType::DMUL &&
      !isValidOp(leftType, op))
    throw TypeError(ctx->getLineNumber(), "Invalid binary operation");

  if (ctx->getBinaryOpType() == expressions::BinaryOpType::DMUL) {
    if (not areBothNumeric(ctx->getLeft(), ctx->getRight()))
      throw TypeError(ctx->getLineNumber(), "Cannot perform dot product on non-numeric types");

    // Handle both vectors and arrays
    std::shared_ptr<types::DataTypeAst> elementType;
    bool isVector = false;
    bool isMultiDimensional = false;

    if (auto originalVectorDataType =
            std::dynamic_pointer_cast<types::VectorTypeAst>(leftDataType)) {
      isVector = true;
      // Get the element type of the vector
      elementType = originalVectorDataType->getElementType();

      // Check if element is an array (making this a multi-dimensional vector)
      if (auto innerArray = std::dynamic_pointer_cast<types::ArrayTypeAst>(elementType)) {
        isMultiDimensional = true;

        // Find the innermost scalar type for potential real promotion
        auto scalarType = innerArray->getType();
        while (auto nestedArray = std::dynamic_pointer_cast<types::ArrayTypeAst>(scalarType)) {
          scalarType = nestedArray->getType();
        }

        // Promote to real if either operand is real type
        if (isArrayRealType(leftType) || isArrayRealType(rightType)) {
          scalarType = std::make_shared<types::RealTypeAst>(ctx->token);
        }

        // Reduce by one dimension: the result element type is innerArray->getType()
        // but with potentially promoted scalar type
        if (std::dynamic_pointer_cast<types::ArrayTypeAst>(innerArray->getType())) {
          // Still multi-dimensional after reduction, need to rebuild the type tree
          // For now, just use the inner array type
          elementType = innerArray->getType();
        } else {
          // Was 2D, now becomes 1D with scalar elements
          elementType = scalarType;
        }

        auto vectorDataType = std::make_shared<types::VectorTypeAst>(ctx->token);
        vectorDataType->setElementType(elementType);
        ctx->setInferredDataType(vectorDataType);
        ctx->setInferredSymbolType(resolvedInferredType(vectorDataType));
      } else {
        // 1D vector: result is scalar (element type)
        // Promote to real if needed
        if (isArrayRealType(leftType) || isArrayRealType(rightType)) {
          elementType = std::make_shared<types::RealTypeAst>(ctx->token);
        }
        ctx->setInferredDataType(elementType);
        ctx->setInferredSymbolType(resolvedInferredType(elementType));
      }
    } else if (auto originalDataType =
                   std::dynamic_pointer_cast<types::ArrayTypeAst>(leftDataType)) {
      // Find the innermost element type by traversing all nested arrays
      elementType = originalDataType->getType();
      while (auto innerArray = std::dynamic_pointer_cast<types::ArrayTypeAst>(elementType)) {
        elementType = innerArray->getType();
      }

      // Promote to real if either operand is real type
      if (isArrayRealType(leftType) || isArrayRealType(rightType)) {
        elementType = std::make_shared<types::RealTypeAst>(ctx->token);
      }

      if (std::dynamic_pointer_cast<types::ArrayTypeAst>(originalDataType->getType())) {
        // Multi-dimensional array: reduce by one dimension
        auto arrayDataType = std::make_shared<types::ArrayTypeAst>(ctx->token);
        arrayDataType->setType(elementType);
        if (!arrayDataType->getSizes().empty()) {
          for (int i = 0; i < originalDataType->getSizes().size() - 1; i++) {
            arrayDataType->pushSize(originalDataType->getSizes()[i]);
          }
        }
        ctx->setInferredDataType(arrayDataType);
        ctx->setInferredSymbolType(resolvedInferredType(arrayDataType));
      } else {
        // 1D array: result is scalar (element type)
        ctx->setInferredDataType(elementType);
        ctx->setInferredSymbolType(resolvedInferredType(elementType));
      }
    }
  }
  // If the operation is == or != or operands are bool set expression type to
  // boolean
  if (ctx->getBinaryOpType() == expressions::BinaryOpType::EQUAL ||
      ctx->getBinaryOpType() == expressions::BinaryOpType::NOT_EQUAL ||
      isOfSymbolType(leftType, "boolean")) {
    auto booleanDataType = std::make_shared<types::BooleanTypeAst>(ctx->token);
    auto booleanTypeSymbol = resolvedInferredType(booleanDataType);
    ctx->setInferredSymbolType(booleanTypeSymbol);
    ctx->setInferredDataType(booleanDataType);
  }

  if (ctx->getBinaryOpType() == expressions::BinaryOpType::DPIPE) {
    bool leftIsEmpty =
        std::dynamic_pointer_cast<symTable::EmptyArrayTypeSymbol>(leftType) != nullptr;
    bool rightIsEmpty =
        std::dynamic_pointer_cast<symTable::EmptyArrayTypeSymbol>(rightType) != nullptr;

    if (leftIsEmpty && rightIsEmpty) {
      // Both empty: result is empty array
      ctx->setInferredSymbolType(std::make_shared<symTable::EmptyArrayTypeSymbol>("empty_array"));
      ctx->setInferredDataType(leftDataType);
    } else if (leftIsEmpty && !rightIsEmpty) {
      // Left empty, right has type: use right's type
      ctx->setInferredSymbolType(rightType);
      ctx->setInferredDataType(rightDataType);
    } else if (!leftIsEmpty && rightIsEmpty) {
      // Right empty, left has type: use left's type
      ctx->setInferredSymbolType(leftType);
      ctx->setInferredDataType(leftDataType);
    } else if (isOfSymbolType(leftType, "vector") && isOfSymbolType(rightType, "vector")) {
      auto leftVectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(leftType);
      auto rightVectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(rightType);

      if (!leftVectorType || !rightVectorType) {
        throw TypeError(ctx->getLineNumber(),
                        "Concatenation (||) requires both operands to be vectors");
      }

      auto leftElementType = leftVectorType->getType();
      auto rightElementType = rightVectorType->getType();

      if (!typesMatch(leftElementType, rightElementType, true)) {
        if ((isOfSymbolType(leftElementType, "integer") &&
             isOfSymbolType(rightElementType, "real")) ||
            (isOfSymbolType(leftElementType, "real") &&
             isOfSymbolType(rightElementType, "integer"))) {
          // Promotion is handled below
        } else {
          throw TypeError(ctx->getLineNumber(),
                          "Vector element types are incompatible for concatenation");
        }
      }

      // Check for promotion
      if (isOfSymbolType(leftElementType, "integer") && isOfSymbolType(rightElementType, "real")) {
        auto realDataType = std::make_shared<types::RealTypeAst>(ctx->token);
        auto resultVectorTypeAst = std::make_shared<types::VectorTypeAst>(ctx->token);
        resultVectorTypeAst->setElementType(realDataType);
        ctx->setInferredDataType(resultVectorTypeAst);
        ctx->setInferredSymbolType(resolvedInferredType(resultVectorTypeAst));
      } else if (isOfSymbolType(leftElementType, "real") &&
                 isOfSymbolType(rightElementType, "integer")) {
        ctx->setInferredSymbolType(leftType);
        ctx->setInferredDataType(leftDataType);
      } else {
        // Element types match, use left's vector type
        ctx->setInferredSymbolType(leftType);
        ctx->setInferredDataType(leftDataType);
      }
    } else {
      // Both have types: promote to real if either is real
      if (isArrayRealType(leftType) || isArrayRealType(rightType)) {
        auto arrayDataType = leftIsEmpty
                                 ? std::dynamic_pointer_cast<types::ArrayTypeAst>(rightDataType)
                                 : std::dynamic_pointer_cast<types::ArrayTypeAst>(leftDataType);

        // Collect array nesting levels
        std::vector<std::shared_ptr<types::ArrayTypeAst>> arrayLevels;
        auto currentType = arrayDataType;
        while (currentType) {
          arrayLevels.push_back(currentType);
          currentType = std::dynamic_pointer_cast<types::ArrayTypeAst>(currentType->getType());
        }

        // Build array type with real as base, preserving nesting structure and sizes
        auto realDataType = std::make_shared<types::RealTypeAst>(ctx->token);
        std::shared_ptr<types::DataTypeAst> resultType = realDataType;

        // Wrap in array types from innermost to outermost, preserving sizes
        for (auto it = arrayLevels.rbegin(); it != arrayLevels.rend(); ++it) {
          auto newArrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
          newArrayType->setType(resultType);
          for (const auto &size : (*it)->getSizes()) {
            newArrayType->pushSize(size);
          }
          resultType = newArrayType;
        }

        ctx->setInferredDataType(resultType);
        ctx->setInferredSymbolType(resolvedInferredType(resultType));
      } else {
        // Both same type (integer): use left's type
        ctx->setInferredSymbolType(leftType);
        ctx->setInferredDataType(leftDataType);
      }
    }
  }
  // if left expr and right expr in cond is real or int then if the operation is
  // <,>,<=,>= then set expression type to boolean
  if (isComparisonOperator(ctx->getBinaryOpType()) && areBothNumeric(leftExpr, rightExpr)) {
    if (auto arraySymbolType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(leftType)) {
      // Get the array data type
      auto arrayDataType = std::dynamic_pointer_cast<types::ArrayTypeAst>(leftDataType);

      // Collect array nesting levels
      std::vector<std::shared_ptr<types::ArrayTypeAst>> arrayLevels;
      auto currentType = arrayDataType;

      while (currentType) {
        arrayLevels.push_back(currentType);
        currentType = std::dynamic_pointer_cast<types::ArrayTypeAst>(currentType->getType());
      }

      // Build array type with boolean as base, preserving nesting structure and sizes
      auto booleanDataType = std::make_shared<types::BooleanTypeAst>(ctx->token);
      std::shared_ptr<types::DataTypeAst> resultType = booleanDataType;

      // Wrap in array types from innermost to outermost, preserving sizes
      for (auto it = arrayLevels.rbegin(); it != arrayLevels.rend(); ++it) {
        auto newArrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
        newArrayType->setType(resultType);
        // Copy sizes from the original array level
        for (const auto &size : (*it)->getSizes()) {
          newArrayType->pushSize(size);
        }
        resultType = newArrayType;
      }

      auto resultTypeSymbol = resolvedInferredType(resultType);
      ctx->setInferredSymbolType(resultTypeSymbol);
      ctx->setInferredDataType(resultType);
    } else {
      // Non-array case: just set to boolean
      auto booleanDataType = std::make_shared<types::BooleanTypeAst>(ctx->token);
      auto booleanTypeSymbol = resolvedInferredType(booleanDataType);
      ctx->setInferredSymbolType(booleanTypeSymbol);
      ctx->setInferredDataType(booleanDataType);
    }
  }

  return {};
}
std::any ValidationWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  auto curScope = ctx->getScope();
  while (curScope && curScope->getScopeType() != symTable::ScopeType::Loop)
    curScope = curScope->getEnclosingScope();
  if (!curScope)
    throw StatementError(ctx->getLineNumber(), "Break statement must be in loop statement");
  return {};
}
std::any ValidationWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  auto curScope = ctx->getScope();
  while (curScope && curScope->getScopeType() != symTable::ScopeType::Loop)
    curScope = curScope->getEnclosingScope();
  if (!curScope)
    throw StatementError(ctx->getLineNumber(), "Continue statement must be in loop statement");
  return {};
}
std::any ValidationWalker::visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) {
  visit(ctx->getCondition());

  if (!isOfSymbolType(ctx->getCondition()->getInferredSymbolType(), "boolean")) {
    throw TypeError(ctx->getLineNumber(), "If statement condition must be of type boolean");
  }
  visit(ctx->getThenBody());
  if (ctx->getElseBody()) {
    visit(ctx->getElseBody());
  }

  return {};
}
std::any ValidationWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw StatementError(ctx->getLineNumber(), "Input statement not allowed in functions");
  }
  visit(ctx->getLVal());
  const auto targetType = ctx->getLVal()->getAssignSymbolType();
  if (!targetType) {
    throw TypeError(ctx->getLineNumber(), "Input target type is not supported");
  }
  const auto &typeName = targetType->getName();
  if (typeName != "integer" && typeName != "real" && typeName != "character" &&
      typeName != "boolean") {
    throw TypeError(ctx->getLineNumber(),
                    "Input only supports integer, real, character, or boolean types");
  }
  return {};
}
std::any ValidationWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw StatementError(ctx->getLineNumber(), "Output statement not allowed in functions");
  }
  visit(ctx->getExpression());
  const auto typeName = ctx->getExpression()->getInferredSymbolType()->getName();
  if (typeName == "tuple" || typeName == "struct")
    throw TypeError(ctx->getLineNumber(), "Cannot print this datatype");
  return {};
}
std::any ValidationWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  visit(ctx->getProto());
  auto proto = ctx->getProto();
  if (proto->getName() == "main") {
    if (!proto->getParams().empty())
      throw MainError(ctx->getLineNumber(), "Main cannot have any arguments");
    if (not proto->getReturnType())
      throw MainError(ctx->getLineNumber(), "Main needs to return an integer");
    if (proto->getReturnType() &&
        resolvedInferredType(proto->getReturnType())->getName() != "integer")
      throw MainError(ctx->getLineNumber(), "Main needs to return an integer");
  }

  if (ctx->getProto()->getReturnType()) {
    // If procedure has a return type, check that we reach a return statement
    const auto blockAst = std::dynamic_pointer_cast<statements::BlockAst>(ctx->getBody());
    if (blockAst && !hasReturnInMethod(blockAst)) {
      throw ReturnError(ctx->getLineNumber(), "Function must have a return statement");
    }
  }
  if (not ctx->getBody()) {
    const auto methodSymbol = symTab->getGlobalScope()->getSymbol(ctx->getProto()->getName());
    const auto procedureDeclaration =
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(methodSymbol->getDef());
    if (not procedureDeclaration->getBody())
      throw DefinitionError(procedureDeclaration->getLineNumber(),
                            "Procedure does not have a definition");
  }
  if (ctx->getBody())
    visit(ctx->getBody());
  return {};
}
std::any
ValidationWalker::visitProcedureParams(std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  visit(ctx->getParamType());
  return {};
}
std::any ValidationWalker::visitProcedureCall(std::shared_ptr<statements::ProcedureCallAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (not methodSymbol)
    throw CallError(ctx->getLineNumber(), "Call statement used on non-procedure type");

  // need to re-resolve to get defined function and setSymbol to latest symbol with body defined
  methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      symTab->getGlobalScope()->resolveSymbol(methodSymbol->getName()));
  ctx->setSymbol(methodSymbol);

  // Cannot call a function using call
  if (methodSymbol->getScopeType() == symTable::ScopeType::Function) {
    throw CallError(ctx->getLineNumber(), "Call statement used on non-procedure type");
  }

  // Get procedure prototype
  auto procedureDecl = std::dynamic_pointer_cast<prototypes::ProcedureAst>(methodSymbol->getDef());
  auto protoType = procedureDecl->getProto();

  // cannot call main
  if (protoType->getName() == "main")
    throw CallError(ctx->getLineNumber(), "Cannot call main function");

  // Validate arguments against parameters
  const auto params = protoType->getParams();
  const auto args = ctx->getArgs();
  validateArgs(params, args, methodSymbol->getScopeType(), ctx->getLineNumber());

  // Cannot call a procedure inside a function
  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw CallError(ctx->getLineNumber(), "Procedure call inside function");
  }

  return {};
}
std::any ValidationWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  auto curScope = ctx->getScope();

  // Recurse up till we reach MethodSymbol scope
  while (!std::dynamic_pointer_cast<symTable::MethodSymbol>(curScope)) {
    if (curScope->getScopeType() == symTable::ScopeType::Global) {
      throw StatementError(ctx->getLineNumber(), "Invalid return");
    }
    curScope = curScope->getEnclosingScope();
  }
  if (curScope->getScopeType() == symTable::ScopeType::Function && not ctx->getExpr())
    throw TypeError(ctx->getLineNumber(), "Cannot have empty return in a function");

  if (ctx->getExpr())
    visit(ctx->getExpr());

  const auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(curScope);
  const auto methodReturnType = methodSymbol->getReturnType();
  const auto statReturnType = ctx->getExpr() ? ctx->getExpr()->getInferredSymbolType() : nullptr;

  if (not methodReturnType && statReturnType)
    throw TypeError(ctx->getLineNumber(), "Return type mismatch");
  if (methodReturnType && not statReturnType)
    throw TypeError(ctx->getLineNumber(), "Return type mismatch");
  if (methodReturnType && statReturnType && not typesMatch(methodReturnType, statReturnType))
    throw TypeError(ctx->getLineNumber(), "Return type mismatch");
  return {};
}
std::any
ValidationWalker::visitTupleElementAssign(std::shared_ptr<statements::TupleElementAssignAst> ctx) {
  const auto idSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (not idSymbol)
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");
  const auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(idSymbol->getType());
  if (not tupleTypeSymbol)
    throw TypeError(ctx->getLineNumber(), "Identifier symbol is not a tuple type");
  if (ctx->getFieldIndex() == 0 ||
      ctx->getFieldIndex() > static_cast<int>(tupleTypeSymbol->getResolvedTypes().size()))
    throw SizeError(ctx->getLineNumber(), "Invalid tuple index");

  ctx->setAssignDataType(tupleTypeSymbol->getUnresolvedType(ctx->getFieldIndex()));
  ctx->setAssignSymbolType(tupleTypeSymbol->getResolvedType(ctx->getFieldIndex()));
  return {};
}
std::any
ValidationWalker::visitTupleUnpackAssign(std::shared_ptr<statements::TupleUnpackAssignAst> ctx) {
  const std::vector<std::shared_ptr<statements::AssignLeftAst>> lVals = ctx->getLVals();
  for (const auto &lVal : lVals) {
    visit(lVal);
  }
  return {};
}
std::any ValidationWalker::visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) {
  // call helper to validate inferred types
  validateTupleAccessInferredTypes(ctx);

  const auto tupleSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(tupleSymbol->getType());
  const int32_t fieldIndex = ctx->getFieldIndex();
  const auto elementType = tupleTypeSymbol->getResolvedTypes()[fieldIndex - 1];

  std::shared_ptr<types::DataTypeAst> dataType;
  const std::string typeName = elementType->getName();

  if (typeName == "integer")
    dataType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  else if (typeName == "real")
    dataType = std::make_shared<types::RealTypeAst>(ctx->token);
  else if (typeName == "character")
    dataType = std::make_shared<types::CharacterTypeAst>(ctx->token);
  else if (typeName == "boolean")
    dataType = std::make_shared<types::BooleanTypeAst>(ctx->token);
  // TODO: introduce part 2 types
  else
    throw TypeError(ctx->getLineNumber(), "Type mismatch");

  ctx->setInferredSymbolType(elementType);
  ctx->setInferredDataType(dataType);

  return {};
}
std::any ValidationWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  auto tupleType = std::make_shared<types::TupleTypeAst>(ctx->token);
  for (const auto &element : ctx->getElements()) {
    visit(element);
    tupleType->addType(element->getInferredDataType());
  }
  ctx->setInferredSymbolType(resolvedInferredType(tupleType));
  ctx->setInferredDataType(tupleType);
  return {};
}
std::any ValidationWalker::visitStruct(std::shared_ptr<expressions::StructLiteralAst> ctx) {
  const auto typeSymbol = ctx->getScope()->resolveType(ctx->getStructTypeName());
  const auto structTypeSymbol = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(typeSymbol);
  const auto structTypeAst =
      std::dynamic_pointer_cast<types::StructTypeAst>(structTypeSymbol->getDef());

  const auto literalElementValues = ctx->getElements();
  const auto symbolElementTypes = structTypeSymbol->getResolvedTypes();

  // number of arguments must be the same
  if (literalElementValues.size() != symbolElementTypes.size())
    throw TypeError(ctx->getLineNumber(), "Incorrect struct member list");

  for (size_t i = 0; i < literalElementValues.size(); ++i) {
    const auto &literalElementValue = literalElementValues[i];
    const auto &symbolElementType = symbolElementTypes[i];
    visit(literalElementValue);

    if (not typesMatch(symbolElementType, literalElementValue->getInferredSymbolType()))
      throw TypeError(ctx->getLineNumber(), "Incorrect struct member list");

    if (symbolElementType->getName() == "tuple" &&
        literalElementValue->getInferredSymbolType()->getName() == "tuple")
      throw TypeError(ctx->getLineNumber(), "Incorrect struct member list");
  }

  ctx->setInferredSymbolType(structTypeSymbol);
  ctx->setInferredDataType(structTypeAst);
  return AstWalker::visitStruct(ctx);
}
std::any
ValidationWalker::visitStructDeclaration(std::shared_ptr<statements::StructDeclarationAst> ctx) {
  visit(ctx->getType());
  return {};
}
std::any ValidationWalker::visitStructElementAssign(
    std::shared_ptr<statements::StructElementAssignAst> ctx) {
  const auto idSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (not idSymbol)
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");

  const auto structTypeSymbol =
      std::dynamic_pointer_cast<symTable::StructTypeSymbol>(idSymbol->getType());
  if (not structTypeSymbol)
    throw TypeError(ctx->getLineNumber(), "Identifier symbol is not a struct type");

  if (not structTypeSymbol->elementNameExist(ctx->getElementName()))
    throw TypeError(ctx->getLineNumber(), "Invalid struct element");

  ctx->setAssignDataType(structTypeSymbol->getUnresolvedType(ctx->getElementName()));
  ctx->setAssignSymbolType(structTypeSymbol->getResolvedType(ctx->getElementName()));
  return {};
}
std::any ValidationWalker::visitStructAccess(std::shared_ptr<expressions::StructAccessAst> ctx) {
  const auto idSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (not idSymbol)
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");

  const auto structTypeSymbol =
      std::dynamic_pointer_cast<symTable::StructTypeSymbol>(idSymbol->getType());
  if (not structTypeSymbol)
    throw TypeError(ctx->getLineNumber(), "Identifier symbol is not a struct type");

  if (not structTypeSymbol->elementNameExist(ctx->getElementName()))
    throw TypeError(ctx->getLineNumber(), "Invalid struct element");

  const auto elementSymbolType = structTypeSymbol->getResolvedType(ctx->getElementName());
  const auto elementDataType = structTypeSymbol->getUnresolvedType(ctx->getElementName());

  ctx->setInferredSymbolType(elementSymbolType);
  ctx->setInferredDataType(elementDataType);

  return {};
}
std::any ValidationWalker::visitStructType(std::shared_ptr<types::StructTypeAst> ctx) {
  for (const auto &elementAst : ctx->getTypes())
    visit(elementAst);

  if (const auto scope = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getScope()))
    throw DefinitionError(ctx->getLineNumber(),
                          "Cannot define a struct in a Function/Procedure parameters");

  const auto typeSymbol = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(ctx->getSymbol());
  for (const auto &resolvedType : typeSymbol->getResolvedTypes()) {
    if (std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(resolvedType) ||
        std::dynamic_pointer_cast<symTable::StructTypeSymbol>(resolvedType)) {
      throw DefinitionError(ctx->getLineNumber(), "Invalid struct declaration");
    }
  }
  return {};
}
std::any ValidationWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  const auto typeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(ctx->getSymbol());
  for (const auto &elementAst : ctx->getTypes())
    visit(elementAst);
  for (const auto &resolvedType : typeSymbol->getResolvedTypes()) {
    if (std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(resolvedType) ||
        std::dynamic_pointer_cast<symTable::StructTypeSymbol>(resolvedType)) {
      throw DefinitionError(ctx->getLineNumber(), "Invalid tuple declaration");
    }
  }
  return {};
}
std::any ValidationWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  visit(ctx->getType());
  return {};
}
std::any ValidationWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  visit(ctx->getProto());
  if (!ctx->getProto()->getReturnType()) {
    throw ReturnError(ctx->getLineNumber(), "Function must have a return type");
  }
  const auto blockAst = std::dynamic_pointer_cast<statements::BlockAst>(ctx->getBody());
  if (blockAst && !hasReturnInMethod(blockAst)) {
    throw ReturnError(ctx->getLineNumber(), "Function must have a return statement");
  }

  if (not ctx->getBody()) {
    const auto methodSymbol = symTab->getGlobalScope()->getSymbol(ctx->getProto()->getName());
    const auto procedureDeclaration =
        std::dynamic_pointer_cast<prototypes::FunctionAst>(methodSymbol->getDef());
    if (not procedureDeclaration->getBody())
      throw DefinitionError(procedureDeclaration->getLineNumber(),
                            "Function does not have a definition");
  }

  if (ctx->getBody())
    visit(ctx->getBody());
  return {};
}
std::any ValidationWalker::visitFunctionParam(std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  visit(ctx->getParamType());
  return {};
}
std::any ValidationWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  for (const auto &param : ctx->getParams())
    visit(param);
  if (ctx->getReturnType())
    visit(ctx->getReturnType());
  return {};
}
std::any ValidationWalker::visitStructFuncCallRouter(
    std::shared_ptr<expressions::StructFuncCallRouterAst> ctx) {
  if (ctx->getIsStruct()) {
    const auto structLiteralAst = ctx->getStructLiteralAst();
    visit(structLiteralAst);
    ctx->setSymbol(structLiteralAst->getSymbol());
    ctx->setScope(structLiteralAst->getScope());
    ctx->setInferredDataType(structLiteralAst->getInferredDataType());
    ctx->setInferredSymbolType(structLiteralAst->getInferredSymbolType());
  } else {
    const auto fpCallAst = ctx->getFuncProcCallAst();
    visit(fpCallAst);
    ctx->setSymbol(fpCallAst->getSymbol());
    ctx->setScope(fpCallAst->getScope());
    ctx->setInferredDataType(fpCallAst->getInferredDataType());
    ctx->setInferredSymbolType(fpCallAst->getInferredSymbolType());
  }

  return {};
}
std::any ValidationWalker::visitFuncProcCall(std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (not methodSymbol)
    throw CallError(ctx->getLineNumber(), "Can only call functions/procedures");

  // need to re-resolve to get defined function and setSymbol to latest symbol with body defined
  methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      symTab->getGlobalScope()->resolveSymbol(methodSymbol->getName()));
  ctx->setSymbol(methodSymbol);

  // Get method prototype
  std::shared_ptr<prototypes::PrototypeAst> protoType;
  if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
    const auto procedureDecl =
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(methodSymbol->getDef());
    protoType = procedureDecl->getProto();
  } else if (methodSymbol->getScopeType() == symTable::ScopeType::Function) {
    const auto functionDecl =
        std::dynamic_pointer_cast<prototypes::FunctionAst>(methodSymbol->getDef());
    protoType = functionDecl->getProto();
  }

  // Validate arguments against parameters
  const auto params = protoType->getParams();
  const auto args = ctx->getArgs();
  validateArgs(params, args, methodSymbol->getScopeType(), ctx->getLineNumber());

  // Indirect way to check if symbol is a procedure
  if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
    // Throw error if procedure called outside an assignment/declaration or procedure call
    if (not inAssignment)
      throw CallError(ctx->getLineNumber(), "Cannot call procedure here");

    // Check if we call a procedure from a function
    const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
    if (curScope && curScope->getScopeType() == symTable::ScopeType::Function)
      throw CallError(ctx->getLineNumber(), "Procedure call inside function");
  }

  // Cannot call a void procedure in an expression
  if (!methodSymbol->getReturnType())
    throw ReturnError(ctx->getLineNumber(), "Return type is null");

  // Cannot call a procedure in a binary expression
  if (inBinaryOp && methodSymbol->getScopeType() == symTable::ScopeType::Procedure)
    throw CallError(ctx->getLineNumber(), "Procedure cannot be called in a binary expression");

  // Cannot call main
  if (protoType->getName() == "main")
    throw CallError(ctx->getLineNumber(), "Cannot call main function");

  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  ctx->setInferredDataType(protoType->getReturnType());
  return {};
}
std::any ValidationWalker::visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
  visit(ctx->getExpr());
  ctx->setInferredSymbolType(ctx->getExpr()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getExpr()->getInferredDataType());
  return {};
}
std::any ValidationWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  const auto boolType = std::make_shared<types::BooleanTypeAst>(ctx->token);
  ctx->setInferredDataType(boolType);
  ctx->setInferredSymbolType(resolvedInferredType(boolType));
  return {};
}

bool arrayCastCompatible(const std::shared_ptr<symTable::ArrayTypeSymbol> &fromArray,
                         const std::shared_ptr<symTable::ArrayTypeSymbol> &toArray) {
  if (!fromArray || !toArray) {
    return false;
  }
  const auto fromElem = fromArray->getType();
  const auto toElem = toArray->getType();

  auto fromElemArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(fromElem);
  auto toElemArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(toElem);
  if (fromElemArray || toElemArray) {
    return fromElemArray && toElemArray && arrayCastCompatible(fromElemArray, toElemArray);
  }

  const auto fromName = fromElem->getName();
  const auto toName = toElem->getName();
  if (utils::isPromotable(fromName, toName)) {
    return true;
  }
  return fromName == toName;
}
std::any ValidationWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  visit(ctx->getExpression());
  visit(ctx->getTargetType());
  const auto exprType = ctx->getExpression()->getInferredSymbolType();
  const auto targetType = ctx->getResolvedTargetType();

  if (auto arrayLiteral =
          std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(ctx->getExpression())) {
    if (arrayLiteral->getElements().empty()) {
      throw TypeError(ctx->getLineNumber(), "Casting non-variable empty arrays [] is not allowed, "
                                            "because a literal empty array does not have a type.");
    }
  }

  if (isTuple(exprType) && isTuple(targetType)) {
    const auto targetTupleTypeSymbol =
        std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(targetType);
    const auto curTupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(exprType);
    const auto targetSubTypes = targetTupleTypeSymbol->getResolvedTypes();
    const auto curSubTypes = curTupleTypeSymbol->getResolvedTypes();

    if (curSubTypes.size() != targetSubTypes.size())
      throw SizeError(ctx->getLineNumber(), "Tuple sizes do not match");
    for (size_t i = 0; i < curSubTypes.size(); i++) {
      if (not utils::isPromotable(curSubTypes[i]->getName(), targetSubTypes[i]->getName())) {
        throw TypeError(ctx->getLineNumber(), "Tuple sub type not promotable");
      }
    }
  } else if (isScalar(exprType) && isScalar(targetType)) {
    // check scalar is promotable
    const auto promoteTo = ctx->getResolvedTargetType()->getName();
    const auto promoteFrom = ctx->getExpression()->getInferredSymbolType()->getName();
    if (not utils::isPromotable(promoteFrom, promoteTo)) {
      throw TypeError(ctx->getLineNumber(), "Type not promotable");
    }
  } else if (auto fromArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(exprType)) {
    auto toArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(targetType);
    if (!toArray || !arrayCastCompatible(fromArray, toArray)) {
      throw TypeError(ctx->getLineNumber(), "Array element type not promotable");
    }
  } else if (isScalar(exprType) &&
             std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(targetType)) {
    auto toArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(targetType);
    auto toElem = toArray->getType();

    std::shared_ptr<symTable::Type> innerType = toElem;
    while (auto innerArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(innerType)) {
      innerType = innerArray->getType();
    }

    if (!utils::isPromotable(exprType->getName(), innerType->getName())) {
      throw TypeError(ctx->getLineNumber(), "Scalar type not promotable to array element type");
    }
  } else {
    throw TypeError(ctx->getLineNumber(), "Illegal cast");
  }
  ctx->setInferredSymbolType(ctx->getResolvedTargetType());
  ctx->setInferredDataType(ctx->getTargetType());
  return {};
}
std::any ValidationWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  const auto charType = std::make_shared<types::CharacterTypeAst>(ctx->token);
  ctx->setInferredDataType(charType);
  ctx->setInferredSymbolType(resolvedInferredType(charType));
  return {};
}
std::any ValidationWalker::visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) {
  const auto dataTypeSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());

  if (not dataTypeSymbol)
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");
  auto astNode = dataTypeSymbol->getDef();

  if (astNode->getNodeType() == NodeType::Declaration) {
    auto dataType = std::dynamic_pointer_cast<statements::DeclarationAst>(astNode)->getType();
    ctx->setInferredDataType(dataType);
    // Use the variable symbol's type directly to preserve inferred sizes
    ctx->setInferredSymbolType(dataTypeSymbol->getType());
  } else if (astNode->getNodeType() == NodeType::FunctionParam) {
    auto dataType =
        std::dynamic_pointer_cast<prototypes::FunctionParamAst>(astNode)->getParamType();
    ctx->setInferredDataType(dataType);
    ctx->setInferredSymbolType(resolvedInferredType(dataType));
  } else if (astNode->getNodeType() == NodeType::ProcedureParam) {
    auto dataType =
        std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(astNode)->getParamType();
    ctx->setInferredDataType(dataType);
    ctx->setInferredSymbolType(resolvedInferredType(dataType));
  } else if (astNode->getNodeType() == NodeType::DomainExpr) {
    // Iterator variable from generator - get type from the variable symbol
    auto varType = dataTypeSymbol->getType();
    if (varType) {
      // Create corresponding data type AST
      auto typeName = varType->getName();
      std::shared_ptr<types::DataTypeAst> dataType;
      if (typeName == "integer") {
        dataType = std::make_shared<types::IntegerTypeAst>(ctx->token);
      } else if (typeName == "real") {
        dataType = std::make_shared<types::RealTypeAst>(ctx->token);
      } else if (typeName == "character") {
        dataType = std::make_shared<types::CharacterTypeAst>(ctx->token);
      } else if (typeName == "boolean") {
        dataType = std::make_shared<types::BooleanTypeAst>(ctx->token);
      }
      if (dataType) {
        ctx->setInferredDataType(dataType);
        ctx->setInferredSymbolType(varType);
      }
    }
  }

  return {};
}
std::any ValidationWalker::visitIdentifierLeft(std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  if (not std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol()))
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");

  const auto dataTypeSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());

  const auto astNode = dataTypeSymbol->getDef();

  if (astNode->getNodeType() == NodeType::Declaration) {
    const auto dataType = std::dynamic_pointer_cast<statements::DeclarationAst>(astNode)->getType();
    ctx->setAssignDataType(dataType);
    // Use the variable symbol's type directly to preserve inferred sizes
    ctx->setAssignSymbolType(dataTypeSymbol->getType());
  } else if (astNode->getNodeType() == NodeType::FunctionParam) {
    const auto dataType =
        std::dynamic_pointer_cast<prototypes::FunctionParamAst>(astNode)->getParamType();
    ctx->setAssignDataType(dataType);
    ctx->setAssignSymbolType(resolvedInferredType(dataType));
  } else if (astNode->getNodeType() == NodeType::ProcedureParam) {
    const auto dataType =
        std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(astNode)->getParamType();
    ctx->setAssignDataType(dataType);
    ctx->setAssignSymbolType(resolvedInferredType(dataType));
  }

  return {};
}
std::any ValidationWalker::visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  auto intType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  ctx->setInferredDataType(intType);
  ctx->setInferredSymbolType(resolvedInferredType(intType));
  return {};
}
std::any ValidationWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  auto realType = std::make_shared<types::RealTypeAst>(ctx->token);
  ctx->setInferredDataType(realType);
  ctx->setInferredSymbolType(resolvedInferredType(realType));
  return {};
}
std::any ValidationWalker::visitArray(std::shared_ptr<expressions::ArrayLiteralAst> ctx) {
  auto arrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
  const auto elements = ctx->getElements();

  if (elements.empty()) {
    ctx->setInferredDataType(arrayType);
    if (!ctx->getInferredSymbolType()) {
      ctx->setInferredSymbolType(std::make_shared<symTable::EmptyArrayTypeSymbol>("empty_array"));
    }
    return {};
  }

  // first pass: find if it's a two-dimensional array and check for >2 dimensions
  visit(elements[0]);
  bool is2DArray = false;

  if (elements[0]->getInferredDataType()->getNodeType() == NodeType::ArrayType) {
    is2DArray = true;
  }

  // second pass: elements must all be the same dimension &
  // third pass: infer type by explicitly checking subtypes
  std::shared_ptr<symTable::Type> expectedSubtype = nullptr;
  std::shared_ptr<types::DataTypeAst> inferredElementType = nullptr;

  for (size_t i = 0; i < elements.size(); i++) {
    auto element = elements[i];

    if (i > 0) {
      visit(element);
    }

    // dimension consistency
    bool isElementArray = (element->getInferredDataType()->getNodeType() == NodeType::ArrayType);
    if (isElementArray != is2DArray) {
      throw TypeError(ctx->getLineNumber(), "Array elements must have the same dimension");
    }

    std::shared_ptr<symTable::Type> currentSubtype;
    if (is2DArray) {
      auto elementArrayType =
          std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(element->getInferredSymbolType());
      if (elementArrayType && elementArrayType->getType()) {
        currentSubtype = elementArrayType->getType();
      }
    } else {
      currentSubtype = element->getInferredSymbolType();
    }

    if (expectedSubtype != nullptr && currentSubtype != nullptr) {
      if (not(typesMatch(currentSubtype, expectedSubtype, true) ||
              typesMatch(expectedSubtype, currentSubtype, true))) {
        throw TypeError(ctx->getLineNumber(), "Type mismatch in Array");
      }

      if (expectedSubtype->getName().find("real") != std::string::npos) {
        continue;
      }
    }
    expectedSubtype = currentSubtype;
    inferredElementType = element->getInferredDataType();
  }

  arrayType->setType(inferredElementType);
  ctx->setInferredDataType(arrayType);
  ctx->setInferredSymbolType(resolvedInferredType(arrayType));
  return {};
}
std::any ValidationWalker::visitArrayAccess(std::shared_ptr<expressions::ArrayAccessAst> ctx) {
  const auto arrayInstance = ctx->getArrayInstance();
  const auto elementIndex = ctx->getElementIndex();
  visit(arrayInstance);
  visit(elementIndex);

  const auto arraySymbolType =
      std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayInstance->getInferredSymbolType());
  if (not arraySymbolType)
    throw TypeError(ctx->getLineNumber(), "Cannot slice or index non array type");
  const auto arrayDataType =
      std::dynamic_pointer_cast<types::ArrayTypeAst>(arrayInstance->getInferredDataType());

  if (elementIndex->getNodeType() == NodeType::SingularIndexExpr) {
    ctx->setInferredSymbolType(arraySymbolType->getType());
    ctx->setInferredDataType(arrayDataType->getType());
  } else if (elementIndex->getNodeType() == NodeType::RangedIndexExpr) {
    ctx->setInferredSymbolType(arraySymbolType);
    ctx->setInferredDataType(arrayDataType);
  }
  return {};
}
std::any
ValidationWalker::visitSingularIndex(std::shared_ptr<expressions::SingularIndexExprAst> ctx) {
  const auto indexExpr = ctx->getSingularIndexExpr();
  visit(indexExpr);
  if (not isOfSymbolType(indexExpr->getInferredSymbolType(), "integer"))
    throw TypeError(ctx->getLineNumber(), "Non integer index provided");
  ctx->setInferredDataType(indexExpr->getInferredDataType());
  ctx->setInferredSymbolType(indexExpr->getInferredSymbolType());
  return {};
}
std::any
ValidationWalker::visitRangedIndexExpr(std::shared_ptr<expressions::RangedIndexExprAst> ctx) {
  const auto leftExpr = ctx->getLeftIndexExpr();
  const auto rightExpr = ctx->getRightIndexExpr();

  visit(leftExpr);
  if (not isOfSymbolType(leftExpr->getInferredSymbolType(), "integer"))
    throw TypeError(ctx->getLineNumber(), "Non integer index provided");

  if (ctx->getRightIndexExpr()) {
    visit(rightExpr);
    if (not isOfSymbolType(rightExpr->getInferredSymbolType(), "integer"))
      throw TypeError(ctx->getLineNumber(), "Non integer index provided");
  }
  // both integers
  ctx->setInferredDataType(leftExpr->getInferredDataType());
  ctx->setInferredSymbolType(leftExpr->getInferredSymbolType());
  return {};
}
std::any
ValidationWalker::visitArrayElementAssign(std::shared_ptr<statements::ArrayElementAssignAst> ctx) {
  const auto arrayInstance = ctx->getArrayInstance();
  const auto elementIndex = ctx->getElementIndex();
  visit(arrayInstance);
  visit(elementIndex);

  const auto arraySymbolType =
      std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayInstance->getAssignSymbolType());
  if (not arraySymbolType)
    throw TypeError(ctx->getLineNumber(), "Cannot slice or index non array type");
  const auto arrayDataType =
      std::dynamic_pointer_cast<types::ArrayTypeAst>(arrayInstance->getAssignDataType());

  if (elementIndex->getNodeType() == NodeType::SingularIndexExpr) {
    ctx->setAssignSymbolType(arraySymbolType->getType());
    ctx->setAssignDataType(arrayDataType->getType());
  } else if (elementIndex->getNodeType() == NodeType::RangedIndexExpr) {
    ctx->setAssignSymbolType(arraySymbolType);
    ctx->setAssignDataType(arrayDataType);
  }
  return {};
}
std::any ValidationWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());

  auto exprType = ctx->getExpression()->getInferredSymbolType();
  auto opType = ctx->getUnaryOpType();
  if (opType == expressions::UnaryOpType::NOT) {
    if (!isOfSymbolType(exprType, "boolean")) {
      throw TypeError(ctx->getLineNumber(), "not operator can only be applied to boolean type");
    }
  } else if (opType == expressions::UnaryOpType::MINUS ||
             opType == expressions::UnaryOpType::PLUS) {
    if (!isOfSymbolType(exprType, "integer") && !isOfSymbolType(exprType, "real")) {
      throw TypeError(ctx->getLineNumber(),
                      "Unary +/- can only be applied to numeric types (integer or real)");
    }
  }

  ctx->setInferredSymbolType(ctx->getExpression()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getExpression()->getInferredDataType());
  return {};
}
std::any ValidationWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {

  if (ctx->getCondition()) {
    visit(ctx->getCondition());
    if (!isOfSymbolType(ctx->getCondition()->getInferredSymbolType(), "boolean")) {
      throw TypeError(ctx->getLineNumber(), "Loop condition must be of type boolean");
    }
  }

  visit(ctx->getBody());
  return {};
}
std::any ValidationWalker::visitIteratorLoop(std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}

std::any ValidationWalker::visitArrayType(std::shared_ptr<types::ArrayTypeAst> ctx) {
  if (ctx->getType()) {
    visit(ctx->getType());
  }

  for (const auto &sizeExpr : ctx->getSizes()) {
    if (sizeExpr) {
      visit(sizeExpr);
    }
  }

  return {};
}

std::any ValidationWalker::visitVectorType(std::shared_ptr<types::VectorTypeAst> ctx) {
  if (ctx->getElementType()) {
    visit(ctx->getElementType());
  }
  return {};
}
std::any ValidationWalker::visitLenMemberFunc(std::shared_ptr<statements::LenMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(
      ctx->getLeft()->getInferredSymbolType());
  if (!vectorTypeSym) {
    throw TypeError(ctx->getLineNumber(), "len can only be used on vector types");
  }
  auto intType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  ctx->setInferredDataType(intType);
  ctx->setInferredSymbolType(resolvedInferredType(intType));
  return {};
}
std::any
ValidationWalker::visitAppendMemberFunc(std::shared_ptr<statements::AppendMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(
      ctx->getLeft()->getInferredSymbolType());
  if (!vectorTypeSym) {
    throw TypeError(ctx->getLineNumber(), "append can only be used on vector types");
  }
  for (const auto &arg : ctx->getArgs()) {
    visit(arg);
  }
  ctx->setInferredSymbolType(ctx->getLeft()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getLeft()->getInferredDataType());
  return {};
}
std::any ValidationWalker::visitPushMemberFunc(std::shared_ptr<statements::PushMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(
      ctx->getLeft()->getInferredSymbolType());
  if (!vectorTypeSym) {
    throw TypeError(ctx->getLineNumber(), "push can only be used on vector types");
  }
  auto elementType = vectorTypeSym->getType();
  for (const auto &arg : ctx->getArgs()) {
    visit(arg);
    auto argType =
        arg->getExpr() ? arg->getExpr()->getInferredSymbolType() : arg->getInferredSymbolType();
    if (!elementType || !argType || !typesMatch(elementType, argType)) {
      throw TypeError(arg->getLineNumber(), "push argument type mismatch");
    }
  }
  ctx->setInferredSymbolType(ctx->getLeft()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getLeft()->getInferredDataType());
  return {};
}
std::any
ValidationWalker::visitConcatMemberFunc(std::shared_ptr<statements::ConcatMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  for (const auto &arg : ctx->getArgs()) {
    visit(arg);
  }
  ctx->setInferredSymbolType(ctx->getLeft()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getLeft()->getInferredDataType());
  return {};
}
std::any
ValidationWalker::visitLengthBuiltinFunc(std::shared_ptr<expressions::LengthBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  visit(ctx->arg);
  const auto argType = ctx->arg->getInferredSymbolType();
  // TODO: support strings
  if (!argType ||
      (argType->getName().substr(0, 5) != "array" && argType->getName().substr(0, 6) != "vector" &&
       argType->getName() != "empty_array")) {
    throw CallError(ctx->getLineNumber(), "length builtin must be called on arrays or vectors");
  }
  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  ctx->setInferredDataType(std::make_shared<types::IntegerTypeAst>(ctx->token));
  return {};
}
std::any
ValidationWalker::visitShapeBuiltinFunc(std::shared_ptr<expressions::ShapeBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  visit(ctx->arg);
  const auto argType = ctx->arg->getInferredSymbolType();
  // TODO: Support strings
  if (!argType ||
      (argType->getName().substr(0, 5) != "array" && argType->getName().substr(0, 6) != "vector" &&
       argType->getName() != "empty_array")) {
    throw CallError(ctx->getLineNumber(), "shape builtin must be called on arrays or vectors");
  }
  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  ctx->setInferredDataType(std::make_shared<types::ArrayTypeAst>(ctx->token));
  return {};
}
std::any
ValidationWalker::visitReverseBuiltinFunc(std::shared_ptr<expressions::ReverseBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  visit(ctx->arg);
  const auto argType = ctx->arg->getInferredSymbolType();
  // TODO: support strings
  if (!argType ||
      (argType->getName().substr(0, 5) != "array" && argType->getName().substr(0, 6) != "vector")) {
    throw CallError(ctx->getLineNumber(), "reverse builtin must be called on arrays or vectors");
  }

  // Set the method symbol's return type to match the argument type
  methodSymbol->setReturnType(argType);
  ctx->setInferredSymbolType(argType);
  ctx->setInferredDataType(ctx->arg->getInferredDataType());
  return {};
}
std::any
ValidationWalker::visitFormatBuiltinFunc(std::shared_ptr<expressions::FormatBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  visit(ctx->arg);
  const auto argType = ctx->arg->getInferredSymbolType();
  if (!argType ||
      (argType->getName().substr(0, 5) != "array" && argType->getName().substr(0, 6) != "vector")) {
    throw CallError(ctx->getLineNumber(), "format builtin must be called on arrays or vectors");
  }
  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  // TODO: Update it for strings
  ctx->setInferredDataType(std::make_shared<types::IntegerTypeAst>(ctx->token));
  return {};
}

std::any ValidationWalker::visitRange(std::shared_ptr<expressions::RangeAst> ctx) {
  visit(ctx->getStart());
  visit(ctx->getEnd());

  auto startType = ctx->getStart()->getInferredSymbolType();
  auto endType = ctx->getEnd()->getInferredSymbolType();

  if (!isOfSymbolType(startType, "integer")) {
    throw TypeError(ctx->getLineNumber(), "Range start must be of type integer");
  }
  if (!isOfSymbolType(endType, "integer")) {
    throw TypeError(ctx->getLineNumber(), "Range end must be of type integer");
  }

  auto intType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  auto arrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
  arrayType->setType(intType);

  ctx->setInferredDataType(arrayType);
  ctx->setInferredSymbolType(resolvedInferredType(arrayType));
  return {};
}

std::any ValidationWalker::visitDomainExpr(std::shared_ptr<expressions::DomainExprAst> ctx) {
  visit(ctx->getDomainExpression());

  auto domainType = ctx->getDomainExpression()->getInferredSymbolType();

  if (domainType && !isCollection(domainType)) {
    throw TypeError(ctx->getLineNumber(), "Domain expression must be a vector or interval");
  }
  auto varSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (varSymbol && domainType) {
    auto arrayTypeSymbol = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(domainType);
    if (arrayTypeSymbol) {
      auto elementType = arrayTypeSymbol->getType();
      varSymbol->setType(elementType);
    }
  }

  ctx->setInferredDataType(ctx->getDomainExpression()->getInferredDataType());
  ctx->setInferredSymbolType(ctx->getDomainExpression()->getInferredSymbolType());
  return {};
}

std::any ValidationWalker::visitGenerator(std::shared_ptr<expressions::GeneratorAst> ctx) {
  for (const auto &domainExpr : ctx->getDomainExprs()) {
    visit(domainExpr);
  }

  visit(ctx->getGeneratorExpression());

  auto generatorExprType = ctx->getGeneratorExpression()->getInferredSymbolType();
  auto generatorExprDataType = ctx->getGeneratorExpression()->getInferredDataType();

  if (!generatorExprType) {
    throw TypeError(ctx->getLineNumber(), "Generator expression type cannot be inferred");
  }

  size_t dimensions = ctx->getDimensionCount();

  if (dimensions == 1) {
    auto arrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
    arrayType->setType(generatorExprDataType);
    ctx->setInferredDataType(arrayType);
    ctx->setInferredSymbolType(resolvedInferredType(arrayType));
  } else if (dimensions == 2) {
    auto innerArrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
    innerArrayType->setType(generatorExprDataType);
    auto arrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
    arrayType->setType(innerArrayType);
    ctx->setInferredDataType(arrayType);
    ctx->setInferredSymbolType(resolvedInferredType(arrayType));
  } else {
    throw TypeError(ctx->getLineNumber(),
                    "Generator supports only 1D or 2D, got " + std::to_string(dimensions) + "D");
  }
  return {};
}
std::any ValidationWalker::visitStreamStateBuiltinFunc(
    std::shared_ptr<expressions::StreamStateBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  auto intType =
      std::dynamic_pointer_cast<symTable::Type>(symTab->getGlobalScope()->resolveType("integer"));
  methodSymbol->setReturnType(intType);
  ctx->setInferredSymbolType(intType);
  ctx->setInferredDataType(std::make_shared<types::IntegerTypeAst>(ctx->token));
  return {};
}
} // namespace gazprea::ast::walkers
