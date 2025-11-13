#include "DebugScene.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include <imgui.h>


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
    debugMode = false;

    pointLight_ = std::make_unique <PointLightClass>();
    pointLight_->Initialize();
    pointLight_->SetPos(Vector3{ 0.0f,-5.0f,0.0f });
    engine_->GetDrawManager()->SetPointLightClass(pointLight_.get());

    spotLight_ = std::make_unique<SpotLightClass>();
    spotLight_->Initialize();
    spotLight_->SetIntensity(0.0f);
    engine_->GetDrawManager()->SetSpotLightClass(spotLight_.get());

    isActiveObj = false;
    isActiveSprite = false;
    isActiveTriangle = false;
    isActiveSphere = true;
    isActiveStanfordBunny = false;
    isActiveUtashTeapot = false;
    isActiveMultiMesh = false;
    isActiveMultiMaterial = false;
    isActiveSuzanne = false;
    isActiveFence_ = false;
    isActiveTerrain_ = true;
    isActiveParticle = false;

    if (isActiveObj) {
        obj = std::make_unique <ObjClass>();
        obj->Initialize(camera_.get(), "sample/plane.gltf");
    }
    if (isActiveSprite) {
        sprite = std::make_unique <Sprite>();
        sprite->Initialize(camera_.get());
    }
    if (isActiveTriangle) {
        triangle_ = std::make_unique <TriangleClass>();
        triangle_->Initialize(camera_.get());
    }
    if (isActiveSphere) {
        sphere = std::make_unique <SphereClass>();
        sphere->Initialize(camera_.get());
    }
    if (isActiveStanfordBunny) {
        stanfordBunny = std::make_unique <ObjClass>();
        stanfordBunny->Initialize(camera_.get(), "sample/bunny.obj");
    }
    if (isActiveUtashTeapot) {
        utashTeapot = std::make_unique <ObjClass>();
        utashTeapot->Initialize(camera_.get(), "sample/teapot.obj");
    }
    if (isActiveMultiMesh) {
        multiMesh = std::make_unique <ObjClass>();
        multiMesh->Initialize(camera_.get(), "sample/multiMesh.obj");
    }
    if (isActiveMultiMaterial) {
        multiMaterial = std::make_unique <ObjClass>();
        multiMaterial->Initialize(camera_.get(), "sample/multiMaterial.obj");
    }
    if (isActiveSuzanne) {
        suzanne = std::make_unique <ObjClass>();
        suzanne->Initialize(camera_.get(), "sample/suzanne.obj");
    }
    if (isActiveFence_) {
        fence_ = std::make_unique <ObjClass>();
        fence_->Initialize(camera_.get(), "sample/fence.obj");
    }
    if (isActiveTerrain_) {
        terrain_ = std::make_unique <ObjClass>();
        terrain_->Initialize(camera_.get(), "sample/terrain.obj");
    }
    if (isActiveParticle) {
        particle = std::make_unique <ParticleClass>();
        particle->Initialize(engine_->GetSrvDescriptorHeap(), camera_.get(), engine_->GetTextureManager(), engine_->GetDebugUI(), "circle.png");
    }
}

