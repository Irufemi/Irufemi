#include "DebugScene.h" // Unified debug UI enabled

#include "Framework/SceneManager.h"

#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "IrufemiEngine/Engine/Core/Math/Math.h"


// デストラクタ
DebugScene::~DebugScene() {

}

// 初期化
void DebugScene::Initialize(IrufemiEngine* engine) {

    BaseScene::Initialize(engine);
    
    Camera* activeCamera = engine_->GetCameraManager()->GetActiveCamera();
    activeCamera->SetTranslate(Vector3{ 0.0f,0.0f,-10.0f });
    activeCamera->UpdateMatrix();

    auto areaLight = std::make_unique<AreaLight>();
    areaLight->color = { 1.0f, 0.5f, 0.5f, 1.0f };
    areaLight->position = { 0.0f, 2.0f, 2.0f };
    areaLight->intensity = 1.0f;
    areaLight->direction = { 0.0f, -1.0f, 0.0f };
    areaLight->range = 10.0f;
    areaLight->size = { 2.0f, 2.0f };
    areaLight->isActive = 1;
    areaLights_.push_back(std::move(areaLight));

    isActiveObj_ = false;
    isActiveSprite_ = false;

    isActiveStanfordBunny_ = false;
    isActiveUtashTeapot_ = false;
    isActiveMultiMesh_ = false;
    isActiveMultiMaterial_ = false;
    isActiveSuzanne_ = false;
    isActiveFence_ = false;
    isActiveTerrain_ = false;


    isActiveVoxelParticle_ = false;
    isActiveGPUParticle_ = false;
    isActiveAnimatedCube_ = false;
    isActiveWalk_ = false;
    isActiveSneakWalk_ = false;
    isActiveSkybox_ = false;
    isActivePrimitiveObj_ = false;

    isActiveLightningCrawl_ = false;
    isActiveImGuiDemo_ = false;

    // 課題用スプライトの初期化
    /*imguiSprite_ = std::make_unique<Sprite>();
    imguiSprite_->Initialize();
    imguiSprite_->SetPosition(100.0f, 100.0f);*/

    if (isActiveSprite_) {
        sprite_ = std::make_unique <Sprite>();
        sprite_->Initialize();
    }

    if (isActiveObj_) {
        obj_ = std::make_unique<StaticModelObject>();
        obj_->Initialize("sample/plane.gltf");
    }
    if (isActiveStanfordBunny_) {
        stanfordBunny_ = std::make_unique <StaticModelObject>();
        stanfordBunny_->Initialize("sample/bunny.obj");
    }
    if (isActiveUtashTeapot_) {
        utashTeapot_ = std::make_unique <StaticModelObject>();
        utashTeapot_->Initialize("sample/teapot.obj");
    }
    if (isActiveMultiMesh_) {
        multiMesh_ = std::make_unique <StaticModelObject>();
        multiMesh_->Initialize("sample/multiMesh.obj");
    }
    if (isActiveMultiMaterial_) {
        multiMaterial_ = std::make_unique <StaticModelObject>();
        multiMaterial_->Initialize("sample/multiMaterial.obj");
    }
    if (isActiveSuzanne_) {
        suzanne_ = std::make_unique <StaticModelObject>();
        suzanne_->Initialize("sample/suzanne.obj");
    }
    if (isActiveFence_) {
        fence_ = std::make_unique <StaticModelObject>();
        fence_->Initialize("sample/fence.obj");
    }
    if (isActiveTerrain_) {
        terrain_ = std::make_unique <StaticModelObject>();
        terrain_->Initialize("sample/terrain.obj");
    }


    if (isActiveGPUParticle_) {
        particleObj_ = std::make_unique<ParticleObject>();
        particleObj_->SetTexturePath("resources/circle.png");
        particleObj_->SetEmitType(0);
        particleObj_->SetVelocity(5.0f);
        particleObj_->SetGravity(0.0f);
        particleObj_->SetDamping(0.05f);
        particleObj_->SetLifeTimeMin(0.3f);
        particleObj_->SetLifeTimeMax(0.6f);
        particleObj_->SetRadius(5.0f);
        particleObj_->SetColor({1.0f, 0.5f, 0.0f, 1.0f});
        particleObj_->Initialize();
    }
    
    if (isActiveVoxelParticle_) {
        voxelParticle_ = std::make_unique<VoxelParticleSystem>();
        voxelParticle_->Initialize("sample/terrain.obj", { 64,64,64 });
    }
    if (isActiveAnimatedCube_) {
        animatedCube_ = std::make_unique<AnimationModel>();
        animatedCube_->Initialize("sample/AnimatedCube.gltf");
    }
    if (isActiveWalk_) {
        walk_ = std::make_unique<AnimationModel>();
        walk_->Initialize("sample/walk.gltf");
    }
    if (isActiveSneakWalk_) {
        sneakWalk_ = std::make_unique<AnimationModel>();
        sneakWalk_->Initialize("sample/sneakWalk.gltf");
    }
    if (isActiveSkybox_) {
        skybox_ = std::make_unique<Skybox>();
        skybox_->Initialize("resources/qwantani_night_puresky_1k_cubemap.dds");
    }

    if (isActivePrimitiveObj_) {
        primitiveObj_ = std::make_unique<Primitive3DObject>();
        primitiveObj_->Initialize(PrimitiveType::Cube);
        primitiveObj_->SetPosition({ 0.0f, 0.0f, 0.0f }); // 他のオブジェクトと被らないように少しずらす
    }

    // 電撃エフェクトの初期化
    lightningCylinder_ = std::make_unique<Primitive3DObject>();
    lightningCylinder_->Initialize(PrimitiveType::Cylinder);
    lightningCylinder_->SetScale({ 0.2f, 10.0f, 0.2f }); // ビームっぽく細長く
    lightningCylinder_->SetPosition({ -2.0f, 0.0f, 0.0f });

    lightningParamsResource_ = engine_->GetDirectXCommon()->CreateBufferResource(sizeof(LightningParams));
    lightningParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightningParamsData_));
    if (lightningParamsData_) {
        *lightningParamsData_ = LightningParams();
        lightningParamsData_->noiseThreshold = 0.2f; // 出現しやすくする
        lightningParamsData_->intensity = 5.0f;      // 輝きを強める
    }

    lightningCylinder_->SetCullingEnabled(false); // 確実に描画されるように一旦OFF
}

