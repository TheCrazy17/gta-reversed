#include "StdInc.h"

#include "RwHelper.h"

void RwHelperInjectHooks() {
    RH_ScopedNamespaceName("RwHelper");
    RH_ScopedCategoryGlobal();

    RH_ScopedGlobalInstall(GetEventGlobalGroup, 0x4ABA50);
    RH_ScopedGlobalInstall(GetNameAndDamage, 0x5370A0);
    RH_ScopedGlobalInstall(GetFirstAtomicCallback, 0x734810);
    RH_ScopedGlobalInstall(GetFirstAtomic, 0x734820);
    RH_ScopedGlobalInstall(Get2DEffectAtomicCallback, 0x734850);
    RH_ScopedGlobalInstall(Get2DEffectAtomic, 0x734880);
    RH_ScopedGlobalInstall(GetFirstObjectCallback, 0x7348B0);
    RH_ScopedGlobalInstall(GetFirstObject, 0x7348C0);
    RH_ScopedGlobalInstall(GetFirstFrameCallback, 0x7348F0);
    RH_ScopedGlobalInstall(GetFirstChild, 0x734900);
    RH_ScopedGlobalInstall(SkinAtomicGetHAnimHierarchCB, 0x734A20);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromSkinClump, 0x734A40);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromFrameCB, 0x734A70);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromFrame, 0x734AB0);
    RH_ScopedGlobalInstall(GetAnimHierarchyFromClump, 0x734B10);
    RH_ScopedGlobalInstall(AtomicRemoveAnimFromSkinCB, 0x734B90);
    RH_ScopedGlobalInstall(RpAtomicConvertGeometryToTL, 0x734BE0);
    RH_ScopedGlobalInstall(RpAtomicConvertGeometryToTS, 0x734C20);
    RH_ScopedGlobalInstall(atomicConvertGeometryToTL, 0x734C60);
    RH_ScopedGlobalInstall(RpClumpConvertGeometryToTL, 0x734CB0);
    RH_ScopedGlobalInstall(atomicConvertGeometryToTS, 0x734CE0);
    RH_ScopedGlobalInstall(RpClumpConvertGeometryToTS, 0x734D30);
    RH_ScopedGlobalInstall(forceLinearFilteringAtomicsCB, 0x734DA0);
    RH_ScopedGlobalInstall(RpGeometryReplaceOldMaterialWithNewMaterial, 0x734DE0);
    RH_ScopedGlobalInstall(RwTexDictionaryFindHashNamedTexture, 0x734E50);
    RH_ScopedGlobalInstall(SetFilterModeOnClumpsTextures, 0x734DC0);
    RH_ScopedGlobalInstall(forceLinearFilteringMatTexturesCB, 0x734D60);
    RH_ScopedGlobalInstall(SetFilterModeOnAtomicsTextures, 0x734D80);
    RH_ScopedGlobalInstall(RemoveRefsCB, 0x7226D0);
}

// 0x4ABA50
CEventGlobalGroup* GetEventGlobalGroup() {
    static auto& globalEvents = StaticRef<CEventGlobalGroup*>(0xA9AF6C);

    if (globalEvents)
        return globalEvents;

    globalEvents = new CEventGlobalGroup(nullptr);
    return globalEvents;
}

// TODO: Check `outName` size (to avoid buffer overflow)
// 0x5370A0
void GetNameAndDamage(const char* name, char* objName, bool& bIsDamageModel) {
    const size_t nodesz = strlen(name);

    const auto TerminatedCopy = [=](size_t offset) {
        strncpy_s(objName, nodesz - offset + 1, name, nodesz - offset);
        objName[nodesz - offset] = 0;
    };

    // EndsWith "_dam"
    if (name[nodesz - 4] == '_' && name[nodesz - 3] == 'd' && name[nodesz - 2] == 'a' && name[nodesz - 1] == 'm'
    ) {
        bIsDamageModel = true;
        TerminatedCopy(sizeof("_dam") - 1);
    }
    else {
        bIsDamageModel = false;
        // EndsWith "_l0" or "_L0"
        if (name[nodesz - 3] == '_' &&
            (name[nodesz - 2] == 'L' || name[nodesz - 2] == 'l') && name[nodesz - 1] == '0'
        ) {
            TerminatedCopy(sizeof("_l0") - 1);
        } else
            strcpy_s(objName, strlen(name) + 1, name);
    }
}

// 0x734810
RpAtomic* GetFirstAtomicCallback(RpAtomic* atomic, void* data) {
    *(RpAtomic**)(data) = atomic;
    return nullptr;
}

