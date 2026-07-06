#pragma once
#include <memory>
#include <queue>
#include <string>
#include "../Common/Vector2.h"
#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/Enemy/Enemy.h"
#include "../Object/Unit/Player/Player.h"
#include "../Battle/BattleUI.h"


namespace App {

    class TutorialMaster {
        friend class BattleUI; // BattleUIからの描画アクセス許可

    public:
        TutorialMaster();
        ~TutorialMaster();

        void Init();
        void Update();
        void Draw() const;
        void Release();

        MapGrid m_mapGrid;

        std::unique_ptr<Player> m_player;
        std::unique_ptr<Enemy> m_enemy;
        std::unique_ptr<BattleUI> m_ui;

    private:
        // ==========================================
        // チュートリアルの進行状態（基本操作に特化）
        // ==========================================
        enum class Step {
            Msg_Welcome,
            Msg_WinLoseRule,
            Msg_MoveRule,
            Wait_Move1,
            Msg_NumberChanged, // ★追加：移動で数字が変わることを教える！
            Msg_OpRule,
            Wait_Move2,
            Msg_BattleRule,
            Wait_Move3,
            Wait_ApplyDamage,
            Msg_Finish
        };

        Step m_currentStep;
        int m_stepTimer;

        IntVector2 m_hoverGrid;

        int m_fontMsg;

        // 目標マス（基本の3ステップのみ）
        IntVector2 m_targetMove1;
        IntVector2 m_targetMove2;
        IntVector2 m_targetMove3;

        void NextStep();
        void DrawMessageWindow(const std::string& text) const;
        void DrawHighlightGrid(const IntVector2& gridPos, unsigned int color) const;
    };

} // namespace App