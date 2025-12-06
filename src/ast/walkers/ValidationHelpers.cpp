#include "CompileTimeExceptions.h"
#include "ast/expressions/ArrayAccessAst.h"
#include "ast/expressions/ArrayLiteralAst.h"
#include "ast/expressions/UnaryAst.h"
#include "ast/types/AliasTypeAst.h"
#include "ast/types/ArrayTypeAst.h"
#include "ast/types/BooleanTypeAst.h"
#include "ast/types/CharacterTypeAst.h"
#include "ast/types/IntegerTypeAst.h"
#include "ast/types/RealTypeAst.h"
#include "ast/types/VectorTypeAst.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/EmptyArrayTypeSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"

#include <ast/walkers/ValidationWalker.h>

namespace gazprea::ast::walkers {

bool ValidationWalker::isScalar(const std::shared_ptr<symTable::Type> &type) {
  if (type->getName() == "integer")
    return true;
  if (type->getName() == "real")
    return true;
  if (type->getName() == "character")
    return true;
  if (type->getName() == "boolean")
    return true;
  return false;
}

bool ValidationWalker::isCollection(const std::shared_ptr<symTable::Type> &type) {
  if (type->getName().substr(0, 5) == "array")
    return true;
  if (type->getName().substr(0, 6) == "vector")
    return true;
  return false;
}

bool ValidationWalker::isTuple(const std::shared_ptr<symTable::Type> &type) {
  if (type->getName() == "tuple")
    return true;
  return false;
}

void ValidationWalker::validateVariableAssignmentTypes(
    std::shared_ptr<statements::IdentifierLeftAst> ctx,
    std::shared_ptr<symTable::Type> exprTypeSymbol) {
  const auto lValSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto lValSymbolType = lValSymbol->getType();
  if (lValSymbol->getQualifier() == Qualifier::Const)
    throw AssignError(ctx->getLineNumber(), "Cannot re-assign a constant value");

  if (not typesMatch(lValSymbolType, exprTypeSymbol))
    throw TypeError(ctx->getLineNumber(), "Type mismatch");
}

void ValidationWalker::validateTupleElementAssignmentTypes(
    std::shared_ptr<statements::TupleElementAssignAst> ctx,
    std::shared_ptr<symTable::Type> exprTypeSymbol) {
  const auto lValSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (lValSymbol->getQualifier() == Qualifier::Const)
    throw AssignError(ctx->getLineNumber(), "Cannot re-assign a constant value");

  const auto tupleType =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(lValSymbol->getType());
  const auto tupleSubType = tupleType->getResolvedTypes()[ctx->getFieldIndex() - 1];

  if (not typesMatch(tupleSubType, exprTypeSymbol))
    throw TypeError(ctx->getLineNumber(), "Type mismatch");
}

void ValidationWalker::validateStructElementAssignmentTypes(
    std::shared_ptr<statements::StructElementAssignAst> ctx,
    std::shared_ptr<symTable::Type> exprTypeSymbol) {
  const auto lValSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (lValSymbol->getQualifier() == Qualifier::Const)
    throw AssignError(ctx->getLineNumber(), "Cannot re-assign a constant value");

  const auto structType =
      std::dynamic_pointer_cast<symTable::StructTypeSymbol>(lValSymbol->getType());
  const auto structSubType = structType->getResolvedType(ctx->getElementName());

  if (not typesMatch(structSubType, exprTypeSymbol))
    throw TypeError(ctx->getLineNumber(), "Type mismatch");
}

void ValidationWalker::validateArrayElementAssignmentTypes(
    std::shared_ptr<statements::ArrayElementAssignAst> ctx,
    std::shared_ptr<symTable::Type> exprTypeSymbol) {
  const auto lValAst = ctx->getArrayInstance();
  const auto lValSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(lValAst->getSymbol());

  if (lValSymbol->getQualifier() == Qualifier::Const)
    throw AssignError(ctx->getLineNumber(), "Cannot re-assign a constant value");

  const auto arrayType =
      std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(lValAst->getAssignSymbolType());

  const auto arraySubType = arrayType->getType();

  if (not typesMatch(ctx->getAssignSymbolType(), exprTypeSymbol))
    throw TypeError(ctx->getLineNumber(), "Type mismatch");
}

void ValidationWalker::validateTupleUnpackAssignmentTypes(
    std::shared_ptr<statements::TupleUnpackAssignAst> ctx,
    std::shared_ptr<symTable::Type> exprTypeSymbol) {
  const auto tupleType = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(exprTypeSymbol);
  if (not tupleType)
    throw TypeError(ctx->getLineNumber(), "Can only unpack tuples");

  const auto varList = ctx->getLVals();
  const auto tupleElements = tupleType->getResolvedTypes();

  if (varList.size() != tupleElements.size())
    throw SizeError(ctx->getLineNumber(), "unpack list size does not match tuple size");

  for (size_t i = 0; i < tupleElements.size(); ++i) {
    if (varList[i]->getNodeType() == NodeType::IdentifierLeft) {
      const auto idAssignStat =
          std::dynamic_pointer_cast<statements::IdentifierLeftAst>(varList[i]);
      validateVariableAssignmentTypes(idAssignStat, tupleElements[i]);
    } else if (varList[i]->getNodeType() == NodeType::TupleElementAssign) {
      const auto tupleElementAssignStat =
          std::dynamic_pointer_cast<statements::TupleElementAssignAst>(varList[i]);
      validateTupleElementAssignmentTypes(tupleElementAssignStat, tupleElements[i]);
    } else if (varList[i]->getNodeType() == NodeType::StructElementAssign) {
      const auto structElementAssignStat =
          std::dynamic_pointer_cast<statements::StructElementAssignAst>(varList[i]);
      validateStructElementAssignmentTypes(structElementAssignStat, tupleElements[i]);
    } else if (varList[i]->getNodeType() == NodeType::ArrayElementAssign) {
      const auto arrayElementAssignStat =
          std::dynamic_pointer_cast<statements::ArrayElementAssignAst>(varList[i]);
      validateArrayElementAssignmentTypes(arrayElementAssignStat, tupleElements[i]);
    }
  }
}

bool ValidationWalker::typesMatch(const std::shared_ptr<symTable::Type> &destination,
                                  const std::shared_ptr<symTable::Type> &source, bool exactMatch) {
  if (isOfSymbolType(destination, "integer") && isOfSymbolType(source, "integer"))
    return true;
  if (isOfSymbolType(destination, "real") && isOfSymbolType(source, "real"))
    return true;
  if (isOfSymbolType(destination, "real") && isOfSymbolType(source, "integer"))
    return true;
  if (isOfSymbolType(destination, "character") && isOfSymbolType(source, "character"))
    return true;
  if (isOfSymbolType(destination, "boolean") && isOfSymbolType(source, "boolean"))
    return true;
  if (isOfSymbolType(destination, "tuple") && isOfSymbolType(source, "tuple")) {
    const auto destTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(destination);
    const auto sourceTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(source);
    return isTupleTypeMatch(destTuple, sourceTuple);
  }
  if (isOfSymbolType(destination, "struct") && isOfSymbolType(source, "struct")) {
    const auto destStruct = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(destination);
    const auto sourceStruct = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(source);
    return destStruct->getStructName() == sourceStruct->getStructName();
  }
  // by default allow array to scalar assignment if element types match
  if (not exactMatch) {
    const auto destArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(destination);
    if (destArray && typesMatch(destArray->getType(), source)) {
      return true;
    }
    const auto destVector = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(destination);
    if (destVector && typesMatch(destVector->getType(), source)) {
      return true;
    }
  }
  // exactMatch is used to verify types within the array
  if (isOfSymbolType(destination, "array") && isOfSymbolType(source, "array")) {
    const auto destArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(destination);
    const auto sourceArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(source);
    return typesMatch(destArray->getType(), sourceArray->getType());
  }
  if (isOfSymbolType(destination, "vector") && isOfSymbolType(source, "array")) {
    const auto destVector = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(destination);
    const auto sourceVector = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(source);
    return typesMatch(destVector->getType(), sourceVector->getType());
  }
  if (isOfSymbolType(destination, "vector") && isOfSymbolType(source, "vector")) {
    const auto destVector = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(destination);
    const auto sourceVector = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(source);
    return typesMatch(destVector->getType(), sourceVector->getType());
  }
  if ((isOfSymbolType(destination, "array") || isOfSymbolType(destination, "vector")) &&
      std::dynamic_pointer_cast<symTable::EmptyArrayTypeSymbol>(source)) {
    return true;
  }
  return false;
}

bool ValidationWalker::isTupleTypeMatch(
    const std::shared_ptr<symTable::TupleTypeSymbol> &destination,
    const std::shared_ptr<symTable::TupleTypeSymbol> &source) {
  const auto destSubTypes = destination->getResolvedTypes();
  const auto sourceSubTypes = source->getResolvedTypes();
  if (destSubTypes.size() != sourceSubTypes.size())
    return false;

  for (size_t i = 0; i < destSubTypes.size(); i++) {
    if (not typesMatch(destSubTypes[i], sourceSubTypes[i]))
      return false;
  }
  return true;
}

bool ValidationWalker::isOfSymbolType(const std::shared_ptr<symTable::Type> &symbolType,
                                      const std::string &typeName) {
  if (!symbolType)
    throw std::runtime_error("SymbolType should not be null\n");

  const auto symbolName = symbolType->getName();
  if (typeName == "array" && symbolName.substr(0, 5) == "array")
    return true;
  if (typeName == "vector" && symbolName.substr(0, 6) == "vector")
    return true;

  return symbolName == typeName;
}

std::shared_ptr<symTable::Scope>
ValidationWalker::getEnclosingFuncProcScope(std::shared_ptr<symTable::Scope> currentScope) {
  while (currentScope != nullptr &&
         (currentScope->getScopeType() != symTable::ScopeType::Function &&
          currentScope->getScopeType() != symTable::ScopeType::Procedure)) {
    currentScope = currentScope->getEnclosingScope();
  }
  return currentScope;
}

int ValidationWalker::opTable[7][17] = {
    //  ^  *  /  %  +  -  <  > <= >= == !=  & or xor ** || by
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0}, // Integer
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0}, // Real
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // Character
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0}, // Boolean
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // Tuple
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // Struct
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // array
};

