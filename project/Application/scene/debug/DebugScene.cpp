#include "DebugScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "math/CameraForGPU.h"
#include "math/PointLight.h"
#include "math/SpotLight.h"
#include "math/DirectionalLight.h"
#include "math/AreaLight.h"
#include "2D/Sprite.h"

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
    isActiveEffect_ = false;
    isActiveAnimatedCube_ = false;
    isActiveWalk_ = false;
    isActiveSneakWalk_ = false;
    isActiveAnimationNode_ = false;
    isActiveAnimationNodeMisc_ = false;
    isActiveMeshPrimitives_ = false;
    isActiveMeshPrimitiveVertexColor_ = false;
    isActiveTextureSampler_ = false;
    isActiveMaterialAlphaBlend_ = false;
    isActiveAnimationSkin_ = false;

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
    if (isActiveAnimationNode_) {
        animationNode_ = std::make_unique<AnimationModel>();
        animationNode_->Initialize(camera_.get(), "test/Animation_Node/Animation_Node_00.gltf");
    }
    if (isActiveAnimationNodeMisc_) {
        animationNodeMisc_ = std::make_unique<AnimationModel>();
        animationNodeMisc_->Initialize(camera_.get(), "test/Animation_NodeMisc/Animation_NodeMisc_00.gltf");
    }
    if (isActiveMeshPrimitives_) {
        meshPrimitives_ = std::make_unique<ObjClass>();
        meshPrimitives_->Initialize(camera_.get(), "test/Mesh_Primitives/Mesh_Primitives_00.gltf");
    }
    if (isActiveMeshPrimitiveVertexColor_) {
        meshPrimitiveVertexColor_ = std::make_unique<ObjClass>();
        meshPrimitiveVertexColor_->Initialize(camera_.get(), "test/Mesh_PrimitiveVertexColor/Mesh_PrimitiveVertexColor_00.gltf");
    }
    if (isActiveTextureSampler_) {
        textureSampler_ = std::make_unique<ObjClass>();
        textureSampler_->Initialize(camera_.get(), "test/Texture_Sampler/Texture_Sampler_00.gltf");
    }
    if (isActiveMaterialAlphaBlend_) {
        materialAlphaBlend_ = std::make_unique<ObjClass>();
        materialAlphaBlend_->Initialize(camera_.get(), "test/Material_AlphaBlend/Material_AlphaBlend_00.gltf");
    }
    if (isActiveAnimationSkin_) {
        animationSkin_ = std::make_unique<AnimationModel>();
        animationSkin_->Initialize(camera_.get(), "test/Animation_Skin/Animation_Skin_00.gltf");
    }

    line2D_ = std::make_unique<Line2DClass>();
    line2D_->Initialize(camera_.get(), { 300.0f,300.0f }, { 360.0f,360.0f });
}

