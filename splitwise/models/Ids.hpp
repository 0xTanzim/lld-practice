#ifndef SPLITWISE_IDS_HPP
#define SPLITWISE_IDS_HPP

#include <string>

// Identifiers are strings because they are printed, stored in maps and compared
// by value, and a separate wrapper class per kind would add nothing here. The
// aliases exist so a signature says which kind of id it wants.
using UserId = std::string;
using GroupId = std::string;
using ExpenseId = std::string;

#endif // SPLITWISE_IDS_HPP
