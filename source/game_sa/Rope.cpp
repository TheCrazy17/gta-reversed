#include "StdInc.h"

#include "Rope.h"
#include "Ropes.h"

void CRope::InjectHooks() {
    RH_ScopedClass(CRope);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(ReleasePickedUpObject, 0x556030);
    RH_ScopedInstall(CreateHookObjectForRope, 0x556070);
    RH_ScopedInstall(UpdateWeightInRope, 0x5561B0);
    RH_ScopedInstall(Remove, 0x556780);
    RH_ScopedInstall(Render, 0x556800);
    RH_ScopedInstall(PickUpObject, 0x5569C0);
    RH_ScopedInstall(Update, 0x557530);
}

// inlined see 0x557959
// use switch like in Android?
// 0x555FB0
bool CRope::DoControlsApply() const {
    return    m_nType == eRopeType::CRANE_MAGNO && CRopes::PlayerControlsCrane == eControlledCrane::MAGNO_CRANE
           || m_nType == eRopeType::WRECKING_BALL && CRopes::PlayerControlsCrane == eControlledCrane::WRECKING_BALL
           || m_nType == eRopeType::CRANE_TROLLEY && CRopes::PlayerControlsCrane == eControlledCrane::LAS_VEGAS_CRANE
           || m_nType == eRopeType::QUARRY_CRANE_ARM && CRopes::PlayerControlsCrane == eControlledCrane::QUARRY_CRANE
           || m_nType == eRopeType::CRANE_MAGNET1
           || m_nType == eRopeType::MAGNET
           || m_nType == eRopeType::CRANE_HARNESS;
}

// 0x556030
void CRope::ReleasePickedUpObject() {
    if (m_pRopeAttachObject) {
        m_pRopeAttachObject->AsPhysical()->physicalFlags.bAttachedToEntity = false;
        m_pRopeAttachObject->AsPhysical()->physicalFlags.bCarriedByRope = false;
        m_pRopeAttachObject = nullptr;
    }
    m_pAttachedEntity->SetUsesCollision(true);
    m_nFlags1 = 60; // 6th, 7th bits set
}

// 0x556070
void CRope::CreateHookObjectForRope() {
    if (m_pAttachedEntity)
        return;

    using namespace ModelIndices;

    const auto modelIndex = [&]() -> ModelIndex {
        switch (m_nType) {
        case eRopeType::CRANE_MAGNET1:
        case eRopeType::CRANE_MAGNO:
        case eRopeType::QUARRY_CRANE_ARM:
        case eRopeType::CRANE_TROLLEY:
            return MI_CRANE_MAGNET;
        case eRopeType::CRANE_HARNESS:
            return MI_CRANE_HARNESS;
        case eRopeType::MAGNET:
            return MI_MINI_MAGNET;
        case eRopeType::WRECKING_BALL:
            return MI_WRECKING_BALL;
        case eRopeType::SWAT:
            return MODEL_INVALID; // Just so the assert below wont be hit.
        default:
            NOTSA_UNREACHABLE(); //assert(0);
        }
    }();
    if (modelIndex == ModelIndex{ MODEL_INVALID }) { // Must do it like this because `ModelIndex` is u16, `MODEL_ID` is i32, and u16 -1 casted to int32 is 0xffff...
        return;
    }

    auto* obj = new CObject(modelIndex, true);
    m_pAttachedEntity = obj;

    obj->RegisterReference(reinterpret_cast<CEntity**>(&m_pAttachedEntity));
    obj->SetPosn(m_aSegments[NUM_ROPE_SEGMENTS - 1]);
    obj->m_nObjectType = OBJECT_TYPE_DECORATION;
    obj->SetIsStatic(false);
    obj->physicalFlags.bAttachedToEntity = true;

    CWorld::Add(m_pAttachedEntity);

    m_pRopeAttachObject = nullptr;
    m_nFlags1 = 0;
}

