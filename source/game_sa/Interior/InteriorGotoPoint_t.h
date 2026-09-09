#pragma once

#include "Base.h"
#include "Vector.h"

// AI navigation point within an interior, added via Interior_c::AddGotoPt (0x591D20).
struct InteriorGotoPoint_t {
    int8    X;
    int8    Y;
    int16   pad;
    CVector Pos;
};
VALIDATE_SIZE(InteriorGotoPoint_t, 0x10);
