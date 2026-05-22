#include "EnemyPartHPBar.h"

#include "Irufemi.h"
#include "Engine/Graphics/Camera/Camera.h"
#include <algorithm>
#include <cmath>

namespace {
// --- 3D空間でのレイアウト定数 ---
constexpr float kPartBarMaxWidth = 8.0f;   ///< 部位用バーの最大幅（大幅に拡大）
constexpr float kPartBarHeight = 1.0f;     ///< 部位用バーの高さ
constexpr float kPartBarPullIn = 4.0f;     ///< モデルへの埋まりを完全に回避するため、カメラ方向に引き寄せる距離

constexpr float kFramePadding = 0.15f; ///< 枠線の余白

// --- アニメーション定数 ---
constexpr float kSmoothSpeed = 2.5f; ///< HP減少アニメーション速度

// --- HP色定義（線形補間用） ---
constexpr float kColorGreenR = 0.15f, kColorGreenG = 0.85f, kColorGreenB = 0.25f;
constexpr float kColorYellowR = 0.95f, kColorYellowG = 0.85f, kColorYellowB = 0.15f;
constexpr float kColorRedR = 0.90f, kColorRedG = 0.15f, kColorRedB = 0.15f;

// --- 背景・枠色 ---
constexpr float kBgR = 0.08f, kBgG = 0.08f, kBgB = 0.08f, kBgA = 0.75f;
constexpr float kFrameR = 0.55f, kFrameG = 0.55f, kFrameB = 0.55f, kFrameA = 0.90f;
} // namespace

