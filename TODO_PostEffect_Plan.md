# ポストエフェクト実装計画書 (PostEffect Implementation Plan)

現在出先での作業用として、今後の実装候補とその詳細な実装内容をすべてまとめたドキュメントです。
（※本環境ではコードの直接編集は行わず、メイン環境に戻った際にスムーズに組み込めるよう案を詰めるにとどめます）

本プロジェクトのアーキテクチャ（`PostProcess.hlsli` への関数化、`CombinedParams` によるシングルパス統合、専用PSOによるマルチパス）に準拠した実装計画となっています。
以下に挙げる全てのエフェクトを実装候補としてストックします。

---

## 🚨 必須リファクタリング (潜在バグの修正)
ユーザーの懸念通り、**「複数のエフェクトを組み合わせた際に破綻する」**問題や、**「シーン遷移時のエフェクトの不具合」**など、現在のアーキテクチャに潜んでいる致命的なバグを発見しました。新しいエフェクトを追加する前に、以下の修正を必ず行ってください。

### [ ] バグ1: 複数適用時の「テクスチャサンプリング先祖返り」バグ
- **対象ファイル**: `PostProcessManager.cpp` (`Draw` 関数のバッチング処理)
- **問題の原因**:
  現在の `PostProcess.PS.hlsl` では、複数のエフェクトを1回のDraw内で `for` ループで回し、`color.rgb` を更新していく仕組み（Combinedパス）になっています。
  しかし、`RadialBlur` や `Glitch` のような「周辺ピクセルを参照するエフェクト（空間フィルタ）」は、引数として渡された `gTexture` を直接サンプリングします。
  そのため、例えば `Grayscale` → `RadialBlur` の順で適用した場合、`RadialBlur` は「ループ内でモノクロになった `color`」ではなく、「パス開始前のカラーテクスチャ(`gTexture`)」をサンプリングしてしまい、**モノクロ効果が無視されて元の色でブレンドされてしまう**という破綻（先祖返り）が起きます。
- **修正方針 (`PostProcessManager.cpp` の修正)**:
  `CombinedPSO` にまとめる条件式（`while` ループ）を見直します。周辺ピクセルを参照するエフェクトがリストに来た場合は、**強制的に一度バッチを区切り、Ping-Pongテクスチャへの書き出し（パスの分割）を挟む**ように変更します。
  ```cpp
  // PostProcessManager.cpp の Draw関数内バッチング条件の修正案
  while (lookAhead < activeModes_.size() && 
         activeModes_[lookAhead] != Mode::Bloom && 
         activeModes_[lookAhead] != Mode::Smoothing && 
         activeModes_[lookAhead] != Mode::GaussianFilter && 
         activeModes_[lookAhead] != Mode::DualKawaseBlur && 
         // ★追加: 空間サンプリングを行うエフェクトはバッチを区切る（単独のCombinedパスとして処理させる）
         activeModes_[lookAhead] != Mode::RadialBlur && 
         activeModes_[lookAhead] != Mode::Glitch &&
         batch.size() < 16) {
      batch.push_back(activeModes_[lookAhead]);
      lookAhead++;
  }
  ```
  ※これにより、空間フィルタは「直前までの色調補正が焼き込まれた最新のテクスチャ」を `gTexture` として正しく読み込めるようになります。

### [ ] バグ2: シーン切り替え時の「エフェクト残存・未適用」バグ
- **対象ファイル**: `PostProcessManager.cpp` / `PostProcessManager.h`
- **問題の原因**:
  1. **残存バグ**: 現在 `ResetAllParams()` 関数は各パラメータ構造体（`NoiseParams` 等）の数値を初期化していますが、有効になっているエフェクトの配列（`activeModes_` および `pendingActiveModes_`）をクリアしていません。そのため、シーンが切り替わっても前のシーンでONにしたエフェクトが掛かりっぱなしになります。
  2. **未適用バグ**: `AddMode()` 等でエフェクトを追加しても、リストは予約用（`pendingActiveModes_`）に入るだけで、毎フレーム呼ばれる `Update()` 関数の中で `CommitPendingModes()` が走るまでは、実際の描画（`activeModes_`）に反映されません。もしシーン初期化処理などで「`Update()` が呼ばれた後にエフェクトを追加」してしまうと、そのフレーム（あるいは描画更新が走るまで）エフェクトが適用されません。
