//=============================================================================
// GPUCulling.CS.hlsl
//=============================================================================

#include "MathUtility.hlsli"

struct TransformData {
    float4 position;
    float4 rotation;
    float4 scale;
    float4 color;
};

struct InstanceData {
    matrix WVP;
    matrix World;
    matrix WorldInverseTranspose;
    float4 color;
};

// Cullingの定数データ
cbuffer CullingData : register(b0) {
    float4 planes[6];        // カメラのFrustum 6平面 (xyz = normal, w = distance)
    uint  maxInstanceCount;  // バッファに存在する最大インスタンス数
    float localRadius;       // モデルのローカルBoundingSphereの半径
    float time;              // IrufemiEngineのTotalTime
    float padding;
};

// 入力: CPUからアップロードされた未計算のTransform
StructuredBuffer<TransformData> InputInstances : register(t0);

// 出力: カリングを生き残ったインスタンス（行列化済み）
RWStructuredBuffer<InstanceData> OutputInstances : register(u0);

// 出力: ExecuteIndirect用の引数バッファ (uintの配列として扱う)
RWStructuredBuffer<uint> CommandBuffer : register(u1);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    uint instanceID = DTid.x;
    
    if (instanceID >= maxInstanceCount) {
        return;
    }

    TransformData td = InputInstances[instanceID];

    /**
     * @brief インスタンスのワールド座標とバウンディングスフィアの計算
     * @details 不用意なアニメーション操作を排除し、正確なTransform座標を取得します。
     *          また、浮動小数点誤差によるカリングのチラつきを防ぐため、保守的なマージン(1.1倍)を持たせます。
     */
    float3 pos = td.position.xyz;
    float3 rot = td.rotation.xyz;
    float3 scale = td.scale.xyz;

    float maxScale = max(scale.x, max(scale.y, scale.z));
    // 保守的カリング (Conservative Culling) として 10% のマージンを追加
    float worldRadius = localRadius * maxScale * 1.1f;

    // --- Matrix Construction ---
    matrix world = MakeAffineMatrix(scale, rot, pos);
    matrix worldInvTrans = MakeInverseTransposeMatrix(scale, rot);

    // --- Frustum Culling ---
    /**
     * @brief 視錐台(Frustum)カリング判定
     * @details C++側で D成分 (-distance) が正しく計算されているため、標準の点と平面の距離公式を使用します。
     */
    bool isVisible = true;
    for (int i = 0; i < 6; ++i) {
        float dist = dot(pos, planes[i].xyz) + planes[i].w;
        if (dist < -worldRadius) {
            isVisible = false;
            break;
        }
    }

    // 可視オブジェクトならOutputに追加し、CommandBufferのInstanceCountをインクリメント
    if (isVisible) {
        uint visibleIndex = 0;
        InterlockedAdd(CommandBuffer[1], 1, visibleIndex);
        
        InstanceData outInst;
        outInst.WVP = matrix(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
        outInst.World = world;
        outInst.WorldInverseTranspose = worldInvTrans;
        outInst.color = td.color;
        
        OutputInstances[visibleIndex] = outInst;
    }
}
