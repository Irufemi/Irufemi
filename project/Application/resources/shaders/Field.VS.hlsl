//======================================================
// FieldCylinder.VS.hlsl
// フィールド円柱用：控えめな砂丘カーブ入り頂点シェーダ
//======================================================

cbuffer TransformCB : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

//--------------------------------------
// 砂丘の高さ関数（ローカルXZベース）
//--------------------------------------
float CalcDuneHeight(float2 localXZ)
{
    // CylinderClass が -radius → +radius の範囲なので正規化
    float r = length(localXZ); // ローカル半径 (例: 0.0 ~ 6.0 など)

    // 最大半径を 1.0 基準に正規化（大きい円盤でも破綻しないため）
    float maxR = 1.0;
    if (r > maxR)
        r = maxR;
    float t = r / maxR;

    // 内側は平ら、外側へ向けて控えめに持ち上げる
    float inner = 0.4; // 0〜0.4まではほぼ平ら
    float amount = 0.25; // 最大持ち上げ高さ（0.25m）
    float f = saturate((t - inner) / (1.0 - inner));

    // 2乗で滑らかカーブ
    f = f * f;

    return f * amount;
}

PSInput main(VSInput input)
{
    PSInput o;

    //-----------------------------
    // ローカルからワールドへ
    //-----------------------------
    float4 worldPos4 = mul(float4(input.pos, 1.0), world);
    float3 worldPos = worldPos4.xyz;

    //-----------------------------
    // ★ 砂丘カーブ：ローカルXZ基準で持ち上げる
    //-----------------------------
    float duneH = CalcDuneHeight(input.pos.xz);
    worldPos.y += duneH; // ここだけ変更

    //-----------------------------
    // 法線（簡易：world変換のみ）
    //-----------------------------
    float3 n = mul((float3x3) world, input.normal);
    o.normal = normalize(n);

    //-----------------------------
    // クリップ座標へ
    //-----------------------------
    float4 viewPos = mul(float4(worldPos, 1.0), view);
    float4 projPos = mul(viewPos, proj);
    o.svpos = projPos;

    //-----------------------------
    // UV そのまま渡す
    //-----------------------------
    o.uv = input.uv;

    return o;
}
