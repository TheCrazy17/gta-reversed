#include "StdInc.h"

#include "MonsterTruck.h"

auto& fWheelExtensionRate = StaticRef<float>(0x8D33AC);
auto& byte_8D33B0         = StaticRef<bool>(0x8D33B0); // true

void CMonsterTruck::InjectHooks() {
    RH_ScopedVirtualClass(CMonsterTruck, 0x8717d8, 71);
    RH_ScopedCategory("Vehicle");

    RH_ScopedInstall(Constructor, 0x6C8D60);

    RH_ScopedInstall(ExtendSuspension, 0x6C7D80);

    RH_ScopedVMTInstall(ProcessEntityCollision, 0x6C8AE0);
    RH_ScopedVMTInstall(ProcessSuspension, 0x6C83A0, { .reversed = false });
    RH_ScopedVMTInstall(ProcessControlCollisionCheck, 0x6C8330);
    RH_ScopedVMTInstall(ProcessControl, 0x6C8250);
    RH_ScopedVMTInstall(SetupSuspensionLines, 0x6C7FB0);
    RH_ScopedVMTInstall(PreRender, 0x6C7DE0);
    RH_ScopedVMTInstall(ResetSuspension, 0x6C7D40);
    RH_ScopedVMTInstall(BurstTyre, 0x6C7D30);
    RH_ScopedVMTInstall(SetUpWheelColModel, 0x6C7D20);
}

// 0x6C8D60
CMonsterTruck::CMonsterTruck(int32 modelIndex, eVehicleCreatedBy createdBy) : CAutomobile(modelIndex, createdBy, false) {
    std::ranges::fill(field_988, 1.0f);
    CMonsterTruck::SetupSuspensionLines();
    autoFlags.bIsMonsterTruck = true;
    m_nVehicleSubType = VEHICLE_TYPE_MTRUCK;
}

// 0x6C8AE0
int32 CMonsterTruck::ProcessEntityCollision(CEntity* entity, CColPoint* colPoint) {
    if (GetStatus() != STATUS_SIMPLE) {
        vehicleFlags.bVehicleColProcessed = true; // OK
    }

    const auto tcm = GetColModel();

    if (physicalFlags.bSkipLineCol || physicalFlags.bProcessingShift || entity->GetIsTypePed()) {
        tcm->GetData()->m_nNumLines = 0; // hmm..... (Later reset back to 4)
    }

    auto wheelColPtsTouchDists{ m_wheelPosition };
    const auto numColPts = CCollision::ProcessColModels(
        GetMatrix(), *tcm,
        entity->GetMatrix(), *entity->GetColModel(),
        *(std::array<CColPoint, 32>*)(colPoint), // trust me bro
        m_wheelColPoint.data(),
        wheelColPtsTouchDists.data(),
        false
    );

    size_t numProcessedLines{};
    if (tcm->GetData()->m_nNumLines) {
        for (auto i = 0; i < MAX_CARWHEELS; i++) {
            const auto  thisWheelTouchDistNow = wheelColPtsTouchDists[i];
            const auto& thisWheelColPtNow = m_wheelColPoint[i];

            if (thisWheelTouchDistNow <= m_wheelPosition[i]) {
                continue;
            }

            if (!(GetUsesCollision() || !numColPts)) { // TODO: Why is this in the loop body?
                continue;
            }

            numProcessedLines++;

            m_fWheelsSuspensionCompression[i] = 0.f;
            m_wheelPosition[i] = thisWheelTouchDistNow;

            m_anCollisionLighting[i] = thisWheelColPtNow.m_nLightingB;
            m_nContactSurface = thisWheelColPtNow.m_nSurfaceTypeB;

            // Same as in CAutomobile::ProcessEntityCollision
            switch (entity->GetType()) {
            case ENTITY_TYPE_VEHICLE:
            case ENTITY_TYPE_OBJECT: {
                CEntity::ChangeEntityReference(m_apWheelCollisionEntity[i], entity->AsPhysical());

                m_vWheelCollisionPos[i] = thisWheelColPtNow.m_vecPoint - entity->GetPosition();
                if (entity->GetIsTypeVehicle()) {
                    m_anCollisionLighting[i] = entity->AsVehicle()->m_anCollisionLighting[i];
                }
                break;
            }
            case ENTITY_TYPE_BUILDING: {
                m_pEntityWeAreOn = entity;
                m_bTunnel = entity->m_bTunnel;
                m_bTunnelTransition = entity->m_bTunnelTransition;
                break;
            }
            }
        }
    } else {
        tcm->GetData()->m_nNumLines = MAX_CARWHEELS; // TODO: Magic (Each wheel has 1 suspension line right now, but hardcoding like this isnt good)
    }

    if (numColPts > 0 || numProcessedLines > 0) {
        AddCollisionRecord(entity);
        if (!entity->GetIsTypeBuilding()) {
            entity->AsPhysical()->AddCollisionRecord(this);
        }
        if (numColPts > 0) {
            if (   entity->GetIsTypeBuilding()
                || (entity->GetIsTypeObject() && entity->AsPhysical()->physicalFlags.bDisableCollisionForce)
            ) {
                SetHasHitWall(true);
            }
        }
    }

    return numColPts;
}