- **修正方針**:
  - `PostProcessManager::Reset()` という完全な初期化関数を新設し、その中で `ResetAllParams()` と `ClearModes()` を両方呼ぶようにします。各シーンの初期化時や終了時には必ずこの `Reset()` を呼ぶようにルール化します。
  - シーン初期化時にエフェクトを適用した場合に即座に反映されるよう、`Draw()` の直前などの安全なタイミングで必ず `CommitPendingModes()` が呼ばれるか確認し、仕様として「エフェクトの追加・変更は `Update` 前に行う」ことをチームに周知します。

---

## 1. 未実装の指定エフェクト（必須・加点項目）

### 1.1. LuminanceBasedOutLine (加点: 5点)
**概要**: 画像の「輝度（Luminance）」をベースにソーベルフィルタを用いて輪郭を抽出するエフェクト。2D調（トゥーン風）の表現において、テクスチャの模様や色の境界に線画を入れるための基礎技術となります。
**アーキテクチャへの組み込み方針**:
- **HLSL**: `PostProcess.hlsli` に `ApplyLuminanceBasedOutline` 関数を追加。
- **シェーダー**: 既存の `PostProcess.PS.hlsl` のシングルパスループ (`CombinedParams`) に組み込む。
- **C++**: `PostProcessManager::CombinedParams` にしきい値や線の色などのパラメータを追加。

**実装詳細 (HLSL - PostProcess.hlsli 追加用)**:
```hlsl
float32_t3 ApplyLuminanceBasedOutline(float32_t3 color, float32_t2 uv, float32_t2 uvStepSize, float32_t threshold, float32_t4 outlineColor, Texture2D<float32_t4> tex, SamplerState smp) {
    const float Gx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
    const float Gy[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };
    
    float valueX = 0.0f;
    float valueY = 0.0f;
    
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float2 offset = float2(x, y) * uvStepSize;
            float3 sampleColor = tex.SampleLevel(smp, uv + offset, 0).rgb;
            float luminance = dot(sampleColor, float3(0.299f, 0.587f, 0.114f));
            
            valueX += luminance * Gx[y + 1][x + 1];
            valueY += luminance * Gy[y + 1][x + 1];
        }
    }
    
    float edgeWeight = sqrt(valueX * valueX + valueY * valueY);
    float factor = smoothstep(threshold - 0.05f, threshold + 0.05f, edgeWeight);
    return lerp(color, outlineColor.rgb, factor * outlineColor.a);
}
```

---

## 2. ユーザー提案の特化エフェクト（2D調・絵画調）
送っていただいた参考画像（印象派・点描画のようなタッチ）や、「3Dを2D調にしたい」というご要望に合わせた、非常に見栄えが良く加点要素（最高20点）としても強力な専用エフェクト案です。

### 2.1. Impressionist / Pointillism Filter (印象派・点描画風フィルタ)
**概要**: 参考画像のように、細かい筆のタッチ（点描）で描かれたような絵画調に3Dの建物等の環境を変換するエフェクト。
**実装詳細**:
- `Noise.hlsli` などの既存のランダム関数を利用し、ピクセルごとにサンプリングUVをランダムな方向へ微小にずらします（筆のタッチのばらつきやキャンバスの凹凸を表現）。
- 取得した色に対してポスタリゼーション（階調化）を行うことで、絵の具のベタ塗り感や混ざり合わない点描特有の質感を強調します。
- さらに既存の「Kuwaharaフィルタ（油絵風）」のアルゴリズムを組み合わせることで、より自然な筆跡（ブラシストローク）を再現することも可能です。

```hlsl
// 点描画風の簡易アルゴリズムイメージ (PostProcess.hlsli 向け)
float32_t3 ApplyPointillism(float32_t3 color, float32_t2 uv, float32_t strokeSize, float32_t colorSteps, Texture2D<float32_t4> tex, SamplerState smp) {
    // 高周波ノイズでUVを散らす
    float2 noiseUV = uv * 500.0f; // キャンバスの細かさ
    float2 jitter = (float2(rand2dTo1d(noiseUV), rand2dTo1d(noiseUV + 1.0f)) - 0.5f) * strokeSize;
    
    // ずらした位置でサンプリング
    float3 sampleColor = tex.SampleLevel(smp, saturate(uv + jitter), 0).rgb;
    
    // 色数の削減 (階調飛び) で絵の具感を出す
    sampleColor = floor(sampleColor * colorSteps) / colorSteps;
    return sampleColor;
}
```

