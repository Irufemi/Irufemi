#include "MT4.h"

#include "function/Math.h"

#include "scene/SceneManager.h"
#include "engine/IrufemiEngine.h"
#include "manager/DebugUI.h"

#include <cmath>

static void ImGuiMatrixScreenPrint([[maybe_unused]] const Matrix4x4& m) {

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

static void ImGuiQuaternionScreenPrint([[maybe_unused]] const Quaternion& q) {

#ifdef USE_IMGUI

    ImGui::Text("%f %f %f %f", q.x, q.y, q.z, q.w);

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

// Quaternionの積
Quaternion MT4::Multiply(const Quaternion& lhs, const Quaternion& rhs) {

    Vector3 qlV = { lhs.x,lhs.y,lhs.z };
    float qlW = lhs.w;
    Vector3 qrV = { rhs.x,rhs.y,rhs.z };
    float qrW = rhs.w;

    float qW = qlW * qrW - Math::Dot(qlV, qrV);
    Vector3 qV = Math::Cross(qlV, qrV) + Math::Multiply(qrW, qlV) + Math::Multiply(qlW, qrV);

    Quaternion result = { qV.x,qV.y,qV.z,qW };

    return result;
}

// 単位Quaternionを返す
Quaternion MT4::IdentityQuaternion() {
    Quaternion result{};
    result.x = 0.0f;
    result.y = 0.0f;
    result.z = 0.0f;
    result.w = 1.0f;
    return result;
}

// 共役Quaternionを返す
Quaternion MT4::Conjugate(const Quaternion& quaternion) {
    Quaternion result{};
    // スカラー部はそのまま、虚部は符号を反転
    result.x = -quaternion.x;
    result.y = -quaternion.y;
    result.z = -quaternion.z;
    result.w = quaternion.w;
    return result;
}

// Quaternionのnormを返す
float MT4::Norm(const Quaternion& quaternion) {
    float norm = std::sqrt(
        quaternion.w * quaternion.w +
        quaternion.x * quaternion.x +
        quaternion.y * quaternion.y +
        quaternion.z * quaternion.z
    );
    return norm;
}

// 正規化したQuaternionを返す
Quaternion MT4::Normalize(const Quaternion& quaternion) {
    constexpr float kEpsilon = 1e-6f;
    float n = Norm(quaternion);
    if (n < kEpsilon) {
        // 長さがほぼ0の場合は元の値をそのまま返す（分母ゼロ回避）
        return quaternion;
    }
    Quaternion result{};
    result.x = quaternion.x / n;
    result.y = quaternion.y / n;
    result.z = quaternion.z / n;
    result.w = quaternion.w / n;
    return result;
}

// 逆Quaternionを返す
Quaternion MT4::Inverse(const Quaternion& quaternion) {
    // 逆元は共役をノルム二乗で割る: q^{-1} = q* / ||q||^2
    constexpr float kEpsilon = 1e-12f;
    // ノルム二乗を直接計算（sqrt を使わず効率的）
    float normSq =
        quaternion.w * quaternion.w +
        quaternion.x * quaternion.x +
        quaternion.y * quaternion.y +
        quaternion.z * quaternion.z;

    if (normSq < kEpsilon) {
        // ノルム二乗がほぼ0なら逆元は定義されないため単位四元数を返す
        return IdentityQuaternion();
    }

    Quaternion conj = Conjugate(quaternion);
    Quaternion result{};
    result.x = conj.x / normSq;
    result.y = conj.y / normSq;
    result.z = conj.z / normSq;
    result.w = conj.w / normSq;
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

#ifdef MT4_01_03

    q1 = { 2.0f,3.0f,4.0f,1.0f };

    q2 = { 1.0f,3.0f,5.0f,2.0f };

    identity = IdentityQuaternion();

    conj = Conjugate(q1);

    inv = Inverse(q1);

    normal = Normalize(q1);

    mul1 = Multiply(q1, q2);

    mul2 = Multiply(q2, q1);

    norm = Norm(q1);

#endif // MT4_01_03

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

    ImGui::Begin("MT4 01_02");

    ImGui::Text("Identity");
    ImGuiQuaternionScreenPrint(identity);
    ImGui::Text("Conjugate");
    ImGuiQuaternionScreenPrint(conj);
    ImGui::Text("Inverse");
    ImGuiQuaternionScreenPrint(inv);
    ImGui::Text("Normalize");
    ImGuiQuaternionScreenPrint(normal);
    ImGui::Text("Multiply(q1, q2)");
    ImGuiQuaternionScreenPrint(mul1);
    ImGui::Text("Multiply(q2, q1)");
    ImGuiQuaternionScreenPrint(mul2);
    ImGui::Text("Norm");
    ImGui::Text("%f", norm);


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