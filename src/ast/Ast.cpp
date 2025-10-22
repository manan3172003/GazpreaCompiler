#include <ast/Ast.h>
namespace gazprea::ast {
Ast::Ast() {}
Ast::Ast(antlr4::Token *token) {}
Ast::Ast(size_t tokenType) {}
bool Ast::isNil() const {}
std::string Ast::typeToString(types::Type t) {}
NodeType Ast::getNodeType() const {}
void Ast::addChild(std::any t) {}
template <class T> void Ast::addChild(std::any t) {}
void Ast::addChild(std::shared_ptr<Ast> t) {}
std::string Ast::toString() const {}
std::string Ast::toStringTree() const {}
Ast::~Ast() {}
} // namespace gazprea::ast