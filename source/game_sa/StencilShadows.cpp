#include "StdInc.h"
#include "StencilShadowObject.h"

// TODO: Statically allocate after reversing RenderForVehicle&RenderForObject.
static inline auto& s_ShadowTrianglePointsUnk         = StaticRef<RxVertexIndex*>(0xC6A170);
static inline auto& s_ShadowTrianglePoints            = StaticRef<CVector*>(0xC6A174);
static inline auto& s_TransformedShadowTrianglePoints = StaticRef<CVector*>(0xC6A178);
static inline auto& s_SunPosNrm                       = StaticRef<CVector>(0x8D5244); // CVector(1.0, 1.0, -2.0)

void CStencilShadows::InjectHooks() {
    RH_ScopedClass(CStencilShadows);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x70F9E0);
    RH_ScopedInstall(Shutdown, 0x711390);
    RH_ScopedInstall(Process, 0x711D90);
    RH_ScopedInstall(GraphicsHighQuality, 0x70F9B0);
    RH_ScopedInstall(UpdateHierarchy, 0x710BC0);
    RH_ScopedInstall(RegisterStencilShadows, 0x711760);
    RH_ScopedInstall(RenderStencilShadows, 0x7113B0);
    RH_ScopedInstall(RenderForVehicle, 0x70FAE0);
    RH_ScopedInstall(RenderForObject, 0x710310);
    RH_ScopedInstall(Render, 0x710D50);
    RH_ScopedInstall(RenderBuffer, 0x710B50);
    RH_ScopedInstall(SunSetPositionFromEntity, 0x710AF0);
    RH_ScopedInstall(sub_710CC0, 0x710CC0);
}

// 0x70F9E0
void CStencilShadows::Init() {
    ZoneScoped;

    RwD3D9SetStencilClear(0);
    pFirstAvailableStencilShadowObject = m_StencilShadowObjects.data();
    pFirstActiveStencilShadowObject = nullptr;

    for (auto&& [i, obj] : rngv::enumerate(m_StencilShadowObjects)) {
        obj.m_pOwner                = nullptr;
        obj.m_NumShadowFaces        = 0;
        obj.m_Type                  = eStencilShadowObjType::NONE;
        obj.m_SizeOfShadowFacesData = 0;
        obj.m_FaceID                = 0;
        obj.m_ShadowFacesData       = 0;

        obj.m_pPrev = i ? &m_StencilShadowObjects[i - 1] : nullptr;
        obj.m_pNext = (i != m_StencilShadowObjects.size() - 1) ? &m_StencilShadowObjects[i + 1] : nullptr;
    }
}

// 0x711390
void CStencilShadows::Shutdown() {
    for (auto* obj = pFirstActiveStencilShadowObject; obj;) {
        auto* next = obj->m_pNext;
        obj->Destroy();
        obj = next;
    }
}

