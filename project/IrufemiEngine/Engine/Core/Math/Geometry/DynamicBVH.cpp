#include "DynamicBVH.h"
#include <algorithm>
#include <cmath>
#include <cassert>


namespace Irufemi {
namespace {
    // AABBのマージン（オブジェクトが少し動いてもRebuildを避けるための「太らせる」余白）
    const float AABB_MARGIN = 0.5f;
    // 実際にAABBがこの倍率以上外にはみ出したらRebuild（Remove -> Insert）する
    const float AABB_BOUNDS_MULTIPLIER = 1.2f;
}

DynamicBVH::DynamicBVH() {
    // メモリアロケーションを防ぐための事前確保
    nodes_.reserve(10000); 
}

void DynamicBVH::Clear() {
    nodes_.clear();
    rootIndex_ = -1;
    freeListFirst_ = -1;
}

int32_t DynamicBVH::AllocateNode() {
    // フリーリストに空きがあれば再利用
    if (freeListFirst_ != -1) {
        int32_t nodeId = freeListFirst_;
        freeListFirst_ = nodes_[nodeId].parentIndex; // フリーリストの次を指している
        
        // 初期化
        nodes_[nodeId].parentIndex = -1;
        nodes_[nodeId].leftChildIndex = -1;
        nodes_[nodeId].rightChildIndex = -1;
        nodes_[nodeId].collider = nullptr;
        return nodeId;
    }

    // なければ新規追加
    nodes_.push_back(BVHNode());
    return static_cast<int32_t>(nodes_.size() - 1);
}

void DynamicBVH::FreeNode(int32_t nodeId) {
    if (nodeId < 0 || nodeId >= nodes_.size()) return;
    
    // nextポインタの代わりにparentIndexを使ってリストを繋ぐ
    nodes_[nodeId].parentIndex = freeListFirst_;
    nodes_[nodeId].leftChildIndex = -1;
    nodes_[nodeId].rightChildIndex = -1;
    nodes_[nodeId].collider = nullptr;
    freeListFirst_ = nodeId;
}

AABB DynamicBVH::MergeAABB(const AABB& a, const AABB& b) const {
    AABB result;
    result.min.x = std::min(a.min.x, b.min.x);
    result.min.y = std::min(a.min.y, b.min.y);
    result.min.z = std::min(a.min.z, b.min.z);
    
    result.max.x = std::max(a.max.x, b.max.x);
    result.max.y = std::max(a.max.y, b.max.y);
    result.max.z = std::max(a.max.z, b.max.z);
    return result;
}

float DynamicBVH::SurfaceArea(const AABB& aabb) const {
    float dx = aabb.max.x - aabb.min.x;
    float dy = aabb.max.y - aabb.min.y;
    float dz = aabb.max.z - aabb.min.z;
    return 2.0f * (dx * dy + dy * dz + dz * dx);
}

bool DynamicBVH::Intersects(const AABB& a, const AABB& b) const {
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z) return false;
    return true;
}

int32_t DynamicBVH::Insert(ColliderComponent* collider, const AABB& aabb) {
    int32_t nodeId = AllocateNode();
    
    // マージンを持たせて太らせる
    AABB fattenedAABB;
    fattenedAABB.min.x = aabb.min.x - AABB_MARGIN;
    fattenedAABB.min.y = aabb.min.y - AABB_MARGIN;
    fattenedAABB.min.z = aabb.min.z - AABB_MARGIN;
    fattenedAABB.max.x = aabb.max.x + AABB_MARGIN;
    fattenedAABB.max.y = aabb.max.y + AABB_MARGIN;
    fattenedAABB.max.z = aabb.max.z + AABB_MARGIN;

    nodes_[nodeId].aabb = fattenedAABB;
    nodes_[nodeId].collider = collider;

    InsertLeaf(nodeId);
    return nodeId;
}

void DynamicBVH::Remove(int32_t nodeId) {
    if (nodeId == -1) return;
    RemoveLeaf(nodeId);
    FreeNode(nodeId);
}