bool ValidationWalker::isValidOp(std::shared_ptr<symTable::Type> type,
                                 expressions::BinaryOpType opType) {
  if (nodeTypeToIndex(type->getName()) == -1)
    throw std::runtime_error("Invalid data type");
  if (std::dynamic_pointer_cast<symTable::EmptyArrayTypeSymbol>(type) &&
      opType == expressions::BinaryOpType::DPIPE)
    return true;
  if (isCollection((type))) {
    // Handle both arrays and vectors
    std::shared_ptr<symTable::Type> elementType;
    if (auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type)) {
      elementType = arrayType->getType();
    } else if (auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type)) {
      elementType = vectorType->getType();
    } else {
      throw std::runtime_error("Collection type is neither array nor vector");
    }

    if (isCollection(elementType))
      return isValidOp(elementType, opType);
    else
      return isValidOp(elementType, expressions::BinaryOpType::MULTIPLY);
  }
  return opTable[nodeTypeToIndex(type->getName())][static_cast<int>(opType)];
}

bool ValidationWalker::isArrayRealType(const std::shared_ptr<symTable::Type> &type) {
  if (type->getName().find("real") != std::string::npos) {
    return true;
  }
  return false;
}

bool ValidationWalker::isVectorRealType(const std::shared_ptr<symTable::Type> &type) {
  std::string name = type->getName();
  if (type->getName().find("real") != std::string::npos) {
    return true;
  }
  return false;
}
void ValidationWalker::promoteIfNeeded(std::shared_ptr<expressions::ExpressionAst> ctx,
                                       std::shared_ptr<symTable::Type> promoteFrom,
                                       std::shared_ptr<symTable::Type> promoteTo,
                                       std::shared_ptr<types::DataTypeAst> promoteToDataType) {
  if (promoteFrom->getName() == "integer" && promoteTo->getName() == "real") {
    ctx->setInferredSymbolType(promoteTo);
    ctx->setInferredDataType(promoteToDataType);
    return;
  }
}

