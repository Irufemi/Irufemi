# Irufemi Engine

[![DebugBuild](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml)
[![ReleaseBuild](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml)
[![DevelopmentBuild](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml)
[![CheckUnwantedFiles](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml/badge.svg)](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml)

DirectX 12をベースにスクラッチから構築した、**GPU-Driven Rendering** および **Data-Oriented Design** 指向の自作3Dゲームエンジンです。
グラフィックスプログラミングの学習に留まらず、商用AAAゲームエンジンにおけるパフォーマンス要求（CPU-GPU同期、ドローコールの極小化、大量オブジェクトの処理）をクリアするための最新アーキテクチャの実証を目的としています。

---

## 🎯 技術的アピールポイント（最適化とモダンアーキテクチャ）

### 1. GPU-Driven Rendering & Compute Pipeline (描画と物理・アニメーションのGPU完全委譲)
**【課題】** スケルタルアニメーションのCPU処理や、数万個に及ぶ破片の物理計算・描画による強烈なCPUバウンド。
**【解決】** 
- **GPU Skinning**: スケルタルアニメーションの頂点変形をCPUや頂点シェーダで行わず、`Compute Shader` を用いて事前演算 (Pre-compute) しています。
- **GPGPU Physics & GPU Culling**: 数万ボクセルの重力・バウンド演算を `Compute Shader` にオフロードし、そのまま `ExecuteIndirect` を用いた GPU Culling で描画コマンドを直接発行。
**【効果】** アニメーションと大量オブジェクトの物理・描画からCPUを完全に解放し、数万の破片が飛び交う圧倒的な破壊表現を 60FPS で実現。

### 2. Data-Oriented Architecture (データ指向と数万オブジェクトの最適化)
**【課題】** 大量のアクター（敵、弾、破片）を管理する際、オブジェクト指向特有のメモリ断片化によるキャッシュミスやポインタ巡回のオーバーヘッド。
**【解決】** 
- **Virtual Entity Manager**: Instance Promotion パターンを採用。オブジェクト群を普段は単なる「座標の配列 (Virtual Transform)」としてキャッシュ効率良く超高速イテレートし、プレイヤーが干渉した瞬間など「本当に必要なときだけ」本物の `GameObject` へ昇格 (Promote) させます。
**【効果】** シーン内に数万のオブジェクトが存在しても、スリープ中のオーバーヘッドを完全にゼロ化。

### 3. Modern Rendering Pipeline & VFX (最新レンダリング基盤と視覚効果)
**【課題】** 複雑なパス依存の解決、テクスチャバインド管理の煩雑さ、半透明パーティクルのZソートにかかるCPU負荷。
**【解決】** 
- **RenderGraph**: 複雑なポストプロセスパスの依存を自動解決し、一時メモリ（Transient Resource）のエイリアシング最適化を実施。
- **Bindless Resources (Descriptor Indexing)**: レガシーな `register(tX)` を廃止。全テクスチャを巨大な配列に乗せ、定数バッファ経由のインデックスで参照するフルBindlessアーキテクチャへ移行。
- **GPU Bitonic Sort**: パーティクルのZソート（カメラからの距離順並び替え）をCPUから `Compute Shader` (ビトニックソート) に完全移行。
**【効果】** マテリアル表現の圧倒的な柔軟性、ドローコールの劇的な削減、および完全なGPUパーティクルシミュレーションを実現。

### 4. セキュアなマルチバッファリングと非同期処理
**【課題】** CPU-GPU間のデータ編集競合（ティアリング）、ダングリングポインタによるクラッシュ、シーンロード時のスパイク。
**【解決】** 
- **マルチバッファリング**: `ConstantBuffer<T>` によるトリプルバッファリングアーキテクチャで競合を完全に排除。
- **Resource Handle System**: リソース管理を `std::shared_ptr` から世代 (Generation) 管理付きの Handle へ移行し、ダングリングポインタを完全防御。
- **非同期分散処理**: スレッドプールを利用し、リソースのロードや負荷の高いレイキャスト判定をバックグラウンドに分散 (Time-Slicing)。
**【効果】** ティアリングとフレームドロップ、メモリリークを完全に防ぐ極めて堅牢な基盤とシームレスなUX。

---

## 📁 プロジェクト構成 (Project Structure)

本ソリューション (`Irufemi.sln`) は、エンジンコアとアプリケーション（ゲームロジック）、およびツール群を完全に分離（関心の分離）した以下の4プロジェクトで構成されています。

| プロジェクト | 種別 | 役割 |
| :--- | :--- | :--- |
| **IrufemiEngine** | 静的ライブラリ (.lib) | 描画・物理・リソース・コンポーネント基盤を提供するエンジンコア |
| **IrufemiEditor** | 静的ライブラリ (.lib) | ImGuiベースのレベルエディタ・デバッグツール群 |
| **Application_solo** | 実行ファイル (.exe) | 個人制作ゲームのロジック・固有シーン・アセンブリ |
| **Application_team** | 実行ファイル (.exe) | チーム制作ゲームのロジック・固有シーン・アセンブリ |

---

## 📂 ディレクトリ構成 (Folder Structure)

### ⚙️ IrufemiEngine (`project/IrufemiEngine/`)
エンジンのコアモジュール群です。特定のゲームに依存する処理は一切含みません。

| ディレクトリ | 役割 |
| :--- | :--- |
| **Engine/** | DirectX12ラッパー、ウィンドウ管理、スレッドプール、各種Manager群 |
| **Renderer/** | RenderGraph、Object3D描画、GPU Particle、PostProcess等の描画パイプライン |
| **Resource/** | ResourceHandleシステムを用いたテクスチャ・モデル・オーディオの安全な管理 |
| **Framework/** | GameObject と Component を用いた軽量かつ高速なECS基盤 |
| **EngineResources/** | HLSLシェーダー、エンジン標準のテクスチャやモデルなどの必須アセット |

### 🎮 Application (`project/Application_solo/` 等)
ゲーム固有のロジックとリソースを格納します。

| ディレクトリ | 役割 |
| :--- | :--- |
| **components/** | ゲーム固有の振る舞い（Player, Enemy, Debris等）を定義するコンポーネント |
| **scene/** | 各シーン（Title, InGame, Clear等）の初期化と状態管理（State Pattern） |
| **UI/** | ゲーム固有のUIコントロール（LoadingScreen, PromptController等） |
| **resources/** | このゲーム専用のテクスチャ、モデル、JSONパラメータ等のアセット群 |

---

## 🎨 アセットパイプライン

### 1. 3Dモデルのエクスポート
- **ルール**: Blender等のツールでは **デフォルト設定（Y-up / 右手座標系）** でエクスポートしてください。
- **処理**: エンジン内部（Assimp読み込み時）でDirectX用の左手座標系へ自動変換されます。

### 2. テクスチャ命名規則 (Linear Workflow)
リニアワークフローを正確に行うため、ファイル名による自動判別を行っています。
- **数値データ (Linear)**: 以下の接尾辞を含めることでガンマ補正をスキップします。
  - `_n` / `normal` (法線)
  - `_ao` (AO)
  - `_m` / `metallic` (メタリック)
  - `_r` / `roughness` (ラフネス)
- **カラーデータ (sRGB)**: 上記以外はすべて色として扱われ、自動的にリニアライズされます。

---

## 📜 開発ガイドライン

- **エンコード**: Unicode (UTF-8 署名なし) を厳守。
- **コメント**: ヘッダーファイルには Doxygen 形式での注釈を記述。
- **GPUリソース管理**: 描画において「毎フレーム内容が更新されるデータ」を作成する場合、生の `ID3D12Resource` を直接 `Map`/`Unmap` することは避け、必ず `ConstantBuffer<T>` または `std::array<ComPtr<ID3D12Resource>, kMaxFramesInFlight>` を利用してマルチバッファ化すること。
- **リソース同期 (Resource Barrier)**: 
  - `pResource = nullptr` を用いた UAV のグローバルバリアは、GPU の並列実行効率を低下させるため使用禁止とします。
  - バリアを張る際は、特定のクラスに依存しない静的ユーティリティ関数（`DirectXUtils::TransitionBarrier` や `DirectXUtils::UAVBarriers` 等）を使用してください。

---

## ⚙️ 動作環境・ビルド手順

- **OS**: Windows 10 / 11
- **IDE**: Visual Studio 2026
- **SDK**: Windows SDK 10.0.26100.7175 以上推奨

**ビルド手順**:
1. リポジトリをクローン後、VS2026でソリューションを開きます。
2. 構成を選択しビルドを実行します。

**【ビルド構成によるシェーダーコンパイルの違い】**
本エンジンは開発時のイテレーション速度と、製品版のパフォーマンスを両立するため、構成によってシェーダーのコンパイル方式が異なります。
- **Debug / Development / Editor ビルド**:
  エンジン実行時に動的コンパイル（Runtime Compile）されます。これにより、シェーダーを編集するたびにプロジェクト全体をビルドし直す手間が省け、迅速なトライ＆エラーが可能です。
- **Release ビルド**:
  Visual Studio の PreBuild イベントとして `CompileShaders.bat` が自動で呼び出され、最高レベルの最適化 (`/O3`) を適用したオフラインコンパイル（事前ビルド）が行われます。実行時のコンパイル負荷が完全にゼロになります。

---

## 📚 使用ライブラリ

- [Assimp](https://github.com/assimp/assimp)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [DirectX 12 Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/)

---

## 📄 ライセンス

このエンジンのソースコードは [MIT License](LICENSE.txt) の下で提供されています。各ライブラリのライセンスについては個別の規定に従ってください。
