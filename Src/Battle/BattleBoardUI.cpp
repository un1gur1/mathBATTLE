#define NOMINMAX
#include <DxLib.h>
#include "BattleBoardUI.h"
#include "BattleViewData.h"
#include "BattleUICommon.h"
#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/UnitBase.h"
#include "../Utility/AppConfig.h"

#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace App {
    using namespace Config;
    using namespace BattleUIDraw;

    void BattleBoardUI::DrawEnemyDangerArea(const BattleViewData& view) const {
        if (!view.map) return;

        const double time = GetNowCount() / 2500.0;
        const int dangerAlpha = static_cast<int>(20 + 15 * std::sin(time * M_PI * 3.0));
        const int dangerFrameAlpha = static_cast<int>(100 + 40 * std::sin(time * M_PI * 3.0));

        const BattleUnitView& enemyView = view.is1PTurn ? view.p2 : view.p1;
        if (enemyView.unit) {
            const Vector2 center = view.map->GetCellCenter(enemyView.gridPos.x, enemyView.gridPos.y);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
            DrawCircleAA(center.x, center.y, 25.0f, 32, COL_DANGER(), FALSE, 2.0f);
        }

        for (const auto& cell : view.enemyDangerCells) {
            const Vector2 center = view.map->GetCellCenter(cell.pos.x, cell.pos.y);
            const unsigned int col = cell.baseReachable ? COL_DANGER_DIM() : COL_WARN();
            const int cx = static_cast<int>(center.x);
            const int cy = static_cast<int>(center.y);
            constexpr int size = 38;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, dangerAlpha);
            DrawBox(cx - size, cy - size, cx + size, cy + size, col, TRUE);

            SetDrawBlendMode(DX_BLENDMODE_ADD, dangerFrameAlpha);
            constexpr int len = 12;
            DrawLine(cx - len, cy - size, cx + len, cy - size, col, 2);
            DrawLine(cx - len, cy + size, cx + len, cy + size, col, 2);
            DrawLine(cx - size, cy - len, cx - size, cy + len, col, 2);
            DrawLine(cx + size, cy - len, cx + size, cy + len, col, 2);
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void BattleBoardUI::DrawMovableArea(const BattleViewData& view) const {
        if (!view.map || !view.playerSelected) return;

        const double time = GetNowCount() / 3000.0;
        const int pulseAlpha = static_cast<int>(30 + 20 * std::sin(time * M_PI * 4.0));
        const int frameAlpha = static_cast<int>(150 + 50 * std::sin(time * M_PI * 4.0));

        const BattleUnitView& activeView = view.is1PTurn ? view.p1 : view.p2;
        if (activeView.unit) {
            const Vector2 center = view.map->GetCellCenter(activeView.gridPos.x, activeView.gridPos.y);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
            DrawCircleAA(center.x, center.y, 25.0f, 32, COL_P1(), FALSE, 2.0f);
        }

        for (const auto& cell : view.movableCells) {
            const Vector2 center = view.map->GetCellCenter(cell.pos.x, cell.pos.y);
            const unsigned int col = cell.warp
                ? COL_INFO()
                : (cell.baseReachable ? GetColor(0, 200, 255) : GetColor(255, 180, 0));

            const int cx = static_cast<int>(center.x);
            const int cy = static_cast<int>(center.y);
            constexpr int size = 40;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
            DrawBox(cx - size, cy - size, cx + size, cy + size, col, TRUE);

            SetDrawBlendMode(DX_BLENDMODE_ADD, frameAlpha);
            constexpr int len = 8;
            DrawLine(cx - size, cy - size, cx - size + len, cy - size, col, 2);
            DrawLine(cx - size, cy - size, cx - size, cy - size + len, col, 2);
            DrawLine(cx + size - len, cy - size, cx + size, cy - size, col, 2);
            DrawLine(cx + size, cy - size, cx + size, cy - size + len, col, 2);
            DrawLine(cx - size, cy + size, cx - size + len, cy + size, col, 2);
            DrawLine(cx - size, cy + size - len, cx - size, cy + size, col, 2);
            DrawLine(cx + size - len, cy + size, cx + size, cy + size, col, 2);
            DrawLine(cx + size, cy + size - len, cx + size, cy + size, col, 2);
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        if (view.hoverMoveValid && view.map->IsWithinBounds(view.hoverMoveGrid.x, view.hoverMoveGrid.y)) {
            const Vector2 center = view.map->GetCellCenter(view.hoverMoveGrid.x, view.hoverMoveGrid.y);
            const int cx = static_cast<int>(center.x);
            const int cy = static_cast<int>(center.y);
            constexpr int size = 42;

            SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
            DrawBox(cx - size, cy - size, cx + size, cy + size, COL_WHITE(), FALSE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawFormatStringToHandle(cx - 14, cy - 35, COL_DANGER(), GetCachedFont(20), "-%d", view.hoverMoveCost);
        }
    }

    void BattleBoardUI::Draw(const BattleViewData& view) const {
        if (!view.map) return;

        view.map->Draw();

        auto drawWarpNodes = [&](const BattleUnitView& unitView, unsigned int baseCol, const char* label) {
            for (const auto& pos : unitView.warpNodes) {
                const Vector2 center = view.map->GetCellCenter(pos.x, pos.y);
                SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
                DrawCircleAA(center.x, center.y, 42.0f, 64, baseCol, FALSE, 3.0f);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
                DrawCircleAA(center.x, center.y, 35.0f, 64, baseCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                const int font18 = GetCachedFont(18);
                const int tw = GetDrawStringWidthToHandle(label, static_cast<int>(std::strlen(label)), font18);
                DrawStringToHandle(static_cast<int>(center.x) - tw / 2, static_cast<int>(center.y) + 15, label, COL_TEXT_MAIN(), font18);
            }
            };

        // 元の表示仕様を維持し、現在ターン側のワープ地点だけを描画する。
        if (view.is1PTurn) {
            drawWarpNodes(view.p1, COL_P1(), "1P ST.");
        }
        else {
            drawWarpNodes(view.p2, COL_P2(), "2P ST.");
        }

        DrawEnemyDangerArea(view);
        DrawMovableArea(view);

        if (view.hasActiveUnit) {
            const Vector2 center = view.map->GetCellCenter(view.activeGrid.x, view.activeGrid.y);
            const double time = GetNowCount() / 1000.0;
            const int alpha = static_cast<int>(120 + 100 * std::sin(time * M_PI * 2.0));
            const unsigned int auraCol = view.activeUnitIs1P ? COL_WARN() : COL_P2();
            const int cx = static_cast<int>(center.x);
            const int cy = static_cast<int>(center.y);
            constexpr int size = 39;
            constexpr int len = 12;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 4);
            DrawBox(cx - size, cy - size, cx + size, cy + size, auraCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            DrawLine(cx - size, cy - size, cx - size + len, cy - size, auraCol, 3);
            DrawLine(cx - size, cy - size, cx - size, cy - size + len, auraCol, 3);
            DrawLine(cx + size - len, cy - size, cx + size, cy - size, auraCol, 3);
            DrawLine(cx + size, cy - size, cx + size, cy - size + len, auraCol, 3);
            DrawLine(cx - size, cy + size, cx - size + len, cy + size, auraCol, 3);
            DrawLine(cx - size, cy + size - len, cx - size, cy + size, auraCol, 3);
            DrawLine(cx + size - len, cy + size, cx + size, cy + size, auraCol, 3);
            DrawLine(cx + size, cy + size - len, cx + size, cy + size, auraCol, 3);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        if (view.p1.unit) view.p1.unit->Draw();
        if (view.p2.unit) view.p2.unit->Draw();
    }

} // namespace App