void DynamicBVH::Update(int32_t nodeId, const AABB& newAABB) {
    if (nodeId == -1 || nodeId >= nodes_.size()) return;

    // 現在のFattened AABBが、新しいAABBを完全に包んでいるかチェック
    const AABB& fatAABB = nodes_[nodeId].aabb;
    bool isContained = 
        newAABB.min.x >= fatAABB.min.x && newAABB.max.x <= fatAABB.max.x &&
        newAABB.min.y >= fatAABB.min.y && newAABB.max.y <= fatAABB.max.y &&
        newAABB.min.z >= fatAABB.min.z && newAABB.max.z <= fatAABB.max.z;

    if (isContained) {
        // ツリー構造を変更する必要なし（超高速）
        return;
    }

    // はみ出してしまった場合は一度ツリーから外して再挿入する
    ColliderComponent* collider = nodes_[nodeId].collider;
    RemoveLeaf(nodeId);

    // 新たに太らせる
    AABB fattenedAABB;
    fattenedAABB.min.x = newAABB.min.x - AABB_MARGIN;
    fattenedAABB.min.y = newAABB.min.y - AABB_MARGIN;
    fattenedAABB.min.z = newAABB.min.z - AABB_MARGIN;
    fattenedAABB.max.x = newAABB.max.x + AABB_MARGIN;
    fattenedAABB.max.y = newAABB.max.y + AABB_MARGIN;
    fattenedAABB.max.z = newAABB.max.z + AABB_MARGIN;

    nodes_[nodeId].aabb = fattenedAABB;
    nodes_[nodeId].collider = collider;

    InsertLeaf(nodeId);
}

void DynamicBVH::InsertLeaf(int32_t leafIndex) {
    if (rootIndex_ == -1) {
        rootIndex_ = leafIndex;
        return;
    }

    // SAH(Surface Area Heuristic)に基づいて最もコストが低いノードを探す
    AABB leafAABB = nodes_[leafIndex].aabb;
    int32_t index = rootIndex_;

    while (!nodes_[index].IsLeaf()) {
        int32_t left = nodes_[index].leftChildIndex;
        int32_t right = nodes_[index].rightChildIndex;

        float area = SurfaceArea(nodes_[index].aabb);

        AABB combinedAABB = MergeAABB(nodes_[index].aabb, leafAABB);
        float combinedArea = SurfaceArea(combinedAABB);

        // 新しい親を作る場合のコスト
        float costCreation = 2.0f * combinedArea;
        // 下に降りる場合のベースコスト
        float costDescent = 2.0f * (combinedArea - area);

        // 左に降りるコスト
        float costLeft;
        AABB aabbLeft = MergeAABB(leafAABB, nodes_[left].aabb);
        if (nodes_[left].IsLeaf()) {
            costLeft = costDescent + SurfaceArea(aabbLeft);
        } else {
            float oldArea = SurfaceArea(nodes_[left].aabb);
            float newArea = SurfaceArea(aabbLeft);
            costLeft = costDescent + (newArea - oldArea);
        }

        // 右に降りるコスト
        float costRight;
        AABB aabbRight = MergeAABB(leafAABB, nodes_[right].aabb);
        if (nodes_[right].IsLeaf()) {
            costRight = costDescent + SurfaceArea(aabbRight);
        } else {
            float oldArea = SurfaceArea(nodes_[right].aabb);
            float newArea = SurfaceArea(aabbRight);
            costRight = costDescent + (newArea - oldArea);
        }

        // どこが一番安いか
        if (costCreation < costLeft && costCreation < costRight) {
            break;
        }

        if (costLeft < costRight) {
            index = left;
        } else {
            index = right;
        }
    }

    int32_t sibling = index;
    int32_t oldParent = nodes_[sibling].parentIndex;
    int32_t newParent = AllocateNode();

    nodes_[newParent].parentIndex = oldParent;
    nodes_[newParent].aabb = MergeAABB(leafAABB, nodes_[sibling].aabb);
    nodes_[newParent].leftChildIndex = sibling;
    nodes_[newParent].rightChildIndex = leafIndex;
    nodes_[newParent].collider = nullptr;

    nodes_[sibling].parentIndex = newParent;
    nodes_[leafIndex].parentIndex = newParent;

    if (oldParent != -1) {
        if (nodes_[oldParent].leftChildIndex == sibling) {
            nodes_[oldParent].leftChildIndex = newParent;
        } else {
            nodes_[oldParent].rightChildIndex = newParent;
        }
    } else {
        rootIndex_ = newParent;
    }

    Refit(nodes_[leafIndex].parentIndex);
}

void DynamicBVH::RemoveLeaf(int32_t leafIndex) {
    if (leafIndex == rootIndex_) {
        rootIndex_ = -1;
        return;
    }

    int32_t parent = nodes_[leafIndex].parentIndex;
    int32_t grandParent = nodes_[parent].parentIndex;
    int32_t sibling = (nodes_[parent].leftChildIndex == leafIndex) ? nodes_[parent].rightChildIndex : nodes_[parent].leftChildIndex;

    if (grandParent != -1) {
        if (nodes_[grandParent].leftChildIndex == parent) {
            nodes_[grandParent].leftChildIndex = sibling;
        } else {
            nodes_[grandParent].rightChildIndex = sibling;
        }
        nodes_[sibling].parentIndex = grandParent;
        FreeNode(parent);
        Refit(grandParent);
    } else {
        rootIndex_ = sibling;
        nodes_[sibling].parentIndex = -1;
        FreeNode(parent);
    }
}

