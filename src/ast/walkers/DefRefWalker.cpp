#include "CompileTimeExceptions.h"
#include "ast/expressions/StructLiteralAst.h"
#include "ast/types/AliasTypeAst.h"
#include "ast/types/ArrayTypeAst.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/TypealiasSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"

#include <ast/walkers/DefRefWalker.h>
#include <symTable/MethodSymbol.h>

namespace gazprea::ast::walkers {
void DefRefWalker::throwIfUndeclaredSymbol(int lineNumber, std::shared_ptr<symTable::Symbol> sym) {
  if (!sym)
    throw SymbolError(lineNumber, "Use of undeclared symbol");
}
void DefRefWalker::throwGlobalError(std::shared_ptr<Ast> ctx) const {
  auto curScope = symTab->getCurrentScope();
  if (curScope->getScopeType() != symTable::ScopeType::Global)
    throw GlobalError(ctx->getLineNumber(), "Symbol can only be defined in Global");
}

void DefRefWalker::throwDuplicateSymbolError(std::shared_ptr<Ast> ctx, const std::string &name,
                                             std::shared_ptr<symTable::Scope> curScope,
                                             bool isType) {
  if (isType) {
    if (curScope->getTypeSymbol(name))
      throw SymbolError(ctx->getLineNumber(), "Duplicate symbol");
  } else {
    if (curScope->getSymbol(name))
      throw SymbolError(ctx->getLineNumber(), "Duplicate symbol");
  }
}

bool DefRefWalker::isTupleTypeMatch(const std::shared_ptr<symTable::TupleTypeSymbol> &destination,
                                    const std::shared_ptr<symTable::TupleTypeSymbol> &source) {
  const auto destSubTypes = destination->getResolvedTypes();
  const auto sourceSubTypes = source->getResolvedTypes();
  if (destSubTypes.size() != sourceSubTypes.size())
    return false;

  for (size_t i = 0; i < destSubTypes.size(); i++) {
    if (not exactTypeMatch(destSubTypes[i], sourceSubTypes[i]))
      return false;
  }
  return true;
}

bool DefRefWalker::exactTypeMatch(const std::shared_ptr<symTable::Type> &destination,
                                  const std::shared_ptr<symTable::Type> &source) {
  if (destination->getName() == "integer" && source->getName() == "integer")
    return true;
  if (destination->getName() == "real" && source->getName() == "real")
    return true;
  if (destination->getName() == "character" && source->getName() == "character")
    return true;
  if (destination->getName() == "boolean" && source->getName() == "boolean")
    return true;
  if (destination->getName() == "tuple" && source->getName() == "tuple") {
    const auto destTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(destination);
    const auto sourceTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(source);
    return isTupleTypeMatch(destTuple, sourceTuple);
  }
  if (destination->getName() == "struct" && source->getName() == "struct") {
    const auto destStruct = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(destination);
    const auto sourceStruct = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(source);
    return destStruct->getStructName() == sourceStruct->getStructName();
  }
  return false;
}

void DefRefWalker::compareProtoTypes(std::shared_ptr<prototypes::PrototypeAst> prev,
                                     std::shared_ptr<prototypes::PrototypeAst> cur,
                                     symTable::ScopeType scopeType) {
  const auto prevParamsList = prev->getParams();
  const auto curParamsList = cur->getParams();

  if (prevParamsList.size() != curParamsList.size())
    throw DefinitionError(cur->getLineNumber(), "Wrong number of parameters");

  for (size_t i = 0; i < prevParamsList.size(); ++i) {
    if (scopeType == symTable::ScopeType::Procedure) {
      const auto prevParam =
          std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(prevParamsList[i]);
      const auto prevParamType =
          std::dynamic_pointer_cast<symTable::Type>(prevParam->getParamType()->getSymbol());
      const auto curParam =
          std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(curParamsList[i]);
      const auto curParamType =
          std::dynamic_pointer_cast<symTable::Type>(curParam->getParamType()->getSymbol());

      if (prevParam->getQualifier() != curParam->getQualifier())
        throw DefinitionError(cur->getLineNumber(), "Parameter qualifier does not match");
      if (not exactTypeMatch(prevParamType, curParamType))
        throw DefinitionError(cur->getLineNumber(), "Parameter type does not match");
    }

    if (scopeType == symTable::ScopeType::Function) {
      const auto prevParam =
          std::dynamic_pointer_cast<prototypes::FunctionParamAst>(prevParamsList[i]);
      const auto prevParamType =
          std::dynamic_pointer_cast<symTable::Type>(prevParam->getParamType()->getSymbol());
      const auto curParam =
          std::dynamic_pointer_cast<prototypes::FunctionParamAst>(curParamsList[i]);
      const auto curParamType =
          std::dynamic_pointer_cast<symTable::Type>(curParam->getParamType()->getSymbol());

      if (not exactTypeMatch(prevParamType, curParamType))
        throw DefinitionError(cur->getLineNumber(), "Parameter type does not match");
    }
  }
  if (scopeType == symTable::ScopeType::Procedure && prev->getReturnType() == nullptr &&
      cur->getReturnType() == nullptr)
    return;
  if (scopeType == symTable::ScopeType::Function && prev->getReturnType() == nullptr &&
      cur->getReturnType() == nullptr)
    throw DefinitionError(cur->getLineNumber(), "Function must have a return type");

  if (prev->getReturnType() && cur->getReturnType()) {
    const auto prevReturnType =
        std::dynamic_pointer_cast<symTable::Type>(prev->getReturnType()->getSymbol());
    const auto curReturnType =
        std::dynamic_pointer_cast<symTable::Type>(cur->getReturnType()->getSymbol());
    if (not exactTypeMatch(prevReturnType, curReturnType))
      throw DefinitionError(cur->getLineNumber(), "Return type does not match");
  } else {
    throw DefinitionError(cur->getLineNumber(), "Return type does not match");
  }
}

std::shared_ptr<symTable::Type>
DefRefWalker::resolvedType(int lineNumber, const std::shared_ptr<types::DataTypeAst> &dataType) {
  std::shared_ptr<symTable::Type> type;
  auto globalScope = symTab->getGlobalScope();
  switch (dataType->getNodeType()) {
  case NodeType::IntegerType:
    type = std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("integer"));
    break;
  case NodeType::RealType:
    type = std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("real"));
    break;
  case NodeType::CharType:
    type = std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("character"));
    break;
  case NodeType::BoolType:
    type = std::dynamic_pointer_cast<symTable::Type>(globalScope->resolveType("boolean"));
    break;
  case NodeType::AliasType: {
    const auto aliasTypeNode = std::dynamic_pointer_cast<types::AliasTypeAst>(dataType);
    const auto aliasSymType = dataType->getScope()->resolveType(aliasTypeNode->getAlias());
    type = std::dynamic_pointer_cast<symTable::Type>(aliasSymType);
    break;
  }
  case NodeType::ArrayType: {
    type = std::dynamic_pointer_cast<symTable::Type>(dataType->getSymbol());
    break;
  }
  case NodeType::VectorType: {
    type = std::dynamic_pointer_cast<symTable::Type>(dataType->getSymbol());
    break;
  }
  case NodeType::TupleType: {
    type = std::dynamic_pointer_cast<symTable::Type>(dataType->getSymbol());
    break;
  }
  case NodeType::StructType: {
    type = std::dynamic_pointer_cast<symTable::Type>(dataType->getSymbol());
    break;
  }
  default:
    break;
  }
  if (!type)
    throw TypeError(lineNumber, "Undeclared type used");
  return type;
}

