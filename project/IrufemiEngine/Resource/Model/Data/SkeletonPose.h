#pragma once

#include "Core/Math/QuaternionTransform.h"
#include "Core/Math/Matrix4x4.h"
#include <vector>
#include <cstdint>
#include <memory>

struct SkeletonData;
struct NodeAnimation;

/**
 * @struct JointPose
 * @brief 各インスタンスごとの動的なボーンの状態
 */
struct JointPose {
    Irufemi::QuaternionTransform transform; // 現在のローカルTransform
    Irufemi::Matrix4x4 localMatrix;         // 現在のローカル行列
    Irufemi::Matrix4x4 skeletonSpaceMatrix; // スケルトン空間（ルートからの累積）行列
};

/**
 * @struct SkeletonPose
 * @brief 各モデル（インスタンス）が持つ現在の姿勢情報
 */
struct SkeletonPose {
    const SkeletonData* data = nullptr; // 参照する静的データ
    std::vector<JointPose> jointPoses;
    
    // --- 最適化用キャッシュ ---
    // 最後に適用したアニメーションのアドレス（変更検知用）
    const void* lastAppliedAnimation = nullptr;
    // Jointインデックスと対象NodeAnimationのポインタを紐付けたリスト
    std::vector<std::pair<int32_t, const NodeAnimation*>> activeAnimationBindings;
};
