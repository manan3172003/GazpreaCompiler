#pragma once
#include <string>

namespace gazprea::utils {

// Promotion type codes
enum PromotionCode {
  IDENTITY = 0,          // id - no conversion needed
  BOOL_TO_CHAR = 1,      // '\0' if false, 0x01 otherwise
  BOOL_TO_INT = 2,       // 1 if true, 0 otherwise
  BOOL_TO_REAL = 3,      // 1.0 if true, 0.0 otherwise
  CHAR_TO_BOOL = 4,      // false if '\0', true otherwise
  CHAR_TO_INT = 5,       // ASCII value as integer
  CHAR_TO_REAL = 6,      // ASCII value as real
  INT_TO_BOOL = 7,       // false if 0, true otherwise
  INT_TO_CHAR = 8,       // unsigned integer value mod 256
  INT_TO_REAL = 9,       // real version of integer
  REAL_TO_INT = 10,      // truncate
  NOT_ALLOWED = -1       // N/A - conversion not allowed
};

// Promotion table: promotionTable[from_type][to_type]
// Indices: 0=boolean, 1=character, 2=integer, 3=real
extern int promotionTable[4][4];

// Check if a promotion from one type to another is allowed
bool isPromotable(const std::string &fromType, const std::string &toType);

// Get the promotion code for converting from one type to another
int getPromotionCode(const std::string &fromType, const std::string &toType);

} // namespace gazprea::utils