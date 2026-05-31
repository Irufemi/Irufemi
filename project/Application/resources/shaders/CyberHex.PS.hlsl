/**
 * @file CyberHex.PS.hlsl
 * @brief サイバー風ヘックスシールド（六角形グリッド）描画用ピクセルシェーダ
 * 
 * @note
 * ==============================================================================
 * Original ShaderToy: "Hexagons - distance" by Inigo Quilez (https://www.shadertoy.com/view/Xd2CGt)
 * 
 * 【ShaderToyからの主な変更点（IrufemiEngine向け最適化）】
 * 
 * 1. 座標系のワールド空間化 (Triplanar Mapping / Cylindrical Mapping)
 *    - 元の `fragCoord.xy / iResolution.y` (スクリーン座標) ではなく、`input.worldPosition` を使用。
 *    - Triplanar: `input.normal` を元にXY, XZ, ZY平面を自動判定し、PlaneのScaleに依存せず常に均一な密度で六角形を描画。
 *    - Cylindrical: トンネル用にZ軸方向の円柱マッピングを実装。ワールド座標から角度(atan2)と半径を算出し、360度継ぎ目のないマッピングを実現（UVの継ぎ目は画面下部の見えない位置に隠蔽）。
 * 
 * 2. マジックナンバーの排除と専用定数バッファ (CyberHexParams) によるパラメータ化
 *    - 元コードで固定値だった `0.10`（縁の太さ）や `0.15`（明るさ）、ハードコードされた発光色をすべて排除。
 *    - 汎用 Material を汚染せず、専用定数バッファ `gCyberHex` を経由して、色、太さ、密度、歪みなどを C++ (ImGui) からリアルタイムで柔軟に変更可能に設計。
 *    - ベースの暗さ（`baseBrightness`）を比例倍率として計算に組み込み、漆黒の背景表現に対応。
 * 
 * 3. 「アニメーション（浮き沈み）」と「UVスクロール」の完全な分離
 *    - 座標計算から時間項を外し、時間(`animTime`)はノイズ(`noise`)や明滅(`sin`)の計算にのみ影響するように数式を分解。
 *    - 全体の移動は `pos += uvScroll` で独立させ、模様の浮き沈みと全体の移動を別々で制御できるように再構築。
 * 
 * 4. プロシージャルノイズへの置換
 *    - 元コードのテクスチャ依存を、自作の `rand` および `noise` (3D Value Noise) に置き換え。
 * 
 * 5. 視覚効果の調整（距離フェード・フリッカー防止・影の統合）
 *    - トンネルモード専用に、カメラから遠ざかるほど暗闇に沈む「距離フェード（フェードアウト）」処理を追加。
 *    - 激しい点滅を防ぐため、明滅計算に独自の振幅（`flickerAmplitude`）調整を導入。
 *    - `Lighting.hlsli` を用いて他のオブジェクトから落ちる影を受け取る処理を追加。
 * ==============================================================================
 */

#include "Object3d.hlsli"
#include "Lighting.hlsli"
#include "PerFrame.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<LightCommonData> gLightCommon : register(b1);
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

SamplerComparisonState gShadowSampler : register(s2);
Texture2D<float32_t> gShadowMap : register(t5);

// 専用のパラメータバッファ (RootSlot::Special -> b6)
struct CyberHexParams {
    float4 edgeColor;
    float edgeThickness;
    float baseBrightness;
    float flickerAmplitude;
    float distortion;

    float density;
    float animationSpeed;
    float uvScrollX;
    float uvScrollY;
    
    float mappingMode;
    float3 padding;
};
ConstantBuffer<CyberHexParams> gCyberHex : register(b6);

#include "Noise.hlsli"

