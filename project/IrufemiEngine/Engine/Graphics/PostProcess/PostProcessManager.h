#pragma once

#include "../../Core/Math/Vector2.h"
#include "../../Core/Math/Vector4.h"
#include "../../Core/Math/Matrix4x4.h"
#include "../DirectX/RenderTexture.h"
#include <d3d12.h>
#include <wrl/client.h>
#ifndef __IInspectable_INTERFACE_DEFINED__
// ComPtr の一部のテンプレート展開で IInspectable が必要になる場合がある
typedef struct IInspectable IInspectable;
#endif
#include <memory>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>

/**
 * @enum PostProcessMode
 * @brief ポストプロセスの各モードを定義する列挙型
 */
enum class PostProcessMode {
    None,               ///< 何も適用しない
    Grayscale,          ///< グレースケール
    Sepia,              ///< セピア調
    Vignette,           ///< ビネット（画面端を暗くする）
    Smoothing,          ///< 平滑化（ぼかし）
    GaussianFilter,     ///< ガウスぼかし
    DepthBasedOutline,  ///< 深度バッファを使用したアウトライン抽出
    RadialBlur,         ///< 放射状ぼかし
    Dissolve,           ///< ディゾルブ（ノイズテクスチャによる消失演出）
    Noise,              ///< ランダムノイズ粒子
    HSV,                ///< HSV色空間による色調整
    ToneMapping,        ///< トーンマッピング（ACES）
    Fade,               ///< フェード（指定色への塗りつぶし）
    Slide,              ///< スライド（ワイプ演出）
    Bloom,              ///< ブルーム（高輝度抽出による発光）
};

class DirectXCommon;

/**
 * @class PostProcessManager
 * @brief ポストプロセス（画面全体にかけるエフェクト）を管理するクラス。
 * 
 * マルチパスレンダリングに対応しており、複数のエフェクトをスタックに追加して重ね掛けできます。
 * 
 * @par シーンでの使用例:
 * @code
 * // 1. マネージャーの取得
 * auto* pp = engine->GetPostProcessManager();
 * 
 * // 2. エフェクトのリセットと追加
 * pp->ClearActiveModes();
 * pp->AddActiveMode(PostProcessMode::DepthBasedOutline);
 * pp->AddActiveMode(PostProcessMode::Noise);
 * 
 * // 3. 各エフェクトのパラメータ調整
 * pp->GetNoiseParams().intensity = 0.2f;
 * @endcode
 */
class PostProcessManager {
public:
    using Mode = PostProcessMode;


    /**
     * @struct NoiseParams
     * @brief ノイズエフェクト用パラメータ
     */
    struct NoiseParams {
        float intensity = 0.5f; ///< ノイズの強度 (0.0 ~ 1.0)
        float time = 0.0f;      ///< 時間経過（内部で更新される）
    };

    /**
     * @struct VignetteParams
     * @brief ビネットエフェクト用パラメータ
     */
    struct VignetteParams {
        float scale = 16.0f;    ///< ビネットの範囲
        float power = 0.8f;     ///< 減衰の強さ
    };

    /**
     * @struct SmoothingParams
     * @brief 平滑化エフェクト用パラメータ
     */
    struct SmoothingParams {
        int32_t kernelSize = 3; ///< カーネルサイズ (奇数推奨)
    };

    /**
     * @struct GaussianParams
     * @brief ガウスぼかし用パラメータ
     */
    struct GaussianParams {
        float sigma = 2.0f;     ///< 標準偏差（ぼけ具合）
        int32_t kernelSize = 3; ///< カーネルサイズ (奇数推奨)
    };

    /**
     * @struct RadialBlurParams
     * @brief 放射状ぼかし用パラメータ
     */
    struct RadialBlurParams {
        Vector2 center = { 0.5f, 0.5f }; ///< ぼかしの中心点 (UV空間 0.0 ~ 1.0)
        float blurWidth = 0.01f;         ///< ぼかしの幅
        int32_t numSamples = 10;         ///< サンプル数
    };

    /**
     * @struct OutlineParams
     * @brief アウトラインエフェクト用パラメータ
     */
    struct OutlineParams {
        Matrix4x4 projectionInverse;    ///< 逆投影行列 (自動でセットされる)
    };