int ValidationWalker::nodeTypeToIndex(const std::string &typeName) {
  if (typeName == "integer")
    return 0;
  if (typeName == "real")
    return 1;
  if (typeName == "character")
    return 2;
  if (typeName == "boolean")
    return 3;
  if (typeName == "tuple")
    return 4;
  if (typeName == "struct")
    return 5;
  if (typeName.substr(0, 5) == "array" || typeName.substr(0, 6) == "vector" ||
      typeName == "empty_array")
    return 6;
  return -1;
}

bool ValidationWalker::hasReturnInMethod(const std::shared_ptr<statements::BlockAst> &block) {
  for (const auto &stat : block->getChildren()) {
    if (stat->getNodeType() == NodeType::Return) {
      return true;
    }
    if (stat->getNodeType() == NodeType::Block) {
      auto nestedBlock = std::dynamic_pointer_cast<statements::BlockAst>(stat);
      if (hasReturnInMethod(nestedBlock)) {
        return true;
      }
    }
    if (stat->getNodeType() == NodeType::Conditional) {
      const auto conditional = std::dynamic_pointer_cast<statements::ConditionalAst>(stat);
      const auto thenBlock =
          std::dynamic_pointer_cast<statements::BlockAst>(conditional->getThenBody());
      auto elseBlock = std::dynamic_pointer_cast<statements::BlockAst>(conditional->getElseBody());

      if (thenBlock && elseBlock && hasReturnInMethod(thenBlock) && hasReturnInMethod(elseBlock)) {
        return true;
      }
    }
  }
  return false;
}

