#pragma once
#include "SceneBase.h"
#include "../Manager/FadeManager.h"

namespace App {
    class Loading;
    class GameScene;
    class TitleScene;
    class ResultScene;
    class TutorialScene;
    class PauseMenu;

    // ==========================================
    // BattleStats: バトルの戦績データ（1人分）
    // ==========================================
    struct BattleStats {
        int totalTurns;      // 共通（経過ターン）
        int playTimeFrames;  // 共通（プレイ時間）
        int totalMoves;      // 個人の移動マス数
        int totalOpsUsed;    // 個人の使用演算子数
        int maxDamage;       // 個人の最大ダメージ
        int m_maxStocks;     // ★復元：最大残機数
    };

    class SceneManager {
    public:
        enum class SCENE_ID {
            NONE, TITLE, TUTORIAL, GAME, RESULT
        };

        static void CreateInstance() { if (instance_ == nullptr) { instance_ = new SceneManager(); } }
        static SceneManager* GetInstance() { return instance_; }
        static void DeleteInstance() { if (instance_ != nullptr) { delete instance_; instance_ = nullptr; } }

        void Init();
        void Init3D();
        void Update();
        void Draw();
        void Delete();
        void ChangeScene(SCENE_ID nextId);
        SCENE_ID GetSceneID() const { return sceneId_; }

        void GameEnd() { isGameEnd_ = true; }
        bool GetGameEnd() const { return isGameEnd_; }

        void SetGameSettings(int players, int mode, int zeroOneScore = 501) {
            playerCount_ = players; gameMode_ = mode; zeroOneScore_ = zeroOneScore;
        }
        int GetPlayerCount() const { return playerCount_; }
        int GetGameMode() const { return gameMode_; }
        int GetZeroOneScore() const { return zeroOneScore_; }
        int GetMaxStocks() const { return zeroOneScore_; }      // ★復元：最大残機数取得
        void TogglePause();

        void SetPlayer1Settings(bool isNPC, int startNum, int startX, int startY) {
            is1P_NPC_ = isNPC; p1StartNum_ = startNum; p1StartX_ = startX; p1StartY_ = startY;
        }
        void SetPlayer2Settings(bool isNPC, int startNum, int startX, int startY) {
            is2P_NPC_ = isNPC; p2StartNum_ = startNum; p2StartX_ = startX; p2StartY_ = startY;
        }

        bool Is1PNPC() const { return is1P_NPC_; }
        bool Is2PNPC() const { return is2P_NPC_; }
        int Get1PStartNum() const { return p1StartNum_; }
        int Get2PStartNum() const { return p2StartNum_; }
        int Get1PStartX() const { return p1StartX_; }
        int Get1PStartY() const { return p1StartY_; }
        int Get2PStartX() const { return p2StartX_; }
        int Get2PStartY() const { return p2StartY_; }

        void SetStageIndex(int idx) { m_stageIndex = idx; }
        int GetStageIndex() const { return m_stageIndex; }

        void SetBattleResult(int winnerPlayer, const BattleStats& p1Stats, const BattleStats& p2Stats) {
            m_winnerPlayer = winnerPlayer;
            m_p1Stats = p1Stats;
            m_p2Stats = p2Stats;
        }
        int GetWinnerPlayer() const { return m_winnerPlayer; }
        const BattleStats& GetP1Stats() const { return m_p1Stats; }
        const BattleStats& GetP2Stats() const { return m_p2Stats; }

        void SetGameEnd(bool isEnd);
        bool IsGameEnd() const;

    private:
        SceneManager();
        ~SceneManager();
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;

        void PerformSceneChange();
        static SceneManager* instance_;

        SceneBase* scene_;
        Loading* load_;
        PauseMenu* pauseMenu_;
        FadeManager m_fade;

        SCENE_ID sceneId_;
        SCENE_ID nextSceneId_;
        bool isChanging_;
        bool isGameEnd_;
        bool isPaused_;
        int playerCount_;
        int gameMode_;
        int zeroOneScore_;
        bool is1P_NPC_;
        bool is2P_NPC_;
        int  p1StartNum_;
        int  p2StartNum_;
        int  p1StartX_;
        int  p1StartY_;
        int  p2StartX_;
        int  p2StartY_;
        int m_stageIndex;

        int m_winnerPlayer;
        BattleStats m_p1Stats;
        BattleStats m_p2Stats;
        bool m_gameEnd = false;
    };
}