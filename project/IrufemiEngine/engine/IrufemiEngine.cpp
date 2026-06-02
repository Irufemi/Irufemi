#include "IrufemiEngine.h"

IrufemiEngine::IrufemiEngine() = default;

#include "Core/Math/Math.h"
#include "Core/Math/Random/Random.h"

#include <DbgHelp.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <format>
#include <string>

#include "../Resource/Audio/Bgm.h"
#include "../Resource/Audio/Se.h"
#include "../Resource/Texture/Texture.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Graphics/DirectX/DirectXUtils.h"
#include "Manager/DebugUI.h"
#include "Manager/PrimitiveManager.h"
#include "Renderer/Core/BaseResource.h"
#include "Renderer/ParticleGPU/GPUParticleManager.h"
#include "Renderer/Effect/Effect.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Renderer/LineInstanced/LineResource.h"
#include "Renderer/Object2D/Object2DResource.h"
#include "Renderer/Object2D/Primitive/Circle2D.h"
#include "Renderer/Object2D/Sprite/Sprite.h"
#include "Renderer/Object2D/Text/Text.h"
#include "Renderer/Object3D/AnimationModel/AnimationModel.h"
#include "Renderer/Object3D/BaseModel/BaseModel.h"
#include "Renderer/Object3D/StaticModelObject/StaticModelObject.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Renderer/Object3D/Primitive/Primitive3DObject.h"

#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Renderer/ParticleGPU/ParticleObject.h"
#include "Renderer/Region/ModelRegion.h"
#include "Renderer/Region/PrimitiveRegion.h"
#include "Renderer/Skybox/Skybox.h"
#include "Graphics/Data/VertexData.h"
#include "Renderer/VoxelParticle/VoxelParticleSystem.h"
#include "Renderer/VoxelParticle/VoxelParticleManager.h"
#include "Renderer/ParticleGPU/GPUParticleManager.h"
#include "Framework/IScene.h"
#include "Framework/Component/ComponentFactory.h"

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

// 繝・せ繝医Λ繧ｯ繧ｿ
IrufemiEngine::~IrufemiEngine() { Finalize(); }