std::shared_ptr<symTable::Type>
ValidationWalker::resolvedInferredType(const std::shared_ptr<types::DataTypeAst> &dataType) {
  auto globalScope = symTab->getGlobalScope();
  switch (dataType->getNodeType()) {
  case NodeType::IntegerType:
    return std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("integer"));
  case NodeType::RealType:
    return std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("real"));
  case NodeType::CharType:
    return std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("character"));
  case NodeType::BoolType:
    return std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("boolean"));
  case NodeType::ArrayType: {
    auto arrayTypeSymbol = std::make_shared<symTable::ArrayTypeSymbol>("array");
    auto arrayDataType = std::dynamic_pointer_cast<types::ArrayTypeAst>(dataType);
    arrayTypeSymbol->setType(resolvedInferredType(arrayDataType->getType()));
    return std::dynamic_pointer_cast<symTable::Type>(arrayTypeSymbol);
  }
  case NodeType::VectorType: {
    const auto vectorTypeSymbol = std::make_shared<symTable::VectorTypeSymbol>("vector");
    const auto vectorDataType = std::dynamic_pointer_cast<types::VectorTypeAst>(dataType);
    vectorTypeSymbol->setType(resolvedInferredType(vectorDataType->getElementType()));
    if (const auto elementArrayType =
            std::dynamic_pointer_cast<types::ArrayTypeAst>(vectorDataType->getElementType())) {
      vectorTypeSymbol->setElementSizeInferenceFlags(elementArrayType->isSizeInferred());
    }
    return std::dynamic_pointer_cast<symTable::Type>(vectorTypeSymbol);
  }
  case NodeType::AliasType: {
    const auto aliasTypeNode = std::dynamic_pointer_cast<types::AliasTypeAst>(dataType);
    const auto aliasSymType = globalScope->resolveType(aliasTypeNode->getAlias());
    return std::dynamic_pointer_cast<symTable::Type>(aliasSymType);
  }
  case NodeType::TupleType: {
    // visiting the tuple
    auto tupleTypeSymbol = std::make_shared<symTable::TupleTypeSymbol>("");
    auto tupleDataType = std::dynamic_pointer_cast<types::TupleTypeAst>(dataType);
    for (const auto &subType : tupleDataType->getTypes()) {
      tupleTypeSymbol->addResolvedType(resolvedInferredType(subType));
    }
    return std::dynamic_pointer_cast<symTable::Type>(tupleTypeSymbol);
  }
  default:
    return {};
  }
}

// Check if type is integer or real
bool ValidationWalker::isNumericType(const std::shared_ptr<symTable::Type> &type) {
  return isOfSymbolType(type, "integer") || isOfSymbolType(type, "real");
}

// Check if operator is a comparison operator
bool ValidationWalker::isComparisonOperator(expressions::BinaryOpType opType) {
  return opType == expressions::BinaryOpType::LESS_THAN ||
         opType == expressions::BinaryOpType::GREATER_THAN ||
         opType == expressions::BinaryOpType::LESS_EQUAL ||
         opType == expressions::BinaryOpType::GREATER_EQUAL;
}

// check if left and right are real or integer
bool ValidationWalker::areBothNumeric(const std::shared_ptr<expressions::ExpressionAst> &left,
                                      const std::shared_ptr<expressions::ExpressionAst> &right) {
  // Check if both are vectors
  if (std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(left->getInferredSymbolType()) &&
      std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(right->getInferredSymbolType())) {
    auto leftVectorType =
        std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(left->getInferredSymbolType());
    auto rightVectorType =
        std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(right->getInferredSymbolType());
    return isVectorNumericType(left->getInferredSymbolType()) &&
           isVectorNumericType(right->getInferredSymbolType());
  }

  // Check if one is a vector and the other is not
  if (std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(left->getInferredSymbolType()) ||
      std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(right->getInferredSymbolType())) {
    if (auto leftVectorTy =
            std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(left->getInferredSymbolType())) {
      return isVectorNumericType(left->getInferredSymbolType()) &&
             isNumericType(right->getInferredSymbolType());
    } else if (auto rightVectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(
                   right->getInferredSymbolType())) {
      return isVectorNumericType(right->getInferredSymbolType()) &&
             isNumericType(left->getInferredSymbolType());
    }
    return false;
  }

  // Check if both are arrays
  if (std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(left->getInferredSymbolType()) &&
      std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(right->getInferredSymbolType())) {
    auto leftArrayType =
        std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(left->getInferredSymbolType());
    auto rightArrayType =
        std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(right->getInferredSymbolType());
    return isArrayNumericType(leftArrayType->getType()) &&
           isArrayNumericType(rightArrayType->getType());
  }

  // Check if one is an array and the other is not
  if (std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(left->getInferredSymbolType()) ||
      std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(right->getInferredSymbolType())) {
    if (auto leftArrayTy =
            std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(left->getInferredSymbolType())) {
      return isArrayNumericType(leftArrayTy->getType()) &&
             isNumericType(right->getInferredSymbolType());
    } else if (auto rightArrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(
                   right->getInferredSymbolType())) {
      return isArrayNumericType(rightArrayType->getType()) &&
             isNumericType(left->getInferredSymbolType());
    }
    return false;
  }

  // Both are scalars
  return isNumericType(left->getInferredSymbolType()) &&
         isNumericType(right->getInferredSymbolType());
}

