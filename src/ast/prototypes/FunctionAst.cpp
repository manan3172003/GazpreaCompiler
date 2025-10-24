#include <ast/prototypes/FunctionAst.h>

namespace gazprea::ast::prototypes {
NodeType FunctionAst::getNodeType() const { return NodeType::Function; }
std::string FunctionAst::toStringTree() const {
  std::stringstream ss;

  ss << "Function " << proto->getName() << "(";
  for (auto it = proto->getArgs().begin(); it != proto->getArgs().end(); ++it) {
    ss << (*it)->toStringTree();
    if (std::next(it) != proto->getArgs().end()) {
      ss << ", ";
    }
  }
  ss << ")";
  ss << " Returns: " << proto->getType() << " " << token->toString() << "\n";
  ss << "└──" << body->toStringTree();
  return ss.str();
}
} // namespace gazprea::ast::prototypes