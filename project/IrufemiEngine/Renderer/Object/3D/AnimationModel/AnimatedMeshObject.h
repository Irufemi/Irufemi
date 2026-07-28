#pragma once
#include "Renderer/System/Core/IRenderable.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Resource/Model/Data/SkeletonData.h"
#include "Resource/Model/Data/SkeletonPose.h"
#include "Resource/Model/Data/SkinCluster.h"
#include "Engine/Graphics/Compute/IComputeTask.h"
#include <string>
#include <cstdint>
#include <memory>
#include <vector>

class PrimitiveBatch;

/**
 * @class AnimatedMeshObject
 * @brief スキニングアニメーション付きのメッシュ描画に特化した純粋な低レイヤークラス。
 * GameObjectに依存せず、外部から与えられたポーズ（SkeletonPose）を元に描画・コンピュート処理を行います。
 */
class AnimatedMeshObject : public IComputeTask, public BaseModel {
public:
    AnimatedMeshObject();
    ~AnimatedMeshObject() override;

    void DispatchCompute() override;
    void Initialize(const std::string& filename);
    
    /// @brief 外部からポーズを渡して更新。nullptrの場合は内部のバインドポーズまたは前回状態を維持。
    void Update(const SkeletonPose* externalPose = nullptr);

    void SyncBeforeDraw() override;
    void Draw() override;
    void DrawOutlineMask() override;
    void Debug(const char* objName = " ");

    const SkeletonData* GetSkeletonData() const;
    SkeletonPose* GetInternalSkeletonPose(); // プログラマの手動制御用

private:
    void InitializeResources();

private:
    SkeletonData skeletonData_;
    SkeletonPose internalPose_;
    const SkeletonPose* currentPose_ = nullptr; // 描画に使用するポーズ

    SkinCluster skinCluster_;
    
    Matrix4x4 localMatrix_;
    Matrix4x4 worldMatrix_;

    uint32_t lastSkinnedFrameIndex_ = 0;
    std::string filename_;
};
