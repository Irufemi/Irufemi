# Irufemi Engine

DirectX 12をベースにした、高機能な自作3Dゲームエンジンです。
グラフィックスプログラミングの学習から応用的なレンダリング技術の構築を目的として開発されています。

[![DebugBuild](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml)
[![ReleaseBuild](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml)
[![DevelopmentBuild](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml)
[![CheckUnwantedFiles](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml/badge.svg)](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml)

## 概要

このプロジェクトは、DirectX 12 APIを直接利用した低レイヤーからのレンダリングパイプラインを構築しており、並列処理や非同期読み込み、高度なシェーダー技術の実装を行っています。

## 主な機能

### 1. レンダリング (Rendering)
- **DirectX 12 描画基盤**: 効率的なディスクリプタ管理とコマンドリスト構築。
- **セキュアなマルチバッファリングアーキテクチャ**:
  - `ConstantBuffer<T>` テンプレートを用いた定数バッファの抽象化。
  - GPUの描画遅延に合わせたリングバッファ構造により、リソース（TransformやMaterialなど）の CPU-GPU 間のデータ編集競合（ティアリング）を完全に排除。
- **マルチパス・セーフなカメラ設計**:
  - 3Dモデル等の描画において、オブジェクトごとにCPU側でViewProjection行列を計算する従来の方式を廃止。
  - 共通カメラバッファ (`gCamera`) をシェーダー内で直接参照しGPU側で計算することで、シャドウマップ生成時など「1フレーム内での複数回描画」における定数バッファの競合バグを根本から防止。
- **Compute / Graphics 完全分離**:
  - `IComputeTask` インターフェースを用いたタスク予約型アーキテクチャ。
  - コンピュートシェーダー（GPUパーティクル等の計算）を描画コマンドから切り離し、DrawManager主導で一括実行することで、マルチパス時の二重実行や処理競合を回避。
- **パーティクルシステム**:
  - **GPU Particle**: 計算シェーダーを活用した大量の粒子シミュレーション。
  - **Voxel Particle**: ボクセルベースの物理挙動を持つパーティクル。
- **環境描画**: スカイボックス、動的なライト設定（平行光源、点光源、スポットライト）。
- **ポストプロセス管理**: マルチパスレンダリングに対応し、以下のエフェクトをサポート。
  - **カラー**: グレースケール、セピア、HSV調整、トーンマッピング (ACES)。
  - **ぼかし**: 平滑化、ガウスぼかし、放射状ぼかし。
  - **演出**: ビネット、ディゾルブ、ノイズ、アウトライン。

### 2. モデル & アニメーション (Model & Animation)
- **アセットインポート**: Assimpライブラリによる多種多様な3Dモデル（.obj, .gltf等）の読み込み。
- **スケルタルアニメーション**: インスタンスごとの再生制御、ボーンによる変形。

### 3. システム & 基盤 (System & Infrastructure)
- **非同期リソース管理**: シーン更新を止めないスレッドプールによる非同期ロード。
- **リファクタリング済み基盤**: FPSコントローラー、シェーダーコンパイラの分離。
- **サウンドシステム**: XAudio2による音声データの読み込みと再生。
- **算術ライブラリ**: 独自の Vector, Matrix, Quaternion クラス。

### 4. デバッグ・ツール (Tools)
- **Debug UI**: ImGui を使用したリアルタイムなパラメータ調整。
- **Doxygen 対応**: ほぼ全コードに対してドキュメントコメントを完備。

## プロジェクト構成

```text
C:.
├── project                # VS ソリューション・プロジェクト
│   └── IrufemiEngine      # エンジン本体
│       ├── Engine         # 基盤システム (Graphics, Platform, Manager)
│       ├── Renderer       # レンダリング関連 (Object3D, Particle, PostProcess)
│       ├── Resource       # リソース管理 (Texture, Model, Audio)
│       └── Framework      # アプリケーションフレームワーク
└── ...
```

## アセットパイプライン

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

## 開発ガイドライン

- **エンコード**: Unicode (UTF-8 署名なし) を厳守。
- **コメント**: ヘッダーファイルには Doxygen 形式での注釈を記述。
- **GPUリソース管理**: 描画において「毎フレーム内容が更新されるデータ」を作成する場合、生の `ID3D12Resource` を直接 `Map`/`Unmap` することは避け、必ず `ConstantBuffer<T>` または `std::array<ComPtr<ID3D12Resource>, kMaxFramesInFlight>` を利用してマルチバッファ化すること。
- **設計**: 疎結合を意識し、可能な限りインターフェースと実装を分離。

## 動作環境

- **OS**: Windows 10 / 11
- **IDE**: Visual Studio 2022
- **SDK**: Windows SDK 10.0.26100.7175 以上推奨

## 使用ライブラリ

- [Assimp](https://github.com/assimp/assimp)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [DirectX 12 Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/)

## ライセンス

このエンジンのソースコードは [MIT License](LICENSE.txt) の下で提供されています。各ライブラリのライセンスについては個別の規定に従ってください。