bool ValidationWalker::isArrayNumericType(const std::shared_ptr<symTable::Type> &type) {
  if (auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type)) {
    auto elementType = arrayType->getType();
    return isArrayNumericType(elementType);
  }
  if (type->getName() == "integer" || type->getName() == "real")
    return true;
  return false;
}

bool ValidationWalker::isVectorNumericType(const std::shared_ptr<symTable::Type> &type) {
  if (auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type)) {
    auto elementType = vectorType->getType();
    if (std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(elementType))
      return isArrayNumericType(elementType);
    else
      return isNumericType(elementType);
  }
  return false;
}

bool ValidationWalker::supportsUnaryOp(const std::shared_ptr<symTable::Type> &type,
                                       expressions::UnaryOpType opType) {
  if (!type)
    return false;

  if (auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(type)) {
    return supportsUnaryOp(arrayType->getType(), opType);
  }

  if (auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type)) {
    return supportsUnaryOp(vectorType->getType(), opType);
  }

  if (opType == expressions::UnaryOpType::NOT) {
    return isOfSymbolType(type, "boolean");
  }

  if (opType == expressions::UnaryOpType::MINUS || opType == expressions::UnaryOpType::PLUS) {
    return isOfSymbolType(type, "integer") || isOfSymbolType(type, "real");
  }

  return false;
}

void ValidationWalker::validateArgs(const std::vector<std::shared_ptr<Ast>> &params,
                                    const std::vector<std::shared_ptr<expressions::ArgAst>> &args,
                                    const symTable::ScopeType scopeType, const int lineNumber) {
  if (args.empty() && params.empty())
    return;
  if (args.size() != params.size())
    throw TypeError(lineNumber, "Arguments size doesn't match the parameters size");

  for (size_t i = 0; i < args.size(); i++) {
    const auto &arg = args[i];
    visit(arg);

    if (scopeType == symTable::ScopeType::Function) {
      const auto &param = std::dynamic_pointer_cast<prototypes::FunctionParamAst>(params[i]);
      const auto &paramVarSymbol =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(param->getSymbol());
      if (not typesMatch(paramVarSymbol->getType(), arg->getInferredSymbolType()))
        throw TypeError(arg->getLineNumber(), "Argument type mismatch");
    } else if (scopeType == symTable::ScopeType::Procedure) {
      const auto &param = std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(params[i]);
      const auto &paramVarSymbol =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(param->getSymbol());

      if (paramVarSymbol->getQualifier() == Qualifier::Var && not arg->isLValue())
        throw TypeError(arg->getLineNumber(), "l-value must be given to var procedure param");

      if (not typesMatch(paramVarSymbol->getType(), arg->getInferredSymbolType()))
        throw TypeError(arg->getLineNumber(), "Argument type mismatch");
    }
  }

  // Check for var parameter aliasing if this is a procedure
  if (scopeType == symTable::ScopeType::Procedure) {
    checkVarArgs(params, args);
  }
}

void ValidationWalker::identifierAliasingCheck(
    std::unordered_map<std::string,
                       std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
        &seenVarMap,
    std::shared_ptr<expressions::ExpressionAst> expr, bool isVar) {
  const auto identifier = std::dynamic_pointer_cast<expressions::IdentifierAst>(expr);
  std::string varIdentifier = identifier->getName();
  // First time seeing this identifier
  if (seenVarMap.find(varIdentifier) == seenVarMap.end()) {
    seenVarMap[varIdentifier].emplace_back(identifier, isVar);
    return;
  }

  // If this identifier was already seen
  for (const auto &seenArg : seenVarMap[varIdentifier]) {
    if (seenArg.second || isVar) {
      throw AliasingError(expr->getLineNumber(),
                          "var parameter cannot share a variable with another parameter");
    }
  }
}

