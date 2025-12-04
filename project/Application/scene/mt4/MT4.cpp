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

// ある方向からある方向への回転
Matrix4x4 MT4::DirectionToDirection(const Vector3& from, const Vector3& to) {
    // 正規化
    Vector3 f = Math::Normalize(from);
    Vector3 t = Math::Normalize(to);

    // 内積から角度のコサインを求める
    float cosTheta = Math::Dot(f, t);

    // 数値誤差対策の閾値
    constexpr float kEpsilon = 1e-6f;

    // ほぼ同じ方向 -> 単位行列（回転不要）
    if (cosTheta > 1.0f - kEpsilon) {
        return Math::MakeIdentity4x4();
    }

    // ほぼ逆方向 -> 軸が定義できない（u x v = 0）になるので、
    // 添付画像の選び方に従って直交ベクトルを選ぶ
    if (cosTheta < -1.0f + kEpsilon) {
        Vector3 axis{ 0.0f, 0.0f, 0.0f };

        // 画像の式:
        // n = [ uy, -ux, 0 ]  (if ux != 0 || uy != 0)
        // n = [ uz, 0, -ux ]  (if ux != 0 || uz != 0)
        // 優先は前者（xy 平面での成分があれば使う）
        if (std::fabs(f.x) > kEpsilon || std::fabs(f.y) > kEpsilon) {
            axis = Vector3{ f.y, -f.x, 0.0f };
        } else if (std::fabs(f.x) > kEpsilon || std::fabs(f.z) > kEpsilon) {
            axis = Vector3{ f.z, 0.0f, -f.x };
        } else {
            // ほとんどゼロベクトルなら任意の軸を選択
            axis = Vector3{ 1.0f, 0.0f, 0.0f };
        }

        // 正規化（安全）
        float axisLen = Math::Length(axis);
        if (axisLen < kEpsilon) {
            axis = Vector3{ 1.0f, 0.0f, 0.0f };
        } else {
            axis = Math::Multiply(1.0f / axisLen, axis);
        }

        constexpr float kPi = 3.14159265358979323846f;
        return MakeRotateAxisAngle(axis, kPi);
    }

    // 通常ケース：軸は cross、角度は acos(dot)
    Vector3 axis = Math::Cross(f, t);
    float axisLen = Math::Length(axis);
    if (axisLen < kEpsilon) {
        // 数値誤差などで axis が小さい場合は単位行列を返す
        return Math::MakeIdentity4x4();
    }
    axis = Math::Multiply(1.0f / axisLen, axis);

    // acos の引数は [-1, 1] にクランプしておく
    float cosClamped = cosTheta;
    if (cosClamped < -1.0f) cosClamped = -1.0f;
    if (cosClamped > 1.0f) cosClamped = 1.0f;
    float angle = std::acos(cosClamped);

    return MakeRotateAxisAngle(axis, angle);
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

#ifdef MT4_01_01

    axis = Math::Normalize(Vector3{ 1.0f,1.0f,1.0f });
    angle = 0.44f;
    rotateMatrix = MakeRotateAxisAngle(axis, angle);

#endif

#ifdef MT4_01_02

    from0 = Math::Normalize(Vector3{ 1.0f,0.7f,0.5f });
    to0 = -from0;
    from1 = Math::Normalize(Vector3{ -0.6f,0.9f,0.2f });
    to1 = Math::Normalize(Vector3{ 0.4f,0.7f,-0.5f });
    rotateMatrix0 = DirectionToDirection(
        Math::Normalize(Vector3{ 1.0f,0.0f,0.0f }), Math::Normalize(Vector3{ -1.0f,0.0f,0.0f })
    );
    rotateMatrix1 = DirectionToDirection(from0, to0);
    rotateMatrix2 = DirectionToDirection(from1, to1);

#endif // MT4_01_02

    
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

#ifdef MT4_01_01

    ImGui::Begin("MT4 01_01");

    ImGuiMatrixScreenPrint(rotateMatrix);

    ImGui::End();

#endif

#ifdef MT4_01_02

    ImGui::Begin("MT4 01_02");

    ImGui::Text("rotateMatrix0");
    ImGuiMatrixScreenPrint(rotateMatrix0);
    ImGui::Text("rotateMatrix1");
    ImGuiMatrixScreenPrint(rotateMatrix1);
    ImGui::Text("rotateMatrix2");
    ImGuiMatrixScreenPrint(rotateMatrix2);

    ImGui::End();
    
#endif // MT4_01_02


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