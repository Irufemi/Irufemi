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

    // --- GPU Animation Calculation ---
    // CPU側の 2.0f 倍速のアニメーションに合わせる。instanceIDで位相を少しずらす。
    float phaseOffset = (float)(instanceID % 100) * 0.1f;
    float animTime = time * 2.0f + phaseOffset;
    
    float3 pos = td.position.xyz;
    pos.y += sin(animTime) * 0.5f;
    
    float3 rot = td.rotation.xyz;
    float3 scale = td.scale.xyz;

    // --- Matrix Construction ---
    matrix world = MakeAffineMatrix(scale, rot, pos);
    matrix worldInvTrans = MakeInverseTransposeMatrix(scale, rot);

    float maxScale = max(scale.x, max(scale.y, scale.z));
    float worldRadius = localRadius * maxScale;

    // 球とFrustumの6平面での交差判定
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
