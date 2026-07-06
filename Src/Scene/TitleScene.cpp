#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <DxLib.h> // ★必ず一番上に配置！

#include "TitleScene.h"
#include "SceneManager.h"
#include "../Input/InputManager.h" 
#include "../Manager/ProceduralAudio.h"
#include "../Manager/NetworkManager.h" 
#include "../../CyberGrid.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    constexpr int WAIT_START_FRAMES = 10;
    constexpr int TARGET_SCORES[3] = { 53, 103, 223 };

    constexpr int MENU_CENTER_OFFSET_Y = -130;
    constexpr int MENU_CUSTOM_OFFSET_X = -300;

    constexpr int MENU_ITEM_BASE_Y = 100;
    constexpr int MENU_ITEM_STEP_Y = 85;
    constexpr int MENU_BOX_HALF_W = 380;
    constexpr int MENU_BOX_H = 55;
    constexpr int MENU_BOX_OFFSET_Y = 10;

    constexpr int CUSTOM_BTN_BASE_X = 140;
    constexpr int CUSTOM_BTN_OFFSET_X = 60;
    constexpr int CUSTOM_BTN_SIZE = 50;

    constexpr int MAP_CELL_SIZE = 45;
    constexpr int MAP_BASE_OFFSET_X = 180;
    constexpr int MAP_BASE_OFFSET_Y = 40;

    constexpr double BLINK_SPEED = 3.0;
    constexpr int BLINK_BASE_ALPHA = 150;
    constexpr int BLINK_AMP_ALPHA = 105;

    inline unsigned int COL_BG() { return GetColor(5, 10, 25); }
    inline unsigned int COL_GRID() { return GetColor(0, 150, 255); }
    inline unsigned int COL_P1() { return GetColor(255, 140, 0); }
    inline unsigned int COL_P2() { return GetColor(0, 150, 255); }
    inline unsigned int COL_TEXT_ON() { return GetColor(255, 180, 0); }
    inline unsigned int COL_TEXT_SUB() { return GetColor(200, 200, 200); }
    inline unsigned int COL_TEXT_DIM() { return GetColor(100, 100, 100); }
    inline unsigned int COL_TEXT_INFO() { return GetColor(200, 100, 255); }
    inline unsigned int COL_TEXT_WARN() { return GetColor(255, 255, 0); }
    inline unsigned int COL_TEXT_ERROR() { return GetColor(255, 100, 100); }
    inline unsigned int COL_TEXT_OFF() { return GetColor(50, 100, 150); }
    inline unsigned int COL_WHITE() { return GetColor(255, 255, 255); }
    inline unsigned int COL_BLACK() { return GetColor(0, 0, 0); }
    inline unsigned int COL_TITLE_MAIN() { return GetColor(220, 245, 255); }
    inline unsigned int COL_TITLE_SUB() { return GetColor(0, 120, 255); }
    inline unsigned int COL_DANGER() { return GetColor(255, 100, 100); }

    // ==========================================
    // ★追加：マウスクリック時の波紋エフェクト管理
    // ==========================================
    struct RippleEffect {
        float x, y;
        float radius;
        float alpha;
    };
    std::vector<RippleEffect> g_ripples;
}

namespace App {

    TitleScene::TitleScene()
        : m_titleState(TitleState::PRESS_START)
        , m_setupStep(SetupStep::SELECT_PLAYERS)
        , m_netStep(NetSetupStep::SELECT_ROLE)
        , m_frameCount(0)
        , m_mainMenuCursor(0)
        , m_exitCursor(0)
        , m_playerCursor(1)
        , m_modeCursor(0)
        , m_scoreCursor(1)
        , m_stocksCursor(0)
        , m_stageCursor(0)
        , m_netRoleCursor(0)
        , m_hostListCursor(0)
        , m_shouldQuit(false)
        , m_fontTitle(-1), m_fontMenu(-1), m_fontSmall(-1), m_fontNumber(-1)
        , m_psHandle(-1), m_cbHandle(-1), m_shaderTime(0.0f)
        , m_players()
    {
    }

    TitleScene::~TitleScene() {}

    void TitleScene::Init() {
        m_frameCount = 0;
        m_titleState = TitleState::PRESS_START;
        m_setupStep = SetupStep::SELECT_PLAYERS;
        m_netStep = NetSetupStep::SELECT_ROLE;

        m_mainMenuCursor = 0;
        m_exitCursor = 0;

        m_playerCursor = 1;
        m_modeCursor = 0;
        m_scoreCursor = 1;
        m_stocksCursor = 0;
        m_stageCursor = 0;
        m_netRoleCursor = 0;
        m_hostListCursor = 0;
        m_shouldQuit = false;

        m_players[0] = { 0, 0, DEF_P1_HP, DEF_P1_X, DEF_P1_Y };
        m_players[1] = { 1, 0, DEF_P2_HP, DEF_P2_X, DEF_P2_Y };

        ProceduralAudio::GetInstance().PlayBGM(true);

        m_fontTitle = CreateFontToHandle("BIZ UD明朝 Medium", 100, 3, DX_FONTTYPE_ANTIALIASING);
        m_fontMenu = CreateFontToHandle("BIZ UDゴシック", 40, 2, DX_FONTTYPE_NORMAL);
        m_fontSmall = CreateFontToHandle("遊ゴシック", 24, 2, DX_FONTTYPE_NORMAL);
        m_fontNumber = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 48, 2, DX_FONTTYPE_ANTIALIASING);

        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_shaderTime = 0.0f;

