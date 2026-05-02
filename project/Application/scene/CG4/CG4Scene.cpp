#include "CG4Scene.h"
#include "Irufemi.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "Renderer/Object3D/AnimationModel/AnimationModel.h"
#include "Graphics/Data/CameraForGPU.h"
#include "Graphics/Data/PointLight.h"
#include "Graphics/Data/SpotLight.h"
#include "Graphics/Data/DirectionalLight.h"
#include "Graphics/Data/AreaLight.h"
#include "Platform/Input/InputManager.h"
#include "Renderer/Effect/Effect.h"
#include "Renderer/Object3D/Primitive/RingClass.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
CG4Scene::~CG4Scene() = default;

void CG4Scene::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // カメラの初期化
    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });
    camera_->UpdateMatrix();

    // デバッグカメラの初期化
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode_ = false;

    // ライトの初期化
    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLight_->direction = { 0.5f, -0.7f, 1.0f };
    directionalLight_->intensity = 1.0f;

    // Skyboxの初期化（フラグが有効な場合のみ）
    if (isActiveSkybox_) {
        skybox_ = std::make_unique<Skybox>();
        skybox_->Initialize(camera_.get(), "resources/qwantani_night_puresky_1k_cubemap.dds");
    }

    // エフェクトの初期化
    effect_ = std::make_unique<Effect>();
    effect_->Initialize(camera_.get(), EffectType::kAura);

    // テスト用リングの初期化
    testRing_ = std::make_unique<RingClass>();
    testRing_->Initialize(camera_.get());
}

void CG4Scene::Update() {
#ifdef USE_IMGUI
    ImGui::Begin("Activation");
    ImGui::Checkbox("Skybox", &isActiveSkybox_);
    ImGui::Checkbox("AnimatedCube", &isActiveAnimatedCube_);
    ImGui::Checkbox("Walk", &isActiveWalk_);
    ImGui::Checkbox("SneakWalk", &isActiveSneakWalk_);
    ImGui::Checkbox("Effect", &isActiveEffect_);
    ImGui::Checkbox("GPUParticle", &isActiveGPUParticle_);
    ImGui::Checkbox("Ring", &isActiveRing_);
    ImGui::End();
#endif

    if (debugMode_) {
        debugCamera_->Update();
        const Camera& dbgCam = debugCamera_->GetCamera();
        camera_->SetViewMatrix(dbgCam.GetViewMatrix());
        camera_->SetTranslate(dbgCam.GetTranslate());
        camera_->SetPerspectiveFovMatrix(dbgCam.GetPerspectiveFovMatrix());
    } else {
        camera_->Update();
    }

    CameraForGPU cameraForGpu;
    cameraForGpu.view = camera_->GetViewMatrix();
    cameraForGpu.projection = camera_->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = camera_->GetTranslate();

    // Skyboxの更新と環境マップの設定
    if (isActiveSkybox_) {
        if (!skybox_) {
            skybox_ = std::make_unique<Skybox>();
            skybox_->Initialize(camera_.get(), "resources/qwantani_night_puresky_1k_cubemap.dds");
        }
        skybox_->Update();
        D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle = skybox_->GetTextureHandle();
        engine_->GetDrawManager()->SetEnvironmentMap(envMapHandle);
    }

    if (isActiveAnimatedCube_) {
        if (!animatedCube_) {
            animatedCube_ = std::make_unique<AnimationModel>();
            animatedCube_->Initialize(camera_.get(), "sample/AnimatedCube.gltf");
        }
        animatedCube_->Update();
    }
    
    if (isActiveWalk_) {
        if (!walk_) {
            walk_ = std::make_unique<AnimationModel>();
            walk_->Initialize(camera_.get(), "sample/walk.gltf");
        }
        walk_->Update();
    }

    if (isActiveSneakWalk_) {
        if (!sneakWalk_) {
            sneakWalk_ = std::make_unique<AnimationModel>();
            sneakWalk_->Initialize(camera_.get(), "sample/sneakWalk.gltf");
        }
        sneakWalk_->Update();
    }

    // Spaceキーでヒットエフェクト発生テスト
    if (isActiveEffect_) {
        if (engine_->GetInputManager()->GetKeyboard()->IsKeyPressed(VK_SPACE)) {
            Vector3 spawnPos = camera_->GetTranslate();
            spawnPos.z += 5.0f;
            effect_->Play(spawnPos);
        }
        if (effect_) {
            effect_->Update();
        }
    }

    if (isActiveGPUParticle_) {
        if (!gpuParticleSystem_) {
            gpuParticleSystem_ = std::make_unique<GPUParticleSystem>();
            gpuParticleSystem_->Initialize(camera_.get(), "resources/circle.png");
        }
        gpuParticleSystem_->Update();
    }

    if (isActiveRing_ && testRing_) {
        testRing_->Update();
    }

    std::vector<PointLight*> pLights;
    std::vector<SpotLight*> sLights;
    std::vector<AreaLight*> aLights;

    // 描画前同期
    if (isActiveEffect_ && effect_) {
        effect_->SyncBeforeDraw();
    }

    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, pLights, sLights, aLights);
}

