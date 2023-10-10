#include "fn_ParseOpaqueEnumDeclaration.h"

#include "fn_CompactSpaces.h"
#include "fn_EatBlank.h"
#include "fn_EatSpace.h"
#include "fn_EatWhileNot.h"
#include "fn_ExtractIdentifier.h"
#include "fn_Trim.h"
#include "fn_TryEatPrefix.h"

#include <fmt/std.h>

#include "s/fn_ParseOpaqueEnumDeclaration.h"
