# Irufemi Engine

[![DebugBuild](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml)
[![ReleaseBuild](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml)
[![DevelopmentBuild](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml)
[![CheckUnwantedFiles](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml/badge.svg)](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml)

C++ と DirectX 12 を用いてスクラッチから構築した、**GPU-Driven Rendering** および **Data-Oriented Design** 指向の自作3Dゲームエンジンです。
商用AAAゲームエンジンにおけるパフォーマンス要求（ロード時間の極小化、万単位の動的オブジェクトの物理演算、空間分割による最適化）をクリアするための、最新アーキテクチャの実証を目的としています。

---

## 🎮 ショーケース：Irufemi Engineで開発されたゲーム群

本エンジンの最大の強みは、「技術的なデモ」に留まらず、実際に**累計個人5回、チーム9回のゲーム制作**の基盤として利用され、他職種（プランナーやデザイナー）を含むチーム開発のイテレーションを支えた「拡張性と実用性の高さ」にあります。

| プロジェクト名 | 概要・ジャンル | 開発体制・期間 | 実装した主なエンジン機能 |
| :--- | :--- | :--- | :--- |
| **Gravity Shooter** | 奥スクロール型 重力シューティング | 1人 (約4ヶ月) | Fake Physicsによる1万個のガレキ制御、Dynamic BVH衝突判定、マルチレイヤーアウトライン |
| **七転び八転び** | 3Dアクションゲーム | 4人 (約4ヶ月) | 非同期ローダー、Compute Shaderによる数百万ボクセル破壊、シーン管理・リソース管理 |
| **血管壊回** | 見下ろし型 疑似3Dアクション | 3人 (約1ヶ月) | 3Dパーティクルによる血流表現、XY平面ベースの2D/3D融合コリジョン処理 |
| **纏当て** | 奥スクロール型 重力シューティング | 4人 (約1ヶ月) | DirectX 12 描画パイプラインの構築、デバッグUI環境の提供、描画用HLSLシェーダー |

※上記ゲームの具体的なグラフィックやシステムについては、以下の技術解説をご参照ください。

---

## 📊 技術的アピールポイント（最適化とモダンアーキテクチャ）

本エンジンにおける中核技術について、「実現したかった要件」「比較検討したトレードオフ」「結果と定量データ」の構成で解説します。

### <a id="desc-perf1"></a>🔹 1. 「処理落ち」の限界を広げるパフォーマンス最適化
> **【要約】1万個のガレキと数百万のボクセル破壊を両立する Data-Oriented 設計**

- **【実現したかったこと】**
  『Gravity Shooter』において「周囲の無数のガレキを引き寄せて敵に放つ」爽快感を実現するため、画面上に1万個を超えるオブジェクトを出す必要があった。また、『七転び八転び』では建物を壊す数百万の破片（ボクセル）を描画したかった。
- **【比較と技術選定 (トレードオフ)】**
  ポインタベースの `GameObject` (OOP) では、メモリ断片化によるキャッシュミスでCPUの限界（16.6ms）がすぐに訪れる。そこで、ゲーム進行に必要な当たり判定（OBB等）はCPUに残し、視覚的な「大量の破片の物理演算」のみを Compute Shader に完全オフロードした。さらに、1万個のガレキの引き寄せ等の演算は真面目に行わず `sin/cos` を用いた「騙しの物理演算（Fake Physics）」で軽量化し、座標データは密配列で管理する `Sparse Set` アーキテクチャを採用した。
- **【結果と得られた知見】**
  画面上に1万個のガレキが存在しても処理落ちしない極限のパフォーマンスを達成。CPU側のディスパッチ負荷を **平均0.066ms** に抑えることに成功し、破壊の瞬間に1～2フレームのみ30FPS相当に落ちるものの即座に 60FPS へ復帰し、ゲームのテンポを維持した。

> **📷 計測環境データ**
> ※ここに「1万個のガレキが飛んでいる画面」と「60FPSが出ているデバッグ表記」のスクリーンショットを配置してください。
> 計測条件：Releaseビルド, RTX 2060, オブジェクト数10,000個

**【図解: Sparse SetとInstance Promotionによるキャッシュ最適化】**
```mermaid
graph TD
    subgraph Data-Oriented [Virtual Entity 密配列キャッシュ]
        Dense[連続メモリ上の座標データ配列<br/>キャッシュヒット率: 極高]
    end
    subgraph Object-Oriented [GameObject プール]
        GO[実体化されたGameObject群<br/>コンポーネントによる複雑な処理]
    end
    Dense -->|干渉時のみ Promote() で実体化| GO
    GO -->|不要になれば Demote() で返却| Dense
    style Dense fill:#2b6cb0,stroke:#3182ce,stroke-width:2px,color:#fff
    style GO fill:#2d3748,stroke:#4a5568,stroke-width:1px,color:#a0aec0
```

---

### <a id="desc-bvh"></a>🔹 2. Dynamic BVH とハードコードの排除
> **【要約】O(N log N)の動的AABBツリーとデータ駆動設計による衝突判定**

- **【実現したかったこと】**
  マップごとの敵の出現位置や大量の背景をコードに直書きすると保守性が下がる。また、万単位のオブジェクトによる総当たり衝突判定（$O(N^2)$）のCPU負荷が課題だった。
- **【比較と技術選定 (トレードオフ)】**
  衝突判定を $O(N \log N)$ に落とすため動的AABBツリー (BVH) を実装。さらに、ツリーのノード管理をポインタで実装するとキャッシュミスが起きるため、あえてポインタを排し **「インデックスベースの配列プール」** を用いた Data-Oriented な実装を選択した。また、ウェーブや環境は JSON/CSV で外部定義し、`EnvironmentManager` でプーリングとバッチ描画（インスタンシング）を自動化するデータ駆動設計を導入した。
- **【結果と得られた知見】**
  プログラマ視点での高い保守性とシーンファイルの肥大化防止を達成。大量のガレキが密集してもFPSが落ちない圧倒的な実行速度（CPUバウンドの解消）を実現した。

> **📷 計測環境データ**
> ※ここに「BVHのデバッグ描画（四角い枠）」の画面か「`std::vector<BVHNode>` でプール管理しているコード」のスクリーンショットを配置してください。

---

### <a id="desc-memory"></a>🔹 3. 開発を止めない堅牢なメモリ管理と非同期スレッド制御
> **【要約】独自スレッドプールによる非同期ローダーとゼロ・アロケーション基盤**

- **【実現したかったこと】**
  高解像度テクスチャのロードがメインスレッドを占有し、アクションゲームで最も重要な「テンポ」を削ぐ数秒間のフリーズ（スパイク）を無くすこと。
- **【比較と技術選定 (トレードオフ)】**
  OS依存の `std::async` ではなく、ゲーム進行を阻害しないよう自前でワーカースレッド数と待機状態を完全制御できるスレッドプールを `std::mutex` を用いてスクラッチ実装。また、毎フレームの動的確保によるヒッチングを防ぐため、起動時に一括確保した `ObjectPool` （ゼロアロケーション）と `generation` (世代) 付き Handle でライフサイクルを管理する手法を導入した。
- **【結果と得られた知見】**
  ロード中の最大スパイクを **約4100ms から 約51ms に激減**。ローディング画面のアニメーションを一切阻害しないシームレスなUXを実現。また、古い世代のHandleアクセスを `nullptr` で弾くため、マルチスレッド環境下でも絶対にクラッシュしない堅牢なメモリ基盤を構築できた。

**【図解: 世代付きHandleによるダングリングポインタの防御】**
```mermaid
graph LR
    subgraph ObjectPool [ObjectPool メモリプール]
        Slot0[Slot 0<br/>Gen: 1, Active]
        Slot1[Slot 1<br/>Gen: 2, Free (next: 2)]
        Slot2[Slot 2<br/>Gen: 1, Free (next: -1)]
    end
    User[ユーザーのHandle<br/>Index: 1, Gen: 1] -.->|古い世代でアクセス| Slot1
    Slot1 -.->|世代アンマッチで<br/>安全に弾く (nullptr返却)| User
    style Slot0 fill:#2b6cb0,color:#fff
    style Slot1 fill:#c53030,color:#fff
    style Slot2 fill:#2d3748,color:#fff
```

---

### <a id="desc-graphics"></a>🔹 4. 高度なグラフィックス表現 (アウトラインと立体的爆煙)
> **【要約】G-Bufferへの直接アクセスとレイマーチングの事前計算最適化**

- **【実現したかったこと】**
  ダークで退廃的なサイバーパンク風世界観を表現するため、単なる3Dモデルではなく手描きコミックのようなリッチな線画表現が必要だった。また、ボスの大迫力な攻撃を表現するため、内部で光が散乱する「煙の厚み（ボリュームレンダリング）」を描画したかった。
- **【比較と技術選定 (トレードオフ)】**
  単一の輪郭線抽出ではなく、「深度（シルエット）」「法線」「輝度」の3つの G-Buffer から複合的にエッジを抽出するマルチレイヤー・アウトラインシェーダーを独自に実装。さらにノーマルマップでオフスクリーンバッファを歪ませる屈折シェーダーにより重力場を表現。煙については、HLSLによるレイマーチングを実装し、負荷を下げるため煙を覆う「境界球（Bounding Sphere）」との交差判定を数学的に事前計算してサンプリング区間を最小限に絞り込んだ。
- **【結果と得られた知見】**
  自作エンジン（RenderGraph）だからこそ可能な G-Buffer への直接アクセスを活かし、3D空間ノイズ（FBM等）を用いた濃密な爆煙の質感を、60FPSの処理時間内に収まるパフォーマンスで実現した。

> **📷 計測環境データ**
> ※ここに「アウトラインや空間の歪みが綺麗にかかっているゲーム画面のアップ」と「可能なら 3つのG-Buffer の白黒サムネイル画像」を配置してください。

---

### <a id="desc-manual"></a>🔹 5. チーム開発を成功に導くツールとドキュメンテーション
> **【要約】Facadeパターン、リフレクションUI、および2万文字の開発ガイドライン**

- **【実現したかったこと】**
  チーム開発において、高度なGPU処理を知らないプランナーや他プログラマーでも、ゲームを実行したままリアルタイムにバランス調整ができ、安全かつ効率的に開発を進められるようにしたかった。
- **【比較と技術選定 (トレードオフ)】**
  パラメータを外部ファイルに手書きする手法はヒューマンエラーが起きやすい。そこで、C++側で変数を登録するだけで自動的にImGuiのインスペクタUIが生成されるリフレクション基盤と、複雑なGPU処理を1行で呼び出せる Facade パターンによるAPI設計（例: `Engine::SpawnEffect`）を提供。また、暗黙の了解を防ぐため「APIの使い方」や「遅延削除のルール」をまとめた独立した **Manual (取扱説明書)** を作成した。
- **【結果と得られた知見】**
  再コンパイルの待ち時間が消滅し、チーム全体のイテレーション速度が劇的に向上。**2万文字規模**のマニュアルを執筆し、「作って、試して、共有する」エンジニアとしてチームの開発基盤を強固に支えることができた。
- 📄 **関連ドキュメント原本**: [`Manual.md`](Manual.md)

---

## 📁 プロジェクト構成 (Project Structure)

本ソリューション (`Irufemi.sln`) は、エンジンコアとアプリケーション（ゲームロジック）を完全に分離（関心の分離）した設計になっています。

```text
WP0/
 ├── project/
 │    ├── IrufemiEngine/            [⚙️ エンジンコア・レイヤー (特定のゲーム依存なし)]
 │    │    ├── Engine/              - DirectX12ラッパー, スレッドプール, 各種Manager
 │    │    ├── Renderer/            - RenderGraph, GPU Particle等の描画パイプライン
 │    │    ├── Resource/            - ResourceHandleシステムを用いたテクスチャ・モデル管理
 │    │    └── Framework/           - GameObjectとComponentを用いたECS基盤, 衝突判定
 │    ├── IrufemiEditor/            [🛠️ エディタUI・ツール (ImGuiベース)]
 │    ├── Application_solo/         [🎮 ゲームアプリケーション・レイヤー (Gravity Shooter 等)]
 │    │    ├── components/          - ゲーム固有の振る舞い (Player, Enemy 等)
 │    │    ├── scene/               - 各シーンの初期化と状態管理
 │    │    └── resources/           - 専用のテクスチャ, モデル, JSONアセット群
 │    ├── Application_team/         [🎮 ゲームアプリケーション・レイヤー (チーム制作用)]
 │    ├── externals/                [📦 サードパーティライブラリ]
 │    └── Irufemi.sln               [🔧 Visual Studio 2026 ソリューション]
 ├── Manual.md                      [📖 チーム向けAPI取扱説明書 (2万文字)]
 └── README.md                      [📖 アーキテクチャ解説ドキュメント (当ファイル)]
```

---

## 📖 API利用例 (API Usage)

チームメンバーが直感的に利用できるように設計されたAPIの一部です。

### 1. リフレクションによるエディタUIの自動生成
変数を登録するだけで、ImGuiのインスペクタにUIが自動生成され、実行中のパラメータ変更とJSON保存が可能になります。

```cpp
#include "Framework/Component/Component.h"

class PlayerStatusComponent : public Component {
public:
    int hp_ = 100;
    float speed_ = 5.0f;

    std::string GetComponentName() const override { return "PlayerStatusComponent"; }

    void OnRegisterProperties() override {
        // これだけで Inspector に反映される
        RegisterProperty("Max HP", &hp_);
        RegisterProperty("Move Speed", &speed_);
    }
};
```

### 2. スレッドプールを用いた非同期レイキャスト (Async Raycast)
メインスレッドを停止させない分散処理（Time-Slicing）による当たり判定の例です。

```cpp
// 非同期でレイキャストを発行 (RaycastAsync)
Ray ray = { cameraPos, dir };
cache.pendingTask = std::make_shared<std::future<std::pair<bool, RaycastHit>>>(
    engine->GetCollisionManager()->RaycastAsync(engine->GetThreadPool(), ray, maxDistance, layerMask)
);

// 別のフレームで結果をポーリング (ノンブロッキング)
if (cache.pendingTask && cache.pendingTask->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    auto result = cache.pendingTask->get();
    bool hit = result.first;
    RaycastHit hitInfo = result.second;
    cache.pendingTask.reset();
}
```

---

## ⚙️ 動作環境・計測環境

- **OS**: Windows 10 / 11
- **IDE**: Visual Studio 2026
- **SDK**: Windows SDK 10.0.26100.7175 以上推奨
- **計測環境**: 
  - CPU: Intel Core i7 / AMD Ryzen 7 相当以上
  - GPU: NVIDIA RTX 2060 相当以上を想定

**ビルド手順**:
1. リポジトリをクローン後、VS2026で `Irufemi.sln` を開きます。
2. 構成を選択しビルドを実行します。
  - `Debug` / `Development`: エンジン実行時に動的コンパイルされ、迅速なイテレーションが可能。
  - `Release`: Visual Studio の PreBuild イベントで最高レベルの最適化 (`/O3`) を適用したオフラインコンパイルが行われます。

---

## 📚 使用ライブラリ

- [Assimp](https://github.com/assimp/assimp)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [DirectX 12 Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/)

---

## 📄 ライセンス

このエンジンのソースコードは [MIT License](LICENSE.txt) の下で提供されています。各ライブラリのライセンスについては個別の規定に従ってください。