// 更新
void DebugScene::Update() {



    // =====
    // ↓ゲームの更新
    // =====
#ifdef USE_IMGUI
    ImGui::Begin("Activation");
    ImGui::Checkbox("Sprite", &isActiveSprite_);
    ImGui::Checkbox("Primitive Test", &isActivePrimitiveObj_);

    ImGui::Checkbox("Obj", &isActiveObj_);
    ImGui::Checkbox("Utash Teapot", &isActiveUtashTeapot_);
    ImGui::Checkbox("Stanford Bunny", &isActiveStanfordBunny_);
    ImGui::Checkbox("MultiMesh", &isActiveMultiMesh_);
    ImGui::Checkbox("MultiMaterial", &isActiveMultiMaterial_);
    ImGui::Checkbox("Suzanne", &isActiveSuzanne_);
    ImGui::Checkbox("Fence", &isActiveFence_);
    ImGui::Checkbox("Terrain", &isActiveTerrain_);


    ImGui::Checkbox("GPU Particle (Code)", &isActiveGPUParticle_);
    ImGui::Checkbox("VoxelParticle", &isActiveVoxelParticle_);
    ImGui::Checkbox("AnimatedCube", &isActiveAnimatedCube_);
    ImGui::Checkbox("Walk", &isActiveWalk_);
    ImGui::Checkbox("SneakWalk", &isActiveSneakWalk_);
    ImGui::Checkbox("Skybox", &isActiveSkybox_);
    ImGui::Checkbox("PrimitiveObj", &isActivePrimitiveObj_);

    ImGui::Checkbox("Lightning Crawl", &isActiveLightningCrawl_);
    ImGui::Checkbox("ImGui Demo", &isActiveImGuiDemo_);
    ImGui::End();

    if (isActiveImGuiDemo_) {
        ImGui::ShowDemoWindow();
    }
#endif

    // 3D


    if (isActiveObj_) {
        if (!obj_) {
            obj_ = std::make_unique<StaticModelObject>();
            obj_->Initialize("sample/plane.gltf");
        }
        obj_->Update();
    }
    if (isActiveUtashTeapot_) {
        if (!utashTeapot_) {
            utashTeapot_ = std::make_unique<StaticModelObject>();
            utashTeapot_->Initialize("sample/teapot.obj");
        }
        utashTeapot_->Debug("Utash Teapot");
        utashTeapot_->Update();
    }
    if (isActiveStanfordBunny_) {
        if (!stanfordBunny_) {
            stanfordBunny_ = std::make_unique<StaticModelObject>();
            stanfordBunny_->Initialize("sample/bunny.obj");
        }
        stanfordBunny_->Debug("Stanford Bunny");
        stanfordBunny_->Update();
    }
    if (isActiveMultiMesh_) {
        if (!multiMesh_) {
            multiMesh_ = std::make_unique<StaticModelObject>();
            multiMesh_->Initialize("sample/multiMesh.obj");
        }
        multiMesh_->Debug("MultiMesh");
        multiMesh_->Update();
    }
    if (isActiveMultiMaterial_) {
        if (!multiMaterial_) {
            multiMaterial_ = std::make_unique<StaticModelObject>();
            multiMaterial_->Initialize("sample/multiMaterial.obj");
        }
        multiMaterial_->Debug("MultiMaterial");
        multiMaterial_->Update();
    }
    if (isActiveSuzanne_) {
        if (!suzanne_) {
            suzanne_ = std::make_unique<StaticModelObject>();
            suzanne_->Initialize("sample/suzanne.obj");
        }
        suzanne_->Update();
    }
    if (isActiveFence_) {
        if (!fence_) {
            fence_ = std::make_unique<StaticModelObject>();
            fence_->Initialize("sample/fence.obj");
        }
        fence_->Debug("Fence");
        fence_->Update();
    }
    if (isActiveTerrain_) {
        if (!terrain_) {
            terrain_ = std::make_unique<StaticModelObject>();
            terrain_->Initialize("sample/terrain.obj");
        }
        terrain_->Debug("Terrain");
        terrain_->Update();
    }


    if (isActiveGPUParticle_) {
        if (!particleObj_) {
            particleObj_ = std::make_unique<ParticleObject>();
            particleObj_->SetTexturePath("resources/circle.png");
            particleObj_->SetEmitType(0);
            particleObj_->SetVelocity(5.0f);
            particleObj_->SetGravity(0.0f);
            particleObj_->SetDamping(0.05f);
            particleObj_->SetLifeTimeMin(0.3f);
            particleObj_->SetLifeTimeMax(0.6f);
            particleObj_->SetRadius(5.0f);
            particleObj_->SetColor({1.0f, 0.5f, 0.0f, 1.0f});
            particleObj_->SetMidColor({1.0f, 0.0f, 0.0f, 1.0f});
            particleObj_->Initialize();
        }
        
#ifdef USE_IMGUI
        ImGui::Begin("Hardcoded Particle Test");
        particleObj_->DebugUI("Hardcoded Particle Test");
        ImGui::End();
#endif

        particleObj_->Update();
        
        // システム全体の線描画は GPUParticleSystem::Update() 内で自動で行われるように変更しました
    } else {
        if (particleObj_) {
            particleObj_.reset();
        }
    }
    
    if (isActiveVoxelParticle_) {
        if (!voxelParticle_) {
            voxelParticle_ = std::make_unique<VoxelParticleSystem>();
            voxelParticle_->Initialize("sample/terrain.obj", { 64,64,64 });
        }
        voxelParticle_->Debug("Voxel Particle");
        voxelParticle_->Update(engine_->GetDeltaTime());
    }
    if (isActiveAnimatedCube_) {
        if (!animatedCube_) {
            animatedCube_ = std::make_unique<AnimationModel>();
            animatedCube_->Initialize("sample/AnimatedCube.gltf");
        }
        animatedCube_->Debug("AnimatedCube");
        animatedCube_->Update();
    }
    if (isActiveWalk_) {
        if (!walk_) {
            walk_ = std::make_unique<AnimationModel>();
            walk_->Initialize("sample/walk.gltf");
        }
        walk_->Debug("Walk");
        walk_->Update();
    }
    if (isActiveSneakWalk_) {
        if (!sneakWalk_) {
            sneakWalk_ = std::make_unique<AnimationModel>();
            sneakWalk_->Initialize("sample/sneakWalk.gltf");
        }
        sneakWalk_->Debug("SneakWalk");
        sneakWalk_->Update();
    }
    if (isActiveSkybox_) {
        if (!skybox_) {
            skybox_ = std::make_unique<Skybox>();
            skybox_->Initialize("resources/qwantani_night_puresky_1k_cubemap.dds");
        }
        skybox_->Update();
    }
    if (isActivePrimitiveObj_) {
        if (!primitiveObj_) {
            primitiveObj_ = std::make_unique<Primitive3DObject>();
            primitiveObj_->Initialize(PrimitiveType::Cube);
        }
        primitiveObj_->Update();
    }

    if (isActiveLightningCrawl_) {
        lightningCylinder_->Update();
    }

    // 2D

    if (isActiveSprite_) {
        if (!sprite_) {
            sprite_ = std::make_unique<Sprite>();
            sprite_->Initialize();
        }
        sprite_->Debug("Sprite");
        sprite_->Update();
    }

    if (engine_->GetInputManager()->IsKeyPressed('P') || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {

        engine_->GetSceneManager()->Request("InGame");
    }

    // =====
    // ↑ゲームの更新
    // =====

    BaseScene::Update();

    // 環境マップをDrawManagerに設定
    if (isActiveSkybox_) {
        D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle = engine_->GetTextureManager()->Resolve(skybox_->GetTextureHandle());
        engine_->GetDrawManager()->SetEnvironmentMap(envMapHandle);
    }
}

void DebugScene::Draw() {
    BaseScene::Draw();
    
    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    engine_->ApplyPSO("Object3D");


    if (isActiveObj_) {
        obj_->Draw();
    }
    if (isActiveUtashTeapot_) {
        utashTeapot_->Draw();
    }
    if (isActiveStanfordBunny_) {
        stanfordBunny_->Draw();
    }
    if (isActiveMultiMesh_) {
        multiMesh_->Draw();
    }
    if (isActiveMultiMaterial_) {
        multiMaterial_->Draw();
    }
    if (isActiveSuzanne_) {
        suzanne_->Draw();
    }
    if (isActiveFence_) {
        fence_->Draw();
    }
    if (isActiveTerrain_) {
        terrain_->Draw();
    }
    if (isActiveAnimatedCube_) {
        animatedCube_->Draw();
    }
    if (isActiveWalk_) {
        walk_->Draw();
    }
    if (isActiveSneakWalk_) {
        sneakWalk_->Draw();
    }

    if (isActivePrimitiveObj_) {
        primitiveObj_->Draw();
    }

    if (isActiveSkybox_) {
        skybox_->Draw();
    }

    if (isActiveLightningCrawl_) {
        // lightningCylinder_->Draw() だと RenderQueue に回されてしまい専用PSOが上書きされるため、
        // 不透明描画の後に専用PSOを適用して描画するよう SubmitPostRender に積む
        lightningCylinder_->SyncBeforeDraw();

        engine_->GetDrawManager()->SubmitPostRender([this]() {
            engine_->SetBlend(BlendMode::kBlendModeAdd);
            engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
            engine_->SetCull(PSOManager::CullMode::None);

            engine_->ApplyPSO("LightningCrawl");
            engine_->BindLightningParams(lightningParamsResource_->GetGPUVirtualAddress());

            RenderPackets::Standard3DPacket packet{};
            packet.resource = lightningCylinder_->GetMesh().resource.get();
            packet.blendMode = BlendMode::kBlendModeAdd;
            packet.depthWrite = PSOManager::DepthWrite::Disable;
            packet.cullMode = PSOManager::CullMode::None;
            engine_->GetDrawManager()->DrawStandard3D(packet);

            // 状態を戻す
            engine_->SetBlend(BlendMode::kBlendModeNormal);
            engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
            engine_->SetCull(PSOManager::CullMode::Back);
        });
    }

    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);

    // Effect と Particle は Additive / DepthDisable で描画
    engine_->SetCull(PSOManager::CullMode::None);
    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyPSO("Particle");





    if (isActiveVoxelParticle_) {
        voxelParticle_->Draw();
    }

    // 2D

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyPSO("Sprite");

    if (isActiveSprite_) {
        sprite_->Draw();
    }

}

void DebugScene::DrawDebugTab() {
#ifdef USE_IMGUI
    BaseScene::DrawDebugTab();
    if (isActiveSkybox_ && skybox_) {
        skybox_->Debug();
    }



    if (isActivePrimitiveObj_ && primitiveObj_) primitiveObj_->Debug("Primitive Object (New)");



    if (isActiveLightningCrawl_ && lightningCylinder_) {
        // オブジェクト標準のデバッグUI（形状やマテリアル）を表示
        lightningCylinder_->Debug("Lightning Cylinder");

        // 同じウィンドウ名で再開して、電撃特有のパラメータを「追記」する
        if (ImGui::Begin("Cylinder: Lightning Cylinder")) {
            ImGui::Separator();
            ImGui::Checkbox("Active Lightning Crawl", &isActiveLightningCrawl_);
            DebugUI::DebugLightning(lightningParamsData_);
        }
        ImGui::End();
    }

#endif
}
