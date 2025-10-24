#include <ast/prototypes/PrototypeAst.h>

namespace gazprea::ast::prototypes {
NodeType PrototypeAst::getNodeType() const { return NodeType::Prototype; }
std::string PrototypeAst::toStringTree() const {
  std::stringstream ss;
  return ss.str();
}
} // namespace gazprea::ast::prototypes