void CG4Scene::Draw() {
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);
    
    // オブジェクトの描画があればここに記述
    if (isActiveAnimatedCube_) {
        animatedCube_->Draw();
    }
    if (isActiveWalk_) {
        walk_->Draw();
    }
    if (isActiveSneakWalk_) {
        sneakWalk_->Draw();
    }

    if (isActiveSkybox_ && skybox_) {
        skybox_->Draw();
    }

    // エフェクトの描画
    if (isActiveEffect_ && effect_) {
        effect_->Draw();
    }

    // リングの描画
    if (isActiveRing_ && testRing_) {
        testRing_->Draw();
    }

    // GPUParticleの描画
    if (isActiveGPUParticle_ && gpuParticleSystem_) {
        gpuParticleSystem_->Draw();
    }
}

void CG4Scene::DrawDebugTab() {
#if defined USE_IMGUI
    if (camera_) {
        if (ImGui::BeginTabItem("CG4 Camera")) {
            ImGui::Checkbox("Debug Camera Mode", &debugMode_);
            if (debugMode_ && debugCamera_) {
                if (ImGui::Button("Top-Down")) debugCamera_->SetPreset(DebugCamera::Preset::TopDown, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Diagonal")) debugCamera_->SetPreset(DebugCamera::Preset::Diagonal, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Front")) debugCamera_->SetPreset(DebugCamera::Preset::Front, *camera_);
                ImGui::SameLine();
                if (ImGui::Button("Snap to Current")) debugCamera_->SetPreset(DebugCamera::Preset::Current, *camera_);

                ImGui::Separator();
                ImGui::Text("Debug Camera Controls");
                debugCamera_->GetCamera().DrawDebugContents();
                float dist = debugCamera_->GetDistance();
                if (ImGui::DragFloat("Orbit Distance", &dist, 0.1f, 1.0f, 1000.0f)) {
                    debugCamera_->SetDistance(dist);
                }
            } else {
                camera_->DrawDebugContents();
            }
            ImGui::EndTabItem();
        }
    }
    
    if (isActiveSkybox_ && skybox_) {
        skybox_->Debug();
    }
    
    if (isActiveAnimatedCube_ && animatedCube_) {
        animatedCube_->Debug("AnimatedCube");
    }
    if (isActiveWalk_ && walk_) {
        walk_->Debug("Walk");
    }
    if (isActiveSneakWalk_ && sneakWalk_) {
        sneakWalk_->Debug("SneakWalk");
    }

    DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);

    if (isActiveEffect_ && effect_) {
        effect_->Debug();
    }

    if (isActiveGPUParticle_ && gpuParticleSystem_) {
        gpuParticleSystem_->Debug();
    }

    if (isActiveRing_ && testRing_) {
        testRing_->Debug("Test Ring");
    }
#endif
}
