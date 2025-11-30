#include "symTable/StructTypeSymbol.h"

#include "Colors.h"
namespace gazprea::symTable {

void StructTypeSymbol::setStructName(std::string name) { structName = name; }
std::string StructTypeSymbol::getStructName() { return structName; }
void StructTypeSymbol::addResolvedType(std::string elementName, const std::shared_ptr<Type> &type) {
  resolvedTypes.push_back(type);
  nameToIdx[elementName] = resolvedTypes.size();
  idxToName[resolvedTypes.size()] = elementName;
}
void StructTypeSymbol::addUnresolvedType(const std::shared_ptr<ast::types::DataTypeAst> &type) {
  unresolvedTypes.push_back(type);
}
const std::shared_ptr<Type> &StructTypeSymbol::getResolvedType(std::string elementName) const {
  return resolvedTypes[nameToIdx.at(elementName) - 1];
}
const std::shared_ptr<ast::types::DataTypeAst> &
StructTypeSymbol::getUnresolvedType(std::string elementName) const {
  return unresolvedTypes[nameToIdx.at(elementName) - 1];
}
const std::vector<std::shared_ptr<Type>> &StructTypeSymbol::getResolvedTypes() const {
  return resolvedTypes;
}
const std::vector<std::shared_ptr<ast::types::DataTypeAst>> &
StructTypeSymbol::getUnresolvedTypes() const {
  return unresolvedTypes;
}
size_t StructTypeSymbol::getIdx(const std::string &name) const { return nameToIdx.at(name); }
bool StructTypeSymbol::elementNameExist(const std::string &elementName) const {
  if (nameToIdx.find(elementName) != nameToIdx.end())
    return true;
  return false;
}
std::string StructTypeSymbol::getName() { return "struct"; }
std::string StructTypeSymbol::toString() {
  std::stringstream ss;
  ss << "StructType " << structName << " ( ";

  // Show resolved types in green
  for (size_t i = 0; i < resolvedTypes.size(); ++i) {
    ss << idxToName.at(i + 1) << ":<";
    ss << KGRN << resolvedTypes[i]->getName() << RST;
    ss << ">";
    if (i < resolvedTypes.size() - 1) {
      ss << ", ";
    }
  }

  ss << " )";
  return ss.str();
}

} // namespace gazprea::symTable