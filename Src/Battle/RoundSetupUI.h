#pragma once

namespace App {
    struct BattleViewData;

    // ラウンドバトル開始時のオーバーレイ表示を担当する。
    // 選択結果やフェーズ遷移はBattleMaster側が管理し、ここでは描画だけ行う。
    class RoundSetupUI {
    public:
        void Draw(const BattleViewData& view) const;
    };
}
