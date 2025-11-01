#include "CompileTimeExceptions.h"
#include "ast/types/AliasTypeAst.h"
#include "symTable/VariableSymbol.h"

#include <ast/walkers/ValidationWalker.h>

namespace gazprea::ast::walkers {

void ValidationWalker::validateVariableAssignmentTypes(
    std::shared_ptr<statements::IdentifierLeftAst> ctx,
    std::shared_ptr<symTable::Type> exprTypeSymbol) {
  const auto lValSymbolType =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol())->getType();
  if (not typesMatch(lValSymbolType, exprTypeSymbol))
    throw TypeError(ctx->getLineNumber(), "Type mismatch");
}

void ValidationWalker::validateTupleElementAssignmentTypes(
    std::shared_ptr<statements::TupleElementAssignAst> ctx,
    std::shared_ptr<symTable::Type> exprTypeSymbol) {
  const auto lValSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto tupleType =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(lValSymbol->getType());
  const auto tupleSubType = tupleType->getResolvedTypes()[ctx->getFieldIndex() - 1];

  if (not typesMatch(tupleSubType, exprTypeSymbol))
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
    } // TODO: add more assignment types in part 2
  }
}

bool ValidationWalker::typesMatch(const std::shared_ptr<symTable::Type> &destination,
                                  const std::shared_ptr<symTable::Type> &source) {
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

  return symbolType->getName() == typeName;
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
  return isNumericType(left->getInferredSymbolType()) &&
         isNumericType(right->getInferredSymbolType());
}

// helper to check for var parameter aliasing
void ValidationWalker::checkVarArgs(const std::shared_ptr<prototypes::PrototypeAst> &proto,
                                    const std::vector<std::shared_ptr<expressions::ArgAst>> &args,
                                    int lineNumber) const {
  auto params = proto->getParams();
  // unordered map to track vars
  std::unordered_map<std::string, bool> seenVarMap;
  for (size_t i = 0; i < args.size(); i++) {
    auto param = std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(params[i]);
    bool isVar = (param->getQualifier() == Qualifier::Var);
    std::string varIdentifier;
    if (args[i]->getExpr()->getNodeType() == NodeType::Identifier) {
      auto identifier = std::dynamic_pointer_cast<expressions::IdentifierAst>(args[i]->getExpr());
      varIdentifier = identifier->getName();
    } else if (args[i]->getExpr()->getNodeType() == NodeType::TupleAccess) {
      // Tuple access: t[0] - just track the tuple name "t"
      auto tupleAccess = std::dynamic_pointer_cast<expressions::TupleAccessAst>(args[i]->getExpr());
      varIdentifier = tupleAccess->getTupleName();
    } else {
      // TODO: Handle array slices in future
      continue;
    }
    // Check if this identifier was already seen
    if (seenVarMap.find(varIdentifier) != seenVarMap.end()) {
      if (seenVarMap[varIdentifier] || isVar) {
        throw AliasingError(lineNumber, "Variable aliasing error: var parameter cannot share "
                                        "a variable with another parameter");
      }
    } else {
      // First time seeing this identifier
      seenVarMap[varIdentifier] = isVar;
    }
  }
}

} // namespace gazprea::ast::walkers