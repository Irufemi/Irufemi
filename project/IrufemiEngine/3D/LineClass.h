#pragma once

#include <d3d12.h>
#include <memory>
#include <cstdint>
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Transform.h"
#include "math/TransformationMatrix.h"
#include "source/D3D12ResourceUtil.h"

// 前方宣言
class Camera;
class DrawManager;
class DirectXCommon;

class Line2DClass
{
public: // メンバ関数
    // コンストラクタ
    Line2DClass();
    // デストラクタ
    ~Line2DClass();

    // 初期化
    void Initialize(Camera* camera, const Vector2& origin, const Vector2& end);

    // 更新
    void Update();

    // 描画
    void Draw();

    // セッター
    void SetOrigin(const Vector2& origin) { origin_ = origin; resource_->vertexData_[0].position = { origin_.x,origin_.y,0.0f,1.0f }; }

    void SetEnd(const Vector2& end) { end_ = end; resource_->vertexData_[1].position = { end_.x,end_.y,0.0f,1.0f }; }

    // ゲッター
    const Vector2& GetOrigin()const { return origin_; }
    const Vector2& GetEnd()const { return end_; }

    // 参照セッター
    static void SetDrawManager(DrawManager* drawManager) { drawManager_ = drawManager; }

private:

    // 始点
    Vector2 origin_{};

    // 終点
    Vector2 end_{};

    // 色
    Vector4 color_{};

    // リソース
    std::unique_ptr<D3D12ResourceUtilLine> resource_ = nullptr;

    // カメラ(ポインタ参照)
    Camera* camera_ = nullptr;

    // DrawManager(静的ポインタ参照)
    static DrawManager* drawManager_;
};

class Line3DClass
{
public: // メンバ関数
    // コンストラクタ
    Line3DClass();
    // デストラクタ
    ~Line3DClass();

    // 初期化
    void Initialize(Camera* camera, const Vector3& origin, const Vector3& end,const Vector4& color);

    // 更新
    void Update();

    // 描画
    void Draw();

    // セッター
    void SetOrigin(const Vector3& origin) { origin_ = origin; resource_->vertexData_[0].position = { origin_.x,origin_.y,origin_.z,1.0f }; }
    void SetEnd(const Vector3& end) { end_ = end; resource_->vertexData_[1].position = { end_.x,end_.y,end_.z,1.0f }; }

    // ゲッター
    const Vector3& GetOrigin()const { return origin_; }
    const Vector3& GetEnd()const { return end_; }

    // 参照セッター
    static void SetDrawManager(DrawManager* drawManager) { drawManager_ = drawManager; }

private:

    // 始点
    Vector3 origin_{};

    // 終点
    Vector3 end_{};

    // 色
    Vector4 color_{};

    // リソース
    std::unique_ptr<D3D12ResourceUtilLine> resource_ = nullptr;

    // カメラ(ポインタ参照)
    Camera* camera_ = nullptr;

    // DrawManager(静的ポインタ参照)
    static DrawManager* drawManager_;

private: // メンバ関数(リソース関連内部ヘルパ)
    // 一括Map
    void Map();

    // 一括UnMap
    void UnMap();

    // 一括CreateResource
    void CreateResource();

};

