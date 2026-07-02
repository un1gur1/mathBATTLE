#pragma once
#include "SceneBase.h"
#include <vector>
#include <string>

namespace App {

    // ==========================================
    // TitleScene: タイトル画面のシーン
    // 用途: メニュー階層管理とゲーム設定
    // ==========================================
    class TitleScene : public SceneBase {
    public:
        // ==========================================
        // TitleState: 大枠のメニュー画面の状態
        // ==========================================
        enum class TitleState {
            PRESS_START,    // [0] タイトルコール（スペースを押してね）
            MAIN_MENU,      // [1] トップメニュー（バトル、通信、チュートリアル等）
            BATTLE_SETUP,   // [2] オフラインバトルの詳細設定
            NETWORK_SETUP,  // [3] ★新規：通信対戦のマッチング画面
            OPTION_MENU,    // [4] オプション・クレジット画面
            EXIT_CONFIRM    // [5] 終了確認画面
        };

        // ==========================================
        // SetupStep: オフラインバトル設定内の進行ステップ
        // ==========================================
        enum class SetupStep {
            SELECT_PLAYERS,         // シングル / 2P
            SELECT_MODE,            // クラシック / ゼロワン
            SELECT_CLASSIC_STOCKS,  // 残機設定
            SELECT_SCORE,           // 目標スコア設定
            SELECT_P1_TYPE,         // 1P操作（プレイヤー/NPC）
            SELECT_P2_TYPE,         // 2P操作（プレイヤー/NPC）
            SELECT_STAGE,           // ステージ選択
            CUSTOM_P1_START,        // 1P初期位置
            CUSTOM_P2_START         // 2P初期位置
        };

        // ==========================================
        // NetSetupStep: 通信対戦マッチングの進行ステップ
        // ==========================================
        enum class NetSetupStep {
            SELECT_ROLE,      // ホストになるか、クライアントになるか
            HOST_WAITING,     // ホストとして待機中（UDP送信中）
            CLIENT_SEARCHING,  // クライアントとして部屋を検索中（UDP受信中）
            CLIENT_WAIT_SETUP

        };

        TitleScene();
        ~TitleScene() override;

        void Init() override;
        void Load() override;
        void LoadEnd() override;
        void Update() override;
        void Draw() override;
        void Release() override;

    private:
        static constexpr int GRID_SIZE = 9;

        static constexpr int DEF_P1_HP = 5;
        static constexpr int DEF_P1_X = 3;
        static constexpr int DEF_P1_Y = 3;

        static constexpr int DEF_P2_HP = 5;
        static constexpr int DEF_P2_X = 7;
        static constexpr int DEF_P2_Y = 7;

        struct PlayerConfig {
            int typeCursor;
            int customCursor;
            int startNum;
            int startX;
            int startY;
        };

        // ==========================================
        // 状態管理・カーソル
        // ==========================================
        TitleState m_titleState;
        SetupStep m_setupStep;
        NetSetupStep m_netStep; // ★通信マッチング用のステート
        int m_frameCount;

        int m_mainMenuCursor;       // トップメニューのカーソル
        int m_exitCursor;           // 終了確認のカーソル

        // バトル設定用のカーソル
        int m_playerCursor;
        int m_modeCursor;
        int m_stocksCursor;
        int m_scoreCursor;
        int m_stageCursor;

        // 通信マッチング用のカーソル
        int m_netRoleCursor;
        int m_hostListCursor;

        PlayerConfig m_players[2];

        bool m_shouldQuit;

        // ==========================================
        // リソースハンドル
        // ==========================================
        int m_fontTitle;
        int m_fontMenu;
        int m_fontSmall;
        int m_fontNumber;

        int m_psHandle = -1;
        int m_cbHandle = -1;
        float m_shaderTime;
    };

} // namespace App