    /**
     * @struct DissolveParams
     * @brief ディゾルブエフェクト用パラメータ
     */
    struct DissolveParams {
        Vector4 edgeColor = { 1.0f, 0.4f, 0.3f, 1.0f }; ///< 境界線の色
        Vector4 backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f }; ///< 背景色 (追加)
        float threshold = 0.0f;                         ///< 消失しきい値 (0.0 ~ 1.0)
        float edgeRange = 0.03f;                        ///< 境界線の幅
        int32_t noiseType = 0;                          ///< 使用するノイズテクスチャのインデックス (0 or 1)
    };

    /**
     * @struct HSVParams
     * @brief HSVエフェクト用パラメータ
     */
    struct HSVParams {
        float hue = 0.0f;        ///< 色相オフセット (-1.0 ~ 1.0)
        float saturation = 0.0f; ///< 彩度オフセット (-1.0 ~ 1.0)
        float value = 0.0f;      ///< 明度オフセット (-1.0 ~ 1.0)
    };

    /**
     * @struct ToneMappingParams
     * @brief トーンマッピングエフェクト用パラメータ
     */
    struct ToneMappingParams {
        float exposure = 1.0f;   ///< 露出補正 (0.0 ~ )
    };

    /**
     * @struct FadeParams
     * @brief フェードエフェクト用パラメータ
     */
    struct FadeParams {
        Vector4 color = { 0.0f, 0.0f, 0.0f, 1.0f }; ///< フェード色
        float intensity = 0.0f;                      ///< 強度 (0.0 ~ 1.0)
    };

    /**
     * @struct SlideParams
     * @brief スライドエフェクト用パラメータ
     */
    struct SlideParams {
        Vector4 color = { 0.0f, 0.0f, 0.0f, 1.0f }; ///< スライドの色
        float threshold = 0.0f;                      ///< 進行度 (0.0 ~ 1.0)
    };

    /**
     * @struct BloomParams
     * @brief ブルームエフェクト用パラメータ
     */
    struct BloomParams {
        Vector2 direction = { 1.0f, 0.0f }; ///< ぼかしの方向 ({1,0}で横, {0,1}で縦)
        float threshold = 0.8f;             ///< 高輝度抽出のしきい値
        float sigma = 3.0f;                 ///< ぼかしの強さ
        float intensity = 1.0f;             ///< ブルームの強度
        int32_t kernelSize = 21;            ///< ぼかしのカーネルサイズ
    };

public:
    /**
     * @brief ポストプロセスの初期化
     * @param dxCommon DirectX基盤クラス
     * @param rtvFormat 最終的な出力先のRTVフォーマット
     */
    void Initialize(DirectXCommon* dxCommon, DXGI_FORMAT rtvFormat);

    /**
     * @brief 作業用バッファ（ピンポンバッファ）の初期化
     * @param width 画面幅
     * @param height 画面高さ
     * @param dxCommon DirectX基盤クラス
     */
    void InitializeBuffers(uint32_t width, uint32_t height, DirectXCommon* dxCommon);

    /**
     * @brief 更新処理
     * @param totalTime 累計時間（ノイズ等のアニメーションに使用）
     */
    void Update(float totalTime);

    /**
     * @brief 描画実行（マルチパス対応）
     * @param commandList コマンドリスト
     * @param srcTexture 元となるレンダリングテクスチャ（メインの描画結果）
     * @param rtvHandle 最終的な出力先（バックバッファ）のRTV
     */
    void Draw(ID3D12GraphicsCommandList* commandList, RenderTexture* srcTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

    // --- Getters & Setters ---

    /** @brief 現在アクティブなエフェクトスタックを取得 */
    const std::vector<Mode>& GetActiveModes() const { return activeModes_; }

    /** @brief エフェクトをスタックに追加 */
    void AddActiveMode(Mode mode) { activeModes_.push_back(mode); }

    /** @brief 指定したエフェクトをスタックから削除 */
    void RemoveActiveMode(Mode mode) {
        activeModes_.erase(std::remove(activeModes_.begin(), activeModes_.end(), mode), activeModes_.end());
    }

    /** @brief 全てのエフェクトを解除（クリア） */
    void ClearActiveModes() { activeModes_.clear(); }

    /** @brief エフェクトスタックを一括設定 */
    void SetActiveModes(const std::vector<Mode>& modes) { activeModes_ = modes; }

    /** @brief 指定したエフェクトが現在有効かチェック */
    bool HasActiveMode(Mode mode) const {
        return std::find(activeModes_.begin(), activeModes_.end(), mode) != activeModes_.end();
    }
    
    /** @brief 互換性のための単一セット (既存リストをクリアして1つ追加) */
    void SetMode(Mode mode) { 
        activeModes_.clear(); 
        if (mode != Mode::None) activeModes_.push_back(mode); 
    }

    /** @brief 互換性のための取得 (リストが空でなければ先頭を返す) */
    Mode GetMode() const { return activeModes_.empty() ? Mode::None : activeModes_.front(); }
    
    // 各エフェクトのパラメータ取得 (シーンからの演出用)
    NoiseParams& GetNoiseParams() { return noiseParams_; }
    VignetteParams& GetVignetteParams() { return vignetteParams_; }
    SmoothingParams& GetSmoothingParams() { return smoothingParams_; }
    GaussianParams& GetGaussianParams() { return gaussianParams_; }
    RadialBlurParams& GetRadialBlurParams() { return radialBlurParams_; }
    OutlineParams& GetOutlineParams() { return outlineParams_; }
    DissolveParams& GetDissolveParams() { return dissolveParams_; }
    HSVParams& GetHSVParams() { return hsvParams_; }
    ToneMappingParams& GetToneMappingParams() { return toneMappingParams_; }
    FadeParams& GetFadeParams() { return fadeParams_; }
    SlideParams& GetSlideParams() { return slideParams_; }
    BloomParams& GetBloomParams() { return bloomParams_; }

    void SetDissolveNoiseHandle(int index, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
        if (index >= 0 && index < 2) dissolveNoiseHandle_[index] = handle;
    }
    
    void SetDepthSrvHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) { depthSrvHandle_ = handle; }

    /** @brief 最終的な描画結果のテクスチャを取得 (ImGui表示用) */
    RenderTexture* GetResultTexture() const { return resultTexture_.get(); }