// 蛻晄悄蛹・
void IrufemiEngine::Initialize(const std::wstring &title,
                               const int32_t &clientWidth,
                               const int32_t &clientHeight) {
  /*CrashHandler*/
  SetUnhandledExceptionFilter(WinApp::ExportDump);

  // 繧ｳ繝ｳ繝昴・繝阪Φ繝医・繝輔ぃ繧ｯ繝医Μ逋ｻ骭ｲ
  ComponentFactory::RegisterAllCoreComponents();

  // 譎る俣險域ｸｬ縺ｮ髢句ｧ・
  startTime_ = std::chrono::steady_clock::now();
  lastFrameTime_ = startTime_;

  // WinApp 繧偵お繝ｳ繧ｸ繝ｳ蜀・〒逕滓・繝ｻ蛻晄悄蛹・COM 蛻晄悄蛹悶ｂ縺薙％縺ｧ螳滓命縺輔ｌ繧・
  winApp_ = std::make_unique<WinApp>();
  if (!winApp_->Initialize(GetModuleHandle(nullptr), clientWidth, clientHeight,
                           title.c_str())) {
    assert(false && "WinApp::Initialize failed");
    return;
  }

  // 繝ｭ繧ｰ繧貞・縺帙ｋ繧医≧縺ｫ縺吶ｋ
  log_ = std::make_unique<Log>();
  log_->Initialize();

  // 荵ｱ謨ｰ繧ｨ繝ｳ繧ｸ繝ｳ縺ｮ繧ｷ繝ｼ繝峨ｒ險ｭ螳・
  Random::SeedEngine();

  // AudioManager縺ｮ逕滓・繝ｻMedia Foundation縺ｮ蛻晄悄蛹・
  audioManager_ = std::make_unique<AudioManager>();
  audioManager_->StartUp();
  // AudioManager縺ｮ蛻晄悄蛹・
  audioManager_->Initialize();
  // "resources"繝輔か繝ｫ繝縺九ｉ髻ｳ螢ｰ繝輔ぃ繧､繝ｫ繧偵☆縺ｹ縺ｦ繝ｭ繝ｼ繝・
  audioManager_->LoadAllSoundsFromFolder("resources/");
  Bgm::SetAudioManager(audioManager_.get());
  Se::SetAudioManager(audioManager_.get());

  // DirectX 蝓ｺ逶､
  dxCommon_ = std::make_unique<DirectXCommon>();
  dxCommon_->SetLog(log_.get());
  dxCommon_->SetEngine(this);
  dxCommon_->Initialize(winApp_->GetHwnd(), winApp_->GetClientWidth(),
                        winApp_->GetClientHeight());

  BaseResource::SetDirectXCommon(dxCommon_.get());
  BaseRegion::SetDirectXCommon(dxCommon_.get());
  Line3DRegion::SetDirectXCommon(dxCommon_.get());

  // --- Dynamic Constant Buffer 縺ｮ蛻晄悄蛹・---
  materialBufferManager_ = std::make_unique<DynamicConstantBuffer<Material>>();
  materialBufferManager_->Initialize(dxCommon_.get(),
                                     65536); // 譛螟ｧ6荳・が繝悶ず繧ｧ繧ｯ繝・

  transformBufferManager_ =
      std::make_unique<DynamicConstantBuffer<TransformationMatrix>>();
  transformBufferManager_->Initialize(dxCommon_.get(),
                                      65536); // 譛螟ｧ6荳・が繝悶ず繧ｧ繧ｯ繝・

  // SRV 繝・せ繧ｯ繝ｪ繝励ち繝励・繝ｫ
  {
    DescriptorPool *srvPool = dxCommon_->GetSrvPool();

    // 豕ｨ蜈･
    Texture::SetDescriptorPool(srvPool);
    BaseRegion::SetSrvAllocator(srvPool);

    Line3DRegion::SetSrvAllocator(srvPool);
  }

  // 繝・け繧ｹ繝√Ε邂｡逅・
  textureManager_ = std::make_unique<TextureManager>();
  textureManager_->Initialize(dxCommon_.get());

  textureManager_->LoadAllFromFolder("resources/");

  // 繝輔か繝ｳ繝育ｮ｡逅・
  fontManager_ = std::make_unique<FontManager>();
  fontManager_->Initialize(this);
  Text::SetFontManager(fontManager_.get());

  // resources/fonts/ 莉･荳九・繝輔か繝ｳ繝医ｒ縺吶∋縺ｦ閾ｪ蜍輔Ο繝ｼ繝・
  fontManager_->LoadAllFromFolder("resources/fonts/");

  // 繝｢繝・Ν邂｡逅・
  modelManager_ = std::make_unique<ModelManager>();
  modelManager_->Initialize(dxCommon_.get(),
                            textureManager_.get()); // dxCommon 繧呈ｸ｡縺・

  ModelRegion::SetModelManager(modelManager_.get()); // Region縺ｫ繧りｨｭ螳・

  // 繝励Μ繝溘ユ繧｣繝也ｮ｡逅・ｼ医す繝ｳ繧ｰ繝ｫ繝医Φ縺ｮ蛻晄悄蛹厄ｼ・
  PrimitiveManager::Initialize();

  // 譌｢蟄牢RV縺ｮ襍ｰ譟ｻ縺ｧ free-list 蜀肴ｧ狗ｯ・
  {
    DescriptorPool *srvPool = dxCommon_->GetSrvPool();
    ID3D12DescriptorHeap *srvHeap = srvPool->GetHeap();
    const uint32_t inc =
        dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto toIndex = [&](D3D12_GPU_DESCRIPTOR_HANDLE h) -> uint32_t {
      if (h.ptr == 0)
        return DescriptorPool::kInvalid;
      const auto heapStart = srvHeap->GetGPUDescriptorHandleForHeapStart().ptr;
      const uint64_t diff = (h.ptr - heapStart);
      return static_cast<uint32_t>(diff / inc);
    };

    std::vector<uint32_t> used;
    // 逋ｽ繝・け繧ｹ繝√Ε
    if (auto white = textureManager_->GetWhiteTextureHandle(); white.ptr != 0) {
      if (auto idx = toIndex(white); idx != DescriptorPool::kInvalid)
        used.push_back(idx);
    }
    // 繝輔か繝ｳ繝医い繝医Λ繧ｹ
    if (fontManager_) {
      if (auto idx = toIndex(fontManager_->GetAtlasSRV()); idx != DescriptorPool::kInvalid)
        used.push_back(idx);
    }
    // 繝・け繧ｹ繝√Ε繧ｭ繝｣繝・す繝･
    for (const std::string &name : textureManager_->GetTextureNames()) {
      auto h = textureManager_->GetTextureHandle(name);
      if (auto idx = toIndex(h); idx != DescriptorPool::kInvalid)
        used.push_back(idx);
    }
    for (uint32_t i = 0; i < srvPool->BaseIndex(); ++i)
      used.push_back(i);

    std::sort(used.begin(), used.end());
    used.erase(std::unique(used.begin(), used.end()), used.end());

    srvPool->RebuildFreeListExcept(used);
  }

  // 蜈･蜉・
  inputManager_ = std::make_unique<InputManager>();
  inputManager_->Initialize(winApp_->GetHwnd());
  winApp_->SetInputManager(inputManager_.get());

  // UI
  ui_ = std::make_unique<DebugUI>();
  ui_->Initialize(winApp_->GetHwnd(), dxCommon_.get());
#ifdef EditorMode
  editorManager_ = std::make_unique<EditorManager>();
  editorManager_->Initialize(this);
#endif
  Sprite::SetDebugUI(ui_.get());
  Text::SetDebugUI(ui_.get());
  Circle2D::SetDebugUI(ui_.get());

  Primitive3DObject::SetDebugUI(ui_.get());


  // 謠冗判
  drawManager_ = std::make_unique<DrawManager>();
  drawManager_->Initialize(dxCommon_.get());
  Sprite::SetDrawManager(drawManager_.get());
  Text::SetDrawManager(drawManager_.get());
  Circle2D::SetDrawManager(drawManager_.get());

  BaseRegion::SetDrawManager(drawManager_.get());

  GPUParticleSystem::SetDrawManager(drawManager_.get());
  Primitive3DObject::SetDrawManager(drawManager_.get());

  GPUParticleSystem::SetEngine(this);
  Line3DRegion::SetDrawManager(drawManager_.get());

  // 繝・け繧ｹ繝√Ε險ｭ螳壹・豕ｨ蜈･
  ui_->SetTextureManager(textureManager_.get());
  drawManager_->SetTextureManager(textureManager_.get());
  Sprite::SetTextureManager(textureManager_.get());
  Circle2D::SetTextureManager(textureManager_.get());

  BaseRegion::SetTextureManager(textureManager_.get());

  GPUParticleSystem::SetTextureManager(textureManager_.get());
    ParticleObject::SetTextureManager(textureManager_.get());
  Primitive3DObject::SetTextureManager(textureManager_.get());

  animationManager_ = std::make_unique<AnimationManager>();
  animationManager_->Initialize(dxCommon_.get());

  BaseModel::SetIrufemiEngine(this);
  winApp_->SetInputManager(inputManager_.get());

  Skybox::SetEngine(this);
  GPUParticleSystem::SetDXCommon(dxCommon_.get());
  VoxelParticleSystem::SetEngine(this);

  voxelParticleManager_ = std::make_unique<VoxelParticleManager>();
  voxelParticleManager_->Initialize(this);

  Circle2D::SetEngine(this);
  Line3DRegion::SetEngine(this);
  Primitive3DObject::SetEngine(this);

  // --- 蜈ｨ逕ｻ髱｢逕ｨ RenderTexture 縺ｮ蛻晄悄蛹・---
  mainRenderTexture_ = std::make_unique<RenderTexture>();
  mainRenderTexture_->Initialize(
      dxCommon_.get(), GetClientWidth(), GetClientHeight(),
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
      {clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]});

  // --- PostProcessManager 縺ｮ蛻晄悄蛹・---
  postProcessManager_ = std::make_unique<PostProcessManager>();
  postProcessManager_->Initialize(dxCommon_.get(), DXGI_FORMAT_R8G8B8A8_UNORM);

  // 繝弱う繧ｺ繝・け繧ｹ繝√Ε縺ｮ繝ｭ繝ｼ繝峨→繝上Φ繝峨Ν險ｭ螳・
  postProcessManager_->SetDissolveNoiseHandle(
      0, textureManager_->GetTextureHandle("resources/noise0.png"));
  postProcessManager_->SetDissolveNoiseHandle(
      1, textureManager_->GetTextureHandle("resources/noise1.png"));

  // --- 豺ｱ蠎ｦ繝舌ャ繝輔ぃ縺ｮ SRV 菴懈・縺ｨ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ縺ｸ縺ｮ險ｭ螳・---
  depthSrvIndex_ = dxCommon_->GetSrvPool()->Allocate();
  D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandleGPU =
      dxCommon_->GetSrvPool()->GetGPUHandle(depthSrvIndex_);

  D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
  depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
  depthSrvDesc.Shader4ComponentMapping =
      D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  depthSrvDesc.Texture2D.MipLevels = 1;
  dxCommon_->GetDevice()->CreateShaderResourceView(
      dxCommon_->GetDepthStencilResource(), &depthSrvDesc,
      dxCommon_->GetSrvPool()->GetCPUHandle(depthSrvIndex_));

  postProcessManager_->SetDepthSrvHandle(depthSrvHandleGPU);

  // --- SceneTransition 縺ｮ蛻晄悄蛹・---
  sceneTransition_ = std::make_unique<SceneTransition>();
  sceneTransition_->Initialize(postProcessManager_.get());

  // WinApp縺ｫ閾ｪ霄ｫ(Engine)縺ｮ繝昴う繝ｳ繧ｿ繧定ｨｭ螳・
  winApp_->SetEngine(this);

  // PSO・医ヱ繧､繝励Λ繧､繝ｳ繧ｹ繝・・繝茨ｼ峨・莠句燕繧ｳ繝ｳ繝代う繝ｫ繧貞ｮ溯｡後＠縲∝ｮ溯｡御ｸｭ縺ｮ繝偵ャ繝・ｼ医き繧ｯ縺､縺搾ｼ峨ｒ髦ｲ豁｢
  if (GetPSOManager()) {
    GetPSOManager()->PreWarmCommonPSOs();
  }

  // 蛻晏屓謠冗判譎ゅ・驕・ｻｶ繝上・繝峨え繧ｧ繧｢繧ｳ繝ｳ繝代う繝ｫ・・IT・峨ｒ髦ｲ豁｢縺吶ｋ縺溘ａ縺ｮ繝繝溘・螳溯｡・
  if (dxCommon_) {
    dxCommon_->PreWarmJITCompile();
  }

  GPUParticleManager::GetInstance()->Initialize();
}

