#include "DebugScene.h"

#include "Framework/SceneManager.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "Graphics/Data/CameraForGPU.h"
#include "Graphics/Data/PointLight.h"
#include "Graphics/Data/SpotLight.h"
#include "Graphics/Data/DirectionalLight.h"
#include "Graphics/Data/AreaLight.h"

// デストラクタ
DebugScene::~DebugScene() {

}

// 初期化
void DebugScene::Initialize(IrufemiEngine* engine) {

    engine_ = engine;

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f,0.0f,-10.0f });

    // 重要：SetTranslate の後で行列を確実に更新しておく
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode_ = false;

    // --- ライトの初期化 ---
    auto pointLight = std::make_unique<PointLight>();
    pointLight->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pointLight->position = { 0.0f, 5.0f, 0.0f };
    pointLight->intensity = 1.0f;
    pointLight->radius = 10.0f;
    pointLight->decay = 1.0f;
    pointLight->isActive = 1;
    pointLights_.push_back(std::move(pointLight));

    auto spotLight = std::make_unique<SpotLight>();
    spotLight->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    spotLight->position = { 2.0f, 1.25f, 0.0f };
    spotLight->distance = 7.0f;
    spotLight->direction = Math::Normalize(Vector3{ -1.0f,-1.0f,0.0f });
    spotLight->intensity = 0.0f; // 初期状態ではOFF
    spotLight->decay = 2.0f;
    spotLight->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
    spotLight->isActive = 1;
    spotLights_.push_back(std::move(spotLight));

    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->color = { 1.0f,1.0f,1.0f,1.0f };
    directionalLight_->direction = { 0.5f,-0.7f,1.0f };
    directionalLight_->intensity = 1.0f;

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
    isActiveEffect_ = false;
    isActiveAnimatedCube_ = false;
    isActiveWalk_ = false;
    isActiveSneakWalk_ = false;
    isActiveSkybox_ = true; 

    // 課題用スプライトの初期化
    /*imguiSprite_ = std::make_unique<Sprite>();
    imguiSprite_->Initialize(camera_.get());
    imguiSprite_->SetPosition(100.0f, 100.0f);*/

    if (isActiveSprite_) {
        sprite_ = std::make_unique <Sprite>();
        sprite_->Initialize(camera_.get());
    }
    if (isActiveTriangle_) {
        triangle_ = std::make_unique <TriangleClass>();
        triangle_->Initialize(camera_.get());
    }
    if (isActiveCube_) {
        cube_ = std::make_unique <CubeClass>();
        cube_->Initialize(camera_.get());
    }
    if (isActivePlane_) {
        plane_ = std::make_unique<PlaneClass>();
        plane_->Initialize(camera_.get());
    }
    if (isActiveSphere_) {
        sphere_ = std::make_unique<SphereClass>();
        sphere_->Initialize(camera_.get());
    }
    if (isActiveCylinder_) {
        cylinder_ = std::make_unique<CylinderClass>();
        cylinder_->Initialize(camera_.get());
    }
    if (isActiveObj_) {
        obj_ = std::make_unique<ObjClass>();
        obj_->Initialize(camera_.get(), "sample/plane.gltf");
    }
    if (isActiveStanfordBunny_) {
        stanfordBunny_ = std::make_unique <ObjClass>();
        stanfordBunny_->Initialize(camera_.get(), "sample/bunny.obj");
    }
    if (isActiveUtashTeapot_) {
        utashTeapot_ = std::make_unique <ObjClass>();
        utashTeapot_->Initialize(camera_.get(), "sample/teapot.obj");
    }
    if (isActiveMultiMesh_) {
        multiMesh_ = std::make_unique <ObjClass>();
        multiMesh_->Initialize(camera_.get(), "sample/multiMesh.obj");
    }
    if (isActiveMultiMaterial_) {
        multiMaterial_ = std::make_unique <ObjClass>();
        multiMaterial_->Initialize(camera_.get(), "sample/multiMaterial.obj");
    }
    if (isActiveSuzanne_) {
        suzanne_ = std::make_unique <ObjClass>();
        suzanne_->Initialize(camera_.get(), "sample/suzanne.obj");
    }
    if (isActiveFence_) {
        fence_ = std::make_unique <ObjClass>();
        fence_->Initialize(camera_.get(), "sample/fence.obj");
    }
    if (isActiveTerrain_) {
        terrain_ = std::make_unique <ObjClass>();
        terrain_->Initialize(camera_.get(), "sample/terrain.obj");
    }
    if (isActiveParticle_) {
        particle_ = std::make_unique<ParticleSystem>();
        particle_->Initialize(camera_.get(), "resources/circle.png",ParticleType::kAccelerationField);
    }
    if (isActiveGPUParticle_) {
        gpuParticle_ = std::make_unique<GPUParticleSystem>();
        gpuParticle_->Initialize(camera_.get(), "resources/circle.png");
    }
    if (isActiveVoxelParticle_) {
        voxelParticle_ = std::make_unique<VoxelParticleSystem>();
        voxelParticle_->Initialize("sample/terrain.obj", { 64,64,64 }, camera_.get());
    }
    if (isActiveEffect_) {
        effect_ = std::make_unique<EffectSystem>();
        effect_->Initialize(camera_.get());
    }
    if (isActiveAnimatedCube_) {
        animatedCube_ = std::make_unique<AnimationModel>();
        animatedCube_->Initialize(camera_.get(), "sample/AnimatedCube.gltf");
    }
    if (isActiveWalk_) {
        walk_ = std::make_unique<AnimationModel>();
        walk_->Initialize(camera_.get(), "sample/walk.gltf");
    }
    if (isActiveSneakWalk_) {
        sneakWalk_ = std::make_unique<AnimationModel>();
        sneakWalk_->Initialize(camera_.get(), "sample/sneakWalk.gltf");
    }
    if (isActiveSkybox_) {
        skybox_ = std::make_unique<Skybox>();
        skybox_->Initialize(camera_.get(),"resources/rostock_laage_airport_4k.dds");
    }

    // RenderTextureの初期化
    renderTexture_ = std::make_unique<RenderTexture>();
    renderTexture_->Initialize(engine_->GetDirectXCommon(), engine_->GetClientWidth(), engine_->GetClientHeight(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, { 1.0f, 0.0f, 0.0f, 1.0f });
    renderTexture_->InitializeSprite(camera_.get());

    // エンジンのデフォルトクリアカラーを「青」に設定
    engine_->SetClearColor(Vector4{ 0.1f, 0.25f, 0.5f, 1.0f });
}

