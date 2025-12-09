// Application/stage/skyDome/SkyDome.h
#pragma once
#include <memory>
#include <string>

class Camera;
class SphereClass;
struct Vector3;

/// DrawManager::DrawSphere(SphereClass*) を使う天球
class SkyDome {
public:
    SkyDome() = default;
    ~SkyDome() = default;

    // camera … いつも通りのカメラ
    // radius … 天球半径（ステージより十分大きく）
    // textureName … 貼るテクスチャ（あとで砂嵐用に変えてOK）
    void Initialize(Camera* camera,
        float radius,
        const std::string& textureName = "resources/uvChecker.png");

    // dt … 1フレームの秒数（なければ 1.0f/60.0f 固定でOK）
    void Update(float deltaTime);

    // 描画（内部で DrawManager::DrawSphere を呼ぶ）
    void Draw();

    // 任意：ステージ中心を固定したいとき用
    void SetCenter(const Vector3& center);

    // カメラ追従を ON/OFF
    void SetFollowCamera(bool enable) { followCamera_ = enable; }

private:
    std::unique_ptr<SphereClass> sphere_;
    Camera* camera_ = nullptr;

    float radius_ = 50.0f;
    bool followCamera_ = true; // デフォはカメラ追従
    float time_ = 0.0f;        // 砂嵐アニメ用に保持


};