// 繧ｯ繝ｪ繧｢繧ｫ繝ｩ繝ｼ繧・float 謖・ｮ壹〒縺阪ｋ 蛻晄悄蛹・
void IrufemiEngine::Initialize(const std::wstring &title,
                               const int32_t &clientWidth,
                               const int32_t &clientHeight, float r, float g,
                               float b, float a) {
  clearColor_ = {r, g, b, a};
  // 譌｢蟄倥・ Initialize 繧貞他縺ｶ(莠呈鋤諤ｧ邯ｭ謖・
  Initialize(title, clientWidth, clientHeight);
}

// 繧ｯ繝ｪ繧｢繧ｫ繝ｩ繝ｼ繧・std::array 謖・ｮ壹〒縺阪ｋ 蛻晄悄蛹・
void IrufemiEngine::Initialize(const std::wstring &title,
                               const int32_t &clientWidth,
                               const int32_t &clientHeight,
                               const std::array<float, 4> &clearColor) {
  clearColor_ = clearColor;
  // 譌｢蟄倥・ Initialize 繧貞他縺ｶ(莠呈鋤諤ｧ邯ｭ謖・
  Initialize(title, clientWidth, clientHeight);
}

// 霑ｽ蜉: Vector4 迚・Initialize
void IrufemiEngine::Initialize(const std::wstring &title,
                               const int32_t &clientWidth,
                               const int32_t &clientHeight,
                               const Vector4 &clearColor) {
  clearColor_ = {clearColor.x, clearColor.y, clearColor.z, clearColor.w};
  Initialize(title, clientWidth, clientHeight);
}

