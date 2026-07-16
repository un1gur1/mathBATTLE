#include "PostProcessManager.h"
#include <DxLib.h>

#include "../Shader/CRTShader.h"
#include "../Shader/GlitchShader.h"
#include"../Shader/CyberFadeShader.h"
#include"../Shader/TitleDiveShader.h"

namespace App {

    PostProcessManager* PostProcessManager::s_instance = nullptr;

    void PostProcessManager::CreateInstance() {
        if (!s_instance) s_instance = new PostProcessManager();
    }
    PostProcessManager* PostProcessManager::GetInstance() { return s_instance; }
    void PostProcessManager::DeleteInstance() {
        if (s_instance) { delete s_instance; s_instance = nullptr; }
    }

    PostProcessManager::PostProcessManager()
        : m_screenHandle(-1), m_psGlitchHandle(-1), m_psCrtHandle(-1), m_cbHandle(-1), m_shaderTime(0.0f) {
    }

    PostProcessManager::~PostProcessManager() { Release(); }

    void PostProcessManager::Init() {
        int sw, sh;
        GetDrawScreenSize(&sw, &sh);

        // ★修正1：アルファチャンネル付き（TRUE）で裏キャンバスを作ることで、合成バグを完全防止！
        m_screenHandle = MakeScreen(sw, sh, TRUE);

        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);

        m_psCrtHandle = LoadPixelShaderFromMem(g_ps_CRT, sizeof(g_ps_CRT));
        m_psGlitchHandle = LoadPixelShaderFromMem(g_ps_Glitch, sizeof(g_ps_Glitch));

        m_psFadeHandle = LoadPixelShaderFromMem(g_ps_Fade, sizeof(g_ps_Fade)); 

        m_psTitleDiveHandle = LoadPixelShaderFromMem(g_ps_TitleDive, sizeof(g_ps_TitleDive)); // ★追加
        m_shaderTime = 0.0f;
    }

    void PostProcessManager::Release() {
        if (m_screenHandle != -1) DeleteGraph(m_screenHandle);
        if (m_cbHandle != -1) DeleteShaderConstantBuffer(m_cbHandle);
        // ※メモリ読み込みの場合は DeleteShader は不要なので削除しました
    }

    void PostProcessManager::BeginDraw() {
        SetDrawScreen(m_screenHandle);
        ClearDrawScreen();
    }

    void PostProcessManager::EndDraw(EffectType type, float intensity, bool enableCrt) {

        SetDrawScreen(DX_SCREEN_BACK);
        m_shaderTime += 0.016f;

        if (type == EffectType::NONE) {
            DrawGraph(0, 0, m_screenHandle, FALSE);
        }
        else {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_shaderTime;
            cb[1] = intensity;
            cb[2] = enableCrt ? 1.0f : 0.0f; // ★cb[2]にCRTの有効化フラグを設定！
            cb[3] = 0.0f;
            UpdateShaderConstantBuffer(m_cbHandle);
            SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);
            if (type == EffectType::GLITCH) SetUsePixelShader(m_psGlitchHandle);
            if (type == EffectType::CRT)    SetUsePixelShader(m_psCrtHandle);
            if (type == EffectType::FADE)   SetUsePixelShader(m_psFadeHandle);
            if (type == EffectType::TITLE_DIVE) SetUsePixelShader(m_psTitleDiveHandle);

            SetUseTextureToShader(0, m_screenHandle);

            int sw, sh;
            GetDrawScreenSize(&sw, &sh);

            // ★修正3：私が省略してしまった「描画頂点（VERTEX2DSHADER）」の完全な設定！
            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0);
                v[i].rhw = 1.0f;
                v[i].dif = GetColorU8(255, 255, 255, 255);
                v[i].spc = GetColorU8(0, 0, 0, 0);
            }
            v[0].pos.x = 0.0f;       v[0].pos.y = 0.0f;       v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = (float)sw;  v[1].pos.y = 0.0f;       v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = 0.0f;       v[2].pos.y = (float)sh;  v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = (float)sw;  v[3].pos.y = 0.0f;       v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = (float)sw;  v[4].pos.y = (float)sh;  v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = 0.0f;       v[5].pos.y = (float)sh;  v[5].u = 0.0f; v[5].v = 1.0f;

            // 画面全体にシェーダーを焼き付ける
            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);

            SetUseTextureToShader(0, -1);
            SetUsePixelShader(-1);
        }
    }

} // namespace App