#pragma once
#include "SceneBase.h"
#include "SceneManager.h"
#include <string>

namespace App {

    class ResultScene : public SceneBase {
    public:
        ResultScene();
        virtual ~ResultScene();

        virtual void Init() override;
        virtual void Load() override;
        virtual void LoadEnd() override;
        virtual void Update() override;
        virtual void Draw() override;
        virtual void Release() override;

    private:
        enum class State {
            FADE_IN,
            COUNT_STATS,
            WAIT_SCORE,
            COUNT_SCORE,
            RANK_STAMP,
            WAIT_INPUT
        };

        State m_state;
        int m_frameCount;
        int m_stateTimer;

        int m_psHandle;
        int m_cbHandle;
        int m_fontTitle;
        int m_fontLabel;
        int m_fontNum;
        int m_fontRank;

        // ★ 1P・2Pのデータをそれぞれ保持
        int m_winner; // 1 or 2
        BattleStats m_p1Stats;
        BattleStats m_p2Stats;

        int m_p1FinalScore;
        int m_p2FinalScore;
        std::string m_p1Rank;
        std::string m_p2Rank;
        unsigned int m_p1RankCol;
        unsigned int m_p2RankCol;

        // カウントアップ用（共通）
        float m_dispTime;
        float m_dispTurns;
        // カウントアップ用（個別）
        float m_dispP1Moves, m_dispP2Moves;
        float m_dispP1Ops, m_dispP2Ops;
        float m_dispP1Dmg, m_dispP2Dmg;
        float m_dispP1Score, m_dispP2Score;

        float m_rankScale;
        float m_bgOffset;

        void CalculateScoreAndRank();
    };

} // namespace App