void IrufemiEngine::Finalize() {
  if (isFinalized_) return;

  // 0. 繧ｷ繝ｼ繝ｳ縺ｨ逕ｻ髱｢驕ｷ遘ｻ繝ｻ繝ｭ繝ｼ繝・ぅ繝ｳ繧ｰ・医％繧後ｉ縺後Μ繧ｽ繝ｼ繧ｹ縺ｮ shared_ptr 繧剃ｿ晄戟縺励※縺・ｋ縺溘ａ譛蜆ｪ蜈茨ｼ・
  if (sceneManager_) {
    sceneManager_.reset();
  }
  if (loadingScreen_) {
    loadingScreen_.reset();
  }
  if (sceneTransition_) {
    sceneTransition_.reset();
  }
  if (cameraManager_) {
    cameraManager_.reset();
  }

  // 繧｢繝励Μ繧ｱ繝ｼ繧ｷ繝ｧ繝ｳ邨ゆｺ・凾縲√す繝ｼ繝ｳ遐ｴ譽・燕縺ｫGPU蜃ｦ逅・・螳御ｺ・ｒ蠕・ｩ溘☆繧・
  if (dxCommon_) {
    dxCommon_->WaitForGPU();
  }

  // 1. 繧ｨ繝・ぅ繧ｿ縺ｨUI (謠冗判繝槭ロ繝ｼ繧ｸ繝｣遲峨↓萓晏ｭ・
  if (ui_) {
    ui_->Shutdown();
    ui_.reset();
  }
#ifdef EditorMode
  if (editorManager_) {
    editorManager_.reset();
  }
#endif

  // 2. 謠冗判繝ｻ繝昴せ繝医・繝ｭ繧ｻ繧ｹ邉ｻ (DirectX蝓ｺ逶､縺ｫ萓晏ｭ・
  if (drawManager_) {
    drawManager_->Finalize();
    drawManager_.reset();
  }
  if (postProcessManager_) {
    postProcessManager_.reset();
  }
  if (mainRenderTexture_) {
    mainRenderTexture_.reset();
  }

  // 3. 繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ繝ｻ繝｢繝・Ν繝ｻ繝・け繧ｹ繝√Ε (繝ｪ繧ｽ繝ｼ繧ｹ縺ｮ螳滉ｽ薙ｒ菫晄戟)
  if (animationManager_) {
    animationManager_.reset();
  }
  if (modelManager_) {
    modelManager_.reset();
  }
  if (textureManager_) {
    textureManager_.reset();
  }
  if (fontManager_) {
    fontManager_->Finalize();
    fontManager_.reset();
  }

  // 4. 螳壽焚繝舌ャ繝輔ぃ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ (DirectX蝓ｺ逶､縺ｮ繝ｪ繧ｽ繝ｼ繧ｹ繧堤峩謗･菫晄戟縺吶ｋ縺溘ａ蜈医↓遐ｴ譽・
  if (materialBufferManager_) {
    materialBufferManager_.reset();
  }
  if (transformBufferManager_) {
    transformBufferManager_.reset();
  }

  // --- 髱咏噪繝昴う繝ｳ繧ｿ縺ｮ繧ｯ繝ｪ繧｢・医ョ繧ｹ繝医Λ繧ｯ繧ｿ縺ｧ縺ｮ荳肴ｭ｣繧｢繧ｯ繧ｻ繧ｹ髦ｲ豁｢・・---
  BaseResource::SetDirectXCommon(nullptr);
  BaseRegion::SetDirectXCommon(nullptr);
  Line3DRegion::SetDirectXCommon(nullptr);

  Texture::SetDescriptorPool(nullptr);
  Texture::SetDirectXCommon(nullptr);
  Texture::SetWhiteTextureResource(nullptr);
  GpuMesh::sDxCommon = nullptr;

  BaseRegion::SetSrvAllocator(nullptr);

  Line3DRegion::SetSrvAllocator(nullptr);

  // DebugUI, DrawManager, TextureManager 遲峨・蜷・け繝ｩ繧ｹ縺ｸ縺ｮ髱咏噪繧ｻ繝・ヨ繧ゅけ繝ｪ繧｢
  Sprite::SetDebugUI(nullptr);
  Circle2D::SetDebugUI(nullptr);
  Primitive3DObject::SetDebugUI(nullptr);


  Sprite::SetDrawManager(nullptr);
  Circle2D::SetDrawManager(nullptr);
  BaseRegion::SetDrawManager(nullptr);

  GPUParticleSystem::SetDrawManager(nullptr);
  Primitive3DObject::SetDrawManager(nullptr);
  Line3DRegion::SetDrawManager(nullptr);

  Sprite::SetTextureManager(nullptr);
  Circle2D::SetTextureManager(nullptr);
  BaseRegion::SetTextureManager(nullptr);

  GPUParticleSystem::SetTextureManager(nullptr);
    ParticleObject::SetTextureManager(nullptr);
  Primitive3DObject::SetTextureManager(nullptr);

  Sprite::SetCameraManager(nullptr);
  ModelRegion::SetModelManager(nullptr);

  GPUParticleSystem::SetEngine(nullptr);
  BaseModel::SetIrufemiEngine(nullptr);
  Skybox::SetEngine(nullptr);
  GPUParticleSystem::SetDXCommon(nullptr);
  VoxelParticleSystem::SetEngine(nullptr);
  if (voxelParticleManager_) {
    voxelParticleManager_.reset();
  }
  GPUParticleManager::GetInstance()->Finalize();
  Circle2D::SetEngine(nullptr);
  Line3DRegion::SetEngine(nullptr);
  Primitive3DObject::SetEngine(nullptr);
  Effect::SetEngine(nullptr);
  Bgm::SetAudioManager(nullptr);
  Se::SetAudioManager(nullptr);

  // 5. 繧ｷ繝ｳ繧ｰ繝ｫ繝医Φ縺ｮ遐ｴ譽・(GPU繝ｪ繧ｽ繝ｼ繧ｹ繧剃ｿ晄戟縺励※縺・ｋ蜿ｯ閭ｽ諤ｧ縺後≠繧九◆繧・dxCommon 遐ｴ譽・燕縺ｫ蜻ｼ縺ｶ)
  PrimitiveManager::Finalize();

  // 6. 蝓ｺ逶､繧ｷ繧ｹ繝・Β (繧ｵ繧ｦ繝ｳ繝峨・蜈･蜉・
  if (audioManager_) {
    audioManager_->Finalize();
    audioManager_.reset();
  }
  if (inputManager_) {
    inputManager_.reset();
  }

  // 5.5 Dynamic Constant Buffers
  // (DirectX蝓ｺ逶､縺ｫ萓晏ｭ倥☆繧九◆繧√‥xCommon_繧医ｊ蜈医↓遐ｴ譽・
  if (materialBufferManager_) {
    materialBufferManager_.reset();
  }
  if (transformBufferManager_) {
    transformBufferManager_.reset();
  }

  // 6. DirectX蝓ｺ逶､
  if (dxCommon_) {
    if (dxCommon_->GetSrvPool() && depthSrvIndex_ != 0xFFFFFFFF) {
      dxCommon_->GetSrvPool()->FreeAfterFence(depthSrvIndex_,
                                              dxCommon_->GetFenceValue());
      depthSrvIndex_ = 0xFFFFFFFF;
    }
    dxCommon_->Finalize();
    dxCommon_.reset();
  }

  // 7. OS繝ｻ繧ｦ繧｣繝ｳ繝峨え
  if (winApp_) {
    winApp_.reset();
  }

  isFinalized_ = true;
}