### 2.2. Toon / Cel-Shader PostProcess (2Dアニメ調フィルタ)
**概要**: 3Dの建物や背景を、フラットな2Dアニメやイラストのように変換するエフェクト。
**実装詳細**:
- 今回実装する `LuminanceBasedOutLine` (テクスチャ境界の線) と、既存の `DepthBasedOutline` (形状境界の線) を組み合わせて、画面全体に精細な「線画」を生成します。
- 線画の内側の色に対してポスタリゼーション（階調化）を適用し、3D特有の滑らかなグラデーションをなくしてアニメ塗りのような「ベタ塗り」にします。
- 既存の `HSV` エフェクトを併用し、彩度(S)を少し上げることで、2Dイラストらしい鮮やかな色合いに補正すると完璧です。

```hlsl
// ポスタリゼーション(階調化)の簡易関数 (PostProcess.hlsli 向け)
float32_t3 ApplyPosterization(float32_t3 color, float32_t steps) {
    return floor(color * steps) / steps;
}
```

### 2.3. Pixelation (ドット絵化・モザイクエフェクト)
**概要**: 3Dゲームを意図的に「レトロなドット絵調」や「PS1風」の低解像度に見せるエフェクト。
**実装詳細**:
- UV座標を強制的に「ブロック単位」で切り捨てる（量子化する）ことで、ピクセルが荒くなったように見せます。トゥーン調の線画やポスタリゼーションと組み合わせることで、完全なドット絵ゲームのようなビジュアルを作ることができます。

```hlsl
// ドット絵化(モザイク)の簡易関数 (PostProcess.hlsli 向け)
// pixelSize は「いくつのピクセルを1つのドットとして扱うか」(例: 4.0 なら4x4ピクセルが1ドットになる)
float32_t3 ApplyPixelation(float32_t2 uv, float32_t pixelSize, float32_t2 resolution, Texture2D<float32_t4> tex, SamplerState smp) {
    // 画面の分割数（ドットの数）を計算
    float2 blocks = resolution / pixelSize;
    // UVをブロック単位に切り捨てる
    float2 pixelatedUV = floor(uv * blocks) / blocks;
    
    // 丸められたUVでサンプリング
    return tex.SampleLevel(smp, pixelatedUV, 0).rgb;
}
```

---

## 3. その他の視覚演出・カメラエフェクト（汎用加点枠）
絵画調や2D調と組み合わせてさらに画面をリッチにするためのスパイス的なエフェクトです。

### 3.1. Night Vision (暗視ゴーグル風エフェクト)
**概要**: FPSゲーム等でよくある、暗闇を可視化する緑色のノイズ混じりカメラエフェクト。
**実装詳細**:
- 元の画像をモノクロ（輝度）に変換し、暗視カメラ特有の「明るい緑色（例: `float3(0.1, 0.95, 0.2)`）」を乗算して色付けする。
- 既存の `ApplyNoise` 関数を利用して、暗視カメラの「砂嵐ノイズ」を強めにブレンドする。
- スキャンライン（横シマ）や、画面の端を暗くする `ApplyVignette` を重ね合わせることで「ゴーグルを覗き込んでいる感」を出す。

```hlsl
// 暗視ゴーグルの簡易アルゴリズム (PostProcess.hlsli 向け)
float32_t3 ApplyNightVision(float32_t3 color, float32_t2 uv, float32_t time, float32_t noiseIntensity) {
    // 1. 輝度抽出と緑色化
    float luminance = dot(color, float3(0.299f, 0.587f, 0.114f));
    float3 nvColor = luminance * float3(0.1f, 0.95f, 0.2f);
    
    // 2. 露出の強調（暗いところを見やすく）
    nvColor = saturate(nvColor * 2.0f);
    
    // 3. ノイズの付加（既存のrand2dTo1dを利用）
    float noise = rand2dTo1d(uv * (time + 1.0f));
    nvColor += (noise - 0.5f) * noiseIntensity;
    
    // 4. スキャンライン（走査線）
    float scanline = sin(uv.y * 800.0f - time * 10.0f) * 0.05f;
    return saturate(nvColor - scanline);
}
```

### 3.2. Kaleidoscope / Compound Eye (万華鏡・虫の複眼エフェクト)
**概要**: 画面が幾何学模様に反射する万華鏡や、虫の目（複眼）を通したような歪んだマルチビジョン。幻覚演出や特殊な敵の視界ジャックなどに使える。
**実装詳細 (万華鏡 / Kaleidoscope)**:
- UV座標を画面中心を原点とした「極座標（角度と距離）」に変換する。
- 角度に対して `mod` や `abs` 演算を行うことで、一定角度ごとにUVが折り返される（鏡映しになる）ように計算し直してサンプリングする。

