#pragma once
#include <string>

namespace gazprea::types {
class Type {
public:
  virtual std::string getName();
  virtual ~Type();
};
} // namespace gazprea::types