// 六角形の距離とセルIDを計算する関数
// 戻り値: { 2d cell id x, 2d cell id y, distance to border, distance to center }
float4 hexagon(float2 p) 
{
    float2 q = float2(p.x * 2.0 * 0.5773503, p.y + p.x * 0.5773503);
    
    float2 pi = floor(q);
    float2 pf = frac(q);

    // 負の数に安全なモジュロ計算 (GLSLのmod互換)
    float v = (pi.x + pi.y) - 3.0 * floor((pi.x + pi.y) / 3.0);

    float ca = step(1.0, v);
    float cb = step(2.0, v);
    float2 ma = step(pf.xy, pf.yx);
    
    // distance to borders
    float e = dot(ma, 1.0 - pf.yx + ca * (pf.x + pf.y - 1.0) + cb * (pf.yx - 2.0 * pf.xy));

    // distance to center    
    p = float2(q.x + floor(0.5 + p.y / 1.5), 4.0 * p.y / 3.0) * 0.5 + 0.5;
    float f = length((frac(p) - 0.5) * float2(1.0, sqrt(3.0) / 2.0));        
    
    return float4(pi + ca - cb * ma, e, f);
}

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) 
{
    PixelShaderOutput output;
    
    // 浮き沈み・明滅アニメーションの進行速度
    float animTime = gPerFrame.time * gCyberHex.animationSpeed;
    
    // PlaneのScaleに依存せず、床や壁で同じ密度になるようワールド座標ベースでマッピングする
    float3 absN = abs(input.normal);
    float2 pos = float2(0.0, 0.0);
    
    if (gCyberHex.mappingMode > 0.5) {
        // 円柱座標系 (Cylindrical Mapping - Z軸方向の円柱)
        // XとYの順序を逆にして、継ぎ目（-PIとPIの境界）が下（Y軸のマイナス方向）に来るようにする
        float angle = atan2(input.worldPosition.x, input.worldPosition.y); // -PI to PI
        float radius = length(input.worldPosition.xy);
        pos = float2(radius * angle, input.worldPosition.z);
    } else {
        // 面の向き（法線）に応じて投影する軸を決定 (Triplanar Mapping)
        if (absN.y > absN.x && absN.y > absN.z) {
            pos = input.worldPosition.xz; // 床・天井
        } else if (absN.x > absN.y && absN.x > absN.z) {
            pos = input.worldPosition.zy; // X軸方向の壁
        } else {
            pos = input.worldPosition.xy; // Z軸方向の壁
        }
    }
    
    // 密度パラメータ
    pos *= gCyberHex.density;

    // UVスクロールの適用
    pos += float2(gPerFrame.time * gCyberHex.uvScrollX, gPerFrame.time * gCyberHex.uvScrollY);

    // 空間を軽く歪ませてサイバーな空間の奥行き・レンズ効果を演出
    pos *= 1.2 + gCyberHex.distortion * length(pos);

    // ==========================================
    // 1. ベースとなるグレーのヘックス（奥の層）
    // ==========================================
    float4 h = hexagon(8.0 * pos);
    float n = noise(float3(0.3 * h.xy + animTime * 0.1, animTime));
    // 元々は0.15などの固定の加算があったが、トンネルの明暗をコントロールしやすくするため
    // gCyberHex.baseBrightness に比例するように修正
    float3 col = gCyberHex.baseBrightness * (1.0 + 3.0 * rand(h.xy + 1.2));
    col *= smoothstep(gCyberHex.edgeThickness, gCyberHex.edgeThickness + 0.01, h.z); // 枠線
    col *= smoothstep(gCyberHex.edgeThickness, gCyberHex.edgeThickness + 0.01, h.w); // 中心
    col *= 1.0 + 0.15 * sin(40.0 * h.z);
    col *= 0.75 + 0.5 * h.z * n;

    // ==========================================
    // 2. シャドウ（影の層）
    // ==========================================
    h = hexagon(6.0 * (pos + 0.1 * float2(-1.3, 1.0)));
    col *= 1.0 - 0.8 * smoothstep(0.45, 0.451, noise(float3(0.3 * h.xy + animTime * 0.1, 0.5 * animTime)));

    // ==========================================
    // 3. 発光するカラーヘックス（手前の層）
    // ==========================================
    h = hexagon(6.0 * pos);
    n = noise(float3(0.3 * h.xy + animTime * 0.1, 0.5 * animTime));
    
    // 発光色
    float3 baseColor = gCyberHex.edgeColor.rgb; 
    // 明滅の強さ (セルごとに位相をずらしつつanimTimeで明滅させる)
    float intensity = (1.0 - gCyberHex.flickerAmplitude) + gCyberHex.flickerAmplitude * sin(rand(h.xy) * 1.5 + animTime * 2.0); 
    float3 colb = baseColor * intensity;
    
    colb *= smoothstep(gCyberHex.edgeThickness, gCyberHex.edgeThickness + 0.01, h.z); // 枠線
    colb *= 1.0 + 0.15 * sin(40.0 * h.z);

    // ==========================================
    // 4. ブレンドとポスト処理
    // ==========================================
    // ノイズ値を使ってベース（奥）とカラー（手前）をブレンド
    col = lerp(col, colb, smoothstep(0.3, 0.6, n));
    
    // トーンマッピング（全体的な明るさを抑える）
    col *= 1.5 / (1.5 + col);

    // キャラクターや建物からの影（ShadowMap）を適用して接地感を出す
    float shadowFactor = CalculateShadow(input.shadowPos, gShadowMap, gShadowSampler, normalize(input.normal), gLightCommon.directionalLight.direction);
    // 影の領域は明るさを30%に落とす
    col *= lerp(0.3, 1.0, shadowFactor);

    // トンネルモードの場合、奥に行くほど暗くフェードアウトさせ、中央に文字が際立つようにする
    if (gCyberHex.mappingMode > 0.5) {
        float dist = length(input.worldPosition.xyz - gPerFrame.cameraWorldPosition);
        float fade = saturate(1.0 - (dist / 800.0));
        fade = pow(fade, 1.5); // フェードカーブを調整して手前は明るく、奥はスッと暗くする
        col *= fade;
    }

    // ビネット効果（四隅を暗くする）
    float2 uv = input.texcoord;
    col *= pow(max(16.0 * uv.x * (1.0 - uv.x) * uv.y * (1.0 - uv.y), 0.0), 0.1);

    output.color = float4(saturate(col), gMaterial.color.a);
    return output;
}
