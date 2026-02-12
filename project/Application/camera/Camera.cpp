#include "Camera.h"

/*開発のUIを出そう*/



#include "manager/DebugUI.h"

#include <cmath>
#include <string>

#include "function/Math.h"

//コンストラクタ
Camera::Camera() {}

//デストラクタ
Camera::~Camera() {}

//初期化
void Camera::Initialize(const int& windowWidth,const int& windowHeight) {
    width_ = static_cast<float>(windowWidth);
    height_ = static_cast<float>(windowHeight);

    // ウィンドウサイズに基づいてアスペクト比と正射影境界を更新
    aspectRatio_ = (height_ != 0.0f) ? (width_ / height_) : 1.0f;
    right_ = width_;
    bottom_ = height_;

    UpdateMatrix();
}

//更新
void Camera::Update([[maybe_unused]]const char *cameraName) {

#if defined USE_IMGUI
    std::string name = std::string("Camera: ") + cameraName;

    // ImGui(デバッグ時のみ)
    ImGui::Begin(name.c_str());
    ImGui::DragFloat3("translate", &translate_.x, 0.1f);
    ImGui::DragFloat3("rotate", &rotate_.x, 0.1f);
    ImGui::End();
#endif

    // 毎フレーム行列を更新する
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
const Matrix4x4& Camera::GetCameraMatrix() { 
    worldMatrix_ = Math::MakeAffineMatrix(scale_, rotate_, translate_);
    return worldMatrix_;
}