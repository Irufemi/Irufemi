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

static void ImGuiVector3ScreenPrint([[maybe_unused]] const Vector3& v) {

#ifdef USE_IMGUI

    ImGui::Text("%f %f %f ", v.x, v.y, v.z);

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

// 任意軸回転を表す Quaternionの生成
Quaternion MT4::MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
    constexpr float kEpsilon = 1e-6f;

    // 軸を正規化（ゼロ長なら X 軸を代替）
    float axisLen = Math::Length(axis);
    Vector3 n = axis;
    if (axisLen < kEpsilon) {
        n = Vector3{ 1.0f, 0.0f, 0.0f };
    } else {
        n = Math::Multiply(1.0f / axisLen, axis);
    }

    // 四元数 (x,y,z,w) : (n * sin(theta/2), cos(theta/2))
    float half = 0.5f * angle;
    float s = std::sin(half);
    Quaternion q = { n.x * s, n.y * s, n.z * s, std::cos(half) };

    // 数値安定性のため正規化して返す
    return Normalize(q);
}

// ベクトルをQuaternionで回転させた結果のベクトルを求める
Vector3 MT4::RotateVector(const Vector3& vector, const Quaternion& quaternion) {
    // 回転四元数は単位四元数であるべきなので正規化して使用
    Quaternion q = Normalize(quaternion);

    // v を (v, 0) として q * v * q^*
    Quaternion p = { vector.x, vector.y, vector.z, 0.0f };
    Quaternion tmp = Multiply(q, p);
    Quaternion qConj = Conjugate(q);
    Quaternion res = Multiply(tmp, qConj);

    return Vector3{ res.x, res.y, res.z };
}

// Quaternionから回転行列を求める
Matrix4x4 MT4::MakeRotateMatrix(const Quaternion& quaternion) {

    Matrix4x4 r = Math::MakeIdentity4x4();

    r.m[0][0] = quaternion.w * quaternion.w + quaternion.x * quaternion.x - quaternion.y * quaternion.y - quaternion.z * quaternion.z;
    r.m[0][1] = 2.0f * (quaternion.x * quaternion.y + quaternion.w * quaternion.z);
    r.m[0][2] = 2.0f * (quaternion.x * quaternion.z - quaternion.w * quaternion.y);
    r.m[1][0] = 2.0f * (quaternion.x * quaternion.y - quaternion.w * quaternion.z);
    r.m[1][1] = quaternion.w * quaternion.w - quaternion.x * quaternion.x + quaternion.y * quaternion.y - quaternion.z * quaternion.z;
    r.m[1][2] = 2.0f * (quaternion.y * quaternion.z + quaternion.w * quaternion.x);
    r.m[2][0] = 2.0f * (quaternion.x * quaternion.z + quaternion.w * quaternion.y);
    r.m[2][1] = 2.0f * (quaternion.y * quaternion.z - quaternion.w * quaternion.x);
    r.m[2][2] = quaternion.w * quaternion.w - quaternion.x * quaternion.x - quaternion.y * quaternion.y + quaternion.z * quaternion.z;

    return r;
}

// 球面線形補間
Quaternion MT4::Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
    // q0, q1 は単位四元数であることが前提（数値誤差は別途管理）
    Quaternion q0_ = q0;

    // q0 と q1 の内積（4次元ベクトルとして）
    float dot = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;

    // 最短経路を取るために内積が負の場合は片方の符号を反転する
    if (dot < 0.0f) {
        q0_.x = -q0.x;
        q0_.y = -q0.y;
        q0_.z = -q0.z;
        q0_.w = -q0.w;
        dot = -dot;
    }

    // dot がほぼ 1 の場合は線形補間（数値安定化）
    constexpr float kDotThreshold = 0.9995f;
    if (dot > kDotThreshold) {
        // NLERP（線形補間）: cost を抑えるため正規化は呼び出し側で必要に応じて行う
        Quaternion result{};
        result.x = q0_.x + t * (q1.x - q0_.x);
        result.y = q0_.y + t * (q1.y - q0_.y);
        result.z = q0_.z + t * (q1.z - q0_.z);
        result.w = q0_.w + t * (q1.w - q0_.w);
        return result;
    }

    // acos の引数を安全にクランプ
    if (dot < -1.0f) dot = -1.0f;
    if (dot >  1.0f) dot =  1.0f;

    // なす角 theta を求め，sin を使ってスケール係数を計算
    float theta = std::acos(dot);
    float sinTheta = std::sin(theta);

    // 正規ケース：scale0, scale1 を計算して補間
    float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
    float scale1 = std::sin(t * theta) / sinTheta;

    Quaternion result{};
    result.x = scale0 * q0_.x + scale1 * q1.x;
    result.y = scale0 * q0_.y + scale1 * q1.y;
    result.z = scale0 * q0_.z + scale1 * q1.z;
    result.w = scale0 * q0_.w + scale1 * q1.w;

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

#ifdef MT4_01_04

    rotation = MakeRotateAxisAngleQuaternion(
        Math::Normalize(Vector3{ 1.0f,0.4f,-0.2f }), 0.45f
    );
    pointY = { 2.1f,-0.9f,1.3f };
    rotateMatrix = MakeRotateMatrix(rotation);
    rotateByQuaternion = RotateVector(pointY, rotation);
    rotateByMatrix = Math::Transform(pointY, rotateMatrix);

#endif // MT4_01_04

#ifdef MT4_01_05

    rotation0 = MakeRotateAxisAngleQuaternion({ 0.71f,0.71f,0.0f }, 0.3f);
    rotation1 = MakeRotateAxisAngleQuaternion({ 0.71f,0.0f,0.71f }, 3.141592f);

    interpolate0 = Slerp(rotation0, rotation1, 0.0f);
    interpolate1 = Slerp(rotation0, rotation1, 0.3f);
    interpolate2 = Slerp(rotation0, rotation1, 0.5f);
    interpolate3 = Slerp(rotation0, rotation1, 0.7f);
    interpolate4 = Slerp(rotation0, rotation1, 1.0f);

#endif // MT4_01_05

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

#ifdef MT4_01_03

    ImGui::Begin("MT4 01_03");

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

#endif // MT4_01_03

#ifdef MT4_01_04

    ImGui::Begin("MT4 01_04");

    ImGui::Text("rotation");
    ImGuiQuaternionScreenPrint(rotation);
    ImGui::Text("rotateMatrix");
    ImGuiMatrixScreenPrint(rotateMatrix);
    ImGui::Text("rotateByQuaternion");
    ImGuiVector3ScreenPrint(rotateByQuaternion);
    ImGui::Text("rotateByMatrix");
    ImGuiVector3ScreenPrint(rotateByMatrix);

#endif // MT4_01_04

#ifdef MT4_01_05

    ImGui::Begin("MT4 01_05");

    ImGui::Text("interpolate0");
    ImGuiQuaternionScreenPrint(interpolate0);
    ImGui::Text("interpolate1");
    ImGuiQuaternionScreenPrint(interpolate1);
    ImGui::Text("interpolate2");
    ImGuiQuaternionScreenPrint(interpolate2);
    ImGui::Text("interpolate3");
    ImGuiQuaternionScreenPrint(interpolate3);
    ImGui::Text("interpolate4");
    ImGuiQuaternionScreenPrint(interpolate4);

#endif // MT4_01_05


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