// 0x734820
RpAtomic* GetFirstAtomic(RpClump* clump) {
    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, GetFirstAtomicCallback, &atomic);
    return atomic;
}

// 0x734850
RpAtomic* Get2DEffectAtomicCallback(RpAtomic* atomic, void* data) {
    if (RpGeometryGet2dFxCount(RpAtomicGetGeometry(atomic)) > 0) {
        *static_cast<RpAtomic**>(data) = atomic;
        return nullptr;
    }
    return atomic;
}

// 0x734880
RpAtomic* Get2DEffectAtomic(RpClump* clump) {
    RpAtomic* atomic{};
    RpClumpForAllAtomics(clump, Get2DEffectAtomicCallback, &atomic);
    return atomic;
}

// 0x7348B0
RwObject* GetFirstObjectCallback(RwObject* object, void* data) {
    *(RwObject**)(data) = object;
    return nullptr;
}

// 0x7348C0
RwObject* GetFirstObject(RwFrame* frame) {
    RwObject* obj{};
    RwFrameForAllObjects(frame, GetFirstObjectCallback, &obj);
    return obj;
}

// 0x7348F0
RwFrame* GetFirstFrameCallback(RwFrame* frame, void* data) {
    *(RwFrame**)(data) = frame;
    return nullptr;
}

// 0x734900
RwFrame* GetFirstChild(RwFrame* frame) {
    RwFrame* child{};
    RwFrameForAllChildren(frame, GetFirstFrameCallback, &child);
    return child;
}

// 0x734A70
RwFrame* GetAnimHierarchyFromFrameCB(RwFrame* frame, void* data) {
    if (const auto hier = RpHAnimFrameGetHierarchy(frame)) {
        *static_cast<RpHAnimHierarchy**>(data) = hier;
        return nullptr;
    }
    RwFrameForAllChildren(frame, GetAnimHierarchyFromFrameCB, data);
    return frame;
}

// 0x734AB0
RpHAnimHierarchy* GetAnimHierarchyFromFrame(RwFrame* frame) {
    assert(frame);
    RpHAnimHierarchy* hier = RpHAnimFrameGetHierarchy(frame);
    if (!hier) {
        RwFrameForAllChildren(frame, GetAnimHierarchyFromFrameCB, &hier);
    }
    return hier;
}

// 0x734B10
RpHAnimHierarchy* GetAnimHierarchyFromClump(RpClump* clump) {
    assert(clump);
    return GetAnimHierarchyFromFrame(RpClumpGetFrame(clump));
}

// 0x734A40
RpHAnimHierarchy* GetAnimHierarchyFromSkinClump(RpClump* clump) {
    RpHAnimHierarchy* anim{};
    RpClumpForAllAtomics(clump, SkinAtomicGetHAnimHierarchCB, &anim);
    return anim;
}

// name not from Android
// 0x734A20
RpAtomic* SkinAtomicGetHAnimHierarchCB(RpAtomic* atomic, void* data) {
    *(RpHAnimHierarchy**)(data) = RpSkinAtomicGetHAnimHierarchy(atomic);
    return nullptr;
}

// 0x734B90
RpAtomic* AtomicRemoveAnimFromSkinCB(RpAtomic* atomic, void* data) {
    if (RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic))) {
        if (RpHAnimHierarchy* hier = RpSkinAtomicGetHAnimHierarchy(atomic)) {
            RtAnimAnimation*& currAnim = hier->currentAnim->pCurrentAnim;
            if (currAnim) {
                RtAnimAnimationDestroy(currAnim);
            }
            currAnim = nullptr;
        }
    }
    return atomic;
}

// 0x734BE0
bool RpAtomicConvertGeometryToTL(RpAtomic* atomic) {
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    auto flags = RpGeometryGetFlags(geometry);
    if (flags & rpGEOMETRYNATIVE || !(flags & rpGEOMETRYTRISTRIP))
        return false;

    RpGeometryLock(geometry, rpGEOMETRYLOCKALL);
    RpGeometrySetFlags(geometry, flags & ~rpGEOMETRYTRISTRIP);
    RpGeometryUnlock(geometry);

    return true;
}

// 0x734C20
bool RpAtomicConvertGeometryToTS(RpAtomic* atomic) {
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    auto flags = RpGeometryGetFlags(geometry);
    if (flags & rpGEOMETRYNATIVE || flags & rpGEOMETRYTRISTRIP)
        return false;

    RpGeometryLock(geometry, rpGEOMETRYLOCKALL);
    RpGeometrySetFlags(geometry, flags | rpGEOMETRYTRISTRIP);
    RpGeometryUnlock(geometry);

    return true;
}