// 0x710D50
void CStencilShadows::Render(const CRGBA& color) {
    uiTempBufferIndicesStored  = 0;
    uiTempBufferVerticesStored = 0;

    const auto intColor = color.ToIntARGB();

    // Adds `numVerts` vertices as triangles, the indices are the count of what is already stored
    const auto AddTriangles = [&](const CVector* verts, uint32 numVerts) {
        sub_710CC0(numVerts, numVerts);

        const auto firstIdx  = uiTempBufferIndicesStored;
        const auto firstVert = uiTempBufferVerticesStored;
        for (auto i = 0u; i < numVerts; i++) {
            aTempBufferIndices[firstIdx + i] = (RxVertexIndex)(firstIdx + i);

            auto& vert     = TempBufferVertices.m_3d[firstVert + i];
            vert.objVertex = { verts[i].x, verts[i].y, verts[i].z };
            vert.color     = intColor;
        }
        uiTempBufferVerticesStored += numVerts;
        uiTempBufferIndicesStored  += numVerts;
    };

    if (!pFirstActiveStencilShadowObject) {
        return;
    }

    for (auto* shadow = pFirstActiveStencilShadowObject; shadow; shadow = shadow->m_pNext) {
        const auto* const verts    = shadow->m_ShadowFacesData;
        const auto        numQuads = (int32)shadow->m_SizeOfShadowFacesData / 6; // Two triangles at a time

        for (auto i = 0; i < numQuads; i++) {
            AddTriangles(verts + i * 6, 6);
        }

        // A triangle is left when there is an odd number of them
        if (((int32)shadow->m_SizeOfShadowFacesData / 3) & 1) {
            AddTriangles(verts + numQuads * 6, 3);
        }
    }

    if (uiTempBufferIndicesStored && uiTempBufferVerticesStored) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
        LittleTest();
        if (RwIm3DTransform(TempBufferVertices.m_3d, uiTempBufferVerticesStored, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, uiTempBufferIndicesStored);
            RwIm3DEnd();
        }
        uiTempBufferVerticesStored = uiTempBufferIndicesStored = 0;
    }
}

// unused
// 0x710AF0
void CStencilShadows::SunSetPositionFromEntity(const CEntity* entity) {
    if (!entity) {
        return;
    }
    s_SunPosNrm = entity->GetPosition().Normalized();
}

// 0x710B50
void CStencilShadows::RenderBuffer(const CVector& pos) {
    s_SunPosNrm = pos.Normalized(); 
}

// 0x710CC0
void CStencilShadows::sub_710CC0(int32 indices, int32 vertices) {
    if (uiTempBufferIndicesStored + indices < TOTAL_TEMP_BUFFER_INDICES
        && uiTempBufferVerticesStored + vertices < TOTAL_TEMP_BUFFER_3DVERTICES) {
        return;
    }

    if (!uiTempBufferIndicesStored || !uiTempBufferVerticesStored) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
    LittleTest();
    if (RwIm3DTransform(TempBufferVertices.m_3d, uiTempBufferVerticesStored, nullptr, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA)) {
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, aTempBufferIndices, uiTempBufferIndicesStored);
        RwIm3DEnd();
    }
    uiTempBufferVerticesStored = uiTempBufferIndicesStored = 0;
}

// 0x7113B0
void CStencilShadows::RenderStencilShadows() {
    ZoneScoped;

    if (!GraphicsHighQuality()) {
        return;
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEFLAT));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,                RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,            RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,                 RWRSTATE(rwBLENDZERO));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,                RWRSTATE(rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONMASK,      RWRSTATE(uint32(-1)));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONWRITEMASK, RWRSTATE(uint32(-1)));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTION,          RWRSTATE(rwSTENCILFUNCTIONALWAYS));
    RwRenderStateSet(rwRENDERSTATESTENCILFAIL,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILPASS,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONREF,       RWRSTATE(0));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONINCR));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLFRONT));

    Render(CRGBA{ 0, 0, 0, 255 });

    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONDECR));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));

    Render(CRGBA{ 0, 0, 0, 255 });

    // WTF is up with these states?
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(FALSE)); // same state
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,                RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(TRUE)); // same state
    RwRenderStateSet(rwRENDERSTATESRCBLEND,                 RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,                RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEFLAT));
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,            RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLNONE));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTIONREF,       RWRSTATE(1u));
    RwRenderStateSet(rwRENDERSTATESTENCILFUNCTION,          RWRSTATE(rwSTENCILFUNCTIONLESSEQUAL));
    RwRenderStateSet(rwRENDERSTATESTENCILFAIL,              RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILZFAIL,             RWRSTATE(rwSTENCILOPERATIONKEEP));
    RwRenderStateSet(rwRENDERSTATESTENCILPASS,              RWRSTATE(rwSTENCILOPERATIONKEEP));

    CSprite2d::InitPerFrame();
    CSprite2d::DrawRect(
        CRect{ 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT },
        CRGBA{ 0, 0, 0, (uint8)(50u * CTimeCycle::m_CurrentColours.m_nShadowStrength / 256) }
    );

    RwRenderStateSet(rwRENDERSTATESTENCILENABLE,            RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,             RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,              RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,        RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE,                RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATECULLMODE,                 RWRSTATE(rwCULLMODECULLBACK));
}

