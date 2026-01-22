#pragma once

#include <d3d12.h>
#include <memory>
#include <cstdint>
#include <vector>
#include <random>
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
class DescriptorPool;

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

    D3D12ResourceUtilLine* GetD3D12Resource() { return this->resource_.get(); }

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

    D3D12ResourceUtilLine* GetD3D12Resource() { return this->resource_.get(); }

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

// 3Dラインのインスタンス描画用クラス
class Line3DRegion {
public:
    Line3DRegion() = default;
    ~Line3DRegion() = default;

    void Initialize(Camera* camera);
    void Update();
    void AddInstance(const Vector3& start, const Vector3& end, const Vector4& color, float life = 1.0f);
    void ClearInstances();
    void BuildInstanceBuffer(bool force = false);
    void Draw();

    // --- DrawManager から参照する Getter 群 ---
    D3D12ResourceUtilLine* GetBaseResource() const { return baseLineResource_.get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvGPU_; }
    UINT GetInstanceCountU32() const { return static_cast<UINT>(activeCount_); }

    // 依存注入
    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetSrvAllocator(DescriptorPool* alloc) { s_srvAllocator_ = alloc; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }

private:
    struct LineInstance {
        Vector3 start;
        Vector3 end;
        Vector4 color;
        float life = 1.0f;
        float age = 0.0f;
        bool active = false;
    };

    // VS 側が読む1インスタンス分のデータ
    struct InstanceData {
        Matrix4x4 WVP;
        Vector4 color;
    };

    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);
    void EnsureInstancingSRV();

private:
    // 静的依存
    static DirectXCommon* dx_;
    static DrawManager* drawManager_;
    static DescriptorPool* s_srvAllocator_;

    Camera* camera_ = nullptr;
    std::unique_ptr<D3D12ResourceUtilLine> baseLineResource_ = nullptr;

    std::vector<LineInstance> instances_;
    size_t activeCount_ = 0;
    size_t maxInstances_ = 8192;

    // インスタンシング用 StructuredBuffer と SRV
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    InstanceData* instanceData_ = nullptr;
    uint32_t instanceCapacity_ = 0;

    uint32_t instancingSrvIndex_ = UINT32_MAX;
    D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvGPU_{};
};