// 更新
void DebugScene::Update() {

    // カメラの通常更新
    if (debugMode) {
        debugCamera_->Update();
        camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
        camera_->SetPerspectiveFovMatrix(debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
        camera_->Update("Camera");
    }

    if (engine_->GetInputManager()->IsKeyPressed('P') || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {

        engine_->GetSceneManager()->Request("InGame");
    }


#if defined(_DEBUG) || defined(DEVELOPMENT)

    ImGui::Begin("GameScene");
    // pointLight 
    pointLight_->Debug();
    // spotLight 
    spotLight_->Debug();

    ImGui::End();

    ImGui::Begin("Texture");
    if (ImGui::Button("allLoadActivate")) {
        engine_->GetTextureManager()->LoadAllFromFolder("resources/");
    }
    ImGui::Checkbox("debugMode", &debugMode);
    ImGui::End();

    ImGui::Begin("Activation");
    ImGui::Checkbox("Obj", &isActiveObj);
    ImGui::Checkbox("Sprite", &isActiveSprite);
    ImGui::Checkbox("Sphere", &isActiveSphere);
    ImGui::Checkbox("Utash Teapot", &isActiveUtashTeapot);
    ImGui::Checkbox("Stanford Bunny", &isActiveStanfordBunny);
    ImGui::Checkbox("MultiMesh", &isActiveMultiMesh);
    ImGui::Checkbox("MultiMaterial", &isActiveMultiMaterial);
    ImGui::Checkbox("Suzanne", &isActiveSuzanne);
    ImGui::Checkbox("Fence", &isActiveFence_);
    ImGui::Checkbox("Terrain", &isActiveTerrain_);
    ImGui::Checkbox("Particle", &isActiveParticle);
    ImGui::End();

#endif // _DEBUG

    // 3D

    if (isActiveObj) {
        if (!obj) {
            obj = std::make_unique<ObjClass>();
            obj->Initialize(camera_.get(),"sample/plane.gltf");
        }
        obj->Update("Plane");
    }
    if (isActiveSphere) {
        if (!sphere) {
            sphere = std::make_unique<SphereClass>();
            sphere->Initialize(camera_.get());
        }
        sphere->Update();
    }
    if (isActiveUtashTeapot) {
        if (!utashTeapot) {
            utashTeapot = std::make_unique<ObjClass>();
            utashTeapot->Initialize(camera_.get(),"sample/teapot.obj");
        }
        utashTeapot->Update("Utash Teapot");
    }
    if (isActiveStanfordBunny) {
        if (!stanfordBunny) {
            stanfordBunny = std::make_unique<ObjClass>();
            stanfordBunny->Initialize(camera_.get(), "sample/bunny.obj");
        }
        stanfordBunny->Update("Stanford Bunny");
    }
    if (isActiveMultiMesh) {
        if (!multiMesh) {
            multiMesh = std::make_unique<ObjClass>();
            multiMesh->Initialize(camera_.get(), "sample/multiMesh.obj");
        }
        multiMesh->Update("MultiMesh");
    }
    if (isActiveMultiMaterial) {
        if (!multiMaterial) {
            multiMaterial = std::make_unique<ObjClass>();
            multiMaterial->Initialize(camera_.get(), "sample/multiMaterial.obj");
        }
        multiMaterial->Update("MultiMaterial");
    }
    if (isActiveSuzanne) {
        if (!suzanne) {
            suzanne = std::make_unique<ObjClass>();
            suzanne->Initialize(camera_.get(), "sample/suzanne.obj");
        }
        suzanne->Update("Suzanne");
    }
    if (isActiveFence_) {
        if (!fence_) {
            fence_ = std::make_unique<ObjClass>();
            fence_->Initialize(camera_.get(), "sample/fence.obj");
        }
        fence_->Update("Fence");
    }
    if (isActiveTerrain_) {
        if (!terrain_) {
            terrain_ = std::make_unique<ObjClass>();
            terrain_->Initialize(camera_.get(), "sample/terrain.obj");
        }
        terrain_->Update("Terrain");
    }
    if (isActiveParticle) {
        if (!particle) {
            particle = std::make_unique <ParticleClass>();
            particle->Initialize(engine_->GetSrvDescriptorHeap(), camera_.get(), engine_->GetTextureManager(), engine_->GetDebugUI());
        }
        particle->Update();
    }

    // 2D

    if (isActiveSprite) {
        if (!sprite) {
            sprite = std::make_unique<Sprite>();
            sprite->Initialize(camera_.get());
        }
        sprite->Update();
    }
}

void DebugScene::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyByGeometryShaderPSO();



    if (isActiveObj) {
        obj->Draw();
    }
    if (isActiveSphere) {
        sphere->Draw();
    }
    if (isActiveUtashTeapot) {
        utashTeapot->Draw();
        
    }
    if (isActiveStanfordBunny) {
        stanfordBunny->Draw();
    }
    if (isActiveMultiMesh) {
        multiMesh->Draw();
    }
    if (isActiveMultiMaterial) {
        multiMaterial->Draw();
    }
    if (isActiveSuzanne) {
        suzanne->Draw();
    }
    if (isActiveFence_) {
        fence_->Draw();
    }
    if (isActiveTerrain_) {
        terrain_->Draw();
    }

    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplyParticlePSO();

    if (isActiveParticle) {
        engine_->GetDrawManager()->DrawParticle(particle.get());
    }

    // 2D

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    if (isActiveSprite) {
        sprite->Draw();
    }
}