#include <ast/statements/StatementAst.h>

namespace gazprea::ast::statements {
class BreakAst : public StatementAst {
public:
  BreakAst(antlr4::Token *token) : StatementAst(token) {}
  NodeType getNodeType() const override;
  std::string toStringTree(std::string prefix) const override;
};
} // namespace gazprea::ast::statements