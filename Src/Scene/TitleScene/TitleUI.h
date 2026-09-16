#pragma once

#include <string>
#include <vector>

#include "TitleCommon.h"
#include "BattleSetup.h"
#include "../../Manager/AccumulationCalc.h"

namespace App {

    struct TitleViewData {
        TitleState titleState = TitleState::PRESS_START;
        NetSetupStep netStep = NetSetupStep::SELECT_ROLE;
        const BattleSetup* setup = nullptr;

        int mainMenuCursor = 0;
        int exitCursor = 0;
        int netRoleCursor = 0;
        int hostListCursor = 0;

        bool networkConnected = false;
        bool isNetworkHost = false;
        std::vector<std::string> hostEntries;
    };

    // タイトル画面の描画と視覚演出だけを担当する。
    // ゲーム開始条件・通信状態遷移・BattleSetupの書き換えは行わない。
    class TitleUI {
    public:
        TitleUI();
        ~TitleUI();

        void Init();
        void Update(bool updateWaitingMiniGame);
        void Draw(const TitleViewData& view);
        void Release();

    private:
        struct BouncingOp {
            float x = 0.0f;
            float y = 0.0f;
            float vx = 0.0f;
            float vy = 0.0f;
            float angle = 0.0f;
            std::string symbol;
            unsigned int color = 0;
        };

        struct RippleEffect {
            float x = 0.0f;
            float y = 0.0f;
            float radius = 0.0f;
            float alpha = 0.0f;
            unsigned int color = 0;
        };

        int m_fontTitle = -1;
        int m_fontMenu = -1;
        int m_fontSmall = -1;
        int m_fontNumber = -1;

        int m_psHandle = -1;
        int m_cbHandle = -1;
        int m_psCrystalHandle = -1;
        int m_cbCrystalHandle = -1;
        int m_psImpactHandle = -1;
        int m_cbImpactHandle = -1;
        int m_impactType = 0;

        float m_shaderTime = 0.0f;
        std::vector<BouncingOp> m_bouncingOps;
        std::vector<RippleEffect> m_ripples;
        AccumulationCalc m_miniGame;
    };

} // namespace App
