# Irufemi Engine

[![DebugBuild](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml)
[![ReleaseBuild](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml)
[![DevelopmentBuild](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml)
[![CheckUnwantedFiles](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml/badge.svg)](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml)

C++ と DirectX 12 を用いてスクラッチから構築した、**GPU-Driven Rendering** および **Data-Oriented Design** 指向の自作3Dゲームエンジンです。
グラフィックスプログラミングの学習に留まらず、商用AAAゲームエンジンにおけるパフォーマンス要求（CPU-GPU同期、ドローコールの極小化、大量オブジェクトの処理）をクリアするための最新アーキテクチャの実証を目的としています。

---

## 📊 技術的アピールポイント（最適化とモダンアーキテクチャ）

本エンジンにおける「開発体験（DX）」と「パフォーマンス」を劇的に向上させた中核技術です。
各項目の「詳細解説」から、採用した技術の背景（トレードオフ）と具体的な定量成果をご覧いただけます。

| 技術アピール・ドキュメント | 要約・実現したこと | 詳細解説 | 実装ソース / ファイル |
| :--- | :--- | :---: | :---: |
| **🚀 直感的な専用エディタ** | リフレクションとFacadeパターンによるUI/API設計で、プログラマーのイテレーションを高速化 | [👇 解説へ](#desc-editor) | [📄 `Component.h`](project/IrufemiEngine/Framework/Component/Component.h) |
| **🏎️ パフォーマンス最適化** | Sparse Set と ComputeShader オフロードで数百万のボクセル破壊と処理落ちを解消 | [👇 解説へ](#desc-perf) | [📄 `VirtualEntityManagerComponent.h`](project/IrufemiEngine/Framework/Component/VirtualEntity/VirtualEntityManagerComponent.h) |
| **🛡️ メモリ・非同期スレッド制御** | ゼロアロケーションと独自スレッドプールによる、ロードスパイクの激減（4100ms→51ms）とクラッシュ防御 | [👇 解説へ](#desc-memory) | [📄 `ObjectPool.h`](project/IrufemiEngine/Engine/Core/Utility/ObjectPool.h) |
| **🔥 高度なグラフィックス表現** | レイマーチングの事前計算による最適化を用いた、ボリュームレンダリング（立体的爆煙）の実装 | [👇 解説へ](#desc-graphics) | [📄 `ParticleGPU.PS.hlsl`](project/IrufemiEngine/EngineResources/shaders/ParticleGPU.PS.hlsl) |
| **📚 チーム開発取扱説明書** | チームメンバーが迷わず安全に開発できる、2万文字規模のAPIとメモリ管理ルールの明文化 | [👇 解説へ](#desc-manual) | [📄 `Manual.md`](Manual.md) |

---

#### <a id="desc-editor"></a>🔹 1. プログラマー自身のイテレーションを極限まで早めるエディタ環境
> **【要約】リフレクション基盤とFacadeパターンの直感的なAPI設計**
- **【実現したかったこと】**
  プログラマー同士での開発において、パラメータ調整のたびに「C++コードを書き直して再ビルドする手間」をなくし、かつ高度なGPU処理を知らないメンバーでも簡単にリッチな演出を出せるようにすること。
- **【比較と技術選定 (トレードオフ)】**
  パラメータを外部ファイル（JSON等）に直接手書きする手法はヒューマンエラーが起きやすい。そこで、C++側で変数を登録するだけで自動的にImGuiのインスペクタUIが生成・保存されるデータ駆動設計を採用。さらに、Facadeパターンを用いて複雑なGPU処理を隠蔽し、1行呼ぶだけで演出を出せる直感的なAPI（例: `Engine::SpawnEffect`）を提供した。
- **【結果と得られた知見】**
  ゲームを実行したままリアルタイムにバランス調整が可能になり、再コンパイルの待ち時間が消滅。プログラマーとしての開発体験（DX: Developer Experience）とチーム全体のイテレーション速度が劇的に向上した。
- 🔗 **関連コード原本**: [`Component.h`](project/IrufemiEngine/Framework/Component/Component.h)

**【最小限のコード抜粋: リフレクションによるUI自動生成】**
```cpp
class PlayerStatusComponent : public Component {
public:
    int hp_ = 100;
    float speed_ = 5.0f;

    // 変数を登録するだけで、ImGuiのインスペクタにUIが自動生成され、JSONにも自動保存される
    void OnRegisterProperties() override {
        RegisterProperty("Max HP", &hp_);
        RegisterProperty("Move Speed", &speed_);
    }
};
```

---

#### <a id="desc-perf"></a>🔹 2. 「処理落ち」の限界を広げるためのパフォーマンス最適化
> **【要約】Compute Shaderへの完全委譲とSparse SetによるData-Oriented ECSの構築**
- **【実現したかったこと】**
  ゲームの表現をリッチにするため、画面上に「数百万のボクセル破壊（破片）」を発生させたい。しかし、単に出現数を増やすと物理演算がCPUの処理限界（16.6ms）を超過し、極端な処理落ちが発生してしまうため、その限界を打ち破りたかった。
- **【比較と技術選定 (トレードオフ)】**
  ポインタベースの `GameObject` では、メモリ断片化によるキャッシュミスでCPUの限界がすぐに訪れる。そこで、ゲーム進行に必要な当たり判定（OBB等）はCPUに残し、視覚的な「大量の破片の物理演算」のみを Compute Shader に完全オフロードする描画特化の設計へ移行。さらに座標データは密配列で管理する `Sparse Set` アーキテクチャを採用した。
- **【結果と得られた知見】**
  CPU側のディスパッチ負荷を **平均0.066ms** に抑えることに成功。破壊の瞬間に1～2フレームのみ30FPS相当に落ちるものの、即座に 60FPS へ復帰し、ゲームのテンポを維持したまま表現の幅を大きく広げることができた。
- 🔗 **関連コード原本**: [`VirtualEntityManagerComponent.h`](project/IrufemiEngine/Framework/Component/VirtualEntity/VirtualEntityManagerComponent.h)

**【図解: Sparse SetとInstance Promotionによるキャッシュ最適化】**
```mermaid
graph TD
    subgraph Data-Oriented [Virtual Entity 密配列キャッシュ]
        Dense[連続メモリ上の座標データ配列<br/>キャッシュヒット率: 激高]
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

#### <a id="desc-memory"></a>🔹 3. 開発を止めないための堅牢なメモリ管理と非同期スレッド制御
> **【要約】ゼロアロケーション基盤と、独自スレッドプールによる安全な非同期ローダー**
- **【実現したかったこと】**
  高解像度テクスチャのロードがメインスレッドを占有し、アクションゲームで最も重要な「テンポ」を削ぐ数秒間のフリーズ（スパイク）を無くすこと。また、非同期処理中に発生する別スレッドからの競合・クラッシュを完全に防ぎたかった。
- **【比較と技術選定 (トレードオフ)】**
  OS依存の `std::async` ではなく、ゲーム進行を阻害しないよう自前でワーカースレッド数と待機状態を完全制御できるスレッドプールを `std::mutex` を用いてスクラッチ実装。また、全てを `std::shared_ptr` で都度確保するとゲーム中のアロケーション負荷がカクつきの原因になるため、起動時に一括確保した `ObjectPool` （ゼロアロケーション）と `generation` 付き Handle でライフサイクルを安全に管理する手法を導入した。
- **【結果と得られた知見】**
  ロード中の最大スパイクを **約4100ms から 約51ms に激減**。ローディング画面のアニメーションを一切阻害しないシームレスなUXを実現した。また、非同期スレッドで処理中にメインスレッドでオブジェクトが破棄されても絶対にクラッシュしない、極めて堅牢な非同期基盤を実現できた。
- 🔗 **関連コード原本**: [`ObjectPool.h`](project/IrufemiEngine/Engine/Core/Utility/ObjectPool.h)

**【図解: Intrusive Free List と 世代付きHandleによる安全なメモリ管理】**
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

#### <a id="desc-graphics"></a>🔹 4. 高度なグラフィックス表現 (ボリュームレンダリング最適化)
> **【要約】レイマーチングの事前計算による立体的爆煙表現**
- **【実現したかったこと】**
  大迫力な攻撃を表現するため、従来の板ポリゴンでは不可能な「煙の厚み」や「内部で光が散乱する自己遮蔽」を描画したかった。
- **【比較と技術選定 (トレードオフ)】**
  GPU負荷が上がるリスクを承知の上で、HLSLによるボリュームレンダリング（レイマーチング）をスクラッチで実装。負荷を極限まで下げるため、煙を覆う「境界球（Bounding Sphere）」との交差判定を数学的に事前計算し、レイを飛ばすサンプリング区間を最小限に絞り込む最適化を施した。
- **【結果と得られた知見】**
  3D空間ノイズ（FBM等）を用いた濃密な爆煙の質感を、許容フレームレート（処理時間内）に収まるパフォーマンスで実現できた。
- 🔗 **関連シェーダー**: [`ParticleGPU.PS.hlsl`](project/IrufemiEngine/EngineResources/shaders/ParticleGPU.PS.hlsl)

---

#### <a id="desc-manual"></a>🔹 5. チーム開発を成功に導くドキュメンテーション
> **【要約】暗黙の了解を明文化し、属人化を防ぐ「2万文字規模」の開発ガイドライン**
- **【実現したかったこと】**
  チーム開発において、他のプログラマーがエンジンの仕様やメモリ管理のルールに迷わず、安全かつ効率的に開発を進められるようにしたかった。
- **【比較と技術選定 (トレードオフ)】**
  ソースコードのコメントだけで伝える手法は書くのが楽だが、属人化しやすく見落とされがち。そこで、知識の資産化として「APIの使い方のベストプラクティス」や「遅延削除のルール」などをまとめた独立した **Manual (取扱説明書)** を作成し、随時更新する手法を選択。
- **【結果と得られた知見】**
  **2万文字規模**のマニュアルを執筆。「作って、試して、共有する」イテレーションを高速化し、エンジニアとしてチームの開発基盤を強固に支えることができた。
- 📄 **関連ドキュメント原本**: [`Manual.md`](Manual.md)

---

## 📁 プロジェクト構成 (Project Structure)

本ソリューション (`Irufemi.sln`) は、エンジンコアとアプリケーション（ゲームロジック）、およびツール群を完全に分離（関心の分離）した以下の4プロジェクトで構成されています。

| プロジェクト | 種別 | 役割 |
| :--- | :--- | :--- |
| **IrufemiEngine** | 静的ライブラリ (.lib) | 描画・物理・リソース・コンポーネント基盤を提供するエンジンコア |
| **IrufemiEditor** | 静的ライブラリ (.lib) | ImGuiベースのレベルエディタ・デバッグツール群 |
| **Application_solo** | 実行ファイル (.exe) | 個人制作ゲームのロジック・固有シーン・アセンブリ |
| **Application_team** | 実行ファイル (.exe) | チーム制作ゲームのロジック・固有シーン・アセンブリ |

### ⚙️ IrufemiEngine (`project/IrufemiEngine/`)
エンジンのコアモジュール群です。特定のゲームに依存する処理は一切含みません。

| ディレクトリ | 役割 |
| :--- | :--- |
| **Engine/** | DirectX12ラッパー、ウィンドウ管理、スレッドプール、各種Manager群 |
| **Renderer/** | RenderGraph、Object3D描画、GPU Particle等の描画パイプライン |
| **Resource/** | ResourceHandleシステムを用いたテクスチャ・モデル・オーディオの安全な管理 |
| **Framework/** | GameObject と Component を用いた軽量かつ高速なECS基盤 |

### 🎮 Application (`project/Application_solo/` 等)
ゲーム固有のロジックとリソースを格納します。

| ディレクトリ | 役割 |
| :--- | :--- |
| **components/** | ゲーム固有の振る舞いを定義するコンポーネント |
| **scene/** | 各シーンの初期化と状態管理（State Pattern） |
| **UI/** | ゲーム固有のUIコントロール |
| **resources/** | このゲーム専用のテクスチャ、モデル、JSON等のアセット群 |

---

## 🎨 アセットパイプライン

### 1. 3Dモデルのエクスポート
- **ルール**: Blender等のツールでは **デフォルト設定（Y-up / 右手座標系）** でエクスポートしてください。
- **処理**: エンジン内部（Assimp読み込み時）でDirectX用の左手座標系へ自動変換されます。

### 2. テクスチャ命名規則 (Linear Workflow)
リニアワークフローを正確に行うため、ファイル名による自動判別を行っています。
- **数値データ (Linear)**: 接尾辞 `_n`, `_ao`, `_m`, `_r` を含めることでガンマ補正をスキップします。
- **カラーデータ (sRGB)**: 上記以外はすべて色として扱われ、自動的にリニアライズされます。

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
