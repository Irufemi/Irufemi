#include "CGScene.h"

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
CGScene::~CGScene() {

}

// 初期化
void CGScene::Initialize(IrufemiEngine* engine) {

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

    isActiveSphere_ = false;
    isActiveTerrain_ = false;
    isActivePlaneObj_ = false;
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

    if (isActiveSphere_) {
        sphere_ = std::make_unique<SphereClass>();
        sphere_->Initialize(camera_.get());
    }
    if (isActiveTerrain_) {
        terrain_ = std::make_unique <ObjClass>();
        terrain_->Initialize(camera_.get(), "sample/terrain.obj");
    }
    if (isActivePlaneObj_) {
        planeObj_ = std::make_unique<ObjClass>();
        planeObj_->Initialize(camera_.get(), "sample/plane.obj");
    }
    if (isActivePlaneGltf_) {
        planeGltf_ = std::make_unique<ObjClass>();
        planeGltf_->Initialize(camera_.get(), "sample/plane.gltf");
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

}

// 更新
void CGScene::Update() {


#ifdef USE_IMGUI

    ImGui::Begin("CGScene");

    if (ImGui::BeginTabBar("CGSceneTabs")) {

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
    ImGui::Checkbox("Sphere", &isActiveSphere_);
    ImGui::Checkbox("Terrain", &isActiveTerrain_);
    ImGui::Checkbox("planeObj", &isActivePlaneObj_);
    ImGui::Checkbox("planeGltf", &isActivePlaneGltf_);
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

#endif // USE_IMGUI

    // --- カメラの更新 ---
    // 現在アクティブなカメラへのポインタ
    Camera* currentCamera = debugMode_ ? const_cast<Camera*>(&debugCamera_->GetCamera()) : camera_.get();
    currentCamera->Update("Camera"); // デバッグカメラも通常カメラもUpdateを呼ぶ

    // =====
    // ↓ゲームの更新
    // =====

    // 3D


    if (isActiveSphere_) {
        if (!sphere_) {
            sphere_ = std::make_unique<SphereClass>();
            sphere_->Initialize(camera_.get());
        }
        sphere_->Debug("Sphere");
        sphere_->Update();
    }
    if (isActiveTerrain_) {
        if (!terrain_) {
            terrain_ = std::make_unique<ObjClass>();
            terrain_->Initialize(camera_.get(), "sample/terrain.obj");
        }
        terrain_->Debug("Terrain");
        terrain_->Update();
    }
    if (isActivePlaneObj_) {
        if (!planeObj_) {
            planeObj_ = std::make_unique<ObjClass>();
            planeObj_->Initialize(camera_.get(), "sample/plane.obj");
        }
        planeObj_->Debug("PlaneObj");
        planeObj_->Update();
    }
    if (isActivePlaneGltf_) {
        if (!planeGltf_) {
            planeGltf_ = std::make_unique<ObjClass>();
            planeGltf_->Initialize(camera_.get(), "sample/plane.gltf");
        }
        planeGltf_->Debug("PlaneGltf");
        planeGltf_->Update();
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

    // =====
    // ↑ゲームの更新
    // =====

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

void CGScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->SetCull(PSOManager::CullMode::Back);

    engine_->ApplyPSO();

    if (isActiveSphere_) {
        sphere_->Draw();
    }
    if (isActiveTerrain_) {
        terrain_->Draw();
    }
    if (isActivePlaneObj_) {
        planeObj_->Draw();
    }
    if (isActivePlaneGltf_) {
        planeGltf_->Draw();
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

    engine_->SetBlend(BlendMode::kBlendModeNormal);

    if (isActiveAnimationSkin_) {
        animationSkin_->Draw();
    }

}