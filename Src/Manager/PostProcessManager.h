#pragma once

namespace App {

    // かけたいエフェクトの種類
    enum class EffectType {
        NONE,       // そのまま（エフェクトなし）
        GLITCH,     // ダメージ時の色ズレ
        CRT,        // ブラウン管風スキャンライン
        FADE,
        TITLE_DIVE,
        HEAVY_BLUR  // 高速移動やワープ時のブラー
    };

    class PostProcessManager {
    public:
        static void CreateInstance();
        static PostProcessManager* GetInstance();
        static void DeleteInstance();

        void Init();
        void Release();

        // 描画の最初に呼ぶ（裏キャンバスへの描画開始）
        void BeginDraw();

        // 描画の最後に呼ぶ（裏キャンバスを表画面に加工転写）
        void EndDraw(EffectType type = EffectType::NONE, float intensity = 1.0f, bool enableCrt = false); 
   
    private:

        PostProcessManager();
        ~PostProcessManager();

        static PostProcessManager* s_instance;

        int m_screenHandle; // 裏のキャンバス（レンダーターゲット）

        // シェーダー関連
        int m_psGlitchHandle;
        int m_psCrtHandle;
        int m_cbHandle;
        int m_psFadeHandle;

        int m_psTitleDiveHandle;

        float m_shaderTime;
    };

} // namespace App