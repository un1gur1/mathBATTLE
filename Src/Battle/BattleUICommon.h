#pragma once

namespace App::BattleUIDraw {

    int GetCachedFont(int size);

    void DrawCyberPanel(
        int x, int y, int w, int h,
        unsigned int baseCol,
        unsigned int edgeCol,
        int alpha = 200
    );

    void DrawCyberButton(
        int x, int y, int w, int h,
        const char* text,
        unsigned int col,
        bool isHover,
        int fontHandle
    );

    void DrawBatteryGauge(
        int x, int y,
        int current,
        int preview,
        int maxStocks,
        unsigned int safeCol,
        unsigned int dangerCol,
        unsigned int baseCol
    );

} // namespace App::BattleUIDraw
