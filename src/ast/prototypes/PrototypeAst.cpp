#include <ast/prototypes/PrototypeAst.h>

namespace gazprea::ast::prototypes {
NodeType PrototypeAst::getNodeType() const { return NodeType::Prototype; }
std::string PrototypeAst::toStringTree(std::string prefix) const {
  std::stringstream ss;
  return ss.str();
}
} // namespace gazprea::ast::prototypes