// 0x6C83A0
void CMonsterTruck::ProcessSuspension() {
    plugin::CallMethod<0x6C83A0, CMonsterTruck*>(this);
}

// 0x6C8330
void CMonsterTruck::ProcessControlCollisionCheck(bool applySpeed) {
    ExtendSuspension();
    CAutomobile::ProcessControlCollisionCheck(applySpeed);

    for (auto i = 0u; i < m_fWheelsSuspensionCompression.size(); i++) {
        if (m_fWheelsSuspensionCompression[i] >= 1.0f) {
            m_fWheelsSuspensionCompression[i] = 1.0f;
        } else {
            m_fWheelsSuspensionCompression[i] = (m_aSuspensionSpringLength[i] - m_wheelPosition[i]) / (m_aSuspensionSpringLength[i] - m_aSuspensionLineLength[i]);
        }
    }
}

// 0x6C8250
void CMonsterTruck::ProcessControl() {
    for (auto i = 0u; i < m_fWheelsSuspensionCompression.size(); i++) {
        if (m_fWheelsSuspensionCompression[i] >= 1.0f) {
            m_fWheelsSuspensionCompression[i] = 1.0f;
        } else {
            m_fWheelsSuspensionCompression[i] = (m_aSuspensionSpringLength[i] - m_wheelPosition[i]) / (m_aSuspensionSpringLength[i] - m_aSuspensionLineLength[i]);
            if (m_fWheelsSuspensionCompression[i] < 0.0f && byte_8D33B0) {
                m_fWheelsSuspensionCompression[i] = 0.0f;
            }
        }
    }

    CAutomobile::ProcessControl();

    if (!GetWasPostponed() && (GetMoveSpeed() != 0.0f || GetTurnSpeed() != 0.0f)) {
        ExtendSuspension();
    }
}

