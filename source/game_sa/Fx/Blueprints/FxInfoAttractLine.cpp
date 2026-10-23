#include "StdInc.h"

#include "FxInfoAttractLine.h"

// 0x4A57C0
FxInfoAttractLine_c::FxInfoAttractLine_c() : FxInfo_c() {
    m_nType = FX_INFO_ATTRACTLINE_DATA;
    m_InterpInfo.Allocate(7);
}

// 0x4A5830
void FxInfoAttractLine_c::Load(FILESTREAM file, int32 version) {
    m_InterpInfo.Load(file);
}

// 0x4A4880
void FindClosestPtOnLine(CVector& out, CVector& lineA, CVector& lineB, CVector& point) {
    const auto lineDir    = lineB - lineA;
    const auto pointToA   = point - lineA;
    const float t         = DotProduct(pointToA, lineDir) / lineDir.SquaredMagnitude();
    out = lineDir * std::clamp(t, 0.0f, 1.0f) + lineA;
}

// 0x4A5850
void FxInfoAttractLine_c::GetValue(float currentTime, float mult, float totalTime, float length, bool useConst, void* info) {
    float values[7];
    if (m_bTimeModeParticle) {
        m_InterpInfo.GetVal(values, mult);
    } else {
        m_InterpInfo.GetVal(values, currentTime / length);
    }

    CVector out;
    auto& lineA = *(CVector*)&values[0];
    auto& lineB = *(CVector*)&values[3];

    auto& movement = *static_cast<MovementInfo_t*>(info);
    FindClosestPtOnLine(out, lineA, lineB, movement.m_Pos);
    CVector in = out - movement.m_Pos;
    auto dist = in.Magnitude();

    if (dist > 0.0f) {
        in.Normalise();
    }

    if (dist > 0.1f) {
        movement.m_Vel += in * values[6] * totalTime;
    }
}