void IrufemiEngine::Execute() {
  // CameraManager險ｭ螳・
  cameraManager_ = std::make_unique<CameraManager>();
  Sprite::SetCameraManager(cameraManager_.get());
  Text::SetCameraManager(cameraManager_.get());
  
  // SceneManager 讒狗ｯ・繧ｨ繝ｳ繧ｸ繝ｳ縺ｯ謇譛峨・縺ｿ)
  sceneManager_ = std::make_unique<SceneManager>(this);

  // 繝ｭ繝ｼ繝・ぅ繝ｳ繧ｰ逕ｻ髱｢縺ｮ讒狗ｯ・
  loadingScreen_ = std::make_unique<LoadingScreen>();
  loadingScreen_->Initialize(this);

  // Application 縺九ｉ縺ｮ逋ｻ骭ｲ繧貞渚譏
  if (sceneRegistrar_) {
    sceneRegistrar_(*sceneManager_);
  }

  // 蛻晄悄繧ｷ繝ｼ繝ｳ縺梧欠螳壹＆繧後※縺・ｌ縺ｰ驕ｷ遘ｻ
  if (!initialSceneName_.empty()) {
    sceneManager_->ChangeTo(initialSceneName_);
  }

  while (winApp_->ProcessMessages()) {
    // 繝輔Ξ繝ｼ繝髢句ｧ区凾縺ｮ譎る俣譖ｴ譁ｰ
    StartFrame();

    // ImGui_
    ui_->FrameStart();

#ifdef EditorMode
    if (editorManager_) {
        editorManager_->DrawEditorUI();
    }
#endif

#ifdef USE_IMGUI
    ui_->FPSDebug();
    ui_->BeginEngineDebugWindow();
    ui_->SceneSelectorTab(sceneManager_.get());
    ui_->PostProcessTab(this);
    GPUParticleManager::GetInstance()->Debug();
    if (auto *scene = sceneManager_->GetCurrentScene()) {
      scene->DrawDebugTab();
    }
    ui_->EndEngineDebugWindow();
#endif // USE_IMGUI

    // 譖ｴ譁ｰ
    audioManager_->Update();
    sceneManager_->Update();
    if (sceneManager_->IsLoading() && loadingScreen_) {
      loadingScreen_->Update(deltaTime_);
    }
    totalTime_ += deltaTime_;
    postProcessManager_->Update(totalTime_);
    sceneTransition_->Update(deltaTime_);
    
    GPUParticleManager::GetInstance()->Update();

    if (voxelParticleManager_) {
        // 繝昴・繧ｺ荳ｭ縺ｯ VoxelParticle 縺ｮ譖ｴ譁ｰ繧偵せ繧ｭ繝・・縺吶ｋ
        bool isPaused = (sceneManager_ && sceneManager_->GetCurrent() == "Pause");
        if (!isPaused) {
            voxelParticleManager_->Update(deltaTime_);
        }
    }

    // 繧､繝ｳ繝励ャ繝医ｒ譖ｴ譁ｰ
    inputManager_->Update();

    // 繝輔Ξ繝ｼ繝騾比ｸｭ蜃ｦ逅・
    ProcessFrame();

    // 謠冗判
    sceneManager_->Draw();

    if (voxelParticleManager_) {
        voxelParticleManager_->Draw();
    }

    GPUParticleManager::GetInstance()->Draw();

    // 縺薙％縺ｧ貅懊∪縺｣縺滓緒逕ｻ繝代こ繝・ヨ繧剃ｸ譁峨↓蜃ｦ逅・☆繧・
    drawManager_->ExecuteRenderQueues(this);

    // 邨ゆｺ・・逅・
    EndFrame();
  }
}