// 更新
void DebugScene::Update() {


#ifdef USE_IMGUI

    ImGui::Begin("DebugScene");

    if (ImGui::BeginTabBar("DebugSceneTabs")) {
        
        //DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);

        //// Texture タブ
        //if (ImGui::BeginTabItem("Texture")) {
        //    if (ImGui::Button("allLoadActivate")) {
        //        engine_->GetTextureManager()->LoadAllFromFolder("resources/");
        //    }
        //    ImGui::EndTabItem();
        //}

        // Debug タブ
        if (ImGui::BeginTabItem("Debug")) {
            ImGui::Checkbox("debugCamera", &debugMode_);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

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
    ImGui::Checkbox("Effect", &isActiveEffect_);
    ImGui::Checkbox("AnimatedCube", &isActiveAnimatedCube_);
    ImGui::Checkbox("Walk", &isActiveWalk_);
    ImGui::Checkbox("SneakWalk", &isActiveSneakWalk_);
    ImGui::Checkbox("Skybox", &isActiveSkybox_);
    ImGui::End();

#endif // _DEBUG

    // =====
    // ↓ゲームの更新
    // =====

    // 3D

    if (isActiveTriangle_) {
        if (!triangle_) {
            triangle_ = std::make_unique<TriangleClass>();
            triangle_->Initialize(camera_.get());
        }
        triangle_->Update();
    }
    if (isActiveCube_) {
        if (!cube_) {
            cube_ = std::make_unique<CubeClass>();
            cube_->Initialize(camera_.get());
        }
        cube_->Debug("Cube");
        cube_->Update();
    }
    if (isActivePlane_) {
        if (!plane_) {
            plane_ = std::make_unique<PlaneClass>();
            plane_->Initialize(camera_.get());
        }
        plane_->Debug("Plane");
        plane_->Update();
    }
    if (isActiveSphere_) {
        if (!sphere_) {
            sphere_ = std::make_unique<SphereClass>();
            sphere_->Initialize(camera_.get());
        }
        sphere_->Debug("Sphere");
        sphere_->Update();
    }
    if (isActiveCylinder_) {
        if (!cylinder_) {
            cylinder_ = std::make_unique<CylinderClass>();
            cylinder_->Initialize(camera_.get());
        }
        cylinder_->Debug("Cylinder");
        cylinder_->Update();
    }
    if (isActiveObj_) {
        if (!obj_) {
            obj_ = std::make_unique<ObjClass>();
            obj_->Initialize(camera_.get(), "sample/plane.gltf");
        }
        obj_->Debug("Plane");
        obj_->Update();
    }
    if (isActiveUtashTeapot_) {
        if (!utashTeapot_) {
            utashTeapot_ = std::make_unique<ObjClass>();
            utashTeapot_->Initialize(camera_.get(), "sample/teapot.obj");
        }
        utashTeapot_->Debug("Utash Teapot");
        utashTeapot_->Update();
    }
    if (isActiveStanfordBunny_) {
        if (!stanfordBunny_) {
            stanfordBunny_ = std::make_unique<ObjClass>();
            stanfordBunny_->Initialize(camera_.get(), "sample/bunny.obj");
        }
        stanfordBunny_->Debug("Stanford Bunny");
        stanfordBunny_->Update();
    }
    if (isActiveMultiMesh_) {
        if (!multiMesh_) {
            multiMesh_ = std::make_unique<ObjClass>();
            multiMesh_->Initialize(camera_.get(), "sample/multiMesh.obj");
        }
        multiMesh_->Debug("MultiMesh");
        multiMesh_->Update();
    }
    if (isActiveMultiMaterial_) {
        if (!multiMaterial_) {
            multiMaterial_ = std::make_unique<ObjClass>();
            multiMaterial_->Initialize(camera_.get(), "sample/multiMaterial.obj");
        }
        multiMaterial_->Debug("MultiMaterial");
        multiMaterial_->Update();
    }
    if (isActiveSuzanne_) {
        if (!suzanne_) {
            suzanne_ = std::make_unique<ObjClass>();
            suzanne_->Initialize(camera_.get(), "sample/suzanne.obj");
        }
        suzanne_->Debug("Suzanne");
        suzanne_->Update();
    }
    if (isActiveFence_) {
        if (!fence_) {
            fence_ = std::make_unique<ObjClass>();
            fence_->Initialize(camera_.get(), "sample/fence.obj");
        }
        fence_->Debug("Fence");
        fence_->Update();
    }
    if (isActiveTerrain_) {
        if (!terrain_) {
            terrain_ = std::make_unique<ObjClass>();
            terrain_->Initialize(camera_.get(), "sample/terrain.obj");
        }
        terrain_->Debug("Terrain");
        terrain_->Update();
    }
    if (isActiveParticle_) {
        if (!particle_) {
            particle_ = std::make_unique <ParticleSystem>();
            particle_->Initialize(camera_.get(), "resources/circle.png", ParticleType::kAccelerationField);
        }
        particle_->Debug("Particle");
        particle_->Update();
    }
    if (isActiveGPUParticle_) {
        if (!gpuParticle_) {
            gpuParticle_ = std::make_unique <GPUParticleSystem>();
            gpuParticle_->Initialize(camera_.get(), "resources/circle.png");
        }
        gpuParticle_->Update();
    }
    if (isActiveVoxelParticle_) {
        if (!voxelParticle_) {
            voxelParticle_ = std::make_unique<VoxelParticleSystem>();
            voxelParticle_->Initialize("sample/terrain.obj", { 64,64,64 }, camera_.get());
        }
        voxelParticle_->Debug("Voxel Particle");
        voxelParticle_->Update(engine_->GetDeltaTime());
    }
    if (isActiveEffect_) {
        if (!effect_) {
            effect_ = std::make_unique <EffectSystem>();
            effect_->Initialize(camera_.get());
        }
        effect_->Debug("Effect");
        effect_->Update();
    }
    if (isActiveAnimatedCube_) {
        if (!animatedCube_) {
            animatedCube_ = std::make_unique<AnimationModel>();
            animatedCube_->Initialize(camera_.get(), "sample/AnimatedCube.gltf");
        }
        animatedCube_->Debug("AnimatedCube");
        animatedCube_->Update();
    }
    if (isActiveWalk_) {
        if (!walk_) {
            walk_ = std::make_unique<AnimationModel>();
            walk_->Initialize(camera_.get(), "sample/walk.gltf");
        }
        walk_->Debug("Walk");
        walk_->Update();
    }
    if (isActiveSneakWalk_) {
        if (!sneakWalk_) {
            sneakWalk_ = std::make_unique<AnimationModel>();
            sneakWalk_->Initialize(camera_.get(), "sample/sneakWalk.gltf");
        }
        sneakWalk_->Debug("SneakWalk");
        sneakWalk_->Update();
    }
    if (isActiveSkybox_) {
        if (!skybox_) {
            skybox_ = std::make_unique<Skybox>();
            skybox_->Initialize(camera_.get(),"resources/rostock_laage_airport_4k.dds");
        }
        skybox_->Debug();
        skybox_->Update();
    }

    // 2D

    if (isActiveSprite_) {
        if (!sprite_) {
            sprite_ = std::make_unique<Sprite>();
            sprite_->Initialize(camera_.get());
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

    // --- カメラの更新 ---
    if (debugMode_) {
        // デバッグカメラを更新
        debugCamera_->Update();
        // デバッグカメラの計算結果をメインカメラに上書きする
        const Camera& dbgCam = debugCamera_->GetCamera();
        camera_->SetViewMatrix(dbgCam.GetViewMatrix());
        camera_->SetTranslate(dbgCam.GetTranslate());
        // プロジェクション行列も念のため同期
        camera_->SetPerspectiveFovMatrix(dbgCam.GetPerspectiveFovMatrix());
    }
    else {
        camera_->Debug("Camera");
        // 通常カメラの更新
        camera_->Update();
    }

    // --- フレーム共通データのセット ---
    // 常に camera_ を参照すればOK
    CameraForGPU cameraForGpu;
    cameraForGpu.view = camera_->GetViewMatrix();
    cameraForGpu.projection = camera_->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = camera_->GetTranslate();

    std::vector<PointLight*> pLights;
    for (const auto& light : pointLights_) {
        pLights.push_back(light.get());
    }
    std::vector<SpotLight*> sLights;
    for (const auto& light : spotLights_) {
        sLights.push_back(light.get());
    }
    std::vector<AreaLight*> aLights;
    for (const auto& light : areaLights_) {
        aLights.push_back(light.get());
    }

    engine_->GetDrawManager()->SetFrameData(cameraForGpu, *directionalLight_, pLights, sLights, aLights);

    //// 環境マップをDrawManagerに設定
    //if (isActiveSkybox_) {
    //    D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle = engine_->GetTextureManager()->GetTextureHandle("resources/rostock_laage_airport_4k.dds");
    //    engine_->GetDrawManager()->SetEnvironmentMap(envMapHandle);
    //}
}

void DebugScene::Draw() {

    // 1. RenderTextureへの描画開始 (画像2のPIX通り、クリア色を「赤」にします)
    engine_->GetDrawManager()->BeginRenderTexture(renderTexture_.get(), { 1.0f, 0.0f, 0.0f, 1.0f });

    if (isActiveSkybox_) {
        skybox_->Draw();
    }

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);
    engine_->ApplyByGeometryShaderPSO();

    if (isActiveTriangle_) {
        triangle_->Draw();
    }

    engine_->ApplyPSO();

    if (isActiveCube_) {
        cube_->Draw();
    }
    if (isActivePlane_) {
        plane_->Draw();
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

    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);

    // Effect と Particle は Additive / DepthDisable で描画
    engine_->SetCull(PSOManager::CullMode::None);
    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyParticlePSO();

    if (isActiveEffect_) {
        effect_->Draw();
    }

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
    engine_->ApplySpritePSO();

    if (isActiveSprite_) {
        sprite_->Draw();
    }

    // 2. RenderTextureへの描画終了
    engine_->GetDrawManager()->EndRenderTexture(renderTexture_.get());

    // 3. 通常の描画（SwapChain）の描画先に戻す
    // (エンジンの PreDraw ですでにクリア済みのバックバッファに描画対象を戻します)
    engine_->GetDrawManager()->SetRenderTargetToBackBuffer();

    // 4. RenderTextureの内容を画面に描画（確認用のイメージが「ImGuiしか画面に出ていない」と言っているので、
    // ここで Draw を呼ばないのが指示通りですが、必要に応じて有効にしてください）
    // renderTexture_->Draw(engine_->GetDrawManager());
}