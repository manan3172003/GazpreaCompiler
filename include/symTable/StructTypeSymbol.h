#pragma once
#include "Symbol.h"
#include "ast/types/AliasTypeAst.h"

#include <vector>

namespace gazprea::symTable {
class StructTypeSymbol final : public Type, public Symbol {

  std::string structName;
  std::unordered_map<std::string, size_t> nameToIdx;
  std::unordered_map<size_t, std::string> idxToName;
  std::vector<std::shared_ptr<Type>> resolvedTypes;
  std::vector<std::shared_ptr<ast::types::DataTypeAst>> unresolvedTypes;

public:
  explicit StructTypeSymbol(const std::string &name) : Symbol(name) {}
  void setStructName(std::string name);
  void addResolvedType(std::string elementName, const std::shared_ptr<Type> &type);
  void addUnresolvedType(const std::shared_ptr<ast::types::DataTypeAst> &type);
  std::string getStructName();
  const std::shared_ptr<Type> &getResolvedType(std::string elementName) const;
  const std::vector<std::shared_ptr<Type>> &getResolvedTypes() const;
  const std::shared_ptr<ast::types::DataTypeAst> &getUnresolvedType(std::string elementName) const;
  const std::vector<std::shared_ptr<ast::types::DataTypeAst>> &getUnresolvedTypes() const;
  bool elementNameExist(const std::string &elementName) const;
  std::string getName() override;
  std::string toString() override;
};
} // namespace gazprea::symTable