// 繝輔Ξ繝ｼ繝髢句ｧ句・逅・
void IrufemiEngine::StartFrame() {
  // 譎る俣縺ｮ譖ｴ譁ｰ
  auto now = std::chrono::steady_clock::now();
  deltaTime_ = std::chrono::duration<float>(now - lastFrameTime_).count();
  totalTime_ = std::chrono::duration<float>(now - startTime_).count();

  // 繧ｲ繝ｼ繝蜀・凾髢薙・譖ｴ譁ｰ・医ち繧､繝繧ｹ繧ｱ繝ｼ繝ｫ繧帝←逕ｨ・・
  gameDeltaTime_ = deltaTime_ * timeScale_;
  gameTime_ += gameDeltaTime_;

  lastFrameTime_ = now;
}

// 繝輔Ξ繝ｼ繝騾比ｸｭ蜃ｦ逅・
void IrufemiEngine::ProcessFrame() {
  // 髱槫酔譛溘せ繝ｬ繝・ラ縺ｧ驕・ｻｶ縺輔ｌ縺ｦ縺・◆SRV縺ｮ譖ｴ譁ｰ繧偵Γ繧､繝ｳ繧ｹ繝ｬ繝・ラ縺ｧ荳諡ｬ驕ｩ逕ｨ縺吶ｋ・医ョ繝ｼ繧ｿ遶ｶ蜷医・髦ｲ豁｢・・
  if (dxCommon_) {
    dxCommon_->FlushPendingSRVUpdates();
  }

  // 繧ｹ繝・・繝医・繝ｪ繧ｻ繝・ヨ・亥燕繝輔Ξ繝ｼ繝縺ｮ謠冗判繧ｹ繝・・繝医ｒ蠑輔″邯吶′縺ｪ縺・ｈ縺・↓縺吶ｋ・・
  currentBlend_ = BlendMode::kBlendModeNormal;
  currentDepth_ = PSOManager::DepthWrite::Enable;
  currentCull_ = PSOManager::CullMode::Back;

  // 謠冗判蜃ｦ逅・↓蜈･繧句燕縺ｫImGui_::Render繧堤ｩ阪・
  ui_->QueueDrawCommands();

  // 1. 繝舌ャ繧ｯ繝舌ャ繝輔ぃ繧偵け繝ｪ繧｢ (蠢ｵ縺ｮ縺溘ａ)
  drawManager_->PreDraw(clearColor_, 1.0f, 0);

  // (Compute Shader縺ｮ荳諡ｬ螳溯｡後・縲ヽenderGraph蜀・・ComputePass縺ｫ遘ｻ陦後＠縺ｾ縺励◆)

  // 2. 繝｡繧､繝ｳ縺ｮ謠冗判蜈医ｒ RenderTexture 縺ｫ蛻・ｊ譖ｿ縺医∵欠螳壹・繧ｯ繝ｪ繧｢繧ｫ繝ｩ繝ｼ縺ｧ繧ｯ繝ｪ繧｢
  drawManager_->BeginRenderTexture(
      mainRenderTexture_.get(),
      Vector4{clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]});
}