std::shared_ptr<expressions::ExpressionAst>
DefRefWalker::createDefaultLiteral(const std::shared_ptr<symTable::Type> &type,
                                   antlr4::Token *token) {
  if (!type) {
    return nullptr;
  }
  if (type->getName() == "integer")
    return std::make_shared<expressions::IntegerLiteralAst>(token, 0);
  if (type->getName() == "real")
    return std::make_shared<expressions::RealLiteralAst>(token, 0.0);
  if (type->getName() == "character") {
    auto charLiteral = std::make_shared<expressions::CharLiteralAst>(token);
    charLiteral->setValue('\0');
    return charLiteral;
  }
  if (type->getName() == "boolean") {
    auto boolLiteral = std::make_shared<expressions::BoolLiteralAst>(token);
    boolLiteral->setValue(false);
    return boolLiteral;
  }
  if (type->getName() == "tuple") {
    auto tupleLiteral = std::make_shared<expressions::TupleLiteralAst>(token);
    const auto tupleTypeAst = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(type);

    for (const auto &elementType : tupleTypeAst->getResolvedTypes()) {
      auto elementDefault = createDefaultLiteral(elementType, token);
      if (elementDefault) {
        tupleLiteral->addElement(elementDefault);
      }
    }
    return tupleLiteral;
  }
  if (type->getName().substr(0, 6) == "vector") {
    const auto vectorType = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(type);
    if (vectorType) {
      auto arrayLiteral = std::make_shared<expressions::ArrayLiteralAst>(token);
      return arrayLiteral;
    }
  }
  if (type->getName() == "struct") {
    const auto structType = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(type);
    const auto structLiteral = std::make_shared<expressions::StructLiteralAst>(token);
    structLiteral->setStructTypeName(structType->getStructName());
    for (const auto &elementType : structType->getResolvedTypes()) {
      const auto elementDefault = createDefaultLiteral(elementType, token);
      if (elementDefault) {
        structLiteral->addElement(elementDefault);
      }
    }
    return structLiteral;
  }
  return nullptr;
}