// Builds the volume of the shadow, the faces that are not facing the sun, and the sides that go from them away from it
static void BuildShadowVolume(CStencilShadowObject* object, CCollisionData* colData, CMatrix& matrix, float extrudeLength) {
    // The direction of the sun in the space of the entity
    CMatrix invMatrix;
    Invert(matrix, invMatrix);
    const auto sunDir   = s_SunPosNrm;
    const auto lightDir = invMatrix.TransformPoint(sunDir) - invMatrix.TransformPoint(CVector{});

    object->m_SizeOfShadowFacesData = 0;

    const auto numVerts = (int32)colData->m_nNumShadowVertices;
    for (auto i = 0; i < numVerts; i++) {
        colData->GetShadTrianglePoint(s_ShadowTrianglePoints[i], i);
    }
    TransformPoints(s_TransformedShadowTrianglePoints, numVerts, matrix, s_ShadowTrianglePoints);

    // The edges of the faces that are not facing the sun, an edge that shows up twice is not on the outline
    uint16 numEdges = 0;
    auto*  edges    = reinterpret_cast<uint16(*)[2]>(s_ShadowTrianglePointsUnk);
    const auto AddOrRemoveEdge = [&](uint16 from, uint16 to) {
        for (uint16 i = 0; i < numEdges; i++) {
            if ((edges[i][0] == from && edges[i][1] == to) || (edges[i][0] == to && edges[i][1] == from)) {
                if (numEdges > 1) {
                    edges[i][0] = edges[numEdges - 1][0];
                    edges[i][1] = edges[numEdges - 1][1];
                }
                numEdges--;
                return;
            }
        }
        edges[numEdges][0] = from;
        edges[numEdges][1] = to;
        numEdges++;
    };

    // The data is as big as the object says it is (Which is kept in `m_FaceID`)
    const auto HasSpaceFor = [&](uint32 numPoints) {
        return object->m_FaceID > object->m_SizeOfShadowFacesData + numPoints;
    };

    // How far the shadow goes
    const auto extrude = sunDir * extrudeLength;

    for (auto i = 0; i < (int32)object->m_NumShadowFaces; i++) {
        const auto& tri = colData->m_pShadowTriangles[i];

        const auto& local0 = s_ShadowTrianglePoints[tri.vA];
        const auto& local1 = s_ShadowTrianglePoints[tri.vB];
        const auto& local2 = s_ShadowTrianglePoints[tri.vC];
        const auto& world0 = s_TransformedShadowTrianglePoints[tri.vA];
        const auto& world1 = s_TransformedShadowTrianglePoints[tri.vB];
        const auto& world2 = s_TransformedShadowTrianglePoints[tri.vC];

        const auto edge1 = local1 - local0;
        const auto edge2 = local2 - local0;
        const CVector normal{
            edge2.z * edge1.y - edge2.y * edge1.z,
            edge2.x * edge1.z - edge2.z * edge1.x,
            edge2.y * edge1.x - edge2.x * edge1.y
        };

        auto* const out = &object->m_ShadowFacesData[object->m_SizeOfShadowFacesData];
        if (normal.x * lightDir.x + normal.y * lightDir.y + lightDir.z * normal.z < 0.0f) {
            // Faces the sun, so only the cap that is far from it
            if (!HasSpaceFor(3)) {
                break;
            }
            out[0] = world0 + extrude;
            out[1] = world2 + extrude;
            out[2] = world1 + extrude;
        } else {
            AddOrRemoveEdge(tri.vA, tri.vB);
            AddOrRemoveEdge(tri.vB, tri.vC);
            AddOrRemoveEdge(tri.vC, tri.vA);
            if (!HasSpaceFor(3)) {
                break;
            }
            out[0] = world0;
            out[1] = world2;
            out[2] = world1;
        }
        object->m_SizeOfShadowFacesData += 3;
    }

    // The sides of the volume are what is left from the outline
    for (uint16 i = 0; i < numEdges; i++) {
        const auto& pointA = s_TransformedShadowTrianglePoints[edges[i][0]];
        const auto& pointB = s_TransformedShadowTrianglePoints[edges[i][1]];

        if (!HasSpaceFor(6)) {
            break;
        }
        auto* const out = &object->m_ShadowFacesData[object->m_SizeOfShadowFacesData];
        out[0] = pointA;
        out[1] = pointB;
        out[2] = pointA + extrude;
        out[3] = pointB;
        out[4] = pointB + extrude;
        out[5] = pointA + extrude;
        object->m_SizeOfShadowFacesData += 6;
    }
}