// 0x6C7FB0
void CMonsterTruck::SetupSuspensionLines() {
    const auto mi       = GetVehicleModelInfo();
    const auto colModel = mi->GetColModel();
    const auto colData  = colModel->GetData();

    m_fSuspensionRadius = mi->m_fWheelSizeFront * 0.5f;

    if (!colData->m_pDisks) {
        colData->bUsesDisks  = true;
        colData->m_nNumLines = 4;
        colData->m_pDisks    = static_cast<CColDisk*>(CMemoryMgr::Malloc(4 * sizeof(CColDisk)));
    } else if (!colData->bUsesDisks) {
        CMemoryMgr::Free(colData->m_pDisks);
        colData->bUsesDisks  = true;
        colData->m_nNumLines = 4;
        colData->m_pDisks    = static_cast<CColDisk*>(CMemoryMgr::Malloc(4 * sizeof(CColDisk)));
    }

    for (auto i = 0; i < 4; i++) {
        CVector wheelPos;
        mi->GetWheelPosn(i, wheelPos, false);

        colData->m_pDisks[i].Set(
            m_fSuspensionRadius,
            wheelPos,
            CVector{ i > 1 ? 1.0f : -1.0f, 0.0f, 0.0f },
            m_fSuspensionRadius * 0.6f,
            SURFACE_WHEELBASE,
            CarWheelToCarPiece((eCarWheel)i)
        );

        m_aSuspensionSpringLength[i] = wheelPos.z + m_pHandlingData->m_fSuspensionUpperLimit;
        m_aSuspensionLineLength[i]   = wheelPos.z + m_pHandlingData->m_fSuspensionLowerLimit;
    }

    m_fFrontHeightAboveRoad = m_fRearHeightAboveRoad =
        (m_fSuspensionRadius - m_aSuspensionSpringLength[0])
      + (m_aSuspensionSpringLength[0] - m_aSuspensionLineLength[0])
      * (1.0f - 1.0f / (m_pHandlingData->m_fSuspensionForceLevel * 4.0f));

    for (auto i = 0u; i < m_wheelPosition.size(); i++) {
        m_fWheelsSuspensionCompression[i] = 1.0f;
        m_wheelPosition[i]                = mi->m_fWheelSizeFront * 0.5f - m_fFrontHeightAboveRoad;
    }

    auto& bb = colModel->GetBoundingBox();
    if (const auto belowWheel = m_fFrontHeightAboveRoad - m_fSuspensionRadius; belowWheel < bb.m_vecMin.z) {
        bb.m_vecMin.z = belowWheel;
    }

    if (const auto radius = std::max(bb.m_vecMin.Magnitude(), bb.m_vecMax.Magnitude()); radius > colModel->GetBoundRadius()) {
        colModel->GetBoundingSphere().m_fRadius = radius;
    }
}

// 0x6C7DE0
void CMonsterTruck::PreRender() {
    for (auto i = 0; i < 4; i++) {
        m_wheelPosition[i] = std::min(m_wheelPosition[i], m_aSuspensionSpringLength[i]);
    }

    CAutomobile::PreRender();

    const auto mi = GetVehicleModelInfo();
    CMatrix mat;
    CVector pos;

    mi->GetWheelPosn(CAR_WHEEL_FRONT_LEFT, pos, false);
    SetTransmissionRotation(m_aCarNodes[MONSTER_TRANSMISSION_F], m_wheelPosition[CAR_WHEEL_FRONT_LEFT], m_wheelPosition[CAR_WHEEL_FRONT_RIGHT], pos, true);

    mi->GetWheelPosn(CAR_WHEEL_REAR_LEFT, pos, false);
    SetTransmissionRotation(m_aCarNodes[MONSTER_TRANSMISSION_R], m_wheelPosition[CAR_WHEEL_REAR_LEFT], m_wheelPosition[CAR_WHEEL_REAR_RIGHT], pos, false);

    if (m_nModelIndex == MODEL_DUMPER && m_aCarNodes[MONSTER_MISC_A]) {
        SetComponentRotation(m_aCarNodes[MONSTER_MISC_A], AXIS_X, (float)m_wMiscComponentAngle * DUMPER_COL_ANGLEMULT, true);
    }
}

// 0x6C7D80
void CMonsterTruck::ExtendSuspension() {
    for (auto i = 0u; i < m_wheelPosition.size(); i++) {
        auto newPos = m_wheelPosition[i] - CTimer::GetTimeStep() * m_fSuspensionRadius * fWheelExtensionRate;
        if (newPos >= m_aSuspensionLineLength[i]) {
            if (newPos > m_aSuspensionSpringLength[i]) {
                newPos = m_aSuspensionSpringLength[i];
            }
        } else {
            newPos = m_aSuspensionLineLength[i];
        }
        m_wheelPosition[i]                = newPos;
        m_fWheelsSuspensionCompression[i] = 1.0f;
    }
}

// 0x6C7D40
void CMonsterTruck::ResetSuspension() {
    CAutomobile::ResetSuspension();
    for (auto i = 0u; i < m_wheelPosition.size(); i++) {
        m_wheelPosition[i] = m_aSuspensionLineLength[i];
        field_988[i]       = 1.0f;
    }
}

// 0x6C7D30
bool CMonsterTruck::BurstTyre(uint8 tyreComponentId, bool bPhysicalEffect) {
    return false;
}

// 0x6C7D20
bool CMonsterTruck::SetUpWheelColModel(CColModel* colModel) {
    return false;
}
