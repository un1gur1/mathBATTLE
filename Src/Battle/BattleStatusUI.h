#pragma once

namespace App {

    struct BattleViewData;
    struct BattleUnitView;

    // ==========================================
    // BattleStatusUI
    // 盤面外のバトル情報表示を担当する。
    // - 1P / 2P ステータスカード
    // - 基本ルール表示
    // - 計算結果プレビュー
    // - 行動フェーズのボタン
    //
    // BattleViewData は計算済みなので、ここではルール計算しない。
    // ==========================================
    class BattleStatusUI {
    public:
        void Draw(const BattleViewData& view, float cursor1P, float cursor2P) const;

    private:
        void DrawUnitCard(
            int x,
            int y,
            const BattleViewData& view,
            const BattleUnitView& unitView,
            bool is1P,
            float cursorX
        ) const;

        void DrawRulePanel(const BattleViewData& view) const;
        void DrawCalculationPanel(const BattleViewData& view) const;
        void DrawActionButtons(const BattleViewData& view) const;
    };

} // namespace App
