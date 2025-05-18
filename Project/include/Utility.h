#pragma once

#include "Types.h"

#define toI(X) static_cast<int>(X)
#define toF(X) static_cast<float>(X)

extern inline string& trim(string& s);
extern inline string& ltrim(string& s);
extern inline string& rtrim(string& s);