std::any DefRefWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  ctx->setScope(symTab->getGlobalScope());
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}
std::any DefRefWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getExpr());
  visit(ctx->getLVal());
  return {};
}
std::any DefRefWalker::visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) {
  throwDuplicateSymbolError(ctx, ctx->getName(), symTab->getCurrentScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  const auto varSymbol =
      std::make_shared<symTable::VariableSymbol>(ctx->getName(), ctx->getQualifier());
  if (ctx->getType()) {
    visit(ctx->getType());
    varSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getType()));
  }

  if (not ctx->getExpr() && ctx->getType()) {
    // integer[*] needs to have an expression
    if (auto arrayType = std::dynamic_pointer_cast<types::ArrayTypeAst>(ctx->getType())) {
      for (auto size : arrayType->getSizes()) {
        if (size)
          throw SyntaxError(ctx->getLineNumber(), "Inferred Array cannot be empty");
      }
    }
    // Set everything to base (false, '\0', 0, 0.0)
    // normal primitives (boolean, character, integer, real)
    // tuples
    const auto type = std::dynamic_pointer_cast<symTable::Type>(ctx->getType()->getSymbol());
    auto defaultExpr = createDefaultLiteral(type, ctx->token);
    ctx->setExpr(defaultExpr);
    visit(ctx->getExpr());
  }
  visit(ctx->getExpr());
  varSymbol->setDef(ctx);
  symTab->getCurrentScope()->defineSymbol(varSymbol);
  ctx->setSymbol(varSymbol);
  return {};
}
std::any
DefRefWalker::visitStructDeclaration(std::shared_ptr<statements::StructDeclarationAst> ctx) {
  visit(ctx->getType());
  const auto typeSymbol = ctx->getType()->getSymbol();
  typeSymbol->setDef(ctx);
  ctx->setSymbol(typeSymbol);
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  visit(ctx->getLeft());
  visit(ctx->getRight());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  auto newScope = std::make_shared<symTable::LocalScope>();
  ctx->setScope(symTab->getCurrentScope());
  symTab->pushScope(newScope);
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  symTab->popScope();
  return {};
}
std::any DefRefWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) {
  visit(ctx->getCondition());
  visit(ctx->getThenBody());
  ctx->setScope(symTab->getCurrentScope());
  // setting the body's scope type to Conditional type
  if (not ctx->getThenBody()->getChildren().empty()) {
    const auto thenBodyScope = std::dynamic_pointer_cast<symTable::LocalScope>(
        ctx->getThenBody()->getChildren()[0]->getScope());
    thenBodyScope->setScopeType(symTable::ScopeType::Conditional);
  }
  if (ctx->getElseBody()) {
    visit(ctx->getElseBody());

    // setting the body's scope type to Conditional type
    if (not ctx->getElseBody()->getChildren().empty()) {
      const auto elseBodyScope = std::dynamic_pointer_cast<symTable::LocalScope>(
          ctx->getElseBody()->getChildren()[0]->getScope());
      elseBodyScope->setScopeType(symTable::ScopeType::Conditional);
    }
  }
  return {};
}
std::any DefRefWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getLVal());
  return {};
}
std::any DefRefWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getExpression());
  return {};
}
std::any DefRefWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  throwGlobalError(ctx);
  const auto symbol = symTab->getCurrentScope()->getSymbol(ctx->getProto()->getName());
  const auto oldMethodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(symbol);
  // if a symbol exists in the same scope with the same name but isn't a method symbol, throw
  // error
  if (symbol && not oldMethodSymbol)
    throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);

  // if the method symbol is a function throw error
  if (oldMethodSymbol && oldMethodSymbol->getScopeType() == symTable::ScopeType::Function)
    throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);

  // if the method symbol is a procedure
  if (oldMethodSymbol && oldMethodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
    const auto prevDecl =
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(oldMethodSymbol->getDef());
    // if the previous declaration has a body throw symbol error
    if (prevDecl->getBody())
      throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);
    // if neither the previous nor the current declaration have a body throw symbol error
    if (not prevDecl->getBody() && not ctx->getBody())
      throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);
  }

  ctx->setScope(symTab->getCurrentScope());

  // Checks if definition has parameter names
  if (ctx->getBody())
    for (const auto &param : ctx->getProto()->getParams()) {
      const auto paramNode = std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(param);
      if (paramNode->getName().empty())
        throw SyntaxError(param->getLineNumber(), "Parameter must have name in definition");
    }

  if (not symbol) {
    const auto methodSymbol = std::make_shared<symTable::MethodSymbol>(
        ctx->getProto()->getName(), ctx->getProto()->getProtoType());
    ctx->getProto()->setSymbol(methodSymbol);
    methodSymbol->setDef(ctx);
    symTab->getCurrentScope()->defineSymbol(methodSymbol);
    symTab->pushScope(methodSymbol);
    visit(ctx->getProto());
    if (ctx->getBody())
      visit(ctx->getBody());
    symTab->popScope();

  } else if (oldMethodSymbol && oldMethodSymbol->getScopeType() == symTable::ScopeType::Procedure &&
             ctx->getBody()) {
    const auto newMethodSymbol = std::make_shared<symTable::MethodSymbol>(
        ctx->getProto()->getName(), ctx->getProto()->getProtoType());
    const auto prevDecl =
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(oldMethodSymbol->getDef());
    ctx->getProto()->setSymbol(newMethodSymbol);
    prevDecl->getProto()->setSymbol(newMethodSymbol);
    newMethodSymbol->setDef(ctx);
    symTab->pushScope(newMethodSymbol);

    // validate prev and current declarations have same prototype
    // prototypes should match
    visit(ctx->getProto());
    compareProtoTypes(prevDecl->getProto(), ctx->getProto(), symTable::ScopeType::Procedure);
    visit(ctx->getBody());
    symTab->popScope();

    // replace the old methodSymbol with the newMethod symbol which has the definition and body
    (std::dynamic_pointer_cast<symTable::BaseScope>(symTab->getCurrentScope())
         ->getSymbols())[prevDecl->getProto()->getName()] = newMethodSymbol;
  }
  return {};
}
std::any DefRefWalker::visitProcedureParams(std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  if (not ctx->getName().empty())
    throwDuplicateSymbolError(ctx, ctx->getName(), symTab->getCurrentScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  const auto varSymbol =
      std::make_shared<symTable::VariableSymbol>(ctx->getName(), ctx->getQualifier());

  visit(ctx->getParamType());
  varSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getParamType()));
  varSymbol->setDef(ctx);

  symTab->getCurrentScope()->defineSymbol(varSymbol);
  ctx->setSymbol(varSymbol);
  return {};
}
std::any DefRefWalker::visitProcedureCall(std::shared_ptr<statements::ProcedureCallAst> ctx) {
  for (const auto &args : ctx->getArgs()) {
    visit(args);
  }
  ctx->setScope(symTab->getCurrentScope());

  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      symTab->getCurrentScope()->resolveSymbol(ctx->getName()));
  throwIfUndeclaredSymbol(ctx->getLineNumber(), methodSymbol);

  ctx->setSymbol(methodSymbol);
  return {};
}
std::any DefRefWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  if (ctx->getExpr())
    visit(ctx->getExpr());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any
