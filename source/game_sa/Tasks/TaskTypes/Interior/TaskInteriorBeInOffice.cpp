#include "StdInc.h"

#include "TaskInteriorBeInOffice.h"
#include "TaskInteriorUseInfo.h"

#include "InteriorGroup_c.h"
#include "InteriorInfo_t.h"

void CTaskInteriorBeInOffice::InjectHooks() {
    RH_ScopedVirtualClass(CTaskInteriorBeInOffice, 0x870218, 11);
    RH_ScopedCategory("Tasks/TaskTypes/Interior");

    RH_ScopedInstall(Constructor, 0x675220);
    RH_ScopedInstall(Destructor, 0x675260);

    
    RH_ScopedInstall(GetInfoForPedToUse, 0x675300);
    RH_ScopedVMTInstall(Clone, 0x675270);
    RH_ScopedVMTInstall(GetTaskType, 0x675250);
    RH_ScopedVMTInstall(CreateNextSubTask, 0x676350);
    RH_ScopedVMTInstall(CreateFirstSubTask, 0x676400);
    RH_ScopedVMTInstall(ControlSubTask, 0x6752F0);
}

// 0x675220
CTaskInteriorBeInOffice::CTaskInteriorBeInOffice(InteriorGroup_c* ig) :
    m_IntGrp{ ig }
{
}

CTaskInteriorBeInOffice::CTaskInteriorBeInOffice(const CTaskInteriorBeInOffice& o) :
    CTaskInteriorBeInOffice{ o.m_IntGrp }
{
}

// 0x675300
void CTaskInteriorBeInOffice::GetInfoForPedToUse(int32* outDur) {
    using enum eInteriorInfoType;

    // How this isn't an infinite loop is beyond me
    eInteriorInfoType type;
    do {
        type = !m_CurrIntInfo || m_CurrIntInfo->Type != UNK_6 ? UNK_6 : UNK_7;
        m_IntGrp->FindInteriorInfo(type, &m_CurrIntInfo, &m_CurrInt);
    } while (!m_CurrIntInfo);

    // Figure out duration now
    // NOTSA: Moved this outside the loop, no need for it to be there. The exe compares the roll with 20 using `<=`,
    // so a roll of 0 to 20 (not just 0 to 19) gives a duration
    *outDur = type == UNK_6
        ? CGeneral::GetRandomNumberInRange(0, 100) <= 20
            ? CGeneral::GetRandomNumberInRange(5'000, 30'000)
            : -1
        : CGeneral::GetRandomNumberInRange(5'000, 10'000);
}

// notsa (code is sa)
CTask* CTaskInteriorBeInOffice::CreateUseInfoTask(CPed* ped, bool bDoInstantly) {
    // Only the exe's CreateNextSubTask (bDoInstantly is false) raises this flag and walks the ped to the spot
    if (!bDoInstantly) {
        ped->bHasJustSoughtCover = true;
    }

    int32 dur;
    GetInfoForPedToUse(&dur);
    return new CTaskInteriorUseInfo{ m_CurrIntInfo, m_CurrInt, dur, bDoInstantly };
}