// 0x710310
void CStencilShadows::RenderForObject(CStencilShadowObject* object) {
    ZoneScoped;

    auto* const entity  = object->m_pOwner;
    auto* const colData = entity->GetColModel()->m_pColData;
    if (!colData || (int32)colData->m_nNumShadowTriangles != (int32)object->m_NumShadowFaces) {
        return;
    }

    if (!entity->m_matrix) {
        entity->AllocateMatrix();
        entity->m_placement.UpdateMatrix(entity->m_matrix);
    }

    BuildShadowVolume(object, colData, *entity->m_matrix, 60.0f);
}

// 0x70FAE0
void CStencilShadows::RenderForVehicle(CStencilShadowObject* object) {
    ZoneScoped;

    auto* const entity = object->m_pOwner;

    // The shadows of the aircraft are much longer
    const auto vehicle       = entity->GetIsTypeVehicle() ? entity->AsVehicle() : nullptr;
    const auto extrudeLength = vehicle && (vehicle->m_nVehicleSubType == VEHICLE_TYPE_PLANE || vehicle->m_nVehicleSubType == VEHICLE_TYPE_HELI)
        ? 40.0f
        : 5.0f;

    auto* const colData = entity->GetColModel()->m_pColData;
    if (!colData || (int32)colData->m_nNumShadowTriangles != (int32)object->m_NumShadowFaces) {
        return;
    }

    if (!entity->m_matrix) {
        entity->AllocateMatrix();
        entity->m_placement.UpdateMatrix(entity->m_matrix);
    }

    // The bikes lean to the sides, and so does the shadow
    CMatrix* matrix = entity->m_matrix;
    if (vehicle && vehicle->m_nVehicleType == VEHICLE_TYPE_BIKE) {
        auto* const bike = vehicle->AsBike();
        bike->CalculateLeanMatrix();
        matrix = &bike->m_mLeanMatrix;
    }

    BuildShadowVolume(object, colData, *matrix, extrudeLength);
}

