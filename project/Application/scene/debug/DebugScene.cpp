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
    isActiveTriangle_ = false;
    isActiveCube_ = false;
    isActivePlane_ = false;
    isActiveSphere_ = false; 
    isActiveCylinder_ = false;
    isActiveStanfordBunny_ = false;
    isActiveUtashTeapot_ = false;
    isActiveMultiMesh_ = false;
    isActiveMultiMaterial_ = false;
    isActiveSuzanne_ = false;
    isActiveFence_ = false;
    isActiveTerrain_ = false;
    isActiveParticle_ = false;
    isActiveGPUParticle_ = false;
    isActiveVoxelParticle_ = false;
    isActiveAnimatedCube_ = false;
    isActiveWalk_ = false;
    isActiveSneakWalk_ = false;
    isActiveSkybox_ = false;
    isActivePrimitiveObj_ = false;
    isActiveGPUParticle_ = false;
    isActiveLightningCrawl_ = false;
    isActiveEnergyCore_ = true;
    isActiveImGuiDemo_ = false;

    // 課題用スプライトの初期化
    /*imguiSprite_ = std::make_unique<Sprite>();
    imguiSprite_->Initialize();
    imguiSprite_->SetPosition(100.0f, 100.0f);*/

    if (isActiveSprite_) {
        sprite_ = std::make_unique <Sprite>();
        sprite_->Initialize();
    }
    if (isActiveTriangle_) {
        triangle_ = std::make_unique <TriangleClass>();
        triangle_->Initialize();
    }
    if (isActiveCube_) {
        cube_ = std::make_unique <CubeClass>();
        cube_->Initialize();
    }
    if (isActivePlane_) {
        plane_ = std::make_unique<PlaneClass>();
        plane_->Initialize();
    }
    if (isActiveSphere_) {
        sphere_ = std::make_unique<SphereClass>();
        sphere_->Initialize();
    }
    if (isActiveCylinder_) {
        cylinder_ = std::make_unique<CylinderClass>();
        cylinder_->Initialize();
    }
    if (isActiveObj_) {
        obj_ = std::make_unique<ObjClass>();
        obj_->Initialize("sample/plane.gltf");
    }
    if (isActiveStanfordBunny_) {
        stanfordBunny_ = std::make_unique <ObjClass>();
        stanfordBunny_->Initialize("sample/bunny.obj");
    }
    if (isActiveUtashTeapot_) {
        utashTeapot_ = std::make_unique <ObjClass>();
        utashTeapot_->Initialize("sample/teapot.obj");
    }
    if (isActiveMultiMesh_) {
        multiMesh_ = std::make_unique <ObjClass>();
        multiMesh_->Initialize("sample/multiMesh.obj");
    }
    if (isActiveMultiMaterial_) {
        multiMaterial_ = std::make_unique <ObjClass>();
        multiMaterial_->Initialize("sample/multiMaterial.obj");
    }
    if (isActiveSuzanne_) {
        suzanne_ = std::make_unique <ObjClass>();
        suzanne_->Initialize("sample/suzanne.obj");
    }
    if (isActiveFence_) {
        fence_ = std::make_unique <ObjClass>();
        fence_->Initialize("sample/fence.obj");
    }
    if (isActiveTerrain_) {
        terrain_ = std::make_unique <ObjClass>();
        terrain_->Initialize("sample/terrain.obj");
    }
    if (isActiveParticle_) {
        particle_ = std::make_unique<ParticleSystem>();
        particle_->Initialize("resources/circle.png",ParticleType::kAccelerationField);
    }
    if (isActiveGPUParticle_) {
        gpuParticle_ = std::make_unique<GPUParticleSystem>();
        gpuParticle_->Initialize("resources/circle.png");
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
        primitiveObj_ = std::make_unique<PrimitiveObjects3DClass>();
        primitiveObj_->Initialize(PrimitiveType::Cube);
        primitiveObj_->SetPosition({ 0.0f, 0.0f, 0.0f }); // 他のオブジェクトと被らないように少しずらす
    }

    // 電撃エフェクトの初期化
    lightningCylinder_ = std::make_unique<CylinderClass>();
    lightningCylinder_->Initialize();
    lightningCylinder_->SetRadius(0.2f); // ビームっぽく細長く
    lightningCylinder_->SetHeight(10.0f);
    lightningCylinder_->SetCenter({ -2.0f, 0.0f, 0.0f });

    lightningParamsResource_ = engine_->GetDirectXCommon()->CreateBufferResource(sizeof(LightningParams));
    lightningParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightningParamsData_));
    if (lightningParamsData_) {
        *lightningParamsData_ = LightningParams();
        lightningParamsData_->noiseThreshold = 0.2f; // 出現しやすくする
        lightningParamsData_->intensity = 5.0f;      // 輝きを強める
    }

    lightningCylinder_->SetCullingEnabled(false); // 確実に描画されるように一旦OFF
    lightningCylinder_->SetCustomPSO(
        engine_->GetPSOManager()->GetPSO("LightningCrawl", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None)
    );
    if (lightningParamsResource_) {
        lightningCylinder_->SetCustomCBVAddress(lightningParamsResource_->GetGPUVirtualAddress());
    }

    // エネルギーコアデモの初期化
    energyCore_ = std::make_unique<PrimitiveObjects3DClass>();
    energyCore_->Initialize(PrimitiveType::Sphere);
    energyCore_->SetScale({ 2.0f, 2.0f, 2.0f });
    energyCore_->SetPosition({ 2.0f, 0.0f, 0.0f }); // 電撃と並べる
    energyCore_->SetColor({ 0.0f, 0.5f, 1.0f, 1.0f }); // シェーダー汎用化に伴い、青色を明示的に指定
    energyCore_->SetCullingEnabled(false);
    energyCore_->SetCustomPSO(
        engine_->GetPSOManager()->GetPSO("EnergyCore", BlendMode::kBlendModePremultiplied, PSOManager::DepthWrite::Disable, PSOManager::CullMode::Back)
    );
}