// 繝輔Ξ繝ｼ繝邨ゆｺ・・逅・
void IrufemiEngine::EndFrame() {
  // (RenderTexture 縺ｮ SRV 蛹悶ｄ繝舌ャ繧ｯ繝舌ャ繝輔ぃ縺ｸ縺ｮ霆｢騾√・
  // 繝昴せ繝医・繝ｭ繧ｻ繧ｹ蜃ｦ逅・・縺吶∋縺ｦ RenderGraph 蜀・〒陦後ｏ繧後∪縺・

  // 謠冗判蜈医′繝舌ャ繧ｯ繝舌ャ繝輔ぃ縺ｫ縺ｪ繧翫√・繧ｹ繝医・繝ｭ繧ｻ繧ｹ・域囓霆｢縺ｪ縺ｩ・峨′謗帙°縺｣縺滉ｸ翫°繧・
  // 蠖ｱ髻ｿ繧貞女縺代↑縺・怙蜑埼擇UI縺ｨ縺励※繝ｭ繝ｼ繝臥判髱｢繧呈緒逕ｻ縺吶ｋ
  if (sceneManager_ && sceneManager_->IsLoading()) {
    if (loadingScreen_) {
      loadingScreen_->Draw(this);
    }
  }

  // --- 霑ｽ蜉: 譛蜑埼擇UI繧ｭ繝･繝ｼ縺ｮ豸亥喧 ---
  drawManager_->ExecuteTopMostQueues(this);

  // 謠冗判蠕悟・逅・
  ui_->QueuePostDrawCommands();
  drawManager_->PostDraw();

  // 5) 繝輔Ξ繝ｼ繝邨らｫｯ縺ｧ驕・ｻｶ隗｣謾ｾ縺ｮ蝗槫庶(繝輔ぉ繝ｳ繧ｹ螳御ｺ・､繧呈ｸ｡縺・
  if (auto *srvPool = dxCommon_->GetSrvPool()) {
    const uint64_t completed = dxCommon_->GetFence()->GetCompletedValue();
    srvPool->GarbageCollect(completed);
  }

  // --- 霑ｽ蜉: 荳ｭ髢薙Μ繧ｽ繝ｼ繧ｹ縺ｮ驕・ｻｶ隗｣謾ｾ繧貞ｮ溯｡・---
  dxCommon_->ClearPendingResources();
}

