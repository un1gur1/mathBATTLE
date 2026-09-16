#define NOMINMAX
#include <DxLib.h>
#include "BattleUICommon.h"
#include "../Utility/AppConfig.h"

#include <cmath>
#include <cstring>
#include <unordered_map>

namespace App::BattleUIDraw {
    using namespace Config;

    int GetCachedFont(int size) {
        static std::unordered_map<int, int> s_fontCache;
        auto it = s_fontCache.find(size);
        if (it == s_fontCache.end()) {
            int handle = CreateFontToHandle("BIZ UDƒSƒVƒbƒN", size, 2, DX_FONTTYPE_ANTIALIASING);
            s_fontCache.emplace(size, handle);
            return handle;
        }
        return it->second;
    }

    void DrawCyberPanel(
        int x, int y, int w, int h,
        unsigned int baseCol,
        unsigned int edgeCol,
        int alpha
    ) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(x, y, x + w, y + h, baseCol, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        const int cl = 12;
        DrawLine(x, y, x + cl, y, edgeCol, 2);
        DrawLine(x, y, x, y + cl, edgeCol, 2);
        DrawLine(x + w - cl, y, x + w, y, edgeCol, 2);
        DrawLine(x + w, y, x + w, y + cl, edgeCol, 2);
        DrawLine(x, y + h, x + cl, y + h, edgeCol, 2);
        DrawLine(x, y + h - cl, x, y + h, edgeCol, 2);
        DrawLine(x + w - cl, y + h, x + w, y + h, edgeCol, 2);
        DrawLine(x + w, y + h - cl, x + w, y + h, edgeCol, 2);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        DrawLine(x + cl, y, x + w - cl, y, edgeCol, 1);
        DrawLine(x + cl, y + h, x + w - cl, y + h, edgeCol, 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void DrawCyberButton(
        int x, int y, int w, int h,
        const char* text,
        unsigned int col,
        bool isHover,
        int fontHandle
    ) {
        if (isHover) {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 180);
            DrawBox(x, y, x + w, y + h, col, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(x, y, x + w, y + h, COL_WHITE(), FALSE);
            int tw = GetDrawStringWidthToHandle(text, static_cast<int>(std::strlen(text)), fontHandle);
            DrawStringToHandle(x + (w - tw) / 2, y + (h - 26) / 2, text, COL_TEXT_DARK(), fontHandle);
        }
        else {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
            DrawBox(x, y, x + w, y + h, GetColor(15, 20, 25), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(x, y, x + w, y + h, col, FALSE);
            int tw = GetDrawStringWidthToHandle(text, static_cast<int>(std::strlen(text)), fontHandle);
            DrawStringToHandle(x + (w - tw) / 2, y + (h - 26) / 2, text, col, fontHandle);
        }
    }

    void DrawBatteryGauge(
        int x, int y,
        int current,
        int preview,
        int maxStocks,
        unsigned int safeCol,
        unsigned int dangerCol,
        unsigned int baseCol
    ) {
        constexpr int bw = 35;
        constexpr int bh = 24;
        constexpr int gap = 8;
        const int f16 = GetCachedFont(16);

        for (int i = 0; i < maxStocks; ++i) {
            const int bx = x + i * (bw + gap);
            const int by = y;
            constexpr int slant = 8;
            int px[4] = { bx + slant, bx + bw, bx + bw - slant, bx };
            int py[4] = { by, by, by + bh, by + bh };

            auto drawQuad = [&](unsigned int color, bool fill) {
                DrawQuadrangleAA(
                    static_cast<float>(px[0]), static_cast<float>(py[0]),
                    static_cast<float>(px[1]), static_cast<float>(py[1]),
                    static_cast<float>(px[2]), static_cast<float>(py[2]),
                    static_cast<float>(px[3]), static_cast<float>(py[3]),
                    color, fill
                );
                };

            if (i < current && i >= preview) {
                int blinkAlpha = static_cast<int>(120 + 120 * std::sin(GetNowCount() / 80.0));
                SetDrawBlendMode(DX_BLENDMODE_ADD, blinkAlpha);
                drawQuad(dangerCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                drawQuad(GetColor(255, 255, 255), FALSE);
                DrawStringToHandle(bx - 2, by - 18, "Á”ï", dangerCol, f16);
            }
            else if (i >= current && i < preview) {
                int blinkAlpha = static_cast<int>(120 + 120 * std::sin(GetNowCount() / 80.0));
                SetDrawBlendMode(DX_BLENDMODE_ADD, blinkAlpha);
                drawQuad(safeCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                drawQuad(GetColor(255, 255, 255), FALSE);
                DrawStringToHandle(bx - 2, by - 18, "‰ñ•œ", safeCol, f16);
            }
            else if (i < current) {
                drawQuad(baseCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
                DrawQuadrangleAA(
                    static_cast<float>(px[0]) + 2, static_cast<float>(py[0]) + 2,
                    static_cast<float>(px[1]) - 2, static_cast<float>(py[1]) + 2,
                    static_cast<float>(px[2]) - 2, static_cast<float>(py[2]) - 2,
                    static_cast<float>(px[3]) + 2, static_cast<float>(py[3]) - 2,
                    COL_WHITE(), TRUE
                );
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            else {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
                drawQuad(GetColor(30, 35, 45), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                drawQuad(GetColor(80, 85, 100), FALSE);
            }
        }
    }

} // namespace App::BattleUIDraw