// 0x5561B0
int8 CRope::UpdateWeightInRope(float x, float y, float z, int32 unused, float* outPos) {
    const auto timeStep = CTimer::GetTimeStep();
    const auto holder   = m_aSegments[m_nSegments];
    const CVector target{ x, y, z };

    // The last segment is where the weight is
    m_aSegments.back() = target;

    const auto maxLength = (float)(NUM_ROPE_SEGMENTS - 1 - m_nSegments) * m_fSegmentLength;
    auto       toTarget  = target - holder;
    const auto distance  = toTarget.Magnitude();

    if (maxLength <= distance) {
        // The rope is as long as it can be, a straight line up to the weight
        const auto pos = toTarget * (maxLength / distance) + holder;
        outPos[0] = pos.x;
        outPos[1] = pos.y;
        outPos[2] = pos.z;

        for (auto i = m_nSegments + 1u; i < NUM_ROPE_SEGMENTS; i++) {
            toTarget.Normalise();
            toTarget *= m_fSegmentLength;
            m_aSegments[i] = toTarget * (float)(i - m_nSegments) + holder;
        }
        return 1;
    }

    // Slack rope, every segment is kept within its length of the next one, a few times over
    for (auto pass = 0; pass < 6; pass++) {
        // From the weight to the holder, it stops at the first segment that is already close enough
        for (auto i = (int32)NUM_ROPE_SEGMENTS - 1; m_nSegments + 1 < i; i--) {
            const auto& segment  = m_aSegments[i];
            auto&       previous = m_aSegments[i - 1];
            const auto  before   = previous;

            const auto length = (segment - previous).Magnitude();
            if (length <= m_fSegmentLength) {
                break;
            }

            previous        = (previous - segment) * (m_fSegmentLength / length) + segment;
            m_aSpeed[i - 1] = (previous - before) * (1.0f / timeStep);
        }

        // From the holder to the weight
        for (auto i = m_nSegments + 1u; i < NUM_ROPE_SEGMENTS - 1; i++) {
            auto&       segment  = m_aSegments[i];
            const auto& previous = m_aSegments[i - 1];
            const auto  delta    = segment - previous;
            const auto  length   = delta.Magnitude();
            if (length > m_fSegmentLength) {
                segment = delta * (m_fSegmentLength / length) + previous;
            }
        }
    }

    // One last time from the weight, without changing the speeds
    for (auto i = (int32)NUM_ROPE_SEGMENTS - 1; m_nSegments + 1 < i; i--) {
        const auto& segment  = m_aSegments[i];
        auto&       previous = m_aSegments[i - 1];

        const auto length = (segment - previous).Magnitude();
        if (length <= m_fSegmentLength) {
            break;
        }
        previous = (previous - segment) * (m_fSegmentLength / length) + segment;
    }
    return 0;
}

// 0x556780
void CRope::Remove() {
    m_nType = eRopeType::NONE;
    if (m_pRopeAttachObject)
        ReleasePickedUpObject();

    if (m_pAttachedEntity) {
        CWorld::Remove(m_pAttachedEntity);
        delete m_pAttachedEntity;
        m_pAttachedEntity = nullptr;
    }
}

// 0x556800
void CRope::Render() {
    // Note: Probably needs adjustments if `NUM_ROPE_SEGMENTS` is changed
    if (!TheCamera.IsSphereVisible(m_aSegments[NUM_ROPE_SEGMENTS / 2], 20.0f))
        return;

    if ((TheCamera.GetPosition() - m_aSegments[0]).Magnitude2D() >= 120.0f)
        return;

    DefinedState();

    const auto GetVertex = [](unsigned i) {
        return &TempBufferVertices.m_3d[i];
    };

    const RwRGBA color = { 0, 0, 0, 128 };
    for (auto i = 0u; i < NUM_ROPE_SEGMENTS; i++) {
        RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color);
        RxObjSpace3DVertexSetPos(GetVertex(i), &m_aSegments[i]);
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,     RWRSTATE(rwFILTERLINEAR));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     RWRSTATE(FALSE));

    if (RwIm3DTransform(TempBufferVertices.m_3d, NUM_ROPE_SEGMENTS, nullptr, 0)) {
        RxVertexIndex indices[] = { // *(RxVertexIndex(*)[64])0x8CD818
            0,  1,  1,  2,  2,  3,  3,  4,
            4,  5,  5,  6,  6,  7,  7,  8,
            8,  9,  9,  10, 10, 11, 11, 12,
            12, 13, 13, 14, 14, 15, 15, 16,
            16, 17, 17, 18, 18, 19, 19, 20,
            20, 21, 21, 22, 22, 23, 23, 24,
            24, 25, 25, 26, 26, 27, 27, 28,
            28, 29, 29, 30, 30, 31, 31, 32
        };
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST, indices, std::size(indices) - 2); // the last two indexes are not used
        RwIm3DEnd();
    }

    if (m_nType == eRopeType::QUARRY_CRANE_ARM) {
        const CVector pos[] = { m_aSegments[0], { 709.32f, 916.20f, 53.0f } }; // Hunter Quarry
        for (auto i = 0u; i < std::size(pos); i++) {
            RxObjSpace3DVertexSetPreLitColor(GetVertex(i), &color);
            RxObjSpace3DVertexSetPos(GetVertex(i), &pos[i]);
        }
        if (RwIm3DTransform(TempBufferVertices.m_3d, std::size(pos), nullptr, 0)) {
            RxVertexIndex indices[] = { 0, 1 };
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST, indices, std::size(indices));
            RwIm3DEnd();
        }
    }
}