void IrufemiEngine::OnResize(int32_t width, int32_t height) {
  if (width <= 0 || height <= 0)
    return;

  // 1. 繧ｹ繝ｯ繝・・繝√ぉ繝ｼ繝ｳ縲∵ｷｱ蠎ｦ繝舌ャ繝輔ぃ縺ｮ繝ｪ繧ｵ繧､繧ｺ
  dxCommon_->ResizeSwapChain(width, height);

  // 2. 繝｡繧､繝ｳ繝ｬ繝ｳ繝繝ｼ繝・け繧ｹ繝√Ε縺ｮ蜀咲函謌・
  mainRenderTexture_->Initialize(
      dxCommon_.get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
      {clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]});

  // 3. 豺ｱ蠎ｦ繝舌ャ繝輔ぃ縺ｮ SRV 蜀堺ｽ懈・ (譌｢蟄倥・繧､繝ｳ繝・ャ繧ｯ繧ｹ繧貞・蛻ｩ逕ｨ)
  if (depthSrvIndex_ != 0xFFFFFFFF) {
    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
    depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSrvDesc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Texture2D.MipLevels = 1;
    dxCommon_->GetDevice()->CreateShaderResourceView(
        dxCommon_->GetDepthStencilResource(), &depthSrvDesc,
        dxCommon_->GetSrvPool()->GetCPUHandle(depthSrvIndex_));

    // 繝昴せ繝医・繝ｭ繧ｻ繧ｹ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ縺ｫ譁ｰ縺励＞SRV繝上Φ繝峨Ν繧定ｨｭ螳・
    postProcessManager_->SetDepthSrvHandle(
        dxCommon_->GetSrvPool()->GetGPUHandle(depthSrvIndex_));
  }
  
  // 4. 繧ｫ繝｡繝ｩ縺ｮ隗｣蜒丞ｺｦ譖ｴ譁ｰ (3D遨ｺ髢薙・豁ｪ縺ｿ髦ｲ豁｢)
  if (cameraManager_) {
      cameraManager_->OnResize(width, height);
  }
}

void IrufemiEngine::SetCursorLocked(bool lock) {
  if (winApp_) {
    winApp_->SetCursorLocked(lock);
  }
}

bool IrufemiEngine::IsCursorLocked() const {
  if (winApp_) {
    return winApp_->IsCursorLocked();
  }
  return false;
}

void IrufemiEngine::ApplyPSO(const std::string& shaderName) {
  // Shadow繝代せ縺ｮ蝣ｴ蜷医・閾ｪ蜍慕噪縺ｫ繧ｷ繝｣繝峨え逕ｨ繧ｷ繧ｧ繝ｼ繝縺ｫ蛻・ｊ譖ｿ縺医ｋ(蜈・・繧ｳ繝ｼ繝峨・莉墓ｧ倡ｶｭ謖・
  if (drawManager_->IsShadowPass()) {
      if (shaderName == "Object3D") {
          auto* pso = GetPSOManager()->GetPSO("Shadow", BlendMode::kBlendModeNone, PSOManager::DepthWrite::Enable, currentCull_);
          if (pso) drawManager_->BindPSO(pso);
          return;
      }
      else if (shaderName == "Skinning") {
          auto* pso = GetPSOManager()->GetPSO("ShadowSkinning", BlendMode::kBlendModeNone, PSOManager::DepthWrite::Enable, currentCull_);
          if (pso) drawManager_->BindPSO(pso);
          return;
      }
      // 縺昴ｌ莉･螟悶・繧ｷ繝｣繝峨え繝代せ縺ｧ縺ｯ謠冗判縺励↑縺・辟｡隕・
      return;
  }
  
  // Skybox逕ｨ縺ｮ迚ｹ谿雁ｯｾ蠢・(蜈・・繧ｳ繝ｼ繝峨〒縺ｯ CullMode::Front 豎ｺ繧∵遠縺｡縺ｧ繝悶Ξ繝ｳ繝峨→豺ｱ蠎ｦ縺ｯ荳崎ｦ√□縺｣縺・
  if (shaderName == "Skybox") {
      auto* pso = GetPSOManager()->GetPSO("Skybox", BlendMode::kBlendModeNone, PSOManager::DepthWrite::Disable, PSOManager::CullMode::Front);
      if (pso) drawManager_->BindPSO(pso);
      return;
  }

  auto* pso = GetPSOManager()->GetPSO(shaderName, currentBlend_, currentDepth_, currentCull_);
  assert(pso && ("PSO is null for " + shaderName).c_str());
  if (pso) {
    drawManager_->BindPSO(pso);
  }
}

void IrufemiEngine::BindLightningParams(D3D12_GPU_VIRTUAL_ADDRESS address) {
  if (address == 0)
    return;
  GetCommandList()->SetGraphicsRootConstantBufferView((UINT)RootSlot::Special,
                                                      address);
}

bool IrufemiEngine::IsAssetLoading() const {
  bool modelsLoaded = !modelManager_ || modelManager_->IsAllLoaded();
  bool texturesLoaded = !textureManager_ || textureManager_->IsAllLoaded();
  bool fontsLoaded = !fontManager_ || fontManager_->IsAllLoaded();
  return !modelsLoaded || !texturesLoaded || !fontsLoaded;
}

