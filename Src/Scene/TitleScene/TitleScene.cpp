#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <DxLib.h>

#include "TitleScene.h"
#include "TitleUILayout.h"
#include "../SceneManager.h"
#include "../../Input/InputManager.h"
#include "../../Manager/ProceduralAudio.h"
#include "../../Manager/NetworkManager.h"

#include <string>

namespace App {
    using namespace TitleLayout;

    TitleScene::TitleScene()
        : m_titleState(TitleState::PRESS_START)
        , m_netStep(NetSetupStep::SELECT_ROLE)
        , m_frameCount(0)
        , m_mainMenuCursor(0)
        , m_exitCursor(0)
        , m_netRoleCursor(0)
        , m_hostListCursor(0)
        , m_warpProgress(0.0f) {
    }

    TitleScene::~TitleScene() = default;

    void TitleScene::Init() {
        m_frameCount = 0;
        m_titleState = TitleState::PRESS_START;
        m_netStep = NetSetupStep::SELECT_ROLE;
        m_mainMenuCursor = 0;
        m_exitCursor = 0;
        m_netRoleCursor = 0;
        m_hostListCursor = 0;
        m_warpProgress = 0.0f;

        m_battleSetup.Init();

        ProceduralAudio::GetInstance().PlayBGM(true);
        m_ui.Init();

        if (NetworkManager::GetInstance() == nullptr) {
            NetworkManager::CreateInstance();
        }
        NetworkManager::GetInstance()->Init();
    }

    void TitleScene::Load() {}
    void TitleScene::LoadEnd() {}

    void TitleScene::Update() {
        auto& input = InputManager::GetInstance();
        ProceduralAudio::GetInstance().Update();

        if (NetworkManager::GetInstance() != nullptr) {
            NetworkManager::GetInstance()->Update();
        }

        ++m_frameCount;
        if (m_frameCount < WAIT_START_FRAMES) return;

        const bool updateWaitingMiniGame =
            m_titleState == TitleState::NETWORK_SETUP &&
            m_netStep == NetSetupStep::CLIENT_WAIT_SETUP &&
            NetworkManager::GetInstance() != nullptr &&
            !NetworkManager::GetInstance()->IsHost();
        m_ui.Update(updateWaitingMiniGame);

        Vector2 m = input.GetMousePos();
        bool mClick = input.IsMouseLeftTrg();
        static Vector2 prevM = m;
        bool mouseMoved = (m.x != prevM.x || m.y != prevM.y);
        prevM = m;

        bool spaceTrg = input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN);
        bool upTrg = input.IsTrgDown(KEY_INPUT_UP) || input.IsTrgDown(KEY_INPUT_W);
        bool downTrg = input.IsTrgDown(KEY_INPUT_DOWN) || input.IsTrgDown(KEY_INPUT_S);
        bool rightTrg = input.IsTrgDown(KEY_INPUT_RIGHT) || input.IsTrgDown(KEY_INPUT_D);
        bool leftTrg = input.IsTrgDown(KEY_INPUT_LEFT) || input.IsTrgDown(KEY_INPUT_A);
        bool bTrg = input.IsTrgDown(KEY_INPUT_B) || input.IsTrgDown(KEY_INPUT_BACK);

        int hitNum = -1;
        for (int i = 0; i < BattleSetup::GRID_SIZE; ++i) {
            if (input.IsTrgDown(KEY_INPUT_1 + i) || input.IsTrgDown(KEY_INPUT_NUMPAD1 + i)) {
                hitNum = i + 1;
            }
        }

        int sw = 0, sh = 0;
        GetDrawScreenSize(&sw, &sh);

        auto HoverBox = [&](int x, int y, int w, int h) {
            return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
            };

        int CX = sw / 2;
        int CY = sh / 2;
        int menuStartY = CY + MENU_CENTER_OFFSET_Y;