// 0x734C60
RpAtomic* atomicConvertGeometryToTL(RpAtomic* atomic, void* data) {
    if (!RpAtomicConvertGeometryToTL(atomic)) {
        *(bool*)(data) = false;
    }
    return atomic;
}

// 0x734CB0
bool RpClumpConvertGeometryToTL(RpClump* clump) {
    bool success{ true };
    RpClumpForAllAtomics(clump, atomicConvertGeometryToTL, &success);
    return success;
}

// 0x734CE0
RpAtomic* atomicConvertGeometryToTS(RpAtomic* atomic, void* data) {
    if (!RpAtomicConvertGeometryToTS(atomic)) {
        *(bool*)(data) = false;
    }
    return atomic;
}

// 0x734D30
bool RpClumpConvertGeometryToTS(RpClump* clump) {
    bool success{ true };
    RpClumpForAllAtomics(clump, atomicConvertGeometryToTS, &success);
    return success;
}

// 0x734D60
RpMaterial* forceLinearFilteringMatTexturesCB(RpMaterial* material, void* data) {
    if (RwTexture* texture = RpMaterialGetTexture(material)) {
        RwTextureSetFilterMode(texture, (RwTextureFilterMode)((unsigned)data));
    }
    return material;
}

// 0x734D80
bool SetFilterModeOnAtomicsTextures(RpAtomic* atomic, RwTextureFilterMode filtering) {
    RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), forceLinearFilteringMatTexturesCB, (void*)(unsigned)filtering);
    return true;
}

// 0x734DA0
RpAtomic* forceLinearFilteringAtomicsCB(RpAtomic* atomic, void* data) {
    SetFilterModeOnAtomicsTextures(atomic, (RwTextureFilterMode)((unsigned)data));
    return atomic;
}

// 0x734DC0
bool SetFilterModeOnClumpsTextures(RpClump* clump, RwTextureFilterMode filtering) {
    RpClumpForAllAtomics(clump, forceLinearFilteringAtomicsCB, (void*)(unsigned)filtering);
    return true;
}

// 0x734DE0
bool RpGeometryReplaceOldMaterialWithNewMaterial(RpGeometry* geometry, RpMaterial* oldMaterial, RpMaterial* newMaterial) {
    bool replaced = false;
    for (auto i = 0; i < geometry->mesh->numMeshes; i++) {
        auto* const mesh = RpGeometryGetMesh(geometry, i);
        if (mesh->material != oldMaterial) {
            continue;
        }
        const auto matIdx = _rpMaterialListFindMaterialIndex(&geometry->matList, mesh->material);
        RpMaterialDestroy(mesh->material);
        geometry->matList.materials[matIdx] = newMaterial;
        mesh->material = newMaterial;
        newMaterial->refCount++;
        replaced = true;
    }
    return replaced;
}

// 0x734E50
RwTexture* RwTexDictionaryFindHashNamedTexture(RwTexDictionary* txd, uint32 hash) {
    for (auto* it = rwLinkListGetFirstLLLink(&txd->texturesInDict); it != rwLinkListGetTerminator(&txd->texturesInDict); it = rwLLLinkGetNext(it)) {
        auto* const texture = rwLLLinkGetData(it, RwTexture, lInDictionary);
        if (CKeyGen::GetUppercaseKey(texture->name) == hash) {
            return texture;
        }
    }
    return nullptr;
}

// 0x734FC0
RpClump* RpClumpGetBoundingSphere(RpClump* clump, RwSphere* sphere, bool bUseLTM) {
    return ((RpClump * (__cdecl*)(RpClump*, RwSphere*, bool))0x734FC0)(clump, sphere, bUseLTM);
}

// 0x735140
void SkinGetBonePositions(RpClump* clump) {
    ((void(__cdecl*)(RpClump*))0x735140)(clump);
}

// 0x7352D0
void SkinSetBonePositions(RpClump* clump) {
    ((void(__cdecl*)(RpClump*))0x7352D0)(clump);
}

// 0x735360
void SkinGetBonePositionsToTable(RpClump* clump, RwV3d* table) {
    ((void(__cdecl*)(RpClump*, RwV3d*))0x735360)(clump, table);
}

// 0x7226D0
RpAtomic* RemoveRefsCB(RpAtomic* atomic, void* data) {
    UNUSED(data);
    auto* modelInfo = CVisibilityPlugins::GetModelInfo(atomic);
    modelInfo->RemoveRef();
    return atomic;
}

// 0x7226F0
void RemoveRefsForAtomic(RpClump* clump) {
    RpClumpForAllAtomics(clump, RemoveRefsCB, nullptr);
}
