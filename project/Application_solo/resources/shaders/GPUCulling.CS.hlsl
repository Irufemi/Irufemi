//=============================================================================
// GPUCulling.CS.hlsl
//=============================================================================

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
    float2 padding;
};

// 入力: CPUからアップロードされた全インスタンス
StructuredBuffer<InstanceData> InputInstances : register(t0);

// 出力: カリングを生き残ったインスタンス
RWStructuredBuffer<InstanceData> OutputInstances : register(u0);

// 出力: ExecuteIndirect用の引数バッファ (uintの配列として扱う)
// CommandBuffer[0] = IndexCountPerInstance
// CommandBuffer[1] = InstanceCount
// CommandBuffer[2] = StartIndexLocation
// CommandBuffer[3] = BaseVertexLocation
// CommandBuffer[4] = StartInstanceLocation
RWStructuredBuffer<uint> CommandBuffer : register(u1);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    uint instanceID = DTid.x;
    
    if (instanceID >= maxInstanceCount) {
        return;
    }

    InstanceData inst = InputInstances[instanceID];

    // World行列から中心座標を抽出
    float3 center = float3(inst.World._41, inst.World._42, inst.World._43);
    
    // World行列から各軸のスケールを抽出して最大スケールを求める
    float scaleX = length(float3(inst.World._11, inst.World._12, inst.World._13));
    float scaleY = length(float3(inst.World._21, inst.World._22, inst.World._23));
    float scaleZ = length(float3(inst.World._31, inst.World._32, inst.World._33));
    
    float maxScale = max(scaleX, max(scaleY, scaleZ));
    float worldRadius = localRadius * maxScale;

    // 球とFrustumの6平面での交差判定
    bool isVisible = true;
    for (int i = 0; i < 6; ++i) {
        float dist = dot(center, planes[i].xyz) + planes[i].w;
        if (dist < -worldRadius) {
            isVisible = false;
            break;
        }
    }

    // 可視オブジェクトならOutputに追加し、CommandBufferのInstanceCountをインクリメント
    if (isVisible) {
        uint visibleIndex = 0;
        InterlockedAdd(CommandBuffer[1], 1, visibleIndex);
        OutputInstances[visibleIndex] = inst;
    }
}
