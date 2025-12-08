#include "Camera.h"

/*開発のUIを出そう*/

#include "manager/DebugUI.h"

#include <cmath>
#include <random>
#include <string>

#include "function/Math.h"

// コンストラクタ
Camera::Camera() {}

// デストラクタ
Camera::~Camera() {}

// 初期化
void Camera::Initialize(int window_width, int window_height) {
  width_ = static_cast<float>(window_width);
  height_ = static_cast<float>(window_height);

  // ウィンドウサイズに基づいてアスペクト比と正射影境界を更新
  aspectRatio_ = (height_ != 0.0f) ? (width_ / height_) : 1.0f;
  right_ = width_;
  bottom_ = height_;

  UpdateMatrix();
}

// 更新
void Camera::Update([[maybe_unused]] const char *cameraName, Vector3 pPos,
                    Vector3 ePos) {

#if defined USE_IMGUI
  std::string name = std::string("Camera: ") + cameraName;

  // ImGui（デバッグ時のみ）
  ImGui::Begin(name.c_str());
  ImGui::DragFloat3("translate", &translate_.x, 0.1f);
  ImGui::DragFloat3("rotate", &rotate_.x, 0.1f);
  ImGui::End();
#endif

  // プレイヤーと敵の位置によってカメラを移動、FOV変更
  Vector3 centerPos = {
      (pPos.x + ePos.x) * 0.5f,
      (pPos.y + ePos.y) * 0.5f,
      (pPos.z + ePos.z) * 0.5f,
  };

  // XZ 平面上での距離
  float dx = pPos.x - ePos.x;
  float dz = pPos.z - ePos.z;
  float distance = std::sqrt(dx * dx + dz * dz);

  // 距離をクランプする
  float minDist = 5.0f;
  float maxDist = 40.0f;

  if (distance < minDist) {
    distance = minDist;
  }

  if (distance > maxDist) {
    distance = maxDist;
  }

  // 正規化した距離
  float t = (distance - minDist) / (maxDist - minDist);

  // カメラ引く量を決める
  float minBack = 25.0f; // 近いときのカメラの後ろ距離
  float maxBack = 30.0f; // 遠いときのカメラの後ろ距離
  float back = minBack + (maxBack - minBack) * t;

  // カメラが見る位置（少し上から）
  Vector3 targetPos{
      centerPos.x,
      centerPos.y + 15.0f, // 見下ろす高さ
      centerPos.z - back,  // 手前に引く
  };

  auto Lerp = [](float a, float b, float s) { return a + (b - a) * s; };

  const float posLerp = 0.1f; // 追従の滑らかさ（0〜1）

  translate_.x = Lerp(translate_.x, targetPos.x, posLerp);
  translate_.y = Lerp(translate_.y, targetPos.y, posLerp);
  translate_.z = Lerp(translate_.z, targetPos.z, posLerp);

  // 距離によってFOVを変更
  float minFov = 35.0f * std::numbers::pi_v<float> / 180.0f; // 寄り
  float maxFov = 60.0f * std::numbers::pi_v<float> / 180.0f; // 引き
  float targetFov = minFov + (maxFov - minFov) * t;
  const float fovLerp = 0.1f;
  fovAngleY_ = Lerp(fovAngleY_, targetFov, fovLerp);

  // ==== カメラシェイク ====
  if (shakeFrame_ > 0 && shakeFrameMax_ > 0) {

    // 残り時間に応じて徐々に弱くする
    float progress =
        static_cast<float>(shakeFrame_) / static_cast<float>(shakeFrameMax_);
    float amp = shakeAmplitude_ * progress;

    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    // X/Y 方向にランダムオフセットを加える
    translate_.x += dist(rng) * amp;
    translate_.y += dist(rng) * amp;

    --shakeFrame_;
  }

  // 毎フレーム行列を更新する
  UpdateMatrix();
}

// ワールド行列の作成
void Camera::MakeWorldMatrix() {

  worldMatrix_ = Math::MakeAffineMatrix(scale_, rotate_, translate_);
}

// ビュー行列の作成
void Camera::MakeViewMatrix() { viewMatrix_ = Math::Inverse(worldMatrix_); }

// 透視投影行列の更新
void Camera::UpdatePerspectiveFovMatrix() {

  perspectiveFovMatrix_ =
      Math::MakePerspectiveFovMatrix(fovAngleY_, aspectRatio_, nearZ_, farZ_);
}

// 正射行列の更新
void Camera::UpdateOrthographicMatrix() {

  orthographicMatrix_ = Math::MakeOrthographicMatrix(
      left_, top_, right_, bottom_, nearClip_, farClip_);
}

// ビューポート行列の更新
void Camera::UpdateViewportMatrix() {

  viewportMatrix_ = Math::MakeViewportMatrix(leftTop_.x, leftTop_.y, width_,
                                             height_, minDepth_, maxDepth_);
}

// 各行列の更新
void Camera::UpdateMatrix() {
  MakeWorldMatrix();
  MakeViewMatrix();
  UpdatePerspectiveFovMatrix();
  UpdateOrthographicMatrix();
  UpdateViewportMatrix();
}

void Camera::StartShake(int durationFrame, float amplitude) {
  shakeFrameMax_ = durationFrame;
  shakeFrame_ = durationFrame;
  shakeAmplitude_ = amplitude;
}

// カメラ行列を取得する
Matrix4x4 Camera::GetCameraMatrix() {
  return Math::MakeAffineMatrix(scale_, rotate_, translate_);
}