void DynamicBVH::Refit(int32_t nodeIndex) {
    while (nodeIndex != -1) {
        int32_t left = nodes_[nodeIndex].leftChildIndex;
        int32_t right = nodes_[nodeIndex].rightChildIndex;

        nodes_[nodeIndex].aabb = MergeAABB(nodes_[left].aabb, nodes_[right].aabb);
        nodeIndex = nodes_[nodeIndex].parentIndex;
    }
}

void DynamicBVH::GetPotentialCollisionPairs(std::vector<std::pair<ColliderComponent*, ColliderComponent*>>& outPairs) const {
    outPairs.clear();
    if (rootIndex_ != -1) {
        ComputePairs(nodes_[rootIndex_].leftChildIndex, nodes_[rootIndex_].rightChildIndex, outPairs);
    }
}

void DynamicBVH::ComputePairs(int32_t node0, int32_t node1, std::vector<std::pair<ColliderComponent*, ColliderComponent*>>& outPairs) const {
    if (node0 == -1 || node1 == -1) return;

    if (!Intersects(nodes_[node0].aabb, nodes_[node1].aabb)) {
        return;
    }

    bool isLeaf0 = nodes_[node0].IsLeaf();
    bool isLeaf1 = nodes_[node1].IsLeaf();

    if (isLeaf0 && isLeaf1) {
        outPairs.push_back({ nodes_[node0].collider, nodes_[node1].collider });
    } else if (isLeaf0) {
        ComputePairs(node0, nodes_[node1].leftChildIndex, outPairs);
        ComputePairs(node0, nodes_[node1].rightChildIndex, outPairs);
    } else if (isLeaf1) {
        ComputePairs(node1, nodes_[node0].leftChildIndex, outPairs);
        ComputePairs(node1, nodes_[node0].rightChildIndex, outPairs);
    } else {
        float area0 = SurfaceArea(nodes_[node0].aabb);
        float area1 = SurfaceArea(nodes_[node1].aabb);

        if (area0 > area1) {
            ComputePairs(nodes_[node0].leftChildIndex, node1, outPairs);
            ComputePairs(nodes_[node0].rightChildIndex, node1, outPairs);
        } else {
            ComputePairs(nodes_[node1].leftChildIndex, node0, outPairs);
            ComputePairs(nodes_[node1].rightChildIndex, node0, outPairs);
        }
    }

    if (!isLeaf0) {
        ComputePairs(nodes_[node0].leftChildIndex, nodes_[node0].rightChildIndex, outPairs);
    }
    if (!isLeaf1) {
        ComputePairs(nodes_[node1].leftChildIndex, nodes_[node1].rightChildIndex, outPairs);
    }
}

void DynamicBVH::Query(const AABB& testAabb, std::vector<ColliderComponent*>& outColliders) const {
    if (rootIndex_ == -1) return;

    std::vector<int32_t> stack;
    stack.reserve(256);
    stack.push_back(rootIndex_);

    while (!stack.empty()) {
        int32_t index = stack.back();
        stack.pop_back();

        if (Intersects(nodes_[index].aabb, testAabb)) {
            if (nodes_[index].IsLeaf()) {
                if (nodes_[index].collider) {
                    outColliders.push_back(nodes_[index].collider);
                }
            } else {
                stack.push_back(nodes_[index].leftChildIndex);
                stack.push_back(nodes_[index].rightChildIndex);
            }
        }
    }
}

void DynamicBVH::RaycastQuery(const Ray& ray, float maxDistance, std::vector<ColliderComponent*>& outHits) const {
    if (rootIndex_ == -1) return;

    std::vector<int32_t> stack;
    stack.reserve(256);
    stack.push_back(rootIndex_);

    // 簡単なAABB vs Ray の判定距離格納用
    float unusedDistance = 0.0f;

    while (!stack.empty()) {
        int32_t index = stack.back();
        stack.pop_back();

        // ノードのAABBとRayが交差するか
        // (AABBに対するレイキャストは枝刈り目的のため、正確な距離よりも交差するかどうかが重要)
        if (Collision::IsCollision(ray, nodes_[index].aabb, unusedDistance)) {

            if (nodes_[index].IsLeaf()) {
                if (nodes_[index].collider) {
                    outHits.push_back(nodes_[index].collider);
                }
            } else {
                stack.push_back(nodes_[index].leftChildIndex);
                stack.push_back(nodes_[index].rightChildIndex);
            }
        }
    }
}

} // namespace Irufemi
