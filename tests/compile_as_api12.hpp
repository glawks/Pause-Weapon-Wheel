// Compile-only shim: exercise the source's SDK 1.12 path when only the newer
// local SDK is installed. This does not replace testing with actual 1.12 SDK.
#pragma once

#include "openrac/mod.hpp"
#include "openrac/games/rac1.hpp"

#undef OPENRAC_MOD_API_MINOR
#define OPENRAC_MOD_API_MINOR 12
