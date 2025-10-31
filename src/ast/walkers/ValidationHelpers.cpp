#include "CompileTimeExceptions.h"
#include "ast/types/AliasTypeAst.h"

#include <ast/walkers/ValidationWalker.h>

namespace gazprea::ast::walkers {

// DO NOT USE FOR BINARY OP COMPARISONS
void ValidationWalker::validateTuple(std::shared_ptr<Ast> ctx,
                                     const std::shared_ptr<symTable::TupleTypeSymbol> &promoteFrom,
                                     const std::shared_ptr<symTable::TupleTypeSymbol> &promoteTo) {
  const auto promoteFromResolvedTypes = promoteFrom->getResolvedTypes();
  const auto promoteToResolvedTypes = promoteTo->getResolvedTypes();

  if (promoteFromResolvedTypes.size() != promoteToResolvedTypes.size())
    throw SizeError(ctx->getLineNumber(), "Tuple sizes do not match");

  for (size_t i = 0; i < promoteFromResolvedTypes.size(); i++) {
    if (promoteFromResolvedTypes[i]->getName() == promoteToResolvedTypes[i]->getName())
      continue;
    if (promoteFromResolvedTypes[i]->getName() == "integer" &&
        promoteToResolvedTypes[i]->getName() == "real")
      continue;
    if (promoteFromResolvedTypes[i]->getName() != promoteToResolvedTypes[i]->getName())
      throw TypeError(ctx->getLineNumber(), "Type mismatch");
  }
}

bool ValidationWalker::isOfSymbolType(const std::shared_ptr<symTable::Type> &symbolType,
                                      const std::string &typeName) {
  if (!symbolType)
    throw std::runtime_error("SymbolType should not be null");

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