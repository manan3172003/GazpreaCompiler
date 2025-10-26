#pragma once
#include <string>

namespace gazprea::symTable {
class Type {
public:
  virtual std::string getName() = 0;
  virtual ~Type();
};
} // namespace gazprea::symTable