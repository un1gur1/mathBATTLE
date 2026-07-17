#pragma once
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include "../Common/Vector2.h"
#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/Enemy/Enemy.h"
#include "../Object/Unit/Player/Player.h"
#include "../Battle/BattleUI.h"

namespace App {

    class TutorialMaster {
        friend class BattleUI;

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

        // ==========================================
        // Åöí«â¡ÅFBattleUI Ç™ÉJÉEÉìÉgÉoÉgÉãÇï`âÊÇ∑ÇÈÇΩÇﬂÇÃïœêî
        // ==========================================
        int m_ruleMode = 0; // 0: CLASSIC, 1: ZERO_ONE
        int m_targetScore = 53;
        int m_p1Score = 0;
        int m_p2Score = 0;
        float m_p1DisplayScore = 0.0f;
        float m_p2DisplayScore = 0.0f;

    private:
        enum class Step {
            Menu,

            // 1. äÓëb
            Basic_Welcome, Basic_Wait_Move1, Basic_Res_Move1, Basic_Wait_Move2, Basic_Res_Move2, Basic_Wait_Move3, Basic_End,

            // 2. é¿ëHá@
            B1_Welcome, B1_Plus_Wait, B1_Plus_Atk, B1_Plus_Apply, B1_Plus_Res,
            B1_Minus_Setup, B1_Minus_Wait, B1_Minus_Atk, B1_Minus_Apply, B1_Minus_Res,
            B1_Mul_Setup, B1_Mul_Wait, B1_Mul_Atk, B1_Mul_Apply, B1_End,

            // 3. é¿ëHáA
            B2_Welcome, B2_Div_Wait, B2_Div_Atk, B2_Div_Apply, B2_Res, B2_End,

            // 4. ÉãÅ[Éãá@
            RuleN_Welcome, RuleN_Wait, RuleN_Atk, RuleN_Apply, RuleN_End,

            // 5. ÉãÅ[ÉãáA
            RuleC_Welcome, RuleC_Wait, RuleC_Atk, RuleC_Apply, RuleC_End
        };

        Step m_currentStep;
        int m_stepTimer;
        int m_menuCursor;
        int m_fontMsg;

        IntVector2 m_hoverGrid;
        IntVector2 m_targetMove1;
        IntVector2 m_targetMove2;
        IntVector2 m_targetMove3;

        void NextStep();
        void StartTutorial(int index);
        void SetupSituation(int sitId);

        bool CheckButtonClick(int x, int y, int w, int h) const;
        int GetCachedFont(int size) const;
        void DrawCyberButton(int x, int y, int w, int h, const char* text, unsigned int col, bool isHover, int fontHandle) const;
        void DrawButtonHighlight(int x, int y, int w, int h, unsigned int col) const;
        void DrawFakeCalcPanel(int aNum, char op, int dNum, int res, bool isDiv, bool guideSelf, bool isCountMode = false) const;

        void DrawMenu() const;
        void DrawMessageWindow(const std::string& text) const;
        void DrawHighlightGrid(const IntVector2& gridPos, unsigned int color) const;
    };

} // namespace App