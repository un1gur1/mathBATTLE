#pragma once

#include "../SceneBase.h"
#include "TitleCommon.h"
#include "BattleSetup.h"
#include "TitleUI.h"

namespace App {

    // タイトル全体の画面遷移と通信セットアップを管理する。
    // 描画/視覚演出はTitleUI、対戦設定データはBattleSetupに分離する。
    class TitleScene : public SceneBase {
    public:
        TitleScene();
        ~TitleScene() override;

        void Init() override;
        void Load() override;
        void LoadEnd() override;
        void Update() override;
        void Draw() override;
        void Release() override;

        bool IsInStandby() const { return m_titleState == TitleState::PRESS_START; }
        bool IsWarping() const { return m_titleState == TitleState::WARP_DIVE; }
        float GetWarpProgress() const { return m_warpProgress; }

    private:
        TitleState m_titleState;
        NetSetupStep m_netStep;
        int m_frameCount;

        int m_mainMenuCursor;
        int m_exitCursor;
        int m_netRoleCursor;
        int m_hostListCursor;

        float m_warpProgress;

        BattleSetup m_battleSetup;
        TitleUI m_ui;
    };

} // namespace App