**実装詳細 (複眼 / Compound Eye)**:
- 画面を細かいグリッド（六角形や円）に分割する。
- 分割した各セルの中で、UV座標を中心に向かって歪ませる（樽型歪み）ことで、「小さなレンズが無数に並んでいる」ように見せる。

```hlsl
// 万華鏡エフェクトの簡易アルゴリズム (PostProcess.hlsli 向け)
float32_t3 ApplyKaleidoscope(float32_t2 uv, float32_t segments, Texture2D<float32_t4> tex, SamplerState smp) {
    // 画面中心を原点(0,0)とする
    float2 centeredUV = uv - 0.5f;
    
    // 極座標（距離と角度）に変換
    float radius = length(centeredUV);
    float angle = atan2(centeredUV.y, centeredUV.x);
    
    // 角度を分割数で折り返す
    float pi = 3.1415926535f;
    float segmentAngle = pi * 2.0f / segments;
    angle = fmod(angle, segmentAngle);
    angle = abs(angle - segmentAngle / 2.0f);
    
    // 直交座標（UV）に戻す
    float2 mappedUV = float2(cos(angle), sin(angle)) * radius + 0.5f;
    
    return tex.SampleLevel(smp, mappedUV, 0).rgb;
}
```

### 3.3. Chromatic Aberration (色収差)
**概要**: 画面端に行くほどRGBのチャンネルが少しずつズレて描画されるレンズの歪みエフェクト。ダッシュ時や被弾時の演出（ゲームへの利用）として組み込みやすい。
**実装詳細**:
```hlsl
float32_t3 ApplyChromaticAberration(float32_t3 color, float32_t2 uv, float32_t intensity, Texture2D<float32_t4> tex, SamplerState smp) {
    float2 centerDist = uv - 0.5f;
    float2 offset = centerDist * intensity;
    
    float r = tex.SampleLevel(smp, uv + offset, 0).r;
    float g = color.g;
    float b = tex.SampleLevel(smp, uv - offset, 0).b;
    return float32_t3(r, g, b);
}
```

### 3.4. Depth of Field (被写界深度 / DoF)
**概要**: カメラのピントが合っている位置（フォーカス距離）はくっきり表示し、それより手前や奥の景色をぼかすエフェクト。リッチな空間表現として非常に評価が高い（難易度高）。
**実装詳細**:
- すでにエンジン内に `DualKawaseBlur`（または Gaussian）が実装されているため、その「ぼかした画像」を `extraTextureIndex` 等で渡す。
- 現在のピクセルの深度値を `depthTex` から取得し、フォーカスしたい深度との差分（Circle of Confusion）を計算。
- 差分が大きいほど「ぼけた画像」のブレンド率を上げ、小さいほど「元のシャープな画像」をそのまま出力する。
```hlsl
// 簡易ロジックイメージ
// float depth = depthTex.SampleLevel(smpPoint, uv, 0).r;
// float blurAmount = smoothstep(focusRange - falloff, focusRange + falloff, abs(z - focusDistance));
// return lerp(sharpColor, blurredColor, blurAmount);
```

### 3.5. Light Shafts (ゴッドレイ / 太陽光の筋)
**概要**: 太陽や強い光源から光の筋が漏れるエフェクト。神々しい表現が可能。
**実装詳細**:
- `RadialBlur` のロジックを応用できる。
- 太陽（光源）のスクリーン座標（UV）を計算してシェーダーに渡す。
- 高輝度部分だけを抽出したテクスチャ（Bloomの `bloomExtract` パス等の結果）をソースとし、光源座標を中心とした `RadialBlur` を掛けて元の画像に加算する。

---

## 4. 映像編集ソフト(AE/Pr)由来の定番エフェクト候補
映像制作でよく使われる基本的なエフェクトのうち、現在未実装でゲームにも転用しやすい強力な候補です。

