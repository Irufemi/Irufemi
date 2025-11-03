#include "Camera.h"

/*開発のUIを出そう*/

#include "../externals/imgui/imgui.h"

#include <cmath>
#include <string>

#include "../function/Math.h"

//コンストラクタ
Camera::Camera() {}

//デストラクタ
Camera::~Camera() {}

//初期化
void Camera::Initialize(int window_width,int window_height) {
    width_ = static_cast<float>(window_width);
    height_ = static_cast<float>(window_height);

    // ウィンドウサイズに基づいてアスペクト比と正射影境界を更新
    aspectRatio_ = (height_ != 0.0f) ? (width_ / height_) : 1.0f;
    right_ = width_;
    bottom_ = height_;

    MakeWorldMatrix();
    MakeViewMatrix();
    UpdatePerspectiveFovMatrix();
    UpdateOrthographicMatrix();
    UpdateViewportMatrix();
}

//更新
void Camera::Update(const char *cameraName) {

#if defined(_DEBUG) || defined(DEVELOPMENT)
    std::string name = std::string("Camera: ") + cameraName;

    // ImGui（デバッグ時のみ）
    ImGui::Begin(name.c_str());
    ImGui::DragFloat3("translate", &translate_.x, 0.1f);
    ImGui::DragFloat3("rotate", &rotate_.x, 0.1f);
    ImGui::End();
#endif

    // Debug/Release を問わず毎フレーム行列を更新する
    UpdateMatrix();
}

//ワールド行列の作成
void Camera::MakeWorldMatrix() {

    worldMatrix_ = Math::MakeAffineMatrix(scale_, rotate_, translate_);

}

//ビュー行列の作成
void Camera::MakeViewMatrix() {

    viewMatrix_ =Math::Inverse(worldMatrix_);

}

//透視投影行列の更新
void Camera::UpdatePerspectiveFovMatrix() {

    perspectiveFovMatrix_ = Math::MakePerspectiveFovMatrix(fovAngleY_, aspectRatio_, nearZ_, farZ_);

}

//正射行列の更新
void Camera::UpdateOrthographicMatrix() {

    orthographicMatrix_ = Math::MakeOrthographicMatrix(left_, top_, right_, bottom_, nearClip_, farClip_);

}

//ビューポート行列の更新
void Camera::UpdateViewportMatrix() {
  
    viewportMatrix_ = Math::MakeViewportMatrix(leftTop_.x, leftTop_.y, width_, height_, minDepth_, maxDepth_);

}

//各行列の更新
void Camera::UpdateMatrix() {
    MakeWorldMatrix();
    MakeViewMatrix();
    UpdatePerspectiveFovMatrix();
    UpdateOrthographicMatrix();
    UpdateViewportMatrix();
}

// カメラ行列を取得する
Matrix4x4 Camera::GetCameraMatrix() { return Math::MakeAffineMatrix(scale_, rotate_, translate_); }