#pragma once

namespace App {

    struct BattleViewData;

    // ==========================================
    // BattleBoardUI
    // 盤面上に描くものだけを担当する。
    // - MapGrid
    // - ワープ地点
    // - 危険範囲 / 移動可能範囲
    // - アクティブ駒ハイライト
    // - Player / Enemy 本体
    // ==========================================
    class BattleBoardUI {
    public:
        void Draw(const BattleViewData& view) const;

    private:
        void DrawEnemyDangerArea(const BattleViewData& view) const;
        void DrawMovableArea(const BattleViewData& view) const;
    };

} // namespace App
