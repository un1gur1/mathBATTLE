#define NOMINMAX
#include "Player.h"
#include <DxLib.h>
#include <cmath>
#include <string>
#include <unordered_map> // ★追加

#include "../../../Shader/CrystalOrbShader.h" 

namespace App {

    static int g_psPlayerCrystalHandle = -1;
    static int g_cbPlayerCrystalHandle = -1;
    static int g_playerFontHandle = -1;
    static int g_badgeFontHandle = -1;

    Player::Player(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks)
        : UnitBase("Player", startGrid, startScreen, number, stocks, maxStocks)
    {
        m_color = GetColor(255, 120, 0);

        if (g_psPlayerCrystalHandle == -1) {
            g_psPlayerCrystalHandle = LoadPixelShaderFromMem(g_ps_CrystalOrb, sizeof(g_ps_CrystalOrb));
            g_cbPlayerCrystalHandle = CreateShaderConstantBuffer(sizeof(float) * 8);
            g_playerFontHandle = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 40, 2, DX_FONTTYPE_ANTIALIASING);
            g_badgeFontHandle = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 26, 2, DX_FONTTYPE_ANTIALIASING);
        }
    }

    void DrawHexagonAA(float cx, float cy, float radius, unsigned int color, bool fill, float thickness = 1.0f, float rotAngle = 0.0f) {
        float angleOffsets[6] = { 0.0f, 60.0f, 120.0f, 180.0f, 240.0f, 300.0f };
        float rad = 3.14159265f / 180.0f;

        if (fill) {
            for (int i = 0; i < 6; ++i) {
                float a1 = angleOffsets[i] * rad + rotAngle;
                float a2 = angleOffsets[(i + 1) % 6] * rad + rotAngle;
                DrawTriangleAA(cx, cy, cx + cos(a1) * radius, cy + sin(a1) * radius, cx + cos(a2) * radius, cy + sin(a2) * radius, color, TRUE);
            }
        }
        else {
            for (int i = 0; i < 6; ++i) {
                float a1 = angleOffsets[i] * rad + rotAngle;
                float a2 = angleOffsets[(i + 1) % 6] * rad + rotAngle;
                DrawLineAA(cx + cos(a1) * radius, cy + sin(a1) * radius, cx + cos(a2) * radius, cy + sin(a2) * radius, color, thickness);
            }
        }
    }

    void Player::DrawUnitGraphic() {
        double time = GetNowCount() / 1000.0;
        float bobbing = (float)(sin(time * 3.0) * 4.0);

        float x = m_screenPos.x;
        float y = m_screenPos.y;
        float unitY = y + bobbing;

        // 1. シャドウ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(80 - (bobbing + 4.0f) * 5.0f));
        DrawOvalAA(x, y + 32.0f, 24.0f - bobbing / 2.0f, 8.0f - bobbing / 4.0f, 64, GetColor(0, 50, 100), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. アウター・ヘックスシールド
        float pulse = (float)(sin(time * 6.0) * 0.5 + 0.5);
        float hexRot = (float)time * 1.2f;

        SetDrawBlendMode(DX_BLENDMODE_ADD, 150 + (int)(pulse * 50));
        DrawHexagonAA(x, unitY, 34.0f, m_color, FALSE, 3.0f, hexRot);
        DrawHexagonAA(x, unitY, 30.0f, m_color, FALSE, 1.0f, -hexRot * 0.8f);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 3. インナーコア
        if (g_psPlayerCrystalHandle != -1 && g_cbPlayerCrystalHandle != -1) {
            SetUsePixelShader(g_psPlayerCrystalHandle);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

            float r = 1.0f, g = 0.47f, b = 0.0f;
            float* cb = (float*)GetBufferShaderConstantBuffer(g_cbPlayerCrystalHandle);
            cb[0] = (float)time; cb[1] = r; cb[2] = g; cb[3] = b;
            cb[4] = 0.0f; cb[5] = 0.0f; cb[6] = 0.0f; cb[7] = 0.0f;
            UpdateShaderConstantBuffer(g_cbPlayerCrystalHandle);
            SetShaderConstantBuffer(g_cbPlayerCrystalHandle, DX_SHADERTYPE_PIXEL, 0);

            float size = 28.0f;
            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                v[i].dif = GetColorU8(255, 255, 255, 255); v[i].spc = GetColorU8(0, 0, 0, 0);
            }
            v[0].pos.x = x - size; v[0].pos.y = unitY - size; v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = x + size; v[1].pos.y = unitY - size; v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = x - size; v[2].pos.y = unitY + size; v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = x + size; v[3].pos.y = unitY - size; v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = x + size; v[4].pos.y = unitY + size; v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = x - size; v[5].pos.y = unitY + size; v[5].u = 0.0f; v[5].v = 1.0f;

            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            SetUsePixelShader(-1);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // ==========================================
        // 4. 数値表示（★パラパラと減るアニメーション！）
        // ==========================================
        static std::unordered_map<const Player*, float> s_displayTotalMap;

        int actualStocks = GetStocks();
        int actualNum = GetNumber();
        int actualTotal = actualStocks * 9 + (actualNum - 1); // 残機を含めた総合パワー

        // 初回表示時
        if (s_displayTotalMap.find(this) == s_displayTotalMap.end()) {
            s_displayTotalMap[this] = (float)actualTotal;
        }

        float& displayTotal = s_displayTotalMap[this];

        // ゲームリセット時など、数値が離れすぎている場合は一瞬で合わせる
        if (std::abs(displayTotal - actualTotal) > 30.0f) {
            displayTotal = (float)actualTotal;
        }

        unsigned int numColor = GetColor(255, 255, 255); // 基本は白

        // 実際の数値に徐々に近づけていく（パラパラ演出）
        if (displayTotal > actualTotal) {
            displayTotal -= 0.18f; // 減るスピード（少し早め）
            if (displayTotal < actualTotal) displayTotal = (float)actualTotal;
            numColor = GetColor(255, 100, 100); // 減少中は赤く光る！
        }
        else if (displayTotal < actualTotal) {
            displayTotal += 0.18f;
            if (displayTotal > actualTotal) displayTotal = (float)actualTotal;
            numColor = GetColor(100, 255, 150); // 増加中は緑に光る！
        }

        // 表示用の数値を逆算
        int displayTotalInt = (int)std::round(displayTotal);
        int currentNum = (displayTotalInt % 9) + 1;
        if (displayTotalInt < 0) currentNum = 0; // 破壊時

        std::string numStr = std::to_string(currentNum);
        int numWidth = GetDrawStringWidthToHandle(numStr.c_str(), (int)numStr.length(), g_playerFontHandle);

        DrawStringToHandle((int)x - numWidth / 2 + 2, (int)unitY - 20 + 2, numStr.c_str(), GetColor(20, 10, 0), g_playerFontHandle);
        DrawStringToHandle((int)x - numWidth / 2, (int)unitY - 20, numStr.c_str(), numColor, g_playerFontHandle); // ★色を適用

        // 5. 演算子バッジ
        char currentOp = GetOp();
        if (currentOp != '\0') {
            float bx = x + 24.0f;
            float by = unitY + 18.0f;

            unsigned int glowCol, baseCol, edgeCol, shadowCol;
            if (currentOp == '+') { glowCol = GetColor(255, 50, 50); baseCol = GetColor(40, 10, 10); edgeCol = GetColor(255, 100, 100); shadowCol = GetColor(150, 0, 0); }
            else if (currentOp == '-') { glowCol = GetColor(50, 150, 255); baseCol = GetColor(10, 20, 40); edgeCol = GetColor(100, 200, 255); shadowCol = GetColor(0, 50, 150); }
            else if (currentOp == '*') { glowCol = GetColor(50, 255, 100); baseCol = GetColor(10, 40, 15); edgeCol = GetColor(100, 255, 150); shadowCol = GetColor(0, 150, 50); }
            else { glowCol = GetColor(200, 50, 255); baseCol = GetColor(30, 10, 40); edgeCol = GetColor(220, 100, 255); shadowCol = GetColor(100, 0, 150); }

            SetDrawBlendMode(DX_BLENDMODE_ADD, 200); DrawHexagonAA(bx, by, 18.0f, glowCol, TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawHexagonAA(bx, by, 15.0f, baseCol, TRUE); DrawHexagonAA(bx, by, 15.0f, edgeCol, FALSE, 2.0f);

            char opStr[2] = { currentOp, '\0' };
            int opW = GetDrawStringWidthToHandle(opStr, 1, g_badgeFontHandle);
            DrawStringToHandle((int)bx - opW / 2 + 1, (int)by - 13 + 1, opStr, shadowCol, g_badgeFontHandle);
            DrawStringToHandle((int)bx - opW / 2, (int)by - 13, opStr, GetColor(255, 255, 255), g_badgeFontHandle);
        }
    }
} // namespace App