void ValidationWalker::tupleAccessAliasingCheck(
    std::unordered_map<std::string,
                       std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
        &seenVarMap,
    std::shared_ptr<expressions::ExpressionAst> expr, bool isVar) {
  const auto tupleAccess = std::dynamic_pointer_cast<expressions::TupleAccessAst>(expr);
  std::string varIdentifier = tupleAccess->getTupleName();
  // First time seeing this identifier
  if (seenVarMap.find(varIdentifier) == seenVarMap.end()) {
    seenVarMap[varIdentifier].emplace_back(tupleAccess, isVar);
    return;
  }

  // If this identifier was already seen
  for (const auto &seenArg : seenVarMap[varIdentifier]) {
    if (seenArg.second || isVar) {
      const auto lVal = seenArg.first;
      if (lVal->getNodeType() == NodeType::Identifier)
        throw AliasingError(expr->getLineNumber(),
                            "var parameter cannot share a variable with another parameter");
      if (lVal->getNodeType() == NodeType::TupleAccess) {
        const auto tupleAccessLVal = std::dynamic_pointer_cast<expressions::TupleAccessAst>(lVal);
        if (tupleAccessLVal->getFieldIndex() == tupleAccess->getFieldIndex())
          throw AliasingError(expr->getLineNumber(),
                              "var parameter cannot share a variable with another parameter");
        seenVarMap[varIdentifier].emplace_back(tupleAccess, isVar);
      }
    }
  }
}

void ValidationWalker::structAccessAliasingCheck(
    std::unordered_map<std::string,
                       std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
        &seenVarMap,
    std::shared_ptr<expressions::ExpressionAst> expr, bool isVar) {
  const auto structAccess = std::dynamic_pointer_cast<expressions::StructAccessAst>(expr);
  std::string varIdentifier = structAccess->getStructName();

  // First time seeing this identifier
  if (seenVarMap.find(varIdentifier) == seenVarMap.end()) {
    seenVarMap[varIdentifier].emplace_back(structAccess, isVar);
    return;
  }

  // If this identifier was already seen
  for (const auto &seenArg : seenVarMap[varIdentifier]) {
    if (seenArg.second || isVar) {
      const auto lVal = seenArg.first;
      if (lVal->getNodeType() == NodeType::Identifier)
        throw AliasingError(expr->getLineNumber(),
                            "var parameter cannot share a variable with another parameter");
      if (lVal->getNodeType() == NodeType::StructAccess) {
        const auto structAccessLVal = std::dynamic_pointer_cast<expressions::StructAccessAst>(lVal);
        if (structAccessLVal->getElementName() == structAccess->getElementName())
          throw AliasingError(expr->getLineNumber(),
                              "var parameter cannot share a variable with another parameter");
        seenVarMap[varIdentifier].emplace_back(structAccess, isVar);
      }
    }
  }
}

void ValidationWalker::arrayAccessAliasingCheck(
    std::unordered_map<std::string,
                       std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
        &seenVarMap,
    std::shared_ptr<expressions::ExpressionAst> expr, bool isVar) {
  const auto arrayAccess = std::dynamic_pointer_cast<expressions::ArrayAccessAst>(expr);
  if (arrayAccess->getArrayInstance()->getNodeType() == NodeType::Identifier) {
    identifierAliasingCheck(seenVarMap, arrayAccess->getArrayInstance(), isVar);
  } else if (arrayAccess->getArrayInstance()->getNodeType() == NodeType::TupleAccess) {
    tupleAccessAliasingCheck(seenVarMap, arrayAccess->getArrayInstance(), isVar);
  } else if (arrayAccess->getArrayInstance()->getNodeType() == NodeType::StructAccess) {
    structAccessAliasingCheck(seenVarMap, arrayAccess->getArrayInstance(), isVar);
  } else if (arrayAccess->getArrayInstance()->getNodeType() == NodeType::ArrayAccess) {
    arrayAccessAliasingCheck(seenVarMap, arrayAccess->getArrayInstance(), isVar);
  }
}

// helper to check for var parameter aliasing
void ValidationWalker::checkVarArgs(const std::vector<std::shared_ptr<Ast>> &params,
                                    const std::vector<std::shared_ptr<expressions::ArgAst>> &args) {
  // unordered map to track vars
  std::unordered_map<std::string,
                     std::vector<std::pair<std::shared_ptr<expressions::ExpressionAst>, bool>>>
      seenVarMap;
  for (size_t i = 0; i < args.size(); i++) {
    const auto param = std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(params[i]);
    const bool isVar = (param->getQualifier() == Qualifier::Var);

    if (args[i]->getExpr()->getNodeType() == NodeType::Identifier) {
      identifierAliasingCheck(seenVarMap, args[i]->getExpr(), isVar);
    } else if (args[i]->getExpr()->getNodeType() == NodeType::TupleAccess) {
      tupleAccessAliasingCheck(seenVarMap, args[i]->getExpr(), isVar);
    } else if (args[i]->getExpr()->getNodeType() == NodeType::StructAccess) {
      structAccessAliasingCheck(seenVarMap, args[i]->getExpr(), isVar);
    } else if (args[i]->getExpr()->getNodeType() == NodeType::ArrayAccess) {
      arrayAccessAliasingCheck(seenVarMap, args[i]->getExpr(), isVar);
    }
  }
}