        m_miniGame.NextStage(false);

        g_ripples.clear(); 

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

        m_shaderTime += 0.0016f;

        bool spaceTrg = input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN);
        bool upTrg = input.IsTrgDown(KEY_INPUT_UP) || input.IsTrgDown(KEY_INPUT_W);
        bool downTrg = input.IsTrgDown(KEY_INPUT_DOWN) || input.IsTrgDown(KEY_INPUT_S);
        bool rightTrg = input.IsTrgDown(KEY_INPUT_RIGHT) || input.IsTrgDown(KEY_INPUT_D);
        bool leftTrg = input.IsTrgDown(KEY_INPUT_LEFT) || input.IsTrgDown(KEY_INPUT_A);
        bool bTrg = input.IsTrgDown(KEY_INPUT_B) || input.IsTrgDown(KEY_INPUT_BACK);

        int hitNum = -1;
        for (int i = 0; i < GRID_SIZE; ++i) {
            if (input.IsTrgDown(KEY_INPUT_1 + i) || input.IsTrgDown(KEY_INPUT_NUMPAD1 + i)) {
                hitNum = i + 1;
            }
        }

        Vector2 m = input.GetMousePos();
        bool mClick = input.IsMouseLeftTrg();
        static Vector2 prevM = m;
        bool mouseMoved = (m.x != prevM.x || m.y != prevM.y);
        prevM = m;

        // ==========================================
        // ★追加：波紋エフェクトの更新処理
        // ==========================================
        if (mClick) {
            // クリックした瞬間に新しい波紋を発生させる
            g_ripples.push_back({ m.x, m.y, 0.0f, 200.0f });
        }
        for (auto& r : g_ripples) {
            r.radius += 12.0f; // 広がるスピード
            r.alpha -= 6.0f;   // 消えるスピード
        }
        // 完全に透明になった波紋を削除
        g_ripples.erase(std::remove_if(g_ripples.begin(), g_ripples.end(), [](const RippleEffect& r) { return r.alpha <= 0; }), g_ripples.end());
        // ==========================================


        auto HoverBox = [&](int x, int y, int w, int h) {
            return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
            };

        int sw, sh;
        GetDrawScreenSize(&sw, &sh);
        int CX = sw / 2;
        int CY = sh / 2;
        int menuStartY = CY + MENU_CENTER_OFFSET_Y;

        int backBtnX = CX - 150, backBtnY = sh - 150, backBtnW = 300, backBtnH = 60;
        bool isBackBtnClicked = mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH);

        bool isCustomState = (m_titleState == TitleState::BATTLE_SETUP && (m_setupStep == SetupStep::CUSTOM_P1_START || m_setupStep == SetupStep::CUSTOM_P2_START));
        int menuCX = isCustomState ? CX + MENU_CUSTOM_OFFSET_X : CX;

        auto HandleMenuInput = [&](int& cursor, int maxItems) {
            if (maxItems <= 0) return;
            for (int i = 0; i < maxItems; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
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
                    m_setupStep = SetupStep::SELECT_PLAYERS;
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
                    m_setupStep = SetupStep::SELECT_MODE;
                    m_players[0].typeCursor = 0;
                    m_players[1].typeCursor = 0;
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
            break;
            case NetSetupStep::CLIENT_WAIT_SETUP:
            {
                SetupPacket packet;

                m_miniGame.Update();

                if (NetworkManager::GetInstance()->ReceiveSetupPacket(packet)) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);

                    auto* sm = SceneManager::GetInstance();

                    if (packet.modeCursor == 0) {
                        int maxStocks = (packet.stocksCursor == 0) ? 1 : (packet.stocksCursor == 1) ? 3 : 5;
                        sm->SetGameSettings(2, packet.modeCursor, maxStocks);
                    }
                    else {
                        int finalScore = TARGET_SCORES[packet.scoreCursor];
                        sm->SetGameSettings(2, packet.modeCursor, finalScore);
                    }
                    sm->SetPlayer1Settings(false, packet.p1StartNum, packet.p1StartX - 1, GRID_SIZE - packet.p1StartY);
                    sm->SetPlayer2Settings(false, packet.p2StartNum, packet.p2StartX - 1, GRID_SIZE - packet.p2StartY);
                    sm->SetStageIndex(packet.stageCursor);

                    sm->ChangeScene(SceneManager::SCENE_ID::GAME);
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
            switch (m_setupStep) {
            case SetupStep::SELECT_PLAYERS:
                HandleMenuInput(m_playerCursor, 2);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_titleState = TitleState::MAIN_MENU;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_playerCursor++;
                    m_players[1].typeCursor = (m_playerCursor == 1) ? 1 : 0;
                    m_setupStep = SetupStep::SELECT_MODE;
                }
                break;

            case SetupStep::SELECT_MODE:
                HandleMenuInput(m_modeCursor, 2);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_setupStep = SetupStep::SELECT_PLAYERS;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_setupStep = (m_modeCursor == 0) ? SetupStep::SELECT_CLASSIC_STOCKS : SetupStep::SELECT_SCORE;
                }
                break;

            case SetupStep::SELECT_CLASSIC_STOCKS:
                HandleMenuInput(m_stocksCursor, 3);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_setupStep = SetupStep::SELECT_MODE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_setupStep = SetupStep::SELECT_P1_TYPE;
                }
                break;

            case SetupStep::SELECT_SCORE:
                HandleMenuInput(m_scoreCursor, 3);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_setupStep = SetupStep::SELECT_MODE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_setupStep = SetupStep::SELECT_P1_TYPE;
                }
                break;

            case SetupStep::SELECT_P1_TYPE:
                HandleMenuInput(m_players[0].typeCursor, 2);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_setupStep = (m_modeCursor == 0) ? SetupStep::SELECT_CLASSIC_STOCKS : SetupStep::SELECT_SCORE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_setupStep = SetupStep::SELECT_P2_TYPE;
                }
                break;

            case SetupStep::SELECT_P2_TYPE:
                HandleMenuInput(m_players[1].typeCursor, 2);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_setupStep = SetupStep::SELECT_P1_TYPE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_setupStep = SetupStep::SELECT_STAGE;
                }
                break;

            case SetupStep::SELECT_STAGE:
                HandleMenuInput(m_stageCursor, 3);
                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    m_setupStep = SetupStep::SELECT_P2_TYPE;
                }
                else if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    m_setupStep = SetupStep::CUSTOM_P1_START;
                }
                break;

            case SetupStep::CUSTOM_P1_START:
            case SetupStep::CUSTOM_P2_START:
            {
                bool is1P = (m_setupStep == SetupStep::CUSTOM_P1_START);
                int pIdx = is1P ? 0 : 1;
                auto& p = m_players[pIdx];

                for (int i = 0; i < 4; ++i) {
                    int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                    int valX = menuCX + CUSTOM_BTN_BASE_X;

                    if (HoverBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                        if (mouseMoved && p.customCursor != i) {
                            p.customCursor = i;
                            ProceduralAudio::GetInstance().PlayPowerSE(2);
                        }
                        if (mClick) {
                            p.customCursor = i;
                            if (i == 3) spaceTrg = true;
                        }
                    }

                    if (i < 3 && mClick) {
                        if (HoverBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE)) {
                            p.customCursor = i; leftTrg = true;
                        }
                        else if (HoverBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE)) {
                            p.customCursor = i; rightTrg = true;
                        }
                    }
                }

                if (hitNum != -1 && p.customCursor < 3) {
                    ProceduralAudio::GetInstance().PlayPowerSE(hitNum);
                    if (p.customCursor == 0) p.startNum = hitNum;
                    else if (p.customCursor == 1) p.startX = hitNum;
                    else if (p.customCursor == 2) p.startY = hitNum;
                    p.customCursor++;
                }

                if (upTrg) { p.customCursor--; if (p.customCursor < 0) p.customCursor = 0; ProceduralAudio::GetInstance().PlayPowerSE(2); }
                if (downTrg) { p.customCursor++; if (p.customCursor > 3) p.customCursor = 3; ProceduralAudio::GetInstance().PlayPowerSE(2); }

                if (p.customCursor < 3) {
                    if (rightTrg) {
                        ProceduralAudio::GetInstance().PlayPowerSE(5);
                        if (p.customCursor == 0) { p.startNum++; if (p.startNum > GRID_SIZE) p.startNum = 1; }
                        if (p.customCursor == 1) { p.startX++; if (p.startX > GRID_SIZE) p.startX = 1; }
                        if (p.customCursor == 2) { p.startY++; if (p.startY > GRID_SIZE) p.startY = 1; }
                    }
                    if (leftTrg) {
                        ProceduralAudio::GetInstance().PlayPowerSE(5);
                        if (p.customCursor == 0) { p.startNum--; if (p.startNum < 1) p.startNum = GRID_SIZE; }
                        if (p.customCursor == 1) { p.startX--; if (p.startX < 1) p.startX = GRID_SIZE; }
                        if (p.customCursor == 2) { p.startY--; if (p.startY < 1) p.startY = GRID_SIZE; }
                    }
                }

                if (bTrg || isBackBtnClicked) {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                    if (p.customCursor > 0) p.customCursor--;
                    else if (is1P) m_setupStep = SetupStep::SELECT_STAGE;
                    else {
                        m_setupStep = SetupStep::CUSTOM_P1_START;
                        m_players[0].customCursor = 3;
                    }
                }
                if (spaceTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(9);
                    if (p.customCursor < 3) p.customCursor++;
                    else {
                        if (is1P) {
                            m_setupStep = SetupStep::CUSTOM_P2_START;
                            m_players[1].customCursor = 0;
                        }
                        else {
                            auto* sm = SceneManager::GetInstance();

                            if (NetworkManager::GetInstance()->GetState() == NetworkManager::State::CONNECTED &&
                                NetworkManager::GetInstance()->IsHost()) {

                                SetupPacket packet;
                                packet.modeCursor = m_modeCursor;
                                packet.stocksCursor = m_stocksCursor;
                                packet.scoreCursor = m_scoreCursor;
                                packet.stageCursor = m_stageCursor;
                                packet.p1StartNum = m_players[0].startNum;
                                packet.p1StartX = m_players[0].startX;
                                packet.p1StartY = m_players[0].startY;
                                packet.p2StartNum = m_players[1].startNum;
                                packet.p2StartX = m_players[1].startX;
                                packet.p2StartY = m_players[1].startY;

                                NetworkManager::GetInstance()->SendSetupPacket(packet);
                            }

                            if (m_modeCursor == 0) {
                                int maxStocks = (m_stocksCursor == 0) ? 1 : (m_stocksCursor == 1) ? 3 : 5;
                                sm->SetGameSettings(m_playerCursor, m_modeCursor, maxStocks);
                            }
                            else {
                                int finalScore = TARGET_SCORES[m_scoreCursor];
                                sm->SetGameSettings(m_playerCursor, m_modeCursor, finalScore);
                            }
                            sm->SetPlayer1Settings(m_players[0].typeCursor == 1, m_players[0].startNum, m_players[0].startX - 1, GRID_SIZE - m_players[0].startY);
                            sm->SetPlayer2Settings(m_players[1].typeCursor == 1, m_players[1].startNum, m_players[1].startX - 1, GRID_SIZE - m_players[1].startY);
                            sm->SetStageIndex(m_stageCursor);

                            sm->ChangeScene(SceneManager::SCENE_ID::GAME);
                        }
                    }
                }
                break;
            }

            } // End switch(m_setupStep)
            break; // End BATTLE_SETUP

        } // End switch(m_titleState)
    }

    void TitleScene::Draw() {
        int sw, sh;
        GetDrawScreenSize(&sw, &sh);
        int CX = sw / 2;
        int CY = sh / 2;

        auto& input = InputManager::GetInstance();
        Vector2 m = input.GetMousePos();

        auto HoverBox = [&](int x, int y, int w, int h) {
            return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
            };

        // 背景描画
        DrawBox(0, 0, sw, sh, COL_BG(), TRUE);

        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            if (cb != nullptr) {
                cb[0] = m_shaderTime;
                cb[1] = (float)sw;
                cb[2] = (float)sh;
                cb[3] = 0.0f;
                UpdateShaderConstantBuffer(m_cbHandle);
                SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);

                SetUsePixelShader(m_psHandle);
                VERTEX2DSHADER v[6];
                for (int i = 0; i < 6; ++i) {
                    v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
                    v[i].dif = GetColorU8(255, 255, 255, 255);
                    v[i].spc = GetColorU8(0, 0, 0, 0);
                }
                v[0].pos.x = 0;  v[0].pos.y = 0;  v[0].u = 0.0f; v[0].v = 0.0f;
                v[1].pos.x = sw; v[1].pos.y = 0;  v[1].u = 1.0f; v[1].v = 0.0f;
                v[2].pos.x = 0;  v[2].pos.y = sh; v[2].u = 0.0f; v[2].v = 1.0f;
                v[3].pos.x = sw; v[3].pos.y = 0;  v[3].u = 1.0f; v[3].v = 0.0f;
                v[4].pos.x = sw; v[4].pos.y = sh; v[4].u = 1.0f; v[4].v = 1.0f;
                v[5].pos.x = 0;  v[5].pos.y = sh; v[5].u = 0.0f; v[5].v = 1.0f;
                DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
                SetUsePixelShader(-1);
            }
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 30);
        for (int i = 0; i < sw; i += MAP_CELL_SIZE) DrawLine(i, 0, i, sh, COL_GRID(), 1);
        for (int j = 0; j < sh; j += MAP_CELL_SIZE) DrawLine(0, j, sw, j, COL_GRID(), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(0, 40, sw, 45, COL_P1(), TRUE);
        DrawBox(0, sh - 45, sw, sh - 40, COL_P1(), TRUE);

        // タイトルロゴ描画
        const char* titleText = "超計算マスBATTLE";
        int titleW = GetDrawStringWidthToHandle(titleText, (int)strlen(titleText), m_fontTitle);
        double t = GetNowCount() / 1000.0;
        float floatY = (float)sin(t * 2.0) * 8.0f;
        int titleX = CX - titleW / 2;
        int titleY = 100 + (int)floatY;

        SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
        for (int i = 0; i < 4; ++i) {
            int offset = i * 2;
            DrawStringToHandle(titleX - offset, titleY - offset, titleText, COL_TITLE_SUB(), m_fontTitle);
            DrawStringToHandle(titleX + offset, titleY + offset, titleText, COL_TITLE_SUB(), m_fontTitle);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        float glitchStrength = (float)sin(t * 10.0) * 3.0f;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
        DrawStringToHandle(titleX + (int)glitchStrength, titleY, titleText, GetColor(255, 0, 100), m_fontTitle);
        DrawStringToHandle(titleX - (int)glitchStrength, titleY, titleText, GetColor(0, 100, 255), m_fontTitle);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawStringToHandle(titleX, titleY, titleText, COL_TITLE_MAIN(), m_fontTitle);

        float scanPos = (float)fmod(t * 1.5, 2.0) - 1.0f;
        int shineX = titleX + (int)(scanPos * titleW * 1.5f);
        SetDrawArea(titleX, titleY, titleX + titleW, titleY + 110);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 180);
        for (int i = 0; i < 20; ++i) {
            DrawLine(shineX + i, titleY, shineX + i - 30, titleY + 100, GetColor(255, 255, 255));
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetDrawArea(0, 0, sw, sh);

        DrawLine(CX - 500, 230, CX + 500, 230, COL_TITLE_SUB(), 5);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
        DrawLine(CX - 500, 230, CX + 500, 230, COL_TITLE_MAIN(), 2);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 描画ヘルパー関数
        int blinkAlpha = (int)(BLINK_BASE_ALPHA + BLINK_AMP_ALPHA * sin(t * M_PI * BLINK_SPEED));

        auto drawBracket = [&](int x, int y, int w, int h, unsigned int col) {
            int d = 15;
            DrawLine(x, y, x + d, y, col, 2);            DrawLine(x, y, x, y + d, col, 2);
            DrawLine(x + w - d, y, x + w, y, col, 2);   DrawLine(x + w, y, x + w, y + d, col, 2);
            DrawLine(x, y + h - d, x, y + h, col, 2);   DrawLine(x, y + h, x + d, y + h, col, 2);
            DrawLine(x + w - d, y + h, x + w, y + h, col, 2); DrawLine(x + w, y + h - d, x + w, y + h, col, 2);
            };

        bool isCustomState = (m_titleState == TitleState::BATTLE_SETUP && (m_setupStep == SetupStep::CUSTOM_P1_START || m_setupStep == SetupStep::CUSTOM_P2_START));
        int menuCX = isCustomState ? CX + MENU_CUSTOM_OFFSET_X : CX;
        int menuStartY = CY + MENU_CENTER_OFFSET_Y;

        auto drawMenuList = [&](int cursor, const char* menuTitle, const std::vector<std::string>& items) {
            if (menuTitle && strlen(menuTitle) > 0) {
                int mtW = GetDrawStringWidthToHandle(menuTitle, (int)strlen(menuTitle), m_fontMenu);
                DrawStringToHandle(menuCX - mtW / 2, menuStartY, menuTitle, COL_GRID(), m_fontMenu);
            }

            for (size_t i = 0; i < items.size(); ++i) {
                int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                unsigned int baseCol = (cursor == i) ? COL_TEXT_ON() : COL_TEXT_OFF();
                int tw = GetDrawStringWidthToHandle(items[i].c_str(), (int)items[i].length(), m_fontMenu);

                if (cursor == i) {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                    DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y,
                        menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, baseCol, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, baseCol);
                    DrawStringToHandle(menuCX - 380, iy, " >>", baseCol, m_fontMenu);
                    DrawStringToHandle(menuCX + 335, iy, "<< ", baseCol, m_fontMenu);
                }
                DrawStringToHandle(menuCX - tw / 2, iy, items[i].c_str(), baseCol, m_fontMenu);
            }
            };

        switch (m_titleState) {

        case TitleState::PRESS_START: {
            const char* pushText = "スペースかクリックでスタート";
            int ptW = GetDrawStringWidthToHandle(pushText, (int)strlen(pushText), m_fontMenu);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha);
            DrawStringToHandle(CX - ptW / 2, CY + 100, pushText, COL_GRID(), m_fontMenu);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;
        }

        case TitleState::MAIN_MENU:
            drawMenuList(m_mainMenuCursor, "", { "オフラインバトル", "通信対戦", "チュートリアル", "オプション", "ゲーム終了" });
            break;

        case TitleState::NETWORK_SETUP:
            switch (m_netStep) {
            case NetSetupStep::CLIENT_WAIT_SETUP:
            {
                const char* msg = "ホストがゲームルールを設定中です...";
                int msgW = GetDrawStringWidthToHandle(msg, (int)strlen(msg), m_fontMenu);
                DrawStringToHandle(CX - msgW / 2, CY - 250, msg, COL_TEXT_SUB(), m_fontMenu);

                m_miniGame.Draw();
                break;
            }
            case NetSetupStep::SELECT_ROLE:
                drawMenuList(m_netRoleCursor, "【 通信対戦 】", { "部屋を作る (ホスト)", "部屋を探す (クライアント)" });
                break;
            case NetSetupStep::HOST_WAITING:
            {
                const char* msg = "対戦相手を待っています...";
                int msgW = GetDrawStringWidthToHandle(msg, (int)strlen(msg), m_fontMenu);
                // ★修正: 電卓が消えたので、文字のY座標のズレを戻しました
                DrawStringToHandle(CX - msgW / 2, CY, msg, COL_TEXT_SUB(), m_fontMenu);

                const char* subMsg = "(同じLAN内のPCから検索可能)";
                int subW = GetDrawStringWidthToHandle(subMsg, (int)strlen(subMsg), m_fontSmall);
                // ★修正: 同様にY座標を戻し、ここにあった m_miniGame.Draw(); を削除しました
                DrawStringToHandle(CX - subW / 2, CY + 60, subMsg, COL_TEXT_SUB(), m_fontSmall);
                break;
            }
            case NetSetupStep::CLIENT_SEARCHING:
            {
                auto hosts = NetworkManager::GetInstance()->GetHostList();
                if (hosts.empty()) {
                    const char* msg = "部屋を探しています...";
                    int msgW = GetDrawStringWidthToHandle(msg, (int)strlen(msg), m_fontMenu);
                    DrawStringToHandle(CX - msgW / 2, CY, msg, COL_TEXT_SUB(), m_fontMenu);
                }
                else {
                    std::vector<std::string> hostStrs;
                    for (const auto& h : hosts) {
                        hostStrs.push_back("ホスト: " + h.playerName + " (" + h.ipString + ")");
                    }
                    if (m_hostListCursor >= (int)hosts.size()) m_hostListCursor = 0;
                    drawMenuList(m_hostListCursor, "【 見つかった部屋 】", hostStrs);
                }
                break;
            }
            }
            break;

        case TitleState::OPTION_MENU:
            drawMenuList(-1, "【 オプション 】", { "※ここに音量設定などを追加", "（現在は準備中です）" });
            break;

        case TitleState::EXIT_CONFIRM:
            drawMenuList(m_exitCursor, "【 ゲームを終了しますか？ 】", { "いいえ (戻る)", "はい (終了)" });
            break;

        case TitleState::BATTLE_SETUP:
            switch (m_setupStep) {
            case SetupStep::SELECT_PLAYERS:
                drawMenuList(m_playerCursor, "【 バトル方式 】", { "シングルバトル", "オフラインバトル" });
                break;
            case SetupStep::SELECT_MODE: {
                drawMenuList(m_modeCursor, "【 プレイモード 】", { "ノーマルバトル", "カウントバトル" });
                const char* modeDesc = (m_modeCursor == 0) ? "相手のバッテリーを削り切れ！四則演算を用いた王道ダメージバトル" : "目標スコアへピタリと着地しろ！極限の頭脳戦モード";
                int descW = GetDrawStringWidthToHandle(modeDesc, (int)strlen(modeDesc), m_fontSmall);
                int descX = menuCX - descW / 2;
                int descY = menuStartY + MENU_ITEM_BASE_Y + 2 * MENU_ITEM_STEP_Y + 40;
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
                DrawBox(descX - 20, descY - 10, descX + descW + 20, descY + 40, COL_BG(), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawBox(descX - 20, descY - 10, descX + descW + 20, descY + 40, m_modeCursor == 0 ? COL_P1() : COL_P2(), FALSE);
                DrawStringToHandle(descX, descY, modeDesc, COL_TEXT_ON(), m_fontSmall);
                break;
            }
            case SetupStep::SELECT_CLASSIC_STOCKS:
                drawMenuList(m_stocksCursor, "【 残機設定 】", { "残機: 1", "残機: 3", "残機: 5" });
                break;
            case SetupStep::SELECT_SCORE:
                drawMenuList(m_scoreCursor, "【 目標スコア設定 】", { "目標スコア: 53", "目標スコア: 103", "目標スコア: 223" });
                break;
            case SetupStep::SELECT_P1_TYPE:
                drawMenuList(m_players[0].typeCursor, "【 1P 操作設定 】", { "プレイヤー", "NPC" });
                break;
            case SetupStep::SELECT_P2_TYPE:
                drawMenuList(m_players[1].typeCursor, "【 2P 操作設定 】", { "プレイヤー", "NPC" });
                break;
            case SetupStep::SELECT_STAGE: {
                drawMenuList(m_stageCursor, "【 ステージ選択 】", { "バランステージ", "マイナステージ", "カオステージ" });
                const char* recommendText = nullptr;
                unsigned int recommendCol = COL_TEXT_OFF();
                if (m_modeCursor == 0) {
                    if (m_stageCursor == 1) { recommendText = "おすすめ！"; recommendCol = GetColor(255, 215, 0); }
                    else { recommendText = "(マイナステージがおすすめ)"; recommendCol = GetColor(150, 150, 180); }
                }
                else { recommendText = "すべておすすめ！"; recommendCol = GetColor(100, 255, 150); }

                int recX = menuCX + 420;
                int recY = menuStartY + 50;
                if (m_modeCursor == 0 && m_stageCursor == 1) recY = menuStartY + MENU_ITEM_BASE_Y + m_stageCursor * MENU_ITEM_STEP_Y + 15;
                DrawStringToHandle(recX, recY, recommendText, recommendCol, m_fontSmall);
                break;
            }
            case SetupStep::CUSTOM_P1_START:
            case SetupStep::CUSTOM_P2_START: {
                bool is1P = (m_setupStep == SetupStep::CUSTOM_P1_START);
                int pIdx = is1P ? 0 : 1;
                auto& p = m_players[pIdx];

                std::string menuTitle = is1P ? "【 1P 初期設定 】" : "【 2P 初期設定 】";
                int mtW = GetDrawStringWidthToHandle(menuTitle.c_str(), (int)menuTitle.length(), m_fontMenu);
                DrawStringToHandle(menuCX - mtW / 2, menuStartY, menuTitle.c_str(), is1P ? COL_P1() : COL_P2(), m_fontMenu);

                const char* items[3] = { "初期体力", "初期Ｘ座標 (1-9)", "初期Ｙ座標 (1-9)" };
                int vals[3] = { p.startNum, p.startX, p.startY };

                for (int i = 0; i < 4; ++i) {
                    int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                    unsigned int color = (p.customCursor == i) ? COL_TEXT_ON() : COL_TEXT_OFF();

                    if (p.customCursor == i) {
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                        DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, color, TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                        drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, color);
                    }

                    if (i < 3) {
                        char nameBuf[128]; sprintf_s(nameBuf, "%-20s", items[i]);
                        DrawStringToHandle(menuCX - 320, iy, nameBuf, color, m_fontMenu);
                        int valX = menuCX + CUSTOM_BTN_BASE_X;
                        bool hoverL = HoverBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE);
                        unsigned int colL = hoverL ? COL_WHITE() : color;
                        DrawBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, valX - 10, iy + 45, colL, FALSE);
                        DrawStringToHandle(valX - 45, iy, "<", colL, m_fontMenu);
                        DrawFormatStringToHandle(valX + 15, iy - 4, color, m_fontNumber, "%d", vals[i]);
                        bool hoverR = HoverBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE);
                        unsigned int colR = hoverR ? COL_WHITE() : color;
                        DrawBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, valX + 110, iy + 45, colR, FALSE);
                        DrawStringToHandle(valX + 75, iy, ">", colR, m_fontMenu);
                    }
                    else {
                        const char* decideText = is1P ? "設定完了 (NEXT)" : "バトル開始 (GAME START)";
                        int tw = GetDrawStringWidthToHandle(decideText, (int)strlen(decideText), m_fontMenu);
                        DrawStringToHandle(menuCX - tw / 2, iy, decideText, color, m_fontMenu);
                        if (p.customCursor == i) {
                            DrawStringToHandle(menuCX - tw / 2 - 60, iy, " >>", color, m_fontMenu);
                            DrawStringToHandle(menuCX + tw / 2 + 20, iy, "<< ", color, m_fontMenu);
                        }
                    }
                }

                // ミニマップ描画
                int mapBaseX = CX + MAP_BASE_OFFSET_X;
                int mapBaseY = menuStartY + MAP_BASE_OFFSET_Y;
                DrawStringToHandle(mapBaseX + 80, mapBaseY - 50, "【 配置プレビュー 】", COL_TITLE_MAIN(), m_fontSmall);
                DrawBox(mapBaseX - 5, mapBaseY - 5, mapBaseX + MAP_CELL_SIZE * GRID_SIZE + 5, mapBaseY + MAP_CELL_SIZE * GRID_SIZE + 5, COL_GRID(), FALSE);
                DrawStringToHandle(mapBaseX - 40, mapBaseY - 40, "Y", COL_GRID(), m_fontSmall);
                DrawStringToHandle(mapBaseX + MAP_CELL_SIZE * GRID_SIZE + 15, mapBaseY + MAP_CELL_SIZE * GRID_SIZE - 10, "X", COL_GRID(), m_fontSmall);

                for (int y = 0; y < GRID_SIZE; ++y) {
                    for (int x = 0; x < GRID_SIZE; ++x) {
                        int drawX = mapBaseX + x * MAP_CELL_SIZE, drawY = mapBaseY + y * MAP_CELL_SIZE;
                        int uiX = x + 1, uiY = GRID_SIZE - y;
                        DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_BG(), TRUE);
                        DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_TITLE_SUB(), FALSE);

                        for (int i = 0; i < 2; ++i) {
                            if (uiX == m_players[i].startX && uiY == m_players[i].startY) {
                                int alpha = ((i == 0 && is1P) || (i == 1 && !is1P)) ? blinkAlpha : 150;
                                SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                                DrawBox(drawX + 2, drawY + 2, drawX + MAP_CELL_SIZE - 2, drawY + MAP_CELL_SIZE - 2, i == 0 ? COL_P1() : COL_P2(), TRUE);
                                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                                DrawFormatStringToHandle(drawX + 16, drawY + 10, COL_BLACK(), m_fontSmall, "%d", m_players[i].startNum);
                            }
                        }
                    }
                }
                break;
            }
            }
            break;
        }

        // 共通戻るボタン
        if (m_titleState != TitleState::PRESS_START && m_titleState != TitleState::MAIN_MENU) {
            int backBtnX = CX - 150, backBtnY = sh - 150, backBtnW = 300, backBtnH = 60;
            bool isHover = HoverBox(backBtnX, backBtnY, backBtnW, backBtnH);
            unsigned int btnCol = isHover ? COL_TEXT_ON() : COL_TEXT_OFF();
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, isHover ? 200 : 120);
            DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, COL_BG(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, btnCol, FALSE);
            const char* backStr = "戻る (B)";
            int bw = GetDrawStringWidthToHandle(backStr, (int)strlen(backStr), m_fontMenu);
            DrawStringToHandle(backBtnX + (backBtnW - bw) / 2, backBtnY + 10, backStr, btnCol, m_fontMenu);
        }

        // オペレーションガイド
        DrawBox(0, sh - 70, sw, sh - 15, GetColor(10, 20, 40), TRUE);
        std::string guideText = "";
        if (m_titleState == TitleState::BATTLE_SETUP && (m_setupStep == SetupStep::CUSTOM_P1_START || m_setupStep == SetupStep::CUSTOM_P2_START)) {
            guideText = "[↑][↓]/CLICK: 項目選択   |   [<][>]/CLICK: 数値変更   |   [1-9]: 直接入力";
        }
        else if (m_titleState != TitleState::PRESS_START) {
            guideText = "[↑][↓]/CLICK: 項目選択   |   [SPACE]: 決定";
        }

        if (!guideText.empty()) {
            int gw = GetDrawStringWidthToHandle(guideText.c_str(), (int)guideText.length(), m_fontSmall);
            int guideX = isCustomState ? (CX - gw / 2 - 100) : (CX - gw / 2);
            DrawStringToHandle(guideX, sh - 55, guideText.c_str(), COL_TEXT_OFF(), m_fontSmall);
        }

        const char* fullscreenGuide = "[F11] 全画面表示 / ウィンドウ切替";
        int fsGuideW = GetDrawStringWidthToHandle(fullscreenGuide, (int)strlen(fullscreenGuide), m_fontSmall);
        DrawStringToHandle(sw - fsGuideW - 20, sh - 55, fullscreenGuide, GetColor(150, 150, 180), m_fontSmall);


        // ==========================================
        // ★追加：マウスクリック波紋エフェクトの描画（一番最後に描画して一番手前に表示）
        // ==========================================
        SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
        for (const auto& r : g_ripples) {
            SetDrawBlendMode(DX_BLENDMODE_ADD, (int)r.alpha);
            // COL_TEXT_ON (山吹色) で円を描画
            DrawCircleAA(r.x, r.y, r.radius, 64, COL_TEXT_ON(), FALSE, 3.0f);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        // ==========================================
    }

    void TitleScene::Release() {
        if (m_fontTitle != -1)  DeleteFontToHandle(m_fontTitle);
        if (m_fontMenu != -1)   DeleteFontToHandle(m_fontMenu);
        if (m_fontSmall != -1)  DeleteFontToHandle(m_fontSmall);
        if (m_fontNumber != -1) DeleteFontToHandle(m_fontNumber);

        if (m_psHandle != -1)   DeleteShader(m_psHandle);
        if (m_cbHandle != -1)   DeleteShaderConstantBuffer(m_cbHandle);
    }

} // namespace App