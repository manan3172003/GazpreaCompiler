#include "utils/ValidationUtils.h"

namespace gazprea::utils {
// Promotion table: promotionTable[from_type][to_type]
// Indices: 0=boolean, 1=character, 2=integer, 3=real
//
// Layout:
//          To:   boolean        character      integer        real
// From: boolean    IDENTITY       BOOL_TO_CHAR   BOOL_TO_INT    BOOL_TO_REAL
//       character  CHAR_TO_BOOL   IDENTITY       CHAR_TO_INT    CHAR_TO_REAL
//       integer    INT_TO_BOOL    INT_TO_CHAR    IDENTITY       INT_TO_REAL
//       real       NOT_ALLOWED    NOT_ALLOWED    REAL_TO_INT    IDENTITY
//
int promotionTable[4][4] = {
    // To:  bool      char         int          real
    {IDENTITY, BOOL_TO_CHAR, BOOL_TO_INT, BOOL_TO_REAL}, // From: boolean
    {CHAR_TO_BOOL, IDENTITY, CHAR_TO_INT, CHAR_TO_REAL}, // From: character
    {INT_TO_BOOL, INT_TO_CHAR, IDENTITY, INT_TO_REAL},   // From: integer
    {NOT_ALLOWED, NOT_ALLOWED, REAL_TO_INT, IDENTITY},   // From: real
};

// Helper function to convert type name to index
static int typeNameToIndex(const std::string &typeName) {
  if (typeName == "boolean")
    return 0;
  if (typeName == "character")
    return 1;
  if (typeName == "integer")
    return 2;
  if (typeName == "real")
    return 3;
  return -1;
}

// Check if a promotion from one type to another is allowed
bool isPromotable(const std::string &fromType, const std::string &toType) {
  int fromIdx = typeNameToIndex(fromType);
  int toIdx = typeNameToIndex(toType);

  if (fromIdx == -1 || toIdx == -1)
    return false;

  return promotionTable[fromIdx][toIdx] != NOT_ALLOWED;
}

// Get the promotion code for converting from one type to another
int getPromotionCode(const std::string &fromType, const std::string &toType) {
  int fromIdx = typeNameToIndex(fromType);
  int toIdx = typeNameToIndex(toType);

  if (fromIdx == -1 || toIdx == -1)
    return NOT_ALLOWED;

  return promotionTable[fromIdx][toIdx];
}

} // namespace gazprea::utils