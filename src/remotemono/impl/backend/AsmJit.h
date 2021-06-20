#pragma once

#include "../../config.h"

#ifndef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#error "BlackBone backend currently needs to be enabled for AsmJit to work."
#endif

// TODO: Find a better way to look for AsmJit. Right now we always depend on BlackBone.
#include <BlackBone/Asm/AsmVariant.hpp>