### 4.1. Displacement Map (ディスプレイスメントマップ / 画面の歪み)
**概要**: AEなどで「ノイズ画像」を読み込ませて画面を歪ませる定番エフェクト。ゲームでは「水中のゆらめき」や「炎の陽炎（ヒートヘイズ）」として必須級の技術です。
**実装詳細**:
- ノイズテクスチャ（または既存のランダム関数）を利用し、時間経過でスクロールするノイズ値を取得。
- そのノイズ値をサンプリング用のUV座標に直接足し合わせる（UVを歪ませる）ことで、画像全体がうねるように見せます。
```hlsl
// ディスプレイスメントマップ(陽炎)の簡易アルゴリズム
float32_t3 ApplyDisplacement(float32_t2 uv, float32_t time, float32_t distortionStrength, Texture2D<float32_t4> tex, SamplerState smp) {
    // 既存のノイズ関数などを使って波打つUVオフセットを生成
    float2 noiseOffset = float2(sin(uv.y * 50.0f + time * 5.0f), cos(uv.x * 50.0f + time * 4.0f));
    float2 distortedUV = uv + noiseOffset * distortionStrength;
    
    return tex.SampleLevel(smp, distortedUV, 0).rgb;
}
```

### 4.2. Directional Blur (方向ブラー / モーションブラー)
**概要**: 指定した一方向（例：横方向のみ、斜め方向のみ）に向かって画像をぼかすエフェクト。ダッシュ時のスピード線や、横に高速移動するオブジェクトに掛けます。
**実装詳細**:
- `RadialBlur` は「中心から外側」へ放射状にサンプリングしますが、こちらは「指定したベクトル（X,Y）」に向かってサンプリング位置をずらしながら平均化します。
```hlsl
// 方向ブラーの簡易アルゴリズム
float32_t3 ApplyDirectionalBlur(float32_t2 uv, float32_t2 direction, float32_t strength, Texture2D<float32_t4> tex, SamplerState smp) {
    float3 result = 0;
    int samples = 10;
    for (int i = 0; i < samples; ++i) {
        float2 offset = direction * (i / (float)samples - 0.5f) * strength;
        result += tex.SampleLevel(smp, uv + offset, 0).rgb;
    }
    return result / samples;
}
```

### 4.3. Color Grading / 3D LUT (カラーグレーディング / トーンカーブ補正)
**概要**: AEの「Lumetriカラー」やトーンカーブのように、画面全体の色調（シャドウ・ミッドトーン・ハイライト）を映画のように一気に補正するエフェクト。
**実装詳細**:
- 本格的にやる場合は、Photoshop等で作った「3D LUT（色の変換テーブル）」のテクスチャを読み込み、現在の画面の色（RGB）をXYZ座標とみなしてテクスチャから色を引き直すという手法を使います。
- 簡易的にやるなら、セピアやHSV変換を統合して `ApplyColorBalance(color, shadowTint, highlightTint)` のように影と光の色を個別に設定できる関数を作ります。

### 4.4. Halftone (ハーフトーン / 漫画風スクリーントーン)
**概要**: 印刷物やアメコミのように、画像の明るさを「ドット（丸）の大きさ」で表現するエフェクト。
**実装詳細**:
- `Pixelation` に似ていますが、ブロック内の「輝度」を計算し、輝度が暗いほど大きな黒い円を描画する計算を行います。

---

## 5. 応用: 特定のオブジェクトやマテリアルへのエフェクト適用
これらポストエフェクト用に作成した関数群（`PostProcess.hlsli`）は、**画面全体だけでなく、単一の3Dオブジェクトやパーティクルに対しても直接使用することが可能です。**

### オブジェクトへの適用の実装状況と今後のタスク
現在、**「エフェクトを計算するための純粋な関数」自体は `PostProcess.hlsli` に完璧に用意されています**が、それを**「オブジェクトを描画するシェーダー側で呼び出す仕組み」はまだ未実装**です。

オブジェクト単体に適用させるには、メイン環境に戻った際に以下の手順でシステムを繋ぎ合わせる（ワイヤリングする）必要があります。

**【実装手順プラン】**
1. **C++側 (マテリアル構造体の拡張)**:
   - `Material` や `Particle` などのコンスタントバッファ（`MaterialData`）に、エフェクトの種類を指定するフラグ（例: `int32_t effectType;`）や強度パラメータ（`float effectIntensity;`）を追加します。
2. **HLSL側 (`Material.hlsli` 等の更新)**:
   - C++側の変更に合わせて、HLSL側の `MaterialData` 構造体にも同じ変数を追加し、データレイアウトを一致させます。
