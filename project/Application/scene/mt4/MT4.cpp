#include "MT4.h"

#include "function/Math.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include <cmath>

static void ImGuiMatrixScreenPrint([[maybe_unused]]const Matrix4x4& m) {
    
#ifdef USE_IMGUI

    ImGui::Text(
        "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
        m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
        m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
        m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
        m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]
    );

#endif // USE_IMGUI
}

// 任意軸回転行列の作成関数
Matrix4x4 MT4::MakeRotateAxisAngle(const Vector3& axis, float angle) {
    Matrix4x4 result = Math::MakeIdentity4x4();
    result.m[0][0] = axis.x * axis.x * (1 - std::cos(angle)) + std::cos(angle);
    result.m[0][1] = axis.x * axis.y * (1 - std::cos(angle)) + axis.z * std::sin(angle);
    result.m[0][2] = axis.x * axis.z * (1 - std::cos(angle)) - axis.y * std::sin(angle);
    result.m[1][0] = axis.x * axis.y * (1 - std::cos(angle)) - axis.z * std::sin(angle);
    result.m[1][1] = axis.y * axis.y * (1 - std::cos(angle)) + std::cos(angle);
    result.m[1][2] = axis.y * axis.z * (1 - std::cos(angle)) + axis.x * std::sin(angle);
    result.m[2][0] = axis.x * axis.z * (1 - std::cos(angle)) + axis.y * std::sin(angle);
    result.m[2][1] = axis.y * axis.z * (1 - std::cos(angle)) - axis.x * std::sin(angle);
    result.m[2][2] = axis.z * axis.z * (1 - std::cos(angle)) + std::cos(angle);

    return result;

}


// 初期化
void MT4::Initialize(IrufemiEngine* engine) {

    engine_ = engine;

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f,0.0f,-10.0f });

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

    axis = Math::Normalize(Vector3{ 1.0f,1.0f,1.0f });
    angle = 0.44f;
    rotateMatrix = MakeRotateAxisAngle(axis, angle);
    
}

// 更新
void MT4::Update() {

    // カメラの通常更新
    if (debugMode) {
        debugCamera_->Update();
        camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
        camera_->SetPerspectiveFovMatrix(debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
        camera_->Update("Camera");
    }

    // MT4の課題内容(IMGUIのTextで表示するためDebugもしくはDevelopmentでのビルド必須)
#ifdef USE_IMGUI

#pragma region 01_01

    ImGui::Begin("MT4 01_01");

    ImGuiMatrixScreenPrint(rotateMatrix);

    ImGui::End();

#pragma endregion

#endif // USE_IMGUI

}

void MT4::Draw() {

    // 3D
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    // 2D

    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();
}