// 更新
void DebugScene::Update() {



    // =====
    // ↓ゲームの更新
    // =====
#ifdef USE_IMGUI
    ImGui::Begin("Activation");
    ImGui::Checkbox("Sprite", &isActiveSprite_);
    ImGui::Checkbox("Triangle", &isActiveTriangle_);
    ImGui::Checkbox("Cube", &isActiveCube_);
    ImGui::Checkbox("Plane", &isActivePlane_);
    ImGui::Checkbox("Sphere", &isActiveSphere_);
    ImGui::Checkbox("Cylinder", &isActiveCylinder_);
    ImGui::Checkbox("Obj", &isActiveObj_);
    ImGui::Checkbox("Utash Teapot", &isActiveUtashTeapot_);
    ImGui::Checkbox("Stanford Bunny", &isActiveStanfordBunny_);
    ImGui::Checkbox("MultiMesh", &isActiveMultiMesh_);
    ImGui::Checkbox("MultiMaterial", &isActiveMultiMaterial_);
    ImGui::Checkbox("Suzanne", &isActiveSuzanne_);
    ImGui::Checkbox("Fence", &isActiveFence_);
    ImGui::Checkbox("Terrain", &isActiveTerrain_);
    ImGui::Checkbox("Particle", &isActiveParticle_);
    ImGui::Checkbox("GPUParticle", &isActiveGPUParticle_);
    ImGui::Checkbox("VoxelParticle", &isActiveVoxelParticle_);
    ImGui::Checkbox("AnimatedCube", &isActiveAnimatedCube_);
    ImGui::Checkbox("Walk", &isActiveWalk_);
    ImGui::Checkbox("SneakWalk", &isActiveSneakWalk_);
    ImGui::Checkbox("Skybox", &isActiveSkybox_);
    ImGui::Checkbox("PrimitiveObj", &isActivePrimitiveObj_);

    ImGui::Checkbox("Lightning Crawl", &isActiveLightningCrawl_);
    ImGui::Checkbox("Energy Core", &isActiveEnergyCore_);
    ImGui::Checkbox("ImGui Demo", &isActiveImGuiDemo_);
    ImGui::End();

    if (isActiveImGuiDemo_) {
        ImGui::ShowDemoWindow();
    }
#endif

    // 3D

    if (isActiveTriangle_) {
        if (!triangle_) {
            triangle_ = std::make_unique<TriangleClass>();
            triangle_->Initialize();
        }
        triangle_->Update();
    }
    if (isActiveCube_) {
        if (!cube_) {
            cube_ = std::make_unique<CubeClass>();
            cube_->Initialize();
        }

#ifdef USE_IMGUI
        // ImGuizmo の操作
        ImGuizmo::BeginFrame();
        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        Camera* activeCamera = engine_->GetCameraManager()->GetActiveCamera();
        Matrix4x4 view = activeCamera->GetViewMatrix();
        Matrix4x4 projection = activeCamera->GetPerspectiveFovMatrix();
        auto& transform = cube_->GetD3D12Resource()->transform_;
        Matrix4x4 world = Math::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

        if (ImGuizmo::Manipulate(&view.m[0][0], &projection.m[0][0], gizmoOperation_, gizmoMode_, &world.m[0][0])) {
            float pos[3], rot[3], scale[3];
            ImGuizmo::DecomposeMatrixToComponents(&world.m[0][0], pos, rot, scale);

            cube_->SetPosition({ pos[0], pos[1], pos[2] });
            cube_->SetRotate({ rot[0] * Math::PI / 180.0f, rot[1] * Math::PI / 180.0f, rot[2] * Math::PI / 180.0f });
            cube_->SetScale({ scale[0], scale[1], scale[2] });
        }

        // ギズモ設定UI
        ImGui::Begin("Gizmo Settings");
        if (ImGui::RadioButton("Translate", gizmoOperation_ == ImGuizmo::TRANSLATE)) gizmoOperation_ = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", gizmoOperation_ == ImGuizmo::ROTATE)) gizmoOperation_ = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", gizmoOperation_ == ImGuizmo::SCALE)) gizmoOperation_ = ImGuizmo::SCALE;

        if (ImGui::RadioButton("Local", gizmoMode_ == ImGuizmo::LOCAL)) gizmoMode_ = ImGuizmo::LOCAL;
        ImGui::SameLine();
        if (ImGui::RadioButton("World", gizmoMode_ == ImGuizmo::WORLD)) gizmoMode_ = ImGuizmo::WORLD;
        ImGui::End();