3. **HLSL側 (`Object3d.PS.hlsl` や `Particle.PS.hlsl` の更新)**:
   - ファイルの先頭に `#include "PostProcess.hlsli"` を追加して関数群を読み込みます。
   - テクスチャをサンプリングした後、`if (gMaterial.effectType == 1)` のようにマテリアルの設定を判定し、`ApplyGlitch` や `ApplyPixelation` を呼び出して出力する色を書き換えます。

**【活用例（ReadMeへのアピールポイントとして極めて有効）】**:
1. **壊れたテレビやモニター**: マテリアルの `effectType` を「グリッチ」に設定するだけで、そのオブジェクトの画面部分だけがバグった表現になります。
2. **魔法や攻撃のリッチなエフェクト (マテリアルエフェクト)**: 斬撃のパーティクルに `ApplyChromaticAberration` (色収差) を掛けて、他にはない独特なエフェクトを作成。
3. **背景のゆらぎ**: 背景のテクスチャ（バックバッファコピー）を用意し、オブジェクトのシェーダー内で `ApplyDisplacement` (ディスプレイスメントマップ) のUVズレなどを応用することで「光学迷彩」や「陽炎」をオブジェクト単位で表現。

---

## 6. アーキテクチャ進化: 「レイヤー別エフェクト管理」の実装計画
（カン君のプレゼン資料に基づく新しいポストプロセス構造の統合プラン）

現在エンジンに実装されているのは、画面全体に一括してエフェクトを掛ける旧来の `PostProcessManager` のみであり、**スライドにあった「レイヤー別管理（PostProcessRunner等）」はまだこのソースコードに統合（マージ）されていません。**

メイン環境に戻った際、既存のエフェクト資産を壊さずに「レイヤー別のポストプロセス切り替え機能」を実装するための設計プランです。

### 6.1. クラス構造の再設計 (C++)
スライドの設計通り、エフェクトを完全に「パス(Pass)」としてオブジェクト指向化し、それらを「レイヤー(Layer)」で管理する構造を実装します。

1. **`IPostProcessPass` 基底クラスの作成**:
   - 既存の `DrawSinglePass` や `Bloom` のロジックを、独立した `Pass` クラス（`class BlurPass`, `class GlitchPass` など）にカプセル化します。
   - `Execute(commandList, RenderTarget* src, RenderTarget* dst)` のような純粋仮想関数を持たせます。
2. **`RenderLayer` クラスの作成**:
   - 各レイヤーは「独自のフルスクリーンテクスチャ（ボード）」と「自身に適用される Pass のリスト」を持ちます。
   - 例えば、「Layer0: 背景」「Layer1: キャラ」「Layer2: UI」のように定義し、オブジェクト描画時にどの Layer のテクスチャに書き込むかを切り替えます。
3. **`PostProcessRunner` クラスの作成**:
   - `PostProcessManager` に代わる上位の管理クラス。
   - 全てのレイヤーを束ね、最終的にすべてのレイヤーのテクスチャを合成（Composite）してバックバッファ（画面）に出力します。

### 6.2. レイヤーごとのエフェクトON/OFF切り替え機能
- **機能要件**: 「UIレイヤーには一切エフェクトを掛けない」「背景レイヤーにはDoFと色収差を掛ける」「キャラレイヤーにはアウトラインだけ掛ける」といった切り替えを動的に行える仕組みを作ります。
- **実装方針**:
  ```cpp
  // 使用イメージ (C++側)
  auto runner = engine->GetPostProcessRunner();
  
  // 背景レイヤー (Layer 0) にエフェクトを追加
  runner->SetPass(0, blurPassCommand_); 
  runner->SetPass(0, chromaticAberrationCommand_);
  
  // キャラレイヤー (Layer 1) にエフェクトを追加
  runner->SetPass(1, outlineCommand_);
  
  // UIレイヤー (Layer 2) はエフェクトなし
  runner->ClearPass(2);
  ```

### 6.3. メモリ(VRAM)の最適化戦略
レイヤー分けを行うと、レイヤーの数だけ画面サイズのレンダーターゲット（ボード）が必要になります。これを最小限に抑えるための最適化も組み込みます。
- **Ping-Pong テクスチャの共有**:
  各レイヤーごとに専用のPing-Pongテクスチャ（2枚）を確保するのではなく、**「エフェクト計算用のPing-Pongテクスチャ」はグローバルで2枚だけ用意し、全てのレイヤーで使い回す**設計にします。
  これにより、「各レイヤーの最終結果を保存するテクスチャ」さえ確保すればよく、エフェクト途中計算のメモリ増大を完全に防げます。
