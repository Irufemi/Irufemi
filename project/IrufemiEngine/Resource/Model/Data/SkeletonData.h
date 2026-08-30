#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <optional>
#include "Core/Math/Matrix4x4.h"
#include "Core/Math/QuaternionTransform.h"

/**
 * @struct JointData
 * @brief 静的なボーン（Joint）定義。全インスタンスで共有される。
 */
struct JointData {
    std::string name;
    int32_t index;
    std::optional<int32_t> parent;
    std::vector<int32_t> children;

    // Bind Pose (初期状態のTransformとローカル行列)
    Irufemi::QuaternionTransform bindTransform;
    Irufemi::Matrix4x4 bindLocalMatrix;
};

/**
 * @struct SkeletonData
 * @brief ボーン階層構造の静的データ。モデルのロード時に1度だけ作成される。
 */
struct SkeletonData {
    int32_t root;
    std::map<std::string, int32_t> jointMap;
    std::vector<JointData> joints;
};