        int backBtnX = CX - 150, backBtnY = sh - 150, backBtnW = 300, backBtnH = 60;
        bool isBackBtnClicked = mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH);

        bool isCustomState = (m_titleState == TitleState::BATTLE_SETUP &&
            (m_battleSetup.step == BattleSetup::Step::CUSTOM_P1_START ||
                m_battleSetup.step == BattleSetup::Step::CUSTOM_P2_START));
        int menuCX = isCustomState ? CX + MENU_CUSTOM_OFFSET_X : CX;

        auto StartGameLogic = [&]() {
            auto* sm = SceneManager::GetInstance();
            if (m_battleSetup.modeCursor == 0) {
                sm->SetGameSettings(m_battleSetup.playerCursor, m_battleSetup.modeCursor, m_battleSetup.GetSelectedMaxStocks());
            }
            else if (m_battleSetup.modeCursor == 1) {
                sm->SetGameSettings(m_battleSetup.playerCursor, m_battleSetup.modeCursor, m_battleSetup.GetSelectedTargetScore());
            }
            else {
                // ラウンドバトルではタイトルでラウンド固有値を決めない。
                // 現段階では試合全体のSTOCKを3に固定してBattle側へ渡す。
                sm->SetGameSettings(m_battleSetup.playerCursor, m_battleSetup.modeCursor, m_battleSetup.GetRoundBattleStocks());
            }

            sm->SetPlayer1Settings(
                m_battleSetup.players[0].typeCursor == 1,
                m_battleSetup.players[0].startNum,
                m_battleSetup.players[0].startX - 1,
                BattleSetup::GRID_SIZE - m_battleSetup.players[0].startY);
            sm->SetPlayer2Settings(
                m_battleSetup.players[1].typeCursor == 1,
                m_battleSetup.players[1].startNum,
                m_battleSetup.players[1].startX - 1,
                BattleSetup::GRID_SIZE - m_battleSetup.players[1].startY);
            sm->SetStageIndex(m_battleSetup.stageCursor);
            sm->ChangeScene(SceneManager::SCENE_ID::GAME);
            };

        auto HandleMenuInput = [&](int& cursor, int maxItems) {
            if (maxItems <= 0) return;
            for (int i = 0; i < maxItems; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W,
                    menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y,
                    MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && cursor != i) {
                        cursor = i;
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                    if (mClick) { cursor = i; spaceTrg = true; }
                }
            }
            if (upTrg) { cursor--; if (cursor < 0) cursor = maxItems - 1; ProceduralAudio::GetInstance().PlayPowerSE(2); }
            if (downTrg) { cursor++; if (cursor >= maxItems) cursor = 0; ProceduralAudio::GetInstance().PlayPowerSE(2); }
            };

        switch (m_titleState) {
        case TitleState::PRESS_START:
            if (spaceTrg || mClick) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                m_titleState = TitleState::WARP_DIVE;
                m_warpProgress = 0.0f;
            }
            break;

        case TitleState::WARP_DIVE:
            m_warpProgress += 0.012f;
            if (m_warpProgress >= 1.0f) {
                m_titleState = TitleState::MAIN_MENU;
                m_frameCount = 0;
            }
            break;

        case TitleState::MAIN_MENU:
            HandleMenuInput(m_mainMenuCursor, 5);
            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                m_titleState = TitleState::PRESS_START;
            }
            else if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                if (m_mainMenuCursor == 0) {
                    m_titleState = TitleState::BATTLE_SETUP;
                    m_battleSetup.step = BattleSetup::Step::SELECT_PLAYERS;
                }
                else if (m_mainMenuCursor == 1) {
                    m_titleState = TitleState::NETWORK_SETUP;
                    m_netStep = NetSetupStep::SELECT_ROLE;
                    m_netRoleCursor = 0;
                }
                else if (m_mainMenuCursor == 2) {
                    SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TUTORIAL);
                }
                else if (m_mainMenuCursor == 3) {
                    m_titleState = TitleState::OPTION_MENU;
                }
                else if (m_mainMenuCursor == 4) {
                    m_titleState = TitleState::EXIT_CONFIRM;
                }
            }
            break;

        case TitleState::NETWORK_SETUP:
            switch (m_netStep) {
            case NetSetupStep::SELECT_ROLE:
                HandleMenuInput(m_netRoleCursor, 2);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_titleState = TitleState::MAIN_MENU;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    if (m_netRoleCursor == 0) {
                        std::string myName = "Player_" + std::to_string(GetNowCount() % 10000);
                        NetworkManager::GetInstance()->StartHost(myName);
                        m_netStep = NetSetupStep::HOST_WAITING;
                    }
                    else {
                        NetworkManager::GetInstance()->StartSearch();
                        m_netStep = NetSetupStep::CLIENT_SEARCHING;
                        m_hostListCursor = 0;
                    }
                }
                break;

            case NetSetupStep::HOST_WAITING:
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    NetworkManager::GetInstance()->Disconnect();
                    m_netStep = NetSetupStep::SELECT_ROLE;
                }
                if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_titleState = TitleState::BATTLE_SETUP;
                    m_battleSetup.step = BattleSetup::Step::SELECT_MODE;
                    m_battleSetup.players[0].typeCursor = 0;
                    m_battleSetup.players[1].typeCursor = 0;
                }
                break;

            case NetSetupStep::CLIENT_SEARCHING:
            {
                auto hosts = NetworkManager::GetInstance()->GetHostList();
                if (!hosts.empty()) {
                    if (m_hostListCursor >= (int)hosts.size()) m_hostListCursor = 0;
                    HandleMenuInput(m_hostListCursor, (int)hosts.size());
                }

                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    NetworkManager::GetInstance()->Disconnect();
                    m_netStep = NetSetupStep::SELECT_ROLE;
                }
                else if (spaceTrg && !hosts.empty()) {
                    ProceduralAudio::GetInstance().PlayPowerSE(8);
                    NetworkManager::GetInstance()->ConnectToHost(hosts[m_hostListCursor].ip);
                }

                if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_netStep = NetSetupStep::CLIENT_WAIT_SETUP;
                }
                break;
            }
            case NetSetupStep::CLIENT_WAIT_SETUP:
            {
                SetupPacket packet;

                // ★ 通信対戦：ホストとクライアントの同期処理
                if (NetworkManager::GetInstance()->IsHost()) {
                    // ホストの場合：クライアント(2P)のセットアップ完了を待つ
                    if (NetworkManager::GetInstance()->ReceiveSetupPacket(packet)) {
                        ProceduralAudio::GetInstance().PlayPowerSE(9);
                        m_battleSetup.players[1].startNum = packet.p2StartNum;
                        m_battleSetup.players[1].startX = packet.p2StartX;
                        m_battleSetup.players[1].startY = packet.p2StartY;
                        StartGameLogic(); // 両方の準備が完了したのでゲーム開始！
                    }

                    if (bTrg || isBackBtnClicked) {
                        ProceduralAudio::GetInstance().PlayErrorSE();
                        m_titleState = TitleState::BATTLE_SETUP;
                        m_battleSetup.step = BattleSetup::Step::CUSTOM_P1_START;
                    }
                }
                else {
                    // クライアントの場合：ホストのルール＆1P設定を待つ

                    if (NetworkManager::GetInstance()->ReceiveSetupPacket(packet)) {
                        ProceduralAudio::GetInstance().PlayPowerSE(9);
                        m_battleSetup.modeCursor = packet.modeCursor;
                        m_battleSetup.stocksCursor = packet.stocksCursor;
                        m_battleSetup.scoreCursor = packet.scoreCursor;
                        m_battleSetup.stageCursor = packet.stageCursor;
                        m_battleSetup.players[0].startNum = packet.p1StartNum;
                        m_battleSetup.players[0].startX = packet.p1StartX;
                        m_battleSetup.players[0].startY = packet.p1StartY;

                        // 自分の(2P)配置選択画面へ移行
                        m_titleState = TitleState::BATTLE_SETUP;
                        m_battleSetup.step = BattleSetup::Step::CUSTOM_P2_START;
                        m_battleSetup.players[1].customCursor = 0;
                    }

                    if (bTrg || isBackBtnClicked) {
                        ProceduralAudio::GetInstance().PlayErrorSE();
                        NetworkManager::GetInstance()->Disconnect();
                        m_netStep = NetSetupStep::SELECT_ROLE;
                    }
                }
                break;
            }
            }
            break;

        case TitleState::OPTION_MENU:
            if (bTrg || isBackBtnClicked || spaceTrg) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                m_titleState = TitleState::MAIN_MENU;
            }
            break;

        case TitleState::EXIT_CONFIRM:
            HandleMenuInput(m_exitCursor, 2);
            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                m_titleState = TitleState::MAIN_MENU;
                m_exitCursor = 0;
            }
            else if (spaceTrg) {
                if (m_exitCursor == 0) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_titleState = TitleState::MAIN_MENU;
                    m_exitCursor = 0;
                }
                else {
                    SceneManager::GetInstance()->SetGameEnd(true);
                    return;
                }
            }
            break;

        case TitleState::BATTLE_SETUP:
            switch (m_battleSetup.step) {
            case BattleSetup::Step::SELECT_PLAYERS:
                HandleMenuInput(m_battleSetup.playerCursor, 2);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_titleState = TitleState::MAIN_MENU;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_battleSetup.playerCursor++;
                    m_battleSetup.players[1].typeCursor = (m_battleSetup.playerCursor == 1) ? 1 : 0;
                    m_battleSetup.step = BattleSetup::Step::SELECT_MODE;
                }
                break;

            case BattleSetup::Step::SELECT_MODE:
                HandleMenuInput(m_battleSetup.modeCursor, 3);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED) {
                        NetworkManager::GetInstance()->Disconnect();
                        m_titleState = TitleState::NETWORK_SETUP;
                        m_netStep = NetSetupStep::SELECT_ROLE;
                    }
                    else {
                        m_battleSetup.step = BattleSetup::Step::SELECT_PLAYERS;
                    }
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    if (m_battleSetup.modeCursor == 0) {
                        m_battleSetup.step = BattleSetup::Step::SELECT_CLASSIC_STOCKS;
                    }
                    else if (m_battleSetup.modeCursor == 1) {
                        m_battleSetup.step = BattleSetup::Step::SELECT_SCORE;
                    }
                    else {
                        const bool isOnline = NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED;
                        if (isOnline) {
                            m_battleSetup.players[0].typeCursor = 0;
                            m_battleSetup.players[1].typeCursor = 0;
                            m_battleSetup.step = BattleSetup::Step::SELECT_STAGE;
                        }
                        else {
                            m_battleSetup.step = BattleSetup::Step::SELECT_P1_TYPE;
                        }
                    }
                }
                break;

            case BattleSetup::Step::SELECT_CLASSIC_STOCKS:
                HandleMenuInput(m_battleSetup.stocksCursor, 3);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_battleSetup.step = BattleSetup::Step::SELECT_MODE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED) {
                        m_battleSetup.players[0].typeCursor = 0;
                        m_battleSetup.players[1].typeCursor = 0;
                        m_battleSetup.step = BattleSetup::Step::SELECT_STAGE;
                    }
                    else {
                        m_battleSetup.step = BattleSetup::Step::SELECT_P1_TYPE;
                    }
                }
                break;

            case BattleSetup::Step::SELECT_SCORE:
                HandleMenuInput(m_battleSetup.scoreCursor, 3);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_battleSetup.step = BattleSetup::Step::SELECT_MODE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED) {
                        m_battleSetup.players[0].typeCursor = 0;
                        m_battleSetup.players[1].typeCursor = 0;
                        m_battleSetup.step = BattleSetup::Step::SELECT_STAGE;
                    }
                    else {
                        m_battleSetup.step = BattleSetup::Step::SELECT_P1_TYPE;
                    }
                }
                break;

            case BattleSetup::Step::SELECT_P1_TYPE:
                HandleMenuInput(m_battleSetup.players[0].typeCursor, 2);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    if (m_battleSetup.modeCursor == 0) m_battleSetup.step = BattleSetup::Step::SELECT_CLASSIC_STOCKS;
                    else if (m_battleSetup.modeCursor == 1) m_battleSetup.step = BattleSetup::Step::SELECT_SCORE;
                    else m_battleSetup.step = BattleSetup::Step::SELECT_MODE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_battleSetup.step = BattleSetup::Step::SELECT_P2_TYPE;
                }
                break;

            case BattleSetup::Step::SELECT_P2_TYPE:
                HandleMenuInput(m_battleSetup.players[1].typeCursor, 2);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_battleSetup.step = BattleSetup::Step::SELECT_P1_TYPE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_battleSetup.step = BattleSetup::Step::SELECT_STAGE;
                }
                break;

            case BattleSetup::Step::SELECT_STAGE:
                HandleMenuInput(m_battleSetup.stageCursor, 3);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED) {
                        if (m_battleSetup.modeCursor == 0) m_battleSetup.step = BattleSetup::Step::SELECT_CLASSIC_STOCKS;
                        else if (m_battleSetup.modeCursor == 1) m_battleSetup.step = BattleSetup::Step::SELECT_SCORE;
                        else m_battleSetup.step = BattleSetup::Step::SELECT_MODE;
                    }
                    else {
                        m_battleSetup.step = BattleSetup::Step::SELECT_P2_TYPE;
                    }
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_battleSetup.step = BattleSetup::Step::CUSTOM_P1_START;
                }
                break;

            case BattleSetup::Step::CUSTOM_P1_START:
            case BattleSetup::Step::CUSTOM_P2_START:
            {
                bool is1P = (m_battleSetup.step == BattleSetup::Step::CUSTOM_P1_START);
                int pIdx = is1P ? 0 : 1;
                auto& p = m_battleSetup.players[pIdx];

                // ★ マップを直接クリックして初期位置を決定する処理
                int mapBaseX = CX + MAP_BASE_OFFSET_X;
                int mapBaseY = menuStartY + MAP_BASE_OFFSET_Y;
                if (m.x >= mapBaseX && m.x < mapBaseX + MAP_CELL_SIZE * BattleSetup::GRID_SIZE &&
                    m.y >= mapBaseY && m.y < mapBaseY + MAP_CELL_SIZE * BattleSetup::GRID_SIZE) {
                    if (mClick) {
                        int gridX = (int)(m.x - mapBaseX) / MAP_CELL_SIZE;
                        int gridY = (int)(m.y - mapBaseY) / MAP_CELL_SIZE;
                        p.startX = gridX + 1;
                        p.startY = BattleSetup::GRID_SIZE - gridY;
                        ProceduralAudio::GetInstance().PlayPowerSE(2); // 配置音
                    }
                }

                const bool isRoundMode = m_battleSetup.IsRoundBattle();
                const int decideIndex = isRoundMode ? 0 : 1;
                const int itemCount = isRoundMode ? 1 : 2;
                if (isRoundMode) p.customCursor = 0;

                // ラウンドバトルではタイトルで初期数字を選ばず、位置だけ決める。
                for (int i = 0; i < itemCount; ++i) {
                    int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                    int valX = menuCX + CUSTOM_BTN_BASE_X;

                    if (HoverBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                        if (mouseMoved && p.customCursor != i) {
                            p.customCursor = i; ProceduralAudio::GetInstance().PlayPowerSE(2);
                        }
                        if (mClick) {
                            p.customCursor = i;
                            if (i == decideIndex) spaceTrg = true;
                        }
                    }

                    if (!isRoundMode && i == 0 && mClick) {
                        if (HoverBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE)) {
                            p.customCursor = i; leftTrg = true;
                        }
                        else if (HoverBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE)) {
                            p.customCursor = i; rightTrg = true;
                        }
                    }
                }

                if (!isRoundMode) {
                    if (hitNum != -1 && p.customCursor == 0) {
                        ProceduralAudio::GetInstance().PlayPowerSE(hitNum);
                        p.startNum = hitNum;
                    }

                    if (upTrg) { p.customCursor--; if (p.customCursor < 0) p.customCursor = 0; ProceduralAudio::GetInstance().PlayPowerSE(2); }
                    if (downTrg) { p.customCursor++; if (p.customCursor > 1) p.customCursor = 1; ProceduralAudio::GetInstance().PlayPowerSE(2); }

                    if (p.customCursor == 0) {
                        if (rightTrg) {
                            ProceduralAudio::GetInstance().PlayPowerSE(5);
                            p.startNum++; if (p.startNum > 9) p.startNum = 1;
                        }
                        if (leftTrg) {
                            ProceduralAudio::GetInstance().PlayPowerSE(5);
                            p.startNum--; if (p.startNum < 1) p.startNum = 9;
                        }
                    }
                }

                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    if (!isRoundMode && p.customCursor > 0) p.customCursor--;
                    else if (is1P) m_battleSetup.step = BattleSetup::Step::SELECT_STAGE;
                    else {
                        m_battleSetup.step = BattleSetup::Step::CUSTOM_P1_START;
                        m_battleSetup.players[0].customCursor = isRoundMode ? 0 : 1;
                    }
                }

                // ★ 決定時のネットワーク送信 ＆ フェーズ移行処理
                if (spaceTrg && p.customCursor == decideIndex) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);

                    if (is1P) {
                        if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED && NetworkManager::GetInstance()->IsHost()) {
                            // ホストの場合：1P設定完了時にクライアントへルールと1P情報を送信し、待機状態へ
                            SetupPacket packet;
                            packet.modeCursor = m_battleSetup.modeCursor;
                            packet.stocksCursor = m_battleSetup.stocksCursor;
                            packet.scoreCursor = m_battleSetup.scoreCursor;
                            packet.stageCursor = m_battleSetup.stageCursor;
                            packet.p1StartNum = m_battleSetup.players[0].startNum;
                            packet.p1StartX = m_battleSetup.players[0].startX;
                            packet.p1StartY = m_battleSetup.players[0].startY;
                            NetworkManager::GetInstance()->SendSetupPacket(packet);

                            m_titleState = TitleState::NETWORK_SETUP;
                            m_netStep = NetSetupStep::CLIENT_WAIT_SETUP;
                        }
                        else {
                            // オフライン：2P設定へ移行
                            m_battleSetup.step = BattleSetup::Step::CUSTOM_P2_START;
                            m_battleSetup.players[1].customCursor = 0;
                        }
                    }
                    else { // is 2P
                        if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED && !NetworkManager::GetInstance()->IsHost()) {
                            // クライアントの場合：自分の設定をホストに送信
                            SetupPacket packet;
                            packet.p2StartNum = m_battleSetup.players[1].startNum;
                            packet.p2StartX = m_battleSetup.players[1].startX;
                            packet.p2StartY = m_battleSetup.players[1].startY;
                            NetworkManager::GetInstance()->SendSetupPacket(packet);
                        }
                        // オフライン・クライアント共にゲーム開始
                        StartGameLogic();
                    }
                }
                else if (spaceTrg && !isRoundMode) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    p.customCursor++;
                }
                break;
            }

            } // End switch(m_battleSetup.step)
            break; // End BATTLE_SETUP

        } // End switch(m_titleState)
    }

    void TitleScene::Draw() {
        TitleViewData view;
        view.titleState = m_titleState;
        view.netStep = m_netStep;
        view.setup = &m_battleSetup;
        view.mainMenuCursor = m_mainMenuCursor;
        view.exitCursor = m_exitCursor;
        view.netRoleCursor = m_netRoleCursor;
        view.hostListCursor = m_hostListCursor;

        if (NetworkManager::GetInstance() != nullptr) {
            view.networkConnected = (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED);
            view.isNetworkHost = NetworkManager::GetInstance()->IsHost();

            const auto hosts = NetworkManager::GetInstance()->GetHostList();
            view.hostEntries.reserve(hosts.size());
            for (const auto& host : hosts) {
                view.hostEntries.push_back("ホスト: " + host.playerName + " (" + host.ipString + ")");
            }
        }

        m_ui.Draw(view);
    }

    void TitleScene::Release() {
        m_ui.Release();
    }

} // namespace App
