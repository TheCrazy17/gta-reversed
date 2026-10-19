#include "StdInc.h"

#include "DecisionSimple.h"
#include "Enums/eTaskType.h"

void CDecisionSimple::SetDefault() {
    m_anTasks.fill(TASK_NONE);
    m_afChances.fill(0.0f);
    m_nCount = 6;
}

// 0x6007A0
void CDecisionSimple::MakeDecision(int32 taskType, int16& outTaskType, int32& outDecisionIndex) {
    outTaskType      = TASK_NONE;
    outDecisionIndex = -1;

    // The task we're looking for is always picked, if it's there
    if (taskType != -1) {
        for (auto i = 0u; i < m_nCount; i++) {
            if (taskType == m_anTasks[i]) {
                outTaskType      = (int16)m_anTasks[i];
                outDecisionIndex = (int32)i;
                break;
            }
        }
    }

    // Otherwise pick by chance
    if (outTaskType == TASK_NONE) {
        const auto roll = (float)CGeneral::GetRandomNumber() * RAND_MAX_FLOAT_RECIPROCAL;
        for (auto i = 0u; i < m_nCount; i++) {
            if (roll <= m_afChances[i]) {
                outTaskType      = (int16)m_anTasks[i];
                outDecisionIndex = (int32)i;
                break;
            }
        }
    }
}