// 0x711D90
void CStencilShadows::Process(CVector& cameraPos) {
    ZoneScoped;

    if (!GraphicsHighQuality()) {
        return;
    }

    static uint8 s_RegisterShadowCounter{}, s_RenderForObjCounter{};

    RegisterStencilShadows(cameraPos, ++s_RegisterShadowCounter % 8);

    // why do we even do this?
    s_ShadowTrianglePointsUnk         = (RxVertexIndex*)CMemoryMgr::Malloc(12'288 * sizeof(RxVertexIndex));
    s_ShadowTrianglePoints            = (CVector*)CMemoryMgr::Malloc(2'048 * sizeof(CVector));
    s_TransformedShadowTrianglePoints = (CVector*)CMemoryMgr::Malloc(2'048 * sizeof(CVector));

    auto i{ 0 };
    for (auto* obj = pFirstActiveStencilShadowObject; obj; obj = obj->m_pNext) {
        switch (obj->m_Type) {
        case eStencilShadowObjType::OBJECT:
            if ((i++ % 4) == s_RenderForObjCounter) {
                RenderForObject(obj);
            }
            break;
        case eStencilShadowObjType::VEHICLE:
            RenderForVehicle(obj);
            break;
        default:
            break;
        }
    }
    s_RenderForObjCounter = (s_RenderForObjCounter + 1) % 4;

    CMemoryMgr::Free(std::exchange(s_ShadowTrianglePointsUnk, nullptr));
    CMemoryMgr::Free(std::exchange(s_ShadowTrianglePoints, nullptr));
    CMemoryMgr::Free(std::exchange(s_TransformedShadowTrianglePoints, nullptr));
}

// 0x70F9B0
bool CStencilShadows::GraphicsHighQuality() {
    return ::GraphicsHighQuality();
}

// 0x710BC0
void CStencilShadows::UpdateHierarchy(CStencilShadowObject*& firstAvailable, CStencilShadowObject*& firstActive, CStencilShadowObject* newOne) {
    if (auto* prev = newOne->m_pPrev) {
        auto* next = newOne->m_pNext;
        if (next) {
            next->m_pPrev = prev;
            newOne->m_pPrev->m_pNext = newOne->m_pNext;
        } else {
            prev->m_pNext = nullptr;
        }
    } else {
        auto* next     = newOne->m_pNext;
        firstAvailable = next;
        if (next) {
            next->m_pPrev = nullptr;
        }
    }
    newOne->m_pNext = firstActive;
    newOne->m_pPrev = nullptr;
    firstActive     = newOne;

    if (newOne->m_pNext) {
        newOne->m_pNext->m_pPrev = newOne;
    }
}

// 0x711760
void CStencilShadows::RegisterStencilShadows(CVector& cameraPos, bool doNotCreateNew) {
    ZoneScoped;

    // The owner is gone, or it is an object that is not in the world anymore
    const auto IsOwnerUnusable = [](CEntity* entity) {
        if (!entity) {
            return true;
        }
        if (entity->GetIsTypeObject()) {
            const auto& flags = entity->AsObject()->objectFlags;
            return !entity->m_bIsVisible || flags.bIsExploded || flags.bIsBroken;
        }
        return false;
    };

    // How far the entity is from the camera (Squared), from the bounding sphere of it and not the center
    const auto DistanceSq = [&](CEntity* entity) {
        auto* const colModel = entity->GetColModel();
        const auto  center   = entity->TransformFromObjectSpace(colModel->m_boundSphere.m_vecCenter);
        const auto  toCamera = cameraPos - center;
        const auto  dist     = toCamera.Magnitude();
        const auto  radius   = colModel->m_boundSphere.m_fRadius;
        const auto  sign     = dist < radius ? -1.0f : 1.0f;
        return sign * (dist - radius) * (dist - radius);
    };

    // The next one is taken before, because the object is taken out of the list when it is destroyed
    const auto ForEachActiveObject = [](auto&& fn) {
        for (auto* obj = pFirstActiveStencilShadowObject; obj;) {
            auto* const next = obj->m_pNext;
            fn(obj);
            obj = next;
        }
    };

    if (doNotCreateNew) {
        ForEachActiveObject([&](CStencilShadowObject* obj) {
            if (IsOwnerUnusable(obj->m_pOwner)) {
                obj->Destroy();
            }
        });
        return;
    }

    // The ones that are too far, or are in another area, are not needed anymore
    ForEachActiveObject([&](CStencilShadowObject* obj) {
        auto* const entity = obj->m_pOwner;

        const auto remove = IsOwnerUnusable(entity) || [&] {
            switch (obj->m_Type) {
            case eStencilShadowObjType::OBJECT:  return DistanceSq(entity) > 2500.0f || !entity->IsInCurrentArea();
            case eStencilShadowObjType::VEHICLE: return DistanceSq(entity) > 2500.0f;
            default:                             return false;
            }
        }();
        if (remove) {
            obj->Destroy();
        }
    });

    if (!pFirstAvailableStencilShadowObject) {
        return;
    }

    const auto HasShadowInfo = [](CEntity* entity) {
        auto* const colModel = entity->GetColModel();
        return colModel && colModel->m_pColData && colModel->m_pColData->bHasShadowInfo;
    };
    const auto HasShadowObject = [](CEntity* entity) {
        for (auto* obj = pFirstActiveStencilShadowObject; obj; obj = obj->m_pNext) {
            if (obj->m_pOwner == entity) {
                return true;
            }
        }
        return false;
    };

    // Returns false when there are no more shadow objects to give
    const auto CreateFor = [&](CEntity* entity, eStencilShadowObjType type) {
        if (!pFirstAvailableStencilShadowObject) {
            return false;
        }
        pFirstAvailableStencilShadowObject->Create(entity, type);
        return pFirstAvailableStencilShadowObject != nullptr;
    };

    for (auto& vehicle : GetVehiclePool()->GetAllValid()) {
        if (HasShadowInfo(&vehicle) && vehicle.m_pCollisionList.m_node && !HasShadowObject(&vehicle) && DistanceSq(&vehicle) <= 2500.0f) {
            if (!CreateFor(&vehicle, eStencilShadowObjType::VEHICLE)) {
                return;
            }
        }
    }
    if (!pFirstAvailableStencilShadowObject) {
        return;
    }

    // The objects close to the camera that have a shadow
    const auto SectorCoord = [](float v) {
        return (int32)std::floor((v - 50.0f) * 0.02f + 60.0f);
    };
    const auto startX = std::max(SectorCoord(cameraPos.x), 0);
    const auto startY = std::max(SectorCoord(cameraPos.y), 0);
    const auto endX   = std::min((int32)std::floor((cameraPos.x + 50.0f) * 0.02f + 60.0f), 119);
    const auto endY   = std::min((int32)std::floor((cameraPos.y + 50.0f) * 0.02f + 60.0f), 119);

    CWorld::AdvanceCurrentScanCode();

    // Returns false when there are no more shadow objects to give
    const auto ProcessEntity = [&](CEntity* entity) {
        if (entity->m_bIsProcObject) {
            return true;
        }
        if (entity->GetIsTypeObject() && IsOwnerUnusable(entity)) {
            return true;
        }
        if (!HasShadowInfo(entity) || entity->IsScanCodeCurrent()) {
            return true;
        }
        entity->SetCurrentScanCode();

        if (!entity->IsInCurrentArea() || CModelInfo::GetModelInfo(entity->m_nModelIndex)->GetModelType() != MODEL_INFO_ATOMIC) {
            return true;
        }
        if (HasShadowObject(entity) || DistanceSq(entity) > 2500.0f) {
            return true;
        }
        return CreateFor(entity, eStencilShadowObjType::OBJECT);
    };

    for (auto sy = startY; sy <= endY; sy++) {
        for (auto sx = startX; sx <= endX; sx++) {
            const auto x = std::min(std::max(sx, 0), 119);
            const auto y = std::min(std::max(sy, 0), 119);

            for (auto* node = CWorld::GetSector(x, y).Buildings.GetNode(), *next = node ? node->Next : nullptr; node; node = next, next = node ? node->Next : nullptr) {
                if (!ProcessEntity(node->Item)) {
                    return;
                }
            }
            for (auto* node = CWorld::GetRepeatSector(sx, sy).Objects.GetNode(), *next = node ? node->Next : nullptr; node; node = next, next = node ? node->Next : nullptr) {
                if (!ProcessEntity(node->Item)) {
                    return;
                }
            }
        }
    }
}
