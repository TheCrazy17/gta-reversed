#pragma once

#include "FxTools.h"

class NOTSA_EXPORT_VTABLE FxInterpInfo_c {
public:
    bool    m_bLooped;  // (4 bytes in Manhunt)
    int8    m_nNumKeys;
    int16   m_nCount;
    uint16* m_pTimes;

public:
    FxInterpInfo_c();
    ~FxInterpInfo_c() = default; // 0x4A8430

    virtual void Load(FILESTREAM file) = 0;

protected:
    // Samples `keys[attribute][key]` at `time` for every attribute, `toDouble` converts a stored key to its real value.
    // Key times are in 1/256 of a time unit. Looped curves wrap `time` around the last key time.
    template<typename T, typename ToDouble>
    void SampleKeys(T* const* keys, float* outValues, float time, ToDouble&& toDouble) const {
        constexpr float KEY_TIME_SCALE = 1.0f / 256.0f;
        const auto KeyTime = [&](int32 i) { return (double)((float)m_pTimes[i] * KEY_TIME_SCALE); };

        if (m_nNumKeys == 1) {
            for (auto i = 0; i < m_nCount; i++) {
                outValues[i] = (float)toDouble(keys[i][0]);
            }
            return;
        }

        if (m_bLooped) {
            const auto duration = KeyTime(m_nNumKeys - 1);
            time = (float)((double)time - (double)(int32)((double)time / duration) * duration);
        }

        auto next = 1;
        while (next < m_nNumKeys && !((double)time < KeyTime(next))) {
            next++;
        }

        if (next >= m_nNumKeys) {
            for (auto i = 0; i < m_nCount; i++) {
                outValues[i] = (float)toDouble(keys[i][m_nNumKeys - 1]);
            }
            return;
        }

        const auto prevTime = KeyTime(next - 1);
        const auto t        = (float)(((double)time - prevTime) / (KeyTime(next) - prevTime));
        for (auto i = 0; i < m_nCount; i++) {
            const auto prev = toDouble(keys[i][next - 1]);
            outValues[i]    = (float)((toDouble(keys[i][next]) - prev) * t + prev);
        }
    }
};
