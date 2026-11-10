#include "StdInc.h"

#include "AutoPilot.h"
#include "PathFind.h"
#include "Curves.h"

void CAutoPilot::InjectHooks() {
    RH_ScopedClass(CAutoPilot);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(RemoveOnePathNode, 0x41B950);
    RH_ScopedInstall(ModifySpeed, 0x41B980);
}

// 0x6D5E20
CAutoPilot::CAutoPilot() : m_aPathFindNodesInfo() {
    _smthNext = 1;
    _smthCurr = 1;

    m_nCarCtrlFlags = 0;

    field_C = 0;
    m_nSpeedScaleFactor = 1000;
    m_nNextLane = 0;
    m_nCurrentLane = 0;
    m_nCarDrivingStyle = DRIVING_STYLE_STOP_FOR_CARS;
    m_nCarMission = eCarMission::MISSION_NONE;
    m_nTempAction = TEMPACT_NONE;
    SetCruiseSpeed(10);
    m_speed = 10.0F;
    m_nPathFindNodesCount = 0;
    m_TargetEntity = nullptr;

    m_nTimeToStartMission = CTimer::GetTimeInMS();
    m_nTimeSwitchedToRealPhysics = CTimer::GetTimeInMS();
    m_LastUpdateTimeMs = 0;

    m_nStraightLineDistance = 20;
    m_ucTempActionMode = 0;
    m_ucCarMissionModeCounter = 0;
    field_41 = 0;
    m_SpeedMult = 1.0f;
    m_ucHeliSpeedMult = 0;
    movementFlags.bIsStopped = false;
    movementFlags.bIsParked = false;
    field_4A = 0;
    m_ucCarFollowDist = 10;
    m_ucHeliTargetDist2 = 10;
    field_50 = CGeneral::GetRandomNumber() % 8 + 2;
    m_vehicleRecordingId = -1;
    m_bPlaneDogfightSomething = false;
    m_ObstructingEntity = nullptr;
    m_fMaxTrafficSpeed = 0.0F;
}

// 0x41B980
void CAutoPilot::ModifySpeed(float target) {
    // Out of range and NaN values become INT32_MIN like in the exe's float to int conversion
    const auto ToInt32 = [](double v) {
        return std::isfinite(v) && v > -2147483649.0 && v < 2147483648.0 ? (int32)v : INT32_MIN;
    };

    // How far along the current curve the vehicle is, measured with the old speed. The time difference is unsigned
    // and the duration signed, as the x87 code loads them
    const auto now      = CTimer::GetTimeInMS();
    const auto progress = (float)((double)(uint32)(now - (uint32)field_C) / (double)(int32)m_nSpeedScaleFactor);

    m_speed = target > 0.01f ? target : 0.01f; // A NaN also ends up as 0.01

    // The exe tests the path node tables of the two link areas here, not the navi node tables
    if (!ThePaths.m_pPathNodes[m_nCurrentPathNodeInfo.m_wAreaId] || !ThePaths.m_pPathNodes[m_nNextPathNodeInfo.m_wAreaId]) {
        return;
    }

    const auto& curLink  = ThePaths.GetCarPathLink(m_nCurrentPathNodeInfo);
    const auto& nextLink = ThePaths.GetCarPathLink(m_nNextPathNodeInfo);

    // The same curve CCarCtrl uses when a vehicle enters a link; only the time to go through it is recomputed
    const auto LinkDir = [](const CCarPathLink& link, float sign) {
        const auto* const raw = reinterpret_cast<const int8*>(&link.m_dir);
        return CVector2D{ (float)raw[0] * 0.01f * sign, (float)raw[1] * 0.01f * sign };
    };
    const auto curDir  = LinkDir(curLink, (float)_smthCurr);
    const auto nextDir = LinkDir(nextLink, (float)_smthNext);

    const auto curOffset  = (curLink.OneWayLaneOffset() + (float)m_nCurrentLane) * 5.4f;
    const auto nextOffset = (nextLink.OneWayLaneOffset() + (float)m_nNextLane) * 5.4f;

    const auto curPos  = curLink.GetNodeCoors();
    const auto nextPos = nextLink.GetNodeCoors();
    const CVector startPos{ curPos.x + curOffset * curDir.y, curPos.y - curOffset * curDir.x, 0.0f };
    const CVector endPos{ nextPos.x + nextOffset * nextDir.y, nextPos.y - nextOffset * nextDir.x, 0.0f };

    const auto scaleFactor = CCurves::CalcSpeedScaleFactor(startPos, endPos, curDir.x, curDir.y, nextDir.x, nextDir.y);
    const auto duration    = ToInt32((double)scaleFactor * (1000.0 / (double)m_speed));
    m_nSpeedScaleFactor    = (uint32)duration;

    // The start time moves so that the vehicle stays at the same point of the curve
    field_C = ToInt32((double)now - (double)progress * (double)duration);
}

// 0x41B950
void CAutoPilot::RemoveOnePathNode() {
    m_nPathFindNodesCount--;
    for (int16 i = 0; i < (int16)m_nPathFindNodesCount; i++) {
        m_aPathFindNodesInfo[i] = m_aPathFindNodesInfo[i + 1];
    }
}

void CAutoPilot::SetCarMission(eCarMission carMission, uint32 timeOffsetMs) {
    m_nCarMission = carMission;
    m_nTimeToStartMission = CTimer::GetTimeInMS() + timeOffsetMs;
}

// notsa
void CAutoPilot::SetTempAction(eAutoPilotTempAction action, uint32 durMs) {
    m_nTempAction = action;
    m_nTempActionTime = CTimer::GetTimeInMS() + durMs;
}