// 更新
void DebugScene::Update() {


#ifdef USE_IMGUI

    ImGui::Begin("DebugScene");

    if (ImGui::BeginTabBar("DebugSceneTabs")) {
        
        DebugUI::DebugLights(directionalLight_.get(), pointLights_, spotLights_, areaLights_);

        // Texture タブ
        if (ImGui::BeginTabItem("Texture")) {
            if (ImGui::Button("allLoadActivate")) {
                engine_->GetTextureManager()->LoadAllFromFolder("resources/");
            }
            ImGui::EndTabItem();
        }

        // Debug タブ
        if (ImGui::BeginTabItem("Debug")) {
            ImGui::Checkbox("debugMode", &debugMode_);
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
    ImGui::Checkbox("Effect", &isActiveEffect_);
    ImGui::Checkbox("AnimatedCube", &isActiveAnimatedCube_);
    ImGui::Checkbox("Walk", &isActiveWalk_);
    ImGui::Checkbox("SneakWalk", &isActiveSneakWalk_);
    ImGui::Checkbox("Aniamtion Node", &isActiveAnimationNode_);
    ImGui::Checkbox("Animation NodeMisc", &isActiveAnimationNodeMisc_);
    ImGui::Checkbox("Mesh Primitives", &isActiveMeshPrimitives_);
    ImGui::Checkbox("Mesh PrimitiveVertexColor", &isActiveMeshPrimitiveVertexColor_);
    ImGui::Checkbox("Texture Sampler", &isActiveTextureSampler_);
    ImGui::Checkbox("Material AlphaBlend", &isActiveMaterialAlphaBlend_);
    ImGui::Checkbox("Animation Skin", &isActiveAnimationSkin_);
    ImGui::End();

    ImGui::Begin("GE");


    ImGui::Text("Hello, world %d", 123);
    if (ImGui::Button("showDemoWindow")) {
        auto MySaveFunction = [&]() { showDemoWindow = !showDemoWindow; return showDemoWindow; };
        showDemoWindow = MySaveFunction();
    }
    static char buf[256] = "";
    ImGui::InputText("string", buf, IM_ARRAYSIZE(buf));
    static float f{};
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

    ImGui::End();

    if (showDemoWindow) {

        // 課題用ImGuiウィンドウ
        ImGui::SetNextWindowSize(ImVec2(500, 100));
        ImGui::Begin("Sprite Control");
        if (imguiSprite_) {
            Vector2 pos = imguiSprite_->GetPosition2D();
            float pos_xy[] = { pos.x, pos.y };
            // スライダーの範囲は仮で0-1280としています
            if (ImGui::SliderFloat2("Position", pos_xy, 0.0f, 1280.0f, "%.1f")) {
                imguiSprite_->SetPosition(pos_xy[0], pos_xy[1]);
            }
        }
        ImGui::End();

        static bool my_tool_active = true;

        ImGui::Begin("My First Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
                if (ImGui::MenuItem("Close", "Ctrl+W")) { my_tool_active = false; }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // ImGui用カラー変数を追加
        static float my_color[4] = { 1.0f, 0.0f, 1.0f, 1.0f };

        // Edit a color stored as 4 floats
        ImGui::ColorEdit4("Color", my_color);

        // グラフの描画
        // Generate samples and plot them
        // グラフに表示するための100個のデータ点を格納する配列を宣言
        float samples[100];
        // sinf() (サイン関数) を使って、波のような形になる値を計算し、samples 配列に格納
        // ImGui::GetTime() を計算に加えることで、グラフが時間と共に左に流れていくようなアニメーションになる
        for (int n = 0; n < 100; n++)
            samples[n] = sinf(n * 0.2f + static_cast<float>(ImGui::GetTime()) * 1.5f);
        // samples 配列のデータを "Samples" というラベルの折れ線グラフとしてImGuiウィンドウ内に描画
        ImGui::PlotLines("Samples", samples, 100);

        // スクロール可能なテキスト領域の表示
        // Display contents in a scrolling region
        // 黄色で見出しを表示
        ImGui::TextColored(ImVec4(my_color[0], my_color[1], my_color[2], my_color[3]), "Important Stuff");
        // スクロール可能な子領域を開始
        ImGui::BeginChild("Scrolling");
        // 50行分のテキストを表示し、スクロールバーを発生させる
        for (int n = 0; n < 50; n++)
            ImGui::Text("%04d: Some text", n);
        // 子領域を終了
        ImGui::EndChild();
        ImGui::End();

        ImGui::ShowDemoWindow();

    }

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
    if (isActiveAnimationNode_) {
        if (!animationNode_) {
            animationNode_ = std::make_unique<AnimationModel>();
            animationNode_->Initialize(camera_.get(), "test/Animation_Node/Animation_Node_00.gltf");
        }
        animationNode_->Debug("Animation Node");
        animationNode_->Update();
    }
    if (isActiveAnimationNodeMisc_) {
        if (!animationNodeMisc_) {
            animationNodeMisc_ = std::make_unique<AnimationModel>();
            animationNodeMisc_->Initialize(camera_.get(), "test/Animation_NodeMisc/Animation_NodeMisc_00.gltf");
        }
        animationNodeMisc_->Debug("Animation NodeMisc");
        animationNodeMisc_->Update();
    }
    if (isActiveMeshPrimitives_) {
        if (!meshPrimitives_) {
            meshPrimitives_ = std::make_unique<ObjClass>();
            meshPrimitives_->Initialize(camera_.get(), "test/Mesh_Primitives/Mesh_Primitives_00.gltf");
        }
        meshPrimitives_->Debug("Mesh Primitives");
        meshPrimitives_->Update();
    }
    if (isActiveMeshPrimitiveVertexColor_) {
        if (!meshPrimitiveVertexColor_) {
            meshPrimitiveVertexColor_ = std::make_unique<ObjClass>();
            meshPrimitiveVertexColor_->Initialize(camera_.get(), "test/Mesh_PrimitiveVertexColor/Mesh_PrimitiveVertexColor_00.gltf");
        }
        meshPrimitiveVertexColor_->Debug("Mesh PrimitiveVertexColor");
        meshPrimitiveVertexColor_->Update();
    }
    if (isActiveTextureSampler_) {
        if (!textureSampler_) {
            textureSampler_ = std::make_unique<ObjClass>();
            textureSampler_->Initialize(camera_.get(), "test/Texture_Sampler/Texture_Sampler_00.gltf");
        }
        textureSampler_->Debug("Texture Sampler");
        textureSampler_->Update();
    }
    if (isActiveMaterialAlphaBlend_) {
        if (!materialAlphaBlend_) {
            materialAlphaBlend_ = std::make_unique<ObjClass>();
            materialAlphaBlend_->Initialize(camera_.get(), "test/Material_AlphaBlend/Material_AlphaBlend_00.gltf");
        }
        materialAlphaBlend_->Debug("Material AlphaBlend");
        materialAlphaBlend_->Update();
    }
    if (isActiveAnimationSkin_) {
        if (!animationSkin_) {
            animationSkin_ = std::make_unique<AnimationModel>();
            animationSkin_->Initialize(camera_.get(), "test/Animation_Skin/Animation_Skin_00.gltf");
        }
        animationSkin_->Debug("Animation Skin");
        animationSkin_->Update();
    }

    line2D_->Update();

    // 2D

    // 課題用スプライトの更新
    if (imguiSprite_) {
        imguiSprite_->Update();
    }

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
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    // --- フレーム共通データのセット ---
    CameraForGPU cameraForGpu;
    cameraForGpu.view = currentCamera->GetViewMatrix();
    cameraForGpu.projection = currentCamera->GetPerspectiveFovMatrix();
    cameraForGpu.worldPosition = currentCamera->GetTranslate();

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
}

void DebugScene::Draw() {

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
    if (isActiveAnimationNode_) {
        animationNode_->Draw();
    }
    if (isActiveAnimationNodeMisc_) {
        animationNodeMisc_->Draw();
    }
    if (isActiveMeshPrimitives_) {
        meshPrimitives_->Draw();
    }
    if (isActiveMeshPrimitiveVertexColor_) {
        meshPrimitiveVertexColor_->Draw();
    }
    if (isActiveTextureSampler_) {
        textureSampler_->Draw();
    }
    if (isActiveMaterialAlphaBlend_) {
        materialAlphaBlend_->Draw();
    }
    if (isActiveAnimationSkin_) {
        animationSkin_->Draw();
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

    // 2D

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    // 課題用スプライトの描画
    if(showDemoWindow){
        if (imguiSprite_) {
            imguiSprite_->Draw();
        }
    }

    if (isActiveSprite_) {
        sprite_->Draw();
    }
    
    /*

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyLinePSO();

    line2D_->Draw();*/

}