// 0x5569C0
void CRope::PickUpObject(CEntity* obj) {
    if (m_pRopeAttachObject == obj)
        return;

    if (m_pRopeAttachObject)
        ReleasePickedUpObject();

    obj->RegisterReference(&m_pAttachedEntity);
    m_pRopeAttachObject = obj;

    // TODO: Move model => world space translation into CEntity
    // MultiplyMatrixWithVector should be used here
    CVector height = { {}, {}, CRopes::FindPickupHeight(obj) };
    m_pAttachedEntity->SetPosn(obj->GetPosition() + obj->GetMatrix().TransformVector(height));
    m_pAttachedEntity->SetUsesCollision(false);

    obj->AsPhysical()->physicalFlags.bAttachedToEntity = true;
    if (obj->GetIsTypeVehicle()) {
        if (obj->GetStatus() == STATUS_SIMPLE)
        {
            obj->SetStatus(STATUS_PHYSICS);
        }
    } else if (obj->GetIsTypeObject()) {
        if (obj->GetIsStatic()) {
            obj->AsObject()->SetIsStatic(false);
            obj->AsObject()->AddToMovingList();
            obj->AsObject()->m_nFakePhysics = 0;
        }
    }
}

// 0x557530
void CRope::Update() {
    static_assert(offsetof(CPhysical, m_fMass) == 0x8C);
    static_assert(offsetof(CPhysical, m_vecMoveSpeed) == 0x44);
    static_assert(offsetof(CPhysical, m_nFakePhysics) == 0xB8);
    static_assert(offsetof(CPhysical, m_pAttachedTo) == 0xFC);

    using namespace ModelIndices;

    const auto MatrixOf = [](CEntity* entity) -> CMatrix& {
        if (!entity->m_matrix) {
            entity->AllocateMatrix();
            entity->m_placement.UpdateMatrix(entity->m_matrix);
        }
        return *entity->m_matrix;
    };

    const auto timeStep = CTimer::GetTimeStep();
    const auto damping  = (float)std::pow((double)0.8f, (double)timeStep); // How much of the speed of a segment is kept

    // Nothing is done for the ropes that are far from the camera
    if (!((TheCamera.GetPosition() - m_aSegments[0]).Magnitude2D() < 200.0f)) {
        return;
    }

    // The flags2 are: 1 = the rope was moved this frame, 4 = the ground has to be looked for
    const auto isGroundChecked = [&] { return (m_nFlags2 & 4) != 0; };

    // The top of the rope falls when nothing is holding it
    if (!(m_nFlags2 & 1) && m_nTime < CTimer::GetTimeInMS()) {
        m_aSpeed[0].z -= timeStep * 0.0015f;
        m_aSegments[0] += m_aSpeed[0] * timeStep;
    }

    if (isGroundChecked() && (CTimer::GetFrameCounter() & 7) == 2) {
        m_fGroundZ = CWorld::FindGroundZFor3DCoord(m_aSegments[0], nullptr, nullptr);
    }

    // What the rope holds up from, the ground is its hook
    const auto IsMagnoCraneModel = [](const CEntity* e) {
        const auto model = e->m_nModelIndex;
        return model == MI_OBJECTFORMAGNOCRANE1 || model == MI_OBJECTFORMAGNOCRANE2 || model == MI_OBJECTFORMAGNOCRANE3 || model == MI_OBJECTFORMAGNOCRANE5;
    };
    if (isGroundChecked() && m_pRopeAttachObject && (m_pRopeAttachObject->GetIsTypeVehicle() || IsMagnoCraneModel(m_pRopeAttachObject))) {
        m_fGroundZ = m_pAttachedEntity->GetPosition().z - 0.5f;
    }

    // The segments follow the one before, they are the ones that are under the holder of the rope
    for (auto i = m_nSegments + 1u; i < NUM_ROPE_SEGMENTS; i++) {
        const auto oldPos = m_aSegments[i];

        m_aSpeed[i].x += (float)((CGeneral::GetRandomNumber() & 0xF) - 8) * 0.001f;
        m_aSpeed[i].y += (float)((CGeneral::GetRandomNumber() & 0xF) - 8) * 0.001f;

        const auto kept = 1.0f - damping;
        m_aSpeed[i]     = m_aSpeed[i - 1] * kept + m_aSpeed[i] * damping;

        m_aSegments[i].z -= timeStep * 0.15f;
        if (isGroundChecked()) {
            m_aSegments[i].z = std::max(m_aSegments[i].z, m_fGroundZ + 0.3f);
        }

        // They are always the same distance from the one before
        auto toSegment = m_aSegments[i] - m_aSegments[i - 1];
        m_aSegments[i] = m_aSegments[i - 1] + toSegment * (m_fTotalLength / toSegment.Magnitude());
        m_aSpeed[i]    = (m_aSegments[i] - oldPos) * (1.0f / timeStep);
    }

    const auto type = m_nType;
    if (type < eRopeType::CRANE_MAGNET1 || type > eRopeType::CRANE_TROLLEY) {
        // Nothing else to do
    } else {
        [&] {
            // The speed of the winch is what the player controls
            if (DoControlsApply()) {
                auto* const pad = CPad::GetPad(0);
                if (type == eRopeType::CRANE_MAGNET1 || type == eRopeType::CRANE_HARNESS || type == eRopeType::MAGNET) {
                    const auto gunUpDown = pad->GetCarGunUpDown();
                    if ((gunUpDown < 0 && CTheScripts::bEnableCraneRaise) || (gunUpDown > 0 && CTheScripts::bEnableCraneLower)) {
                        m_fSegmentLength -= (float)gunUpDown * timeStep * 1e-5f;
                    }
                    m_fSegmentLength = std::min(m_fSegmentLength, 0.84f);
                } else {
                    if (CTheScripts::bEnableCraneRaise) {
                        const auto delta = (float)pad->NewState.ButtonSquare * timeStep * 1e-5f;
                        if (delta > 0.0f && delta + m_fSegmentLength < 0.9f) {
                            AudioEngine.ReportMissionAudioEvent(0x68, m_pRopeHolder->AsPhysical(), 0.0f, 1.0f);
                        }
                        m_fSegmentLength += delta;
                    }
                    if (CTheScripts::bEnableCraneLower) {
                        const auto delta = (float)pad->NewState.ButtonCross * timeStep * 1e-5f;
                        if (delta > 0.0f && m_fSegmentLength - delta > 0.01f) {
                            AudioEngine.ReportMissionAudioEvent(0x68, m_pRopeHolder->AsPhysical(), 0.0f, 1.0f);
                        }
                        m_fSegmentLength -= delta;
                    }
                }
            }
            m_fSegmentLength = std::max(m_fSegmentLength, 0.01f);
            m_fSegmentLength = std::min(m_fSegmentLength, 0.9f);

            // What is hanging from the rope is the hook, or what the hook carries
            CPhysical* body;
            float      blend; // How much of the speed of the rope is given to the holder
            if (!m_pRopeAttachObject) {
                body  = m_pAttachedEntity->AsPhysical();
                blend = 0.06f;
                body->m_bUsesCollision = true;
            } else {
                body = m_pRopeAttachObject->AsPhysical();
                auto mass = body->m_fMass;
                if (body->m_nModelIndex == MODEL_SECURICA) {
                    mass = 750.0f;
                }
                blend = std::min(0.13f * 0.00083333335f * mass + 0.06f, 0.5f);
                body->m_nFakePhysics = 0;
            }

            CVector relSpeed{};
            if (body) {
                CVector newPos;
                if (UpdateWeightInRope(body->GetPosition().x, body->GetPosition().y, body->GetPosition().z, 0x3DCCCCCD, &newPos.x)) {
                    body->SetPosn(newPos);

                    // The speed is the one of the holder plus what the body has on its own
                    auto* const holder    = m_pRopeHolder->AsPhysical();
                    auto        holderSpeed = holder->GetSpeed(m_aSegments[0] - holder->GetPosition());
                    if (type == eRopeType::CRANE_MAGNO || type == eRopeType::WRECKING_BALL || type == eRopeType::QUARRY_CRANE_ARM || type == eRopeType::CRANE_TROLLEY) {
                        holderSpeed = CVector{};
                    }
                    relSpeed = body->m_vecMoveSpeed - holderSpeed;

                    // What moves away along the rope is stopped
                    auto toBody = body->GetPosition() - m_aSegments[0];
                    toBody.Normalise();
                    const auto alongRope = DotProduct(toBody, relSpeed);
                    if (alongRope > 0.0f) {
                        relSpeed -= toBody * alongRope;
                    }

                    const auto speedChange = (relSpeed + holderSpeed) - body->m_vecMoveSpeed;
                    body->m_vecMoveSpeed += speedChange * (1.0f - blend);
                    holder->m_vecMoveSpeed -= speedChange * blend;

                    if (!m_pRopeAttachObject) {
                        // The hook looks at the holder
                        MatrixOf(body).ForceUpVector(-toBody);
                    } else {
                        // Whatever it carries turns slowly to look at it
                        const CMatrix oldMatrix{ MatrixOf(body) };
                        MatrixOf(body).ForceUpVector(-toBody);
                        const CMatrix newMatrix{ MatrixOf(body) };
                        auto&         matrix = MatrixOf(body);
                        matrix.GetRight()   = oldMatrix.GetRight() * 0.9f + newMatrix.GetRight() * 0.1f;
                        matrix.GetForward() = oldMatrix.GetForward() * 0.9f + newMatrix.GetForward() * 0.1f;
                        matrix.GetUp()      = oldMatrix.GetUp() * 0.9f + newMatrix.GetUp() * 0.1f;
                    }
                }

                // The hook is where the thing it carries is
                if (m_pRopeAttachObject) {
                    auto* const hook = m_pAttachedEntity->AsPhysical();
                    MatrixOf(hook)        = MatrixOf(m_pRopeAttachObject);
                    hook->m_vecMoveSpeed   = m_pRopeAttachObject->AsPhysical()->m_vecMoveSpeed;
                    const auto height      = CModelInfo::GetModelInfo(m_pRopeAttachObject->m_nModelIndex)->GetColModel()->GetBoundingBox().m_vecMax.z;
                    hook->SetPosn(MatrixOf(m_pRopeAttachObject).TransformPoint({ 0.0f, 0.0f, height }));
                }
            }

            // The end of the rope, where things get picked up
            const auto tip = m_aSegments[NUM_ROPE_SEGMENTS - 1];

            if (m_pRopeAttachObject) {
                if (DoControlsApply() && CTheScripts::bEnableCraneRelease) {
                    if (CPad::GetPad(0)->CarGunJustDown() != 0) {
                        ReleasePickedUpObject();
                    }
                }
                if (m_pRopeAttachObject && (m_pRopeAttachObject->AsPhysical()->m_nPhysicalFlags & PHYSICAL_DESTROYED)) {
                    ReleasePickedUpObject();
                }
                if (m_pRopeAttachObject && m_pRopeAttachObject->GetIsTypeVehicle()) {
                    auto* const vehicle = m_pRopeAttachObject->AsVehicle();
                    if (vehicle->m_nVehicleSubType == VEHICLE_TYPE_BIKE && vehicle->m_pDriver) {
                        ReleasePickedUpObject();
                    }
                }
                return;
            }

            if (m_nFlags1) {
                m_nFlags1--;
                return;
            }

            // The things this rope can pick up
            auto canPickUpMagnoCraneObjects = false;
            auto canPickUpQuarryObjects     = false;
            auto canPickUpTrolleyObjects    = false;
            auto canPickUpWongDish          = false;
            auto canPickUpKmbRock           = false;
            auto canPickUpKmbPlank          = false;
            auto canPickUpKmbBomb           = false;
            auto canPickUpOnlyRCVehicle     = false;
            auto canPickUpVehicles          = true;
            switch (type) {
            case eRopeType::CRANE_MAGNET1:
                break;
            case eRopeType::CRANE_HARNESS: { // Peds
                auto& pool = *GetPedPool();
                for (auto i = pool.GetSize(); i-- > 0;) {
                    auto* const ped = pool.GetAt(i);
                    if (!ped || ped->m_nPedState == PEDSTATE_DEAD || ped->IsPlayer() || ped->bInVehicle) {
                        continue;
                    }
                    if (!((ped->GetPosition() - tip).Magnitude() < 2.5f)) {
                        continue;
                    }

                    m_pRopeAttachObject = ped;
                    ped->RegisterReference(reinterpret_cast<CEntity**>(&m_pRopeAttachObject));
                    ped->m_nPhysicalFlags |= 0x80000000;

                    const auto height = CModelInfo::GetModelInfo(ped->m_nModelIndex)->GetColModel()->GetBoundingBox().m_vecMax.z;
                    m_pAttachedEntity->SetPosn(MatrixOf(ped).TransformPoint({ 0.0f, 0.0f, height }));
                    m_pAttachedEntity->m_bUsesCollision = false;

                    // The height used here is what was left over from the speed of the body, that is how the game does it
                    const auto diff = tip.z - relSpeed.z;
                    if (diff > 0.0f) {
                        const auto invMass = 1.0f / m_fMass;
                        m_fSegmentLength   = std::max((m_fSegmentLength - diff * invMass) - invMass * m_fTotalLength, 0.01f);
                    }
                    break;
                }
                return;
            }
            case eRopeType::MAGNET:
                canPickUpWongDish = canPickUpKmbRock = canPickUpKmbPlank = canPickUpKmbBomb = canPickUpOnlyRCVehicle = true;
                break;
            case eRopeType::CRANE_MAGNO:
                canPickUpMagnoCraneObjects = true;
                break;
            case eRopeType::QUARRY_CRANE_ARM:
                canPickUpQuarryObjects = true;
                break;
            case eRopeType::CRANE_TROLLEY:
                canPickUpTrolleyObjects = true;
                canPickUpVehicles       = false;
                break;
            default:
                return;
            }

            // Takes the rope down to where the thing is, and the weight of it makes the rope longer
            const auto ShortenRope = [&](float pointZ) {
                const auto diff = tip.z - pointZ;
                if (diff > 0.0f) {
                    const auto invMass = 1.0f / m_fMass;
                    m_fSegmentLength   = std::max((m_fSegmentLength - diff * invMass) - invMass * m_fTotalLength, 0.01f);
                }
            };

            if (canPickUpVehicles) {
                auto& pool = *GetVehiclePool();
                for (auto i = pool.GetSize(); i-- > 0;) {
                    auto* const vehicle = pool.GetAt(i);
                    if (!vehicle) {
                        continue;
                    }

                    const auto subType = vehicle->m_nVehicleSubType;
                    const auto canBeCarried = subType == VEHICLE_TYPE_AUTOMOBILE
                        || (subType == VEHICLE_TYPE_BIKE && !vehicle->m_pDriver)
                        || subType == VEHICLE_TYPE_MTRUCK
                        || vehicle->m_nModelIndex == MODEL_DINGHY
                        || vehicle->m_nModelIndex == MODEL_VORTEX;
                    if (!canBeCarried) {
                        continue;
                    }
                    if ((vehicle->m_nPhysicalFlags & PHYSICAL_DESTROYED) || !vehicle->vehicleFlags.bWinchCanPickMeUp) {
                        continue;
                    }
                    if (canPickUpOnlyRCVehicle && vehicle->m_nModelIndex != MODEL_RCTIGER) {
                        continue;
                    }
                    if (vehicle->m_ropeType != 0) {
                        continue;
                    }

                    const auto height = CModelInfo::GetModelInfo(vehicle->m_nModelIndex)->GetColModel()->GetBoundingBox().m_vecMax.z;
                    const auto point  = MatrixOf(vehicle).TransformPoint({ 0.0f, 0.0f, height });
                    if (!((point - tip).Magnitude() < 2.5f)) {
                        continue;
                    }

                    m_pRopeAttachObject = vehicle;
                    vehicle->RegisterReference(reinterpret_cast<CEntity**>(&m_pRopeAttachObject));
                    vehicle->m_nPhysicalFlags |= 0x80000000;
                    if (vehicle->GetStatus() == STATUS_SIMPLE) {
                        vehicle->SetStatus(STATUS_PHYSICS);
                    }

                    m_pAttachedEntity->SetPosn(MatrixOf(vehicle).TransformPoint({ 0.0f, 0.0f, height }));
                    m_pAttachedEntity->m_bUsesCollision = false;
                    ShortenRope(point.z);
                    break;
                }
            }

            // Objects that are picked by the magnet
            if (!(canPickUpMagnoCraneObjects || canPickUpQuarryObjects || canPickUpTrolleyObjects || canPickUpWongDish || canPickUpKmbRock || canPickUpKmbPlank || canPickUpKmbBomb)) {
                return;
            }
            if (m_pRopeAttachObject) {
                return;
            }

            auto& pool = *GetObjectPool();
            for (auto i = pool.GetSize(); i-- > 0;) {
                auto* const obj = pool.GetAt(i);
                if (!obj || !obj->objectFlags.bCanBeAttachedToMagnet) {
                    continue;
                }

                const auto model = obj->m_nModelIndex;
                const auto canBeCarried = (canPickUpMagnoCraneObjects && (IsMagnoCraneModel(obj) || model == MI_OBJECTFORMAGNOCRANE4))
                    || (canPickUpTrolleyObjects && model == MI_OBJECTFORBUILDINGSITECRANE1)
                    || (canPickUpQuarryObjects && (model == MI_QUARY_ROCK1 || model == MI_QUARY_ROCK2 || model == MI_QUARY_ROCK3 || model == MI_DEAD_TIED_COP) && !obj->m_pAttachedTo)
                    || (canPickUpWongDish && model == MI_WONG_DISH)
                    || (canPickUpKmbRock && model == MI_KMB_ROCK)
                    || (canPickUpKmbPlank && model == MI_KMB_PLANK)
                    || (canPickUpKmbBomb && model == MI_KMB_BOMB);
                if (!canBeCarried) {
                    continue;
                }

                // The beams of the magno crane are picked by their highest end, and are turned so that end is the top
                const auto height   = CModelInfo::GetModelInfo(model)->GetColModel()->GetBoundingBox().m_vecMax.z;
                auto       point    = MatrixOf(obj).TransformPoint({ 0.0f, 0.0f, height });
                auto       numTurns = 0;
                if (IsMagnoCraneModel(obj)) {
                    const auto bottom = MatrixOf(obj).TransformPoint({ 0.0f, 0.0f, -height });
                    if (point.z < bottom.z) {
                        point    = bottom;
                        numTurns = 2;
                    }
                    const auto side = MatrixOf(obj).TransformPoint({ height, 0.0f, 0.0f });
                    if (point.z < side.z) {
                        point    = side;
                        numTurns = 3;
                    }
                    const auto otherSide = MatrixOf(obj).TransformPoint({ -height, 0.0f, 0.0f });
                    if (point.z < otherSide.z) {
                        point    = otherSide;
                        numTurns = 1;
                    }
                }
                if (!((point - tip).Magnitude() < 2.5f)) {
                    continue;
                }

                m_pRopeAttachObject = obj;
                obj->RegisterReference(reinterpret_cast<CEntity**>(&m_pRopeAttachObject));
                obj->m_nPhysicalFlags |= 0x80000000;

                m_pAttachedEntity->SetPosn(MatrixOf(obj).TransformPoint({ 0.0f, 0.0f, height }));
                m_pAttachedEntity->m_bUsesCollision = false;

                if ((obj->m_nFlags & 4) || (obj->m_nFlags & 0x40000)) {
                    obj->SetIsStatic(false);
                    obj->AddToMovingList();
                }
                obj->physicalFlags.bAttachedToEntity = true;

                ShortenRope(point.z);

                for (; numTurns > 0; numTurns--) {
                    auto&      matrix = MatrixOf(obj);
                    const auto right  = matrix.GetRight();
                    matrix.GetRight() = matrix.GetUp();
                    matrix.GetUp()    = -right;
                }
                break;
            }
        }();
    }

    // Ropes that fell down are gone
    if (!(m_nFlags2 & 1) && m_aSegments[0].z < -50.0f) {
        Remove();
    }
    m_nFlags2 &= ~1;
}