private:
    void CreatePSOs();
    void CreateConstantBuffers();
    void DrawSinglePass(ID3D12GraphicsCommandList* commandList, Mode mode, RenderTexture* srcTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, bool isFinalPass = false, ID3D12PipelineState* psoOverride = nullptr);
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(size_t size);

private:
    DirectXCommon* dxCommon_ = nullptr;
    ID3D12Device* device_ = nullptr;
    ID3D12RootSignature* rootSig_ = nullptr;
    DXGI_FORMAT rtvFormat_ = DXGI_FORMAT_UNKNOWN;

    Mode mode_ = Mode::None; // 互換性用（内部では不使用にする）
    std::vector<Mode> activeModes_;
    
    // ピンポンバッファ
    std::array<std::unique_ptr<RenderTexture>, 2> workTextures_;
    // 最終結果保持用テクスチャ (ImGui 表示等に使用)
    std::unique_ptr<RenderTexture> resultTexture_;

    // PSOs
    struct PipelineSet {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    };
    // モードに対応するPSOを保持 (中間パス用: _UNORM)
    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 15> psos_;
    // 最終パス用 (スワップチェーン等の _SRGB 形式用)
    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 15> finalPsos_;

    // ブルーム専用 PSO (内部のパス用)
    Microsoft::WRL::ComPtr<ID3D12PipelineState> bloomExtractPSO_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> bloomBlurHPSO_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> bloomBlurVPSO_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> bloomCombinePSO_;

    // Constant Buffers
    Microsoft::WRL::ComPtr<ID3D12Resource> noiseCB_;
    NoiseParams* mappedNoise_ = nullptr;
    NoiseParams noiseParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vignetteCB_;
    VignetteParams* mappedVignette_ = nullptr;
    VignetteParams vignetteParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> smoothingCB_;
    SmoothingParams* mappedSmoothing_ = nullptr;
    SmoothingParams smoothingParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> gaussianCB_;
    GaussianParams* mappedGaussian_ = nullptr;
    GaussianParams gaussianParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> radialBlurCB_;
    RadialBlurParams* mappedRadialBlur_ = nullptr;
    RadialBlurParams radialBlurParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> outlineCB_;
    OutlineParams* mappedOutline_ = nullptr;
    OutlineParams outlineParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> dissolveCB_;
    DissolveParams* mappedDissolve_ = nullptr;
    DissolveParams dissolveParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> hsvCB_;
    HSVParams* mappedHsv_ = nullptr;
    HSVParams hsvParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> toneMappingCB_;
    ToneMappingParams* mappedToneMapping_ = nullptr;
    ToneMappingParams toneMappingParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> fadeCB_;
    FadeParams* mappedFade_ = nullptr;
    FadeParams fadeParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> slideCB_;
    SlideParams* mappedSlide_ = nullptr;
    SlideParams slideParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> bloomCB_;
    BloomParams* mappedBloom_ = nullptr;
    BloomParams bloomParams_;

    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandle_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2> dissolveNoiseHandle_{};

    // 状態追跡用
    std::array<D3D12_RESOURCE_STATES, 2> workTextureStates_ = { D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
    D3D12_RESOURCE_STATES resultTextureState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};
