#pragma once

#include <string>

#include "BattleBoardUI.h"
#include "BattleLogUI.h"
#include "BattleStatusUI.h"
#include "CommUI.h"
#include "RoundSetupUI.h"

namespace App {

    struct BattleViewData;
    class TutorialMaster;

    // ==========================================
    // BattleUI
    // バトルUI全体の描画順を管理する司令塔。
    //
    // 個別責務は以下へ分離:
    // - BattleBoardUI  : 盤面
    // - BattleStatusUI : ステータス / 計算プレビュー / 操作ボタン
    // - BattleLogUI    : 戦況ログ
    // - CommUI         : 通信UI
    //
    // 通常バトルでは BattleViewData だけを受け取り、
    // ゲームルールの計算は行わない。
    // ==========================================
    class BattleUI {
    public:
        BattleUI();
        ~BattleUI();

        void Init();
        void Update(float effectIntensity, int p1Num, int p2Num);

        void Draw(const BattleViewData& view) const;

        // チュートリアルは既存構造との互換性を維持。
        void Draw(const TutorialMaster& master) const;

        void AddLog(const std::string& message);
        void ScrollLog(int wheelDelta, float mouseX, float mouseY);

        // 既存コード互換用。実体は BattleUICommon 側の共有キャッシュ。
        static int GetCachedFont(int size);

    private:
        void DrawBackground() const;
        void DrawHeader(const BattleViewData& view) const;
        void DrawTurnStartCutIn(const BattleViewData& view) const;
        void DrawPauseButton(const BattleViewData& view) const;
        void DrawFinishOverlay(const BattleViewData& view) const;

        int m_psHandle;
        int m_cbHandle;
        float m_shaderTime;

        float m_uiCursorX_1P;
        float m_uiCursorX_2P;

        BattleBoardUI m_boardUI;
        BattleStatusUI m_statusUI;
        BattleLogUI m_logUI;
        RoundSetupUI m_roundSetupUI;
        CommUI m_commUI;
    };

} // namespace App