#endif

        cube_->Update();
    }
    if (isActivePlane_) {
        if (!plane_) {
            plane_ = std::make_unique<PlaneClass>();
            plane_->Initialize();
        }
        plane_->Update();
    }
    if (isActiveSphere_) {
        if (!sphere_) {
            sphere_ = std::make_unique<SphereClass>();
            sphere_->Initialize();
        }
        sphere_->Update();
    }
    if (isActiveCylinder_) {
        if (!cylinder_) {
            cylinder_ = std::make_unique<CylinderClass>();
            cylinder_->Initialize();
        }
        cylinder_->Update();
    }
    if (isActiveObj_) {
        if (!obj_) {
            obj_ = std::make_unique<ObjClass>();
            obj_->Initialize("sample/plane.gltf");
        }
        obj_->Update();
    }
    if (isActiveUtashTeapot_) {
        if (!utashTeapot_) {
            utashTeapot_ = std::make_unique<ObjClass>();
            utashTeapot_->Initialize("sample/teapot.obj");
        }
        utashTeapot_->Debug("Utash Teapot");
        utashTeapot_->Update();
    }
    if (isActiveStanfordBunny_) {
        if (!stanfordBunny_) {
            stanfordBunny_ = std::make_unique<ObjClass>();
            stanfordBunny_->Initialize("sample/bunny.obj");
        }
        stanfordBunny_->Debug("Stanford Bunny");
        stanfordBunny_->Update();
    }
    if (isActiveMultiMesh_) {
        if (!multiMesh_) {
            multiMesh_ = std::make_unique<ObjClass>();
            multiMesh_->Initialize("sample/multiMesh.obj");
        }
        multiMesh_->Debug("MultiMesh");
        multiMesh_->Update();
    }
    if (isActiveMultiMaterial_) {
        if (!multiMaterial_) {
            multiMaterial_ = std::make_unique<ObjClass>();
            multiMaterial_->Initialize("sample/multiMaterial.obj");
        }
        multiMaterial_->Debug("MultiMaterial");
        multiMaterial_->Update();
    }
    if (isActiveSuzanne_) {
        if (!suzanne_) {
            suzanne_ = std::make_unique<ObjClass>();
            suzanne_->Initialize("sample/suzanne.obj");
        }
        suzanne_->Update();
    }
    if (isActiveFence_) {
        if (!fence_) {
            fence_ = std::make_unique<ObjClass>();
            fence_->Initialize("sample/fence.obj");
        }
        fence_->Debug("Fence");
        fence_->Update();
    }
    if (isActiveTerrain_) {
        if (!terrain_) {
            terrain_ = std::make_unique<ObjClass>();
            terrain_->Initialize("sample/terrain.obj");
        }
        terrain_->Debug("Terrain");
        terrain_->Update();
    }
    if (isActiveParticle_) {
        if (!particle_) {
            particle_ = std::make_unique <ParticleSystem>();
            particle_->Initialize("resources/circle.png", ParticleType::kAccelerationField);
        }
        particle_->Debug("Particle");
        particle_->Update();
    }
    if (isActiveGPUParticle_) {
        if (!gpuParticle_) {
            gpuParticle_ = std::make_unique <GPUParticleSystem>();
            gpuParticle_->Initialize("resources/circle.png");
        }
        gpuParticle_->Update();
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
            primitiveObj_ = std::make_unique<PrimitiveObjects3DClass>();
            primitiveObj_->Initialize(PrimitiveType::Cube);
        }
        primitiveObj_->Update();
    }

    if (isActiveLightningCrawl_) {
        lightningCylinder_->Update();
    }
    if (isActiveEnergyCore_) {
        energyCore_->Update();
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
        D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle = skybox_->GetTextureHandle();
        engine_->GetDrawManager()->SetEnvironmentMap(envMapHandle);
    }
}

void DebugScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    engine_->ApplyPSO("Object3D");

    if (isActiveTriangle_) {
        triangle_->Draw();
    }
    if (isActivePlane_) {
        plane_->Draw();
    }
    if (isActiveCube_) {
        cube_->Draw();
    }
    if (isActiveSphere_) {
        sphere_->Draw();
    }
    if (isActiveCylinder_) {
        cylinder_->Draw();
    }
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
        // パケット（キュー）に積まれる描画ステートを設定
        engine_->SetBlend(BlendMode::kBlendModeAdd);
        engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
        engine_->SetCull(PSOManager::CullMode::None);

        // 通常通りDrawを呼ぶだけで、DrawManager内で適切なパスと順序（カスタムPSOとCBV付き）で描画される
        lightningCylinder_->Draw();

        // 状態を戻す
        engine_->SetBlend(BlendMode::kBlendModeNormal);
        engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
        engine_->SetCull(PSOManager::CullMode::Back);
    }

    if (isActiveEnergyCore_) {
        // パケット（キュー）に積まれる描画ステートを設定
        engine_->SetBlend(BlendMode::kBlendModeAdd);
        engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
        engine_->SetCull(PSOManager::CullMode::None);

        // 通常通りDrawを呼ぶだけで、DrawManager内で適切なパスと順序（カスタムPSO付き）で描画される
        energyCore_->Draw();

        // 状態を戻す
        engine_->SetBlend(BlendMode::kBlendModeNormal);
        engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
        engine_->SetCull(PSOManager::CullMode::Back);
    }

    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);

    // Effect と Particle は Additive / DepthDisable で描画
    engine_->SetCull(PSOManager::CullMode::None);
    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyPSO("Particle");

    if (isActiveParticle_) {
        particle_->Draw();
    }

    if (isActiveGPUParticle_) {
        gpuParticle_->Draw();
    }

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

    if (isActiveCube_ && cube_) cube_->Debug("Cube");
    if (isActivePlane_ && plane_) plane_->Debug("Plane");
    if (isActiveSphere_ && sphere_) sphere_ ->Debug("Sphere");
    if (isActiveCylinder_ && cylinder_) cylinder_->Debug("Cylinder");

    if (isActivePrimitiveObj_ && primitiveObj_) primitiveObj_->Debug("Primitive Object (New)");

    if (isActiveGPUParticle_ && gpuParticle_) {
        gpuParticle_->Debug();
    }

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

    if (isActiveEnergyCore_ && energyCore_) {
        energyCore_->Debug("Energy Core Sphere");
    }

    DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);
#endif
}