void EnemyPartHPBar::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
    barMaxWidth_ = kPartBarMaxWidth;
    barHeight_ = kPartBarHeight;

    // --- 枠線スプライト（一番奥） ---
    barFrame_ = std::make_unique<PlaneClass>();
    barFrame_->Initialize("resources/whiteTexture.png");
    barFrame_->SetScale({barMaxWidth_ + kFramePadding * 2.0f, barHeight_ + kFramePadding * 2.0f, 1.0f});
    barFrame_->SetColor(Vector4{kFrameR, kFrameG, kFrameB, kFrameA});
    barFrame_->SetCullingEnabled(false);
    barFrame_->SetCastShadows(false);
    if (engine) barFrame_->SetCustomPSO(engine->GetPSOManager()->GetPSO("Object3D", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
    if (auto* mat = barFrame_->GetD3D12Resource()->GetMaterialData()) {
        mat->enableLighting = 0;
        mat->lightingMode = 0;
    }

    // --- 背景スプライト（真ん中） ---
    barBg_ = std::make_unique<PlaneClass>();
    barBg_->Initialize("resources/whiteTexture.png");
    barBg_->SetScale({barMaxWidth_, barHeight_, 1.0f});
    barBg_->SetColor(Vector4{kBgR, kBgG, kBgB, kBgA});
    barBg_->SetCullingEnabled(false);
    barBg_->SetCastShadows(false);
    if (engine) barBg_->SetCustomPSO(engine->GetPSOManager()->GetPSO("Object3D", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
    if (auto* mat = barBg_->GetD3D12Resource()->GetMaterialData()) {
        mat->enableLighting = 0;
        mat->lightingMode = 0;
    }

    // --- HP充填スプライト（一番手前） ---
    barFill_ = std::make_unique<PlaneClass>();
    barFill_->Initialize("resources/whiteTexture.png");
    barFill_->SetScale({barMaxWidth_, barHeight_, 1.0f});
    barFill_->SetColor(Vector4{kColorGreenR, kColorGreenG, kColorGreenB, 1.0f});
    barFill_->SetCullingEnabled(false);
    barFill_->SetCastShadows(false);
    if (engine) barFill_->SetCustomPSO(engine->GetPSOManager()->GetPSO("Object3D", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
    if (auto* mat = barFill_->GetD3D12Resource()->GetMaterialData()) {
        mat->enableLighting = 0;
        mat->lightingMode = 0;
    }

    displayRatio_ = 1.0f;
}

void EnemyPartHPBar::Update(float hpRatio, const Vector3& targetWorldPos, float pullRadius) {
    Camera* camera = engine_ ? engine_->GetCameraManager()->GetActiveCamera() : nullptr;
    if (!camera) return;

    // 表示用の割合をスムーズに近づける
    float dt = 1.0f / 60.0f; // 固定フレーム想定
    if (displayRatio_ > hpRatio) {
        displayRatio_ -= kSmoothSpeed * dt;
        if (displayRatio_ < hpRatio)
            displayRatio_ = hpRatio;
    } else {
        displayRatio_ = hpRatio;
    }
    displayRatio_ = (std::max)(0.0f, (std::min)(1.0f, displayRatio_));

    // バー幅の更新
    float fillWidth = barMaxWidth_ * displayRatio_;
    if (fillWidth < 0.001f && displayRatio_ > 0.0f)
        fillWidth = 0.001f; // 最低表示幅

    // 基準位置（部位の上、GameScene側ですでに高さオフセットが計算されて渡される想定）
    Vector3 basePos = targetWorldPos;

    // 敵モデル自身への埋まりを防ぐため、カメラ方向に一定距離引き寄せる
    float actualPullIn = (pullRadius > 0.0f) ? pullRadius * 1.1f : kPartBarPullIn;
    Vector3 toCam = { camera->GetTranslate().x - basePos.x, camera->GetTranslate().y - basePos.y, camera->GetTranslate().z - basePos.z };
    float distToCam = std::sqrt(toCam.x * toCam.x + toCam.y * toCam.y + toCam.z * toCam.z);
    if (distToCam > 0.001f) {
        Vector3 toCamNorm = { toCam.x / distToCam, toCam.y / distToCam, toCam.z / distToCam };
        float pullDist = (std::min)(actualPullIn, distToCam * 0.8f); // カメラを通り越さないように制限
        basePos.x += toCamNorm.x * pullDist;
        basePos.y += toCamNorm.y * pullDist;
        basePos.z += toCamNorm.z * pullDist;
    }

    // 常にカメラの方を向かせるための行列作成
    Matrix4x4 billboardMat = Math::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, camera->GetRotate(), basePos);

    // --- 枠線の配置 (一番奥 Z + 0.02) ---
    barFrame_->SetScale({barMaxWidth_ + kFramePadding * 2.0f, barHeight_ + kFramePadding * 2.0f, 1.0f});
    Vector3 frameLocal = {0.0f, 0.0f, 0.02f};
    barFrame_->SetTranslate(Math::Transform(frameLocal, billboardMat));
    barFrame_->SetRotate(camera->GetRotate());
    barFrame_->Update();

    // --- 背景の配置 (中 Z + 0.01) ---
    barBg_->SetScale({barMaxWidth_, barHeight_, 1.0f});
    Vector3 bgLocal = {0.0f, 0.0f, 0.01f};
    barBg_->SetTranslate(Math::Transform(bgLocal, billboardMat));
    barBg_->SetRotate(camera->GetRotate());
    barBg_->Update();

    // --- 充填の配置 (一番手前 Z + 0.00) ---
    barFill_->SetScale({fillWidth, barHeight_, 1.0f});
    float offsetX = -(barMaxWidth_ - fillWidth) * 0.5f; // 幅が変わるので、中心を左に寄せる
    Vector3 fillLocal = {offsetX, 0.0f, 0.0f};
    barFill_->SetTranslate(Math::Transform(fillLocal, billboardMat));
    barFill_->SetRotate(camera->GetRotate());
    barFill_->Update();

    // 色の更新
    UpdateBarColor(displayRatio_);
}

void EnemyPartHPBar::Draw(bool isUI) {
    // 描画順: 枠 → 背景 → 充填
    if (barFrame_)
        barFrame_->Draw(isUI);
    if (barBg_)
        barBg_->Draw(isUI);
    if (barFill_)
        barFill_->Draw(isUI);
}

void EnemyPartHPBar::UpdateBarColor(float hpRatio) {
    float r, g, b;

    if (hpRatio > 0.5f) {
        float t = (hpRatio - 0.5f) * 2.0f; // 1.0(緑) → 0.0(黄)
        r = kColorYellowR + (kColorGreenR - kColorYellowR) * t;
        g = kColorYellowG + (kColorGreenG - kColorYellowG) * t;
        b = kColorYellowB + (kColorGreenB - kColorYellowB) * t;
    } else {
        float t = hpRatio * 2.0f; // 0.5(黄) → 0.0(赤)
        r = kColorRedR + (kColorYellowR - kColorRedR) * t;
        g = kColorRedG + (kColorYellowG - kColorRedG) * t;
        b = kColorRedB + (kColorYellowB - kColorRedB) * t;
    }

    barFill_->SetColor(Vector4{r, g, b, 1.0f});
}
