#include "StdInc.h"

#include "CarFXRenderer.h"
#include "CustomCarEnvMapPipeline.h"
#include "ClothesBuilder.h"

void CCarFXRenderer::InjectHooks() {
    RH_ScopedClass(CCarFXRenderer);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(RegisterPlugins, 0x5D5B00);
    RH_ScopedInstall(Initialise, 0x5D5AC0);
    RH_ScopedInstall(InitialiseDirtTexture, 0x5D5BC0);
    RH_ScopedInstall(Shutdown, 0x5D5AD0);
    RH_ScopedInstall(PreRenderUpdate, 0x5D5B10);
    RH_ScopedInstall(IsCCPCPipelineAttached, 0x5D5B80);
    RH_ScopedInstall(CustomCarPipeAtomicSetup, 0x5D5B20);
    RH_ScopedInstall(CustomCarPipeClumpSetup, 0x5D5B40);
    RH_ScopedInstall(SetCustomFXAtomicRenderPipelinesVMICB, 0x5D5B60);
    RH_ScopedInstall(SetFxEnvMapLightMult, 0x5D5BA0);
}

// 0x5D5B00
bool CCarFXRenderer::RegisterPlugins() {
    return CCustomCarEnvMapPipeline::RegisterPlugin() != 0;
}

// 0x5D5AC0
bool CCarFXRenderer::Initialise() {
    return CCustomCarEnvMapPipeline::CreatePipe() != 0;
}

// 0x5D5BC0
bool CCarFXRenderer::InitialiseDirtTexture() {
    CTxdStore::ScopedTXDSlot txd{ "vehicle" };

    auto* const grunge = RwTextureRead("vehiclegrunge256", nullptr);
    RwTextureSetFilterMode(grunge, rwFILTERLINEAR);

    const auto width  = RwRasterGetWidth(RwTextureGetRaster(grunge));
    const auto height = RwRasterGetHeight(RwTextureGetRaster(grunge));

    // Each copy fades the grunge in from white, the first one is clean and the last one is almost the original
    for (auto i = 0; i < NUM_DIRT_TEXTURES; i++) {
        auto* const texture = CClothesBuilder::CopyTexture(grunge);
        ms_aDirtTextures[i] = texture;
        RwTextureSetName(texture, "vehiclegrunge256");

        auto* const raster = RwTextureGetRaster(texture);
        auto*       pixel  = RwRasterLock(raster, 0, rwRASTERLOCKREADWRITE);
        // The rows are taken as `width * 4` bytes long, like the original does
        for (auto p = 0; p < width * height; p++, pixel += 4) {
            for (auto c = 0; c < 3; c++) { // The alpha stays as it is
                pixel[c] = static_cast<RwUInt8>(pixel[c] * i / NUM_DIRT_TEXTURES + (255 * NUM_DIRT_TEXTURES - 255 * i) / NUM_DIRT_TEXTURES);
            }
        }
        RwRasterUnlock(raster);
    }

    return true;
}

// 0x5D5AD0
void CCarFXRenderer::Shutdown() {
    for (auto& texture : ms_aDirtTextures) {
        RwTextureDestroy(texture);
    }
    CCustomCarEnvMapPipeline::DestroyPipe();
}

// 0x5D5B10
void CCarFXRenderer::PreRenderUpdate() {
    CCustomCarEnvMapPipeline::PreRenderUpdate();
}

// 0x5D5B80
bool CCarFXRenderer::IsCCPCPipelineAttached(RpAtomic* atomic) {
    return GetPipelineID(atomic) == 0x53F2009A;
}

// 0x5D5B20
void CCarFXRenderer::CustomCarPipeAtomicSetup(RpAtomic* atomic) {
    CCustomCarEnvMapPipeline::CustomPipeAtomicSetup(atomic);
}

// 0x5D5B40
void CCarFXRenderer::CustomCarPipeClumpSetup(RpClump* clump) {
    RpClumpForAllAtomics(clump, SetCustomFXAtomicRenderPipelinesVMICB, nullptr);
}

// 0x5D5B60
RpAtomic* CCarFXRenderer::SetCustomFXAtomicRenderPipelinesVMICB(RpAtomic* atomic, void* data) {
    CCustomCarEnvMapPipeline::CustomPipeAtomicSetup(atomic);
    return atomic;
}

// 0x5D5BA0
void CCarFXRenderer::SetFxEnvMapLightMult(float multiplier) {
    gSpecIntensity = multiplier;
}