DefRefWalker::visitTupleElementAssign(std::shared_ptr<statements::TupleElementAssignAst> ctx) {
  auto symbol = symTab->getCurrentScope()->resolveSymbol(ctx->getTupleName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setScope(symTab->getCurrentScope());
  ctx->setSymbol(symbol);
  return {};
}
std::any
DefRefWalker::visitStructElementAssign(std::shared_ptr<statements::StructElementAssignAst> ctx) {
  const auto symbol = symTab->getCurrentScope()->resolveSymbol(ctx->getStructName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setScope(symTab->getCurrentScope());
  ctx->setSymbol(symbol);
  return {};
}
std::any
DefRefWalker::visitTupleUnpackAssign(std::shared_ptr<statements::TupleUnpackAssignAst> ctx) {
  for (const auto &lVal : ctx->getLVals()) {
    visit(lVal);
  }
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) {
  auto symbol = symTab->getCurrentScope()->resolveSymbol(ctx->getTupleName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setScope(symTab->getCurrentScope());
  ctx->setSymbol(symbol);
  return {};
}
std::any DefRefWalker::visitStructAccess(std::shared_ptr<expressions::StructAccessAst> ctx) {
  const auto symbol = symTab->getCurrentScope()->resolveSymbol(ctx->getStructName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setScope(symTab->getCurrentScope());
  ctx->setSymbol(symbol);
  return {};
}
std::any DefRefWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  for (const auto &element : ctx->getElements()) {
    visit(element);
  }
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitStruct(std::shared_ptr<expressions::StructLiteralAst> ctx) {
  for (const auto &element : ctx->getElements()) {
    visit(element);
  }
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  auto tupleTypeSymbol = std::make_shared<symTable::TupleTypeSymbol>("tuple");
  ctx->setScope(symTab->getCurrentScope());
  for (const auto &subType : ctx->getTypes()) {
    visit(subType);
    tupleTypeSymbol->addResolvedType(resolvedType(ctx->getLineNumber(), subType));
  }
  tupleTypeSymbol->setDef(ctx);
  ctx->setSymbol(tupleTypeSymbol);
  return {};
}
std::any DefRefWalker::visitAliasType(std::shared_ptr<types::AliasTypeAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  const auto typeSymbol =
      std::dynamic_pointer_cast<symTable::Symbol>(resolvedType(ctx->getLineNumber(), ctx));
  ctx->setSymbol(typeSymbol);
  return {};
}
std::any DefRefWalker::visitIntegerType(std::shared_ptr<types::IntegerTypeAst> ctx) {
  const auto typeSymbol =
      std::dynamic_pointer_cast<symTable::Symbol>(resolvedType(ctx->getLineNumber(), ctx));
  ctx->setSymbol(typeSymbol);
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitRealType(std::shared_ptr<types::RealTypeAst> ctx) {
  const auto typeSymbol =
      std::dynamic_pointer_cast<symTable::Symbol>(resolvedType(ctx->getLineNumber(), ctx));
  ctx->setSymbol(typeSymbol);
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitCharacterType(std::shared_ptr<types::CharacterTypeAst> ctx) {
  const auto typeSymbol =
      std::dynamic_pointer_cast<symTable::Symbol>(resolvedType(ctx->getLineNumber(), ctx));
  ctx->setSymbol(typeSymbol);
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitBooleanType(std::shared_ptr<types::BooleanTypeAst> ctx) {
  const auto typeSymbol =
      std::dynamic_pointer_cast<symTable::Symbol>(resolvedType(ctx->getLineNumber(), ctx));
  ctx->setSymbol(typeSymbol);
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitArrayType(std::shared_ptr<types::ArrayTypeAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getType());
  for (auto size : ctx->getSizes())
    visit(size);
  auto arrayTypeSymbol = std::make_shared<symTable::ArrayTypeSymbol>("array");
  arrayTypeSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getType()));
  arrayTypeSymbol->setDef(ctx);

  for (const auto &sizeExpr : ctx->getSizes()) {
    visit(sizeExpr);
  }

  ctx->setSymbol(arrayTypeSymbol);
  return {};
}
std::any DefRefWalker::visitVectorType(std::shared_ptr<types::VectorTypeAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getElementType());
  const auto vectorTypeSymbol = std::make_shared<symTable::VectorTypeSymbol>("vector");
  vectorTypeSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getElementType()));
  if (const auto elementArrayType =
          std::dynamic_pointer_cast<types::ArrayTypeAst>(ctx->getElementType())) {
    vectorTypeSymbol->setElementSizeInferenceFlags(elementArrayType->isSizeInferred());
  }
  vectorTypeSymbol->setDef(ctx);
  ctx->setSymbol(vectorTypeSymbol);
  return {};
}
std::any DefRefWalker::visitStructType(std::shared_ptr<types::StructTypeAst> ctx) {
  throwDuplicateSymbolError(ctx, ctx->getStructName(), symTab->getGlobalScope(), true);
  throwDuplicateSymbolError(ctx, ctx->getStructName(), symTab->getGlobalScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  const auto structTypeSymbol = std::make_shared<symTable::StructTypeSymbol>("struct");
  structTypeSymbol->setStructName(ctx->getStructName());
  const auto unresolvedTypes = ctx->getTypes();
  for (size_t i = 0; i < unresolvedTypes.size(); i++) {
    visit(unresolvedTypes[i]);
    const auto resolvedSubType =
        resolvedType(unresolvedTypes[i]->getLineNumber(), unresolvedTypes[i]);
    const auto elementName = ctx->getElementName(i + 1);
    structTypeSymbol->addResolvedType(elementName, resolvedSubType);
    structTypeSymbol->addUnresolvedType(unresolvedTypes[i]);
  }

  structTypeSymbol->setDef(ctx);
  ctx->setSymbol(structTypeSymbol);
  symTab->getCurrentScope()->defineTypeSymbol(structTypeSymbol);
  symTab->getCurrentScope()->defineSymbol(structTypeSymbol);

  return {};
}
std::any DefRefWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  throwDuplicateSymbolError(ctx, ctx->getAlias(), symTab->getGlobalScope(), true);
  ctx->setScope(symTab->getCurrentScope());
  auto typealiasSymbol = std::make_shared<symTable::TypealiasSymbol>(ctx->getAlias());
  visit(ctx->getType());
  typealiasSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getType()));
  symTab->getGlobalScope()->defineTypeSymbol(typealiasSymbol);
  typealiasSymbol->setDef(ctx);
  ctx->setSymbol(typealiasSymbol);
  return {};
}
std::any DefRefWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  throwGlobalError(ctx);
  const auto symbol = symTab->getCurrentScope()->getSymbol(ctx->getProto()->getName());
  const auto oldMethodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(symbol);
  // if a symbol exists in the same scope with the same name but isn't a method symbol, throw
  // error
  if (symbol && not oldMethodSymbol)
    throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);

  // if the method symbol is a procedure throw error
  if (oldMethodSymbol && oldMethodSymbol->getScopeType() == symTable::ScopeType::Procedure)
    throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);

  // if the method symbol is a function
  if (oldMethodSymbol && oldMethodSymbol->getScopeType() == symTable::ScopeType::Function) {
    const auto prevDecl =
        std::dynamic_pointer_cast<prototypes::FunctionAst>(oldMethodSymbol->getDef());
    // if the previous declaration has a body throw symbol error
    if (prevDecl->getBody())
      throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);
    // if neither the previous nor the current declaration have a body throw symbol error
    if (not prevDecl->getBody() && not ctx->getBody())
      throwDuplicateSymbolError(ctx, ctx->getProto()->getName(), symTab->getCurrentScope(), false);
  }

  ctx->setScope(symTab->getCurrentScope());

  if (ctx->getBody())
    for (const auto &param : ctx->getProto()->getParams()) {
      const auto paramNode = std::dynamic_pointer_cast<prototypes::FunctionParamAst>(param);
      if (paramNode->getName().empty())
        throw SyntaxError(param->getLineNumber(), "Parameter must have name in definition");
    }

  if (not symbol) {
    const auto methodSymbol = std::make_shared<symTable::MethodSymbol>(
        ctx->getProto()->getName(), ctx->getProto()->getProtoType());
    ctx->getProto()->setSymbol(methodSymbol);
    methodSymbol->setDef(ctx);
    symTab->getCurrentScope()->defineSymbol(methodSymbol);
    symTab->pushScope(methodSymbol);
    visit(ctx->getProto());
    if (ctx->getBody())
      visit(ctx->getBody());
    symTab->popScope();

  } else if (oldMethodSymbol && oldMethodSymbol->getScopeType() == symTable::ScopeType::Function &&
             ctx->getBody()) {
    const auto newMethodSymbol = std::make_shared<symTable::MethodSymbol>(
        ctx->getProto()->getName(), ctx->getProto()->getProtoType());
    const auto prevDecl =
        std::dynamic_pointer_cast<prototypes::FunctionAst>(oldMethodSymbol->getDef());
    ctx->getProto()->setSymbol(newMethodSymbol);
    prevDecl->getProto()->setSymbol(newMethodSymbol);
    newMethodSymbol->setDef(ctx);
    symTab->pushScope(newMethodSymbol);

    // validate prev and current declarations have same prototype
    // prototypes should match
    visit(ctx->getProto());
    compareProtoTypes(prevDecl->getProto(), ctx->getProto(), symTable::ScopeType::Function);
    visit(ctx->getBody());
    symTab->popScope();

    // replace the old methodSymbol with the newMethod symbol which has the definition and body
    (std::dynamic_pointer_cast<symTable::BaseScope>(symTab->getCurrentScope())
         ->getSymbols())[prevDecl->getProto()->getName()] = newMethodSymbol;
  }
  return {};
}
std::any DefRefWalker::visitFunctionParam(std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  if (not ctx->getName().empty())
    throwDuplicateSymbolError(ctx, ctx->getName(), symTab->getCurrentScope(), false);
  ctx->setScope(symTab->getCurrentScope());
  const auto varSymbol =
      std::make_shared<symTable::VariableSymbol>(ctx->getName(), ctx->getQualifier());

  visit(ctx->getParamType());
  varSymbol->setType(resolvedType(ctx->getLineNumber(), ctx->getParamType()));
  varSymbol->setDef(ctx);

  ctx->setSymbol(varSymbol);
  symTab->getCurrentScope()->defineSymbol(varSymbol);
  return {};
}
std::any DefRefWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  for (const auto &param : ctx->getParams()) {
    visit(param);
  }
  if (ctx->getReturnType())
    visit(ctx->getReturnType());

  const auto methodSym = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (ctx->getReturnType() != nullptr) { // void procedures can have empty type
    methodSym->setReturnType(resolvedType(ctx->getLineNumber(), ctx->getReturnType()));
  }

  return {};
}
std::any
DefRefWalker::visitStructFuncCallRouter(std::shared_ptr<expressions::StructFuncCallRouterAst> ctx) {
  const auto fpCallAst = ctx->getFuncProcCallAst();
  const auto resolvedSymbol = symTab->getCurrentScope()->resolveSymbol(ctx->getCallName());

  if (std::dynamic_pointer_cast<symTable::MethodSymbol>(resolvedSymbol)) {
    visit(fpCallAst);
    ctx->setInferredDataType(fpCallAst->getInferredDataType());
    ctx->setInferredSymbolType(fpCallAst->getInferredSymbolType());
    ctx->setScope(fpCallAst->getScope());
    ctx->setSymbol(fpCallAst->getSymbol());
  } else if (std::dynamic_pointer_cast<symTable::StructTypeSymbol>(resolvedSymbol)) {
    const auto structLiteralAst = std::make_shared<expressions::StructLiteralAst>(ctx->token);
    structLiteralAst->setStructTypeName(ctx->getCallName());
    for (const auto &arg : fpCallAst->getArgs()) {
      const auto expr = arg->getExpr();
      structLiteralAst->addElement(expr);
    }

    ctx->setStructLiteralAst(structLiteralAst);

    visit(structLiteralAst);

    ctx->setInferredDataType(structLiteralAst->getInferredDataType());
    ctx->setInferredSymbolType(structLiteralAst->getInferredSymbolType());
    ctx->setScope(structLiteralAst->getScope());
    ctx->setSymbol(structLiteralAst->getSymbol());
  } else {
    throw SymbolError(ctx->getLineNumber(),
                      "Unable to find a struct/function/procedure with this Id");
  }
  return {};
}
std::any DefRefWalker::visitFuncProcCall(std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  for (const auto &args : ctx->getArgs()) {
    visit(args);
  }
  ctx->setScope(symTab->getCurrentScope());
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      symTab->getCurrentScope()->resolveSymbol(ctx->getName()));
  throwIfUndeclaredSymbol(ctx->getLineNumber(), methodSymbol);
  ctx->setSymbol(methodSymbol);
  return {};
}
std::any DefRefWalker::visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getExpr());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  visit(ctx->getTargetType());
  visit(ctx->getExpression());
  ctx->setResolvedTargetType(resolvedType(ctx->getLineNumber(), ctx->getTargetType()));
  return {};
}
std::any DefRefWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  auto symbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(
      symTab->getCurrentScope()->resolveSymbol(ctx->getName()));
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setSymbol(symbol);
  return {};
}
std::any DefRefWalker::visitIdentifierLeft(std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  auto symbol = symTab->getCurrentScope()->resolveSymbol(ctx->getName());
  throwIfUndeclaredSymbol(ctx->getLineNumber(), symbol);
  ctx->setSymbol(symbol);
  return {};
}
std::any DefRefWalker::visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitArray(std::shared_ptr<expressions::ArrayLiteralAst> ctx) {
  for (const auto &element : ctx->getElements()) {
    visit(element);
  }
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
  ctx->setScope(symTab->getCurrentScope());
  if (ctx->getCondition())
    visit(ctx->getCondition());
  visit(ctx->getBody());

  // setting the body's scope type to Loop type
  if (not ctx->getBody()->getChildren().empty()) {
    const auto loopScope = std::dynamic_pointer_cast<symTable::LocalScope>(
        ctx->getBody()->getChildren()[0]->getScope());
    loopScope->setScopeType(symTable::ScopeType::Loop);
  }

  return {};
}
std::any DefRefWalker::visitIteratorLoop(std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
std::any DefRefWalker::visitLenMemberFunc(std::shared_ptr<statements::LenMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitAppendMemberFunc(std::shared_ptr<statements::AppendMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitPushMemberFunc(std::shared_ptr<statements::PushMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
std::any DefRefWalker::visitConcatMemberFunc(std::shared_ptr<statements::ConcatMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  ctx->setScope(symTab->getCurrentScope());
  return {};
}
} // namespace gazprea::ast::walkers
