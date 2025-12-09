// Application/stage/skyDome/SkyDome.cpp
#include "SkyDome.h"

#include "3D/SphereClass.h"
#include "Application/camera/Camera.h"
#include "math/Vector3.h"          // Vector3 のパスはプロジェクトに合わせて
#include"engine/directX/directxcommon.h" // DirectXCommon のパスはプロジェクトに合わせて

void SkyDome::Initialize(Camera* camera,
    float radius,
    const std::string& textureName)
{
    camera_ = camera;
    radius_ = radius;

    sphere_ = std::make_unique<SphereClass>();

    // とりあえず原点＆指定半径で初期化
    sphere_->Initialize(camera_, textureName);
    sphere_->SetCenter(Vector3{ 0.0f, 0.0f, 0.0f });
    sphere_->SetRadius(radius_);
    sphere_->SetColor(Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }); // 必要なら色調整
}

void SkyDome::SetCenter(const Vector3& center)
{
    if (!sphere_) return;
    sphere_->SetCenter(center);
}

void SkyDome::Update(float deltaTime)
{
    if (!sphere_) return;

    time_ += deltaTime;

    // カメラ追従（Camera::GetTranslate を使う：GetPosition ではない）
    if (followCamera_ && camera_) {
        Vector3 camPos = camera_->GetTranslate();
        camPos.y = 0.0f; // 地面高さに固定したいなら
        sphere_->SetCenter(camPos);
    }

    // 半径は固定だが、何かアニメさせたい場合はここで SetRadius/SetRotate など

    sphere_->Update();
}

void SkyDome::Draw()
{
    if (!sphere_) return;
    sphere_->Draw(); // 中で DrawManager::DrawSphere が呼ばれる
}