void ValidationWalker::validateTupleAccessInferredTypes(
    std::shared_ptr<expressions::TupleAccessAst> ctx) {
  // Get the tuple's variable symbol
  const auto tupleSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (!tupleSymbol) {
    throw SymbolError(ctx->getLineNumber(), "Tuple identifier symbol is not a variable");
  }
  const auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(tupleSymbol->getType());
  if (!tupleTypeSymbol) {
    throw TypeError(ctx->getLineNumber(), "Identifier symbol is not a tuple type");
  }
  const int32_t fieldIndex = ctx->getFieldIndex();
  const size_t tupleSize = tupleTypeSymbol->getResolvedTypes().size();

  if (fieldIndex == 0 || fieldIndex > static_cast<int32_t>(tupleSize)) {
    throw SizeError(ctx->getLineNumber(), "Invalid tuple index");
  }
}

bool ValidationWalker::isLiteralExpression(
    const std::shared_ptr<expressions::ExpressionAst> &expr) {
  if (!expr) {
    return false;
  }
  const auto nodeType = expr->getNodeType();
  // Check if it's a primitive literal
  if (nodeType == NodeType::IntegerLiteral || nodeType == NodeType::RealLiteral ||
      nodeType == NodeType::CharLiteral || nodeType == NodeType::BoolLiteral) {
    return true;
  }
  // Check if it's a tuple literal with all literal elements
  if (nodeType == NodeType::TupleLiteral) {
    auto tupleLiteral = std::dynamic_pointer_cast<expressions::TupleLiteralAst>(expr);
    for (const auto &element : tupleLiteral->getElements()) {
      if (!isLiteralExpression(element)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

std::shared_ptr<types::DataTypeAst>
createDataTypeFromSymbol(const std::shared_ptr<symTable::Type> &symbolType, antlr4::Token *token) {
  if (!symbolType || !token)
    return nullptr;
  const auto name = symbolType->getName();
  if (name == "integer")
    return std::make_shared<types::IntegerTypeAst>(token);
  if (name == "real")
    return std::make_shared<types::RealTypeAst>(token);
  if (name == "character")
    return std::make_shared<types::CharacterTypeAst>(token);
  if (name == "boolean")
    return std::make_shared<types::BooleanTypeAst>(token);
  if (name.substr(0, 5) == "array") {
    const auto arrayType = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(symbolType);
    if (!arrayType)
      return nullptr;
    auto dataType = std::make_shared<types::ArrayTypeAst>(token);
    dataType->setType(createDataTypeFromSymbol(arrayType->getType(), token));
    return dataType;
  }
  if (name.substr(0, 6) == "vector") {
    const auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(symbolType);
    if (!vectorType)
      return nullptr;
    auto dataType = std::make_shared<types::VectorTypeAst>(token);
    dataType->setElementType(createDataTypeFromSymbol(vectorType->getType(), token));
    return dataType;
  }
  return nullptr;
}

void ensureLiteralDataType(const std::shared_ptr<expressions::ArrayLiteralAst> &literal,
                           const std::shared_ptr<symTable::Type> &targetType) {
  if (!literal || !targetType)
    return;
  auto literalDataType =
      std::dynamic_pointer_cast<types::ArrayTypeAst>(literal->getInferredDataType());
  if (!literalDataType) {
    literalDataType = std::make_shared<types::ArrayTypeAst>(literal->token);
    literal->setInferredDataType(literalDataType);
  }
  if (literalDataType->getType())
    return;

  if (const auto targetArray = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(targetType)) {
    literalDataType->setType(createDataTypeFromSymbol(targetArray->getType(), literal->token));
  } else if (const auto targetVector =
                 std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(targetType)) {
    literalDataType->setType(createDataTypeFromSymbol(targetVector->getType(), literal->token));
  }
}

void ValidationWalker::ensureArrayLiteralType(
    const std::shared_ptr<expressions::ExpressionAst> &expr,
    const std::shared_ptr<symTable::Type> &targetType) {
  if (!expr || !targetType)
    return;

  if (expr->getNodeType() == NodeType::ArrayAccess) {
    const auto arrayAccess = std::dynamic_pointer_cast<expressions::ArrayAccessAst>(expr);
    if (!arrayAccess)
      return;
    const auto indexExpr = arrayAccess->getElementIndex();
    std::shared_ptr<symTable::Type> arrayInstanceTarget = nullptr;
    if (indexExpr && indexExpr->getNodeType() == NodeType::SingularIndexExpr) {
      auto inferredArrayType = std::make_shared<symTable::ArrayTypeSymbol>("array");
      inferredArrayType->setType(targetType);
      arrayInstanceTarget = inferredArrayType;
    } else if (indexExpr && indexExpr->getNodeType() == NodeType::RangedIndexExpr) {
      arrayInstanceTarget = targetType;
    }
    ensureArrayLiteralType(arrayAccess->getArrayInstance(), arrayInstanceTarget);
    return;
  }

  if (expr->getNodeType() != NodeType::ArrayLiteral)
    return;

  const auto literal = std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(expr);
  auto currentType = expr->getInferredSymbolType();
  const bool alreadyResolved =
      currentType && !std::dynamic_pointer_cast<symTable::EmptyArrayTypeSymbol>(currentType);
  if (alreadyResolved)
    return;

  if (isOfSymbolType(targetType, "array")) {
    expr->setInferredSymbolType(targetType);
    ensureLiteralDataType(literal, targetType);
    return;
  }

  if (isOfSymbolType(targetType, "vector")) {
    const auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(targetType);
    if (!vectorType)
      return;
    const auto literalArrayType = std::make_shared<symTable::ArrayTypeSymbol>("array");
    literalArrayType->setType(vectorType->getType());
    expr->setInferredSymbolType(literalArrayType);
    ensureLiteralDataType(literal, literalArrayType);
  }
}

static void
recordVectorFirstElementSizes(const std::shared_ptr<expressions::ArrayLiteralAst> &literal,
                              size_t depth, std::vector<int> &recordedSizes) {
  if (!literal)
    return;

  if (recordedSizes.size() <= depth) {
    recordedSizes.push_back(static_cast<int>(literal->getElements().size()));
  } else {
    recordedSizes[depth] = static_cast<int>(literal->getElements().size());
  }

  if (!literal->getElements().empty()) {
    const auto nestedLiteral =
        std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(literal->getElements().front());
    if (nestedLiteral)
      recordVectorFirstElementSizes(nestedLiteral, depth + 1, recordedSizes);
  }
}

static void
validateVectorLiteralWithinInferred(const std::vector<int> &limits,
                                    const std::shared_ptr<expressions::ArrayLiteralAst> &literal,
                                    size_t depth, int lineNumber) {
  if (!literal || depth >= limits.size())
    return;
  const auto currentSize = literal->getElements().size();
  if (currentSize > static_cast<size_t>(limits[depth]))
    throw SizeError(lineNumber, "Inferred vector element size mismatch");

  for (const auto &child : literal->getElements()) {
    if (const auto nestedLiteral = std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(child)) {
      validateVectorLiteralWithinInferred(limits, nestedLiteral, depth + 1, lineNumber);
    }
  }
}

void ValidationWalker::inferVectorSize(
    const std::shared_ptr<symTable::VectorTypeSymbol> &vectorType,
    const std::shared_ptr<expressions::ExpressionAst> &expr) {

  if (!vectorType)
    return;

  vectorType->inferredElementSize.clear();
  vectorType->isScalar = true;
  vectorType->isElement2D = false;
  const auto &inferenceFlags = vectorType->getElementSizeInferenceFlags();
  const bool shouldInfer =
      std::any_of(inferenceFlags.begin(), inferenceFlags.end(), [](bool flag) { return flag; });

  const auto arrayLiteral = std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(expr);
  if (!arrayLiteral)
    return;

  const auto &elements = arrayLiteral->getElements();
  vectorType->inferredSize = static_cast<int>(elements.size());
  if (elements.empty())
    return;

  const auto firstElement =
      !elements.empty() ? std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(elements.front())
                        : nullptr;

  if (firstElement) {
    vectorType->isScalar = false;
    if (shouldInfer) {
      recordVectorFirstElementSizes(firstElement, 0, vectorType->inferredElementSize);
      vectorType->isElement2D = vectorType->inferredElementSize.size() > 1;

      for (size_t i = 1; i < elements.size(); ++i) {
        const auto elementLiteral =
            std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(elements[i]);
        if (!elementLiteral)
          continue;
        validateVectorLiteralWithinInferred(vectorType->inferredElementSize, elementLiteral, 0,
                                            elements[i]->getLineNumber());
      }
    }
  } else {
    vectorType->isScalar = true;
    vectorType->isElement2D = false;
  }
}

} // namespace gazprea::ast::walkers
