#define NOMINMAX
#include "TutorialMaster.h"
#include <DxLib.h>
#include "../Input/InputManager.h"
#include "../Scene/SceneManager.h"
#include "../Manager/ProceduralAudio.h"

namespace App {

    TutorialMaster::TutorialMaster()
        : m_currentStep(Step::Menu), m_stepTimer(0), m_menuCursor(0)
        , m_hoverGrid(-1, -1), m_mapGrid(80, Vector2(600, 120)), m_fontMsg(-1) {
    }

    TutorialMaster::~TutorialMaster() { Release(); }

    void TutorialMaster::Init() {
        m_currentStep = Step::Menu;
        m_stepTimer = 0;
        m_menuCursor = 0;
        m_fontMsg = CreateFontToHandle("BIZ UDゴシック", 24, 2, DX_FONTTYPE_ANTIALIASING);

        SceneManager::GetInstance()->SetGameSettings(1, 0, 3);
        m_ui = std::make_unique<BattleUI>();
        m_ui->Init();
        m_player.reset();
        m_enemy.reset();
    }

    void TutorialMaster::SetupSituation(int sitId) {
        auto* sm = SceneManager::GetInstance();
        m_mapGrid.ClearItems();
        m_player.reset();
        m_enemy.reset();

        if (sitId == 6) {
            sm->SetGameSettings(1, 1, 53);
            m_mapGrid.SetRuleModeAndStage(BattleRuleMode::ZERO_ONE, 0);
            m_ruleMode = 1;
            m_targetScore = 53;
            m_p1Score = 45;
            m_p2Score = 45;
            m_p1DisplayScore = 45.0f;
            m_p2DisplayScore = 45.0f;
        }
        else {
            sm->SetGameSettings(1, 0, 3);
            m_mapGrid.SetRuleModeAndStage(BattleRuleMode::CLASSIC, 0);
            m_ruleMode = 0;
        }

        m_ui = std::make_unique<BattleUI>();
        m_ui->Init();

        if (sitId == 0) {
            m_player = std::make_unique<Player>(IntVector2(4, 7), m_mapGrid.GetCellCenter(4, 7), 9, 3, 3);
            m_player->SetOp('\0');
            m_targetMove1 = IntVector2(4, 6); m_targetMove2 = IntVector2(4, 4); m_targetMove3 = IntVector2(4, 1);
        }
        else if (sitId == 1) {
            m_player = std::make_unique<Player>(IntVector2(4, 6), m_mapGrid.GetCellCenter(4, 6), 5, 3, 3);
            m_enemy = std::make_unique<Enemy>(IntVector2(4, 3), m_mapGrid.GetCellCenter(4, 3), 4, 3, 3);
            m_mapGrid.SetItemAt(4, 4, '+');
            m_targetMove1 = IntVector2(4, 4); m_targetMove2 = IntVector2(4, 3);
        }
        else if (sitId == 2) {
            m_player = std::make_unique<Player>(IntVector2(4, 5), m_mapGrid.GetCellCenter(4, 5), 3, 3, 3);
            m_enemy = std::make_unique<Enemy>(IntVector2(4, 3), m_mapGrid.GetCellCenter(4, 3), 8, 3, 3);
            m_mapGrid.SetItemAt(4, 4, '-');
            m_targetMove1 = IntVector2(4, 4); m_targetMove2 = IntVector2(4, 3);
        }
        else if (sitId == 3) {
            m_player = std::make_unique<Player>(IntVector2(4, 7), m_mapGrid.GetCellCenter(4, 7), 7, 1, 3);
            m_enemy = std::make_unique<Enemy>(IntVector2(4, 3), m_mapGrid.GetCellCenter(4, 3), 8, 3, 3);
            m_mapGrid.SetItemAt(4, 4, '*');
            m_targetMove1 = IntVector2(4, 4); m_targetMove2 = IntVector2(4, 3);
        }
        else if (sitId == 4) {
            m_player = std::make_unique<Player>(IntVector2(4, 6), m_mapGrid.GetCellCenter(4, 6), 8, 3, 3);
            m_enemy = std::make_unique<Enemy>(IntVector2(4, 3), m_mapGrid.GetCellCenter(4, 3), 2, 3, 3);
            m_mapGrid.SetItemAt(4, 4, '/');
            m_targetMove1 = IntVector2(4, 4); m_targetMove2 = IntVector2(4, 3);
        }
        else if (sitId == 5) {
            m_player = std::make_unique<Player>(IntVector2(4, 6), m_mapGrid.GetCellCenter(4, 6), 8, 3, 3);
            // ★論理修正：すでに敵のバッテリーが「0」の状態でスタート！
            m_enemy = std::make_unique<Enemy>(IntVector2(4, 3), m_mapGrid.GetCellCenter(4, 3), 9, 0, 3);
            m_mapGrid.SetItemAt(4, 4, '-');
            m_targetMove1 = IntVector2(4, 4); m_targetMove2 = IntVector2(4, 3);
        }
        else if (sitId == 6) {
            m_player = std::make_unique<Player>(IntVector2(4, 6), m_mapGrid.GetCellCenter(4, 6), 5, 3, 3);
            m_enemy = std::make_unique<Enemy>(IntVector2(4, 3), m_mapGrid.GetCellCenter(4, 3), 5, 3, 3);
            m_mapGrid.SetItemAt(4, 4, '+');
            m_targetMove1 = IntVector2(4, 4); m_targetMove2 = IntVector2(4, 3);
        }
    }

    void TutorialMaster::StartTutorial(int index) {
        m_stepTimer = 0;
        if (index == 0) { SetupSituation(0); m_currentStep = Step::Basic_Welcome; }
        else if (index == 1) { SetupSituation(1); m_currentStep = Step::B1_Welcome; }
        else if (index == 2) { SetupSituation(4); m_currentStep = Step::B2_Welcome; }
        else if (index == 3) { SetupSituation(5); m_currentStep = Step::RuleN_Welcome; }
        else if (index == 4) { SetupSituation(6); m_currentStep = Step::RuleC_Welcome; }

        m_ui->AddLog(">>> チュートリアルを開始します");
    }

    bool TutorialMaster::CheckButtonClick(int x, int y, int w, int h) const {
        auto pos = InputManager::GetInstance().GetMousePos();
        return pos.x >= x && pos.x <= x + w && pos.y >= y && pos.y <= y + h;
    }

    int TutorialMaster::GetCachedFont(int size) const {
        static std::unordered_map<int, int> s_fontCache;
        if (s_fontCache.find(size) == s_fontCache.end()) {
            s_fontCache[size] = CreateFontToHandle("BIZ UDゴシック", size, 2, DX_FONTTYPE_ANTIALIASING);
        }
        return s_fontCache[size];
    }

    void TutorialMaster::DrawCyberButton(int x, int y, int w, int h, const char* text, unsigned int col, bool isHover, int fontHandle) const {
        if (isHover) {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 180); DrawBox(x, y, x + w, y + h, col, TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(x, y, x + w, y + h, GetColor(255, 255, 255), FALSE);
            int tw = GetDrawStringWidthToHandle(text, (int)strlen(text), fontHandle);
            DrawStringToHandle(x + (w - tw) / 2, y + (h - 26) / 2, text, GetColor(0, 0, 0), fontHandle);
        }
        else {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180); DrawBox(x, y, x + w, y + h, GetColor(15, 20, 25), TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(x, y, x + w, y + h, col, FALSE);
            int tw = GetDrawStringWidthToHandle(text, (int)strlen(text), fontHandle);
            DrawStringToHandle(x + (w - tw) / 2, y + (h - 26) / 2, text, col, fontHandle);
        }
    }

    void TutorialMaster::DrawButtonHighlight(int x, int y, int w, int h, unsigned int col) const {
        double time = GetNowCount() / 1000.0;
        int alpha = (int)(150 + 105 * sin(time * 3.14159 * 4.0));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(x - 5, y - 5, x + w + 5, y + h + 5, col, FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void TutorialMaster::DrawFakeCalcPanel(int aNum, char op, int dNum, int res, bool isDiv, bool guideSelf, bool isCountMode) const {
        int f64 = GetCachedFont(64), f22 = GetCachedFont(22), f16 = GetCachedFont(16);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220); DrawBox(600, 830, 1320, 990, GetColor(5, 5, 8), TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawLine(600, 830, 1320, 830, GetColor(255, 165, 0), 2);

        int calcY = 830 + 22;
        DrawStringToHandle(662, calcY - 18, isDiv ? "X座標" : "自分", GetColor(255, 165, 0), f16);
        DrawStringToHandle(750, calcY - 18, isDiv ? "Y座標" : "相手", GetColor(60, 150, 255), f16);

        DrawFormatStringToHandle(660, calcY, GetColor(255, 255, 255), f64, "%d %c %d =", aNum, op, dNum);
        DrawFormatStringToHandle(1020, calcY, GetColor(255, 255, 255), f64, "%d", res);

        if (isCountMode) {
            DrawStringToHandle(630, 830 + 92, "スコアを加算する対象を選択してください", GetColor(120, 120, 130), f22);
            DrawFormatStringToHandle(630, 830 + 122, GetColor(100, 255, 150), f22, "実行時、選択した対象のスコアに %d を加算！", res);
        }
        else {
            DrawStringToHandle(630, 830 + 92, "反映する対象を選択してください", GetColor(120, 120, 130), f22);
            if (isDiv) DrawFormatStringToHandle(630, 830 + 122, GetColor(100, 255, 150), f22, "実行時、選択した対象のワープを (%d, %d) に設置！", aNum, dNum);
        }

        int by = 960;
        bool hoverSelf = CheckButtonClick(600, by, 220, 60);
        bool hoverEnemy = CheckButtonClick(850, by, 220, 60);

        if (guideSelf) DrawButtonHighlight(600, by, 220, 60, GetColor(100, 255, 150));
        else DrawButtonHighlight(850, by, 220, 60, GetColor(255, 100, 100));

        DrawCyberButton(600, by, 220, 60, "自分", GetColor(100, 255, 150), hoverSelf, f22);
        DrawCyberButton(850, by, 220, 60, "相手", GetColor(255, 100, 100), hoverEnemy, f22);
    }

    void TutorialMaster::Update() {
        m_stepTimer++;
        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();

        if (m_ruleMode == 1) {
            m_p1DisplayScore += (m_p1Score - m_p1DisplayScore) * 0.15f;
            m_p2DisplayScore += (m_p2Score - m_p2DisplayScore) * 0.15f;
            if (std::abs(m_p1Score - m_p1DisplayScore) < 0.5f) m_p1DisplayScore = (float)m_p1Score;
            if (std::abs(m_p2Score - m_p2DisplayScore) < 0.5f) m_p2DisplayScore = (float)m_p2Score;
        }

        auto& input = InputManager::GetInstance();
        m_hoverGrid = m_mapGrid.ScreenToGrid(input.GetMousePos());
        Vector2 mPos = input.GetMousePos();
        bool mClick = input.IsMouseLeftTrg();
        bool spaceTrg = input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN);
        bool proceedInput = (mClick || spaceTrg);

        if (m_currentStep == Step::Menu) {
            if (input.IsTrgDown(KEY_INPUT_UP) || input.IsTrgDown(KEY_INPUT_W)) { m_menuCursor--; if (m_menuCursor < 0) m_menuCursor = 5; ProceduralAudio::GetInstance().PlayPowerSE(2); }
            if (input.IsTrgDown(KEY_INPUT_DOWN) || input.IsTrgDown(KEY_INPUT_S)) { m_menuCursor++; if (m_menuCursor > 5) m_menuCursor = 0; ProceduralAudio::GetInstance().PlayPowerSE(2); }
            int menuCX = 1920 / 2, menuStartY = 1080 / 2 - 170;
            for (int i = 0; i < 6; ++i) {
                if (mPos.x >= menuCX - 380 && mPos.x <= menuCX + 380 && mPos.y >= menuStartY + i * 85 - 20 && mPos.y <= menuStartY + i * 85 + 60) {
                    static Vector2 prevM = mPos;
                    if ((mPos.x != prevM.x || mPos.y != prevM.y) && m_menuCursor != i) { m_menuCursor = i; ProceduralAudio::GetInstance().PlayPowerSE(2); }
                    prevM = mPos;
                    if (mClick) { m_menuCursor = i; spaceTrg = true; }
                }
            }
            if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                if (m_menuCursor == 5) SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);
                else StartTutorial(m_menuCursor);
            }
            return;
        }

        auto moveAndChange = [&](IntVector2 target, int newPower, char op = '\0') {
            ProceduralAudio::GetInstance().PlayPowerSE(5);
            std::queue<Vector2> p; p.push(m_mapGrid.GetCellCenter(target.x, target.y));
            m_player->StartMove(target, p); m_player->SetNumber(newPower);
            if (op != '\0') { m_player->SetOp(op); m_mapGrid.SetItemAt(target.x, target.y, '\0'); }
            };

        auto checkMoveFinish = [&](IntVector2 target) {
            if (!m_player->IsMoving() && m_player->GetGridPos() == target) { NextStep(); return true; }
            return false;
            };

        switch (m_currentStep) {
        case Step::Basic_Welcome: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::Basic_Wait_Move1: if (!checkMoveFinish(m_targetMove1)) { if (mClick && m_hoverGrid == m_targetMove1 && !m_player->IsMoving()) moveAndChange(m_targetMove1, 8); } break;
        case Step::Basic_Res_Move1: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::Basic_Wait_Move2: if (!checkMoveFinish(m_targetMove2)) { if (mClick && m_hoverGrid == m_targetMove2 && !m_player->IsMoving()) moveAndChange(m_targetMove2, 6); } break;
        case Step::Basic_Res_Move2: if (proceedInput && m_stepTimer > 30) { m_player->SetNumber(7); ProceduralAudio::GetInstance().PlayPowerSE(8); NextStep(); } break;
        case Step::Basic_Wait_Move3: if (!checkMoveFinish(m_targetMove3)) { if (mClick && m_hoverGrid == m_targetMove3 && !m_player->IsMoving()) moveAndChange(m_targetMove3, 4); } break;
        case Step::Basic_End: if (proceedInput && m_stepTimer > 30) m_currentStep = Step::Menu; break;

        case Step::B1_Welcome: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::B1_Plus_Wait: if (!checkMoveFinish(m_targetMove1)) { if (mClick && m_hoverGrid == m_targetMove1 && !m_player->IsMoving()) moveAndChange(m_targetMove1, 3, '+'); } break;
        case Step::B1_Plus_Atk:
            if (mClick && m_hoverGrid == m_targetMove2 && !m_player->IsMoving()) { ProceduralAudio::GetInstance().PlayPowerSE(6); NextStep(); } break;
        case Step::B1_Plus_Apply:
            if (mClick && CheckButtonClick(850, 960, 220, 60)) { ProceduralAudio::GetInstance().PlayPowerSE(4); m_enemy->SetNumber(7); m_player->SetOp('\0'); NextStep(); }
            else if (mClick) ProceduralAudio::GetInstance().PlayErrorSE();
            break;
        case Step::B1_Plus_Res: if (proceedInput && m_stepTimer > 30) { SetupSituation(2); m_currentStep = Step::B1_Minus_Setup; } break;

        case Step::B1_Minus_Setup: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::B1_Minus_Wait: if (!checkMoveFinish(m_targetMove1)) { if (mClick && m_hoverGrid == m_targetMove1 && !m_player->IsMoving()) moveAndChange(m_targetMove1, 2, '-'); } break;
        case Step::B1_Minus_Atk:
            if (mClick && m_hoverGrid == m_targetMove2 && !m_player->IsMoving()) { ProceduralAudio::GetInstance().PlayPowerSE(6); NextStep(); } break;
        case Step::B1_Minus_Apply:
            if (mClick && CheckButtonClick(850, 960, 220, 60)) { ProceduralAudio::GetInstance().PlayPowerSE(4); m_enemy->SetNumber(-6); m_player->SetOp('\0'); NextStep(); }
            else if (mClick) ProceduralAudio::GetInstance().PlayErrorSE();
            break;
        case Step::B1_Minus_Res: if (proceedInput && m_stepTimer > 30) { SetupSituation(3); m_currentStep = Step::B1_Mul_Setup; } break;

        case Step::B1_Mul_Setup: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::B1_Mul_Wait: if (!checkMoveFinish(m_targetMove1)) { if (mClick && m_hoverGrid == m_targetMove1 && !m_player->IsMoving()) moveAndChange(m_targetMove1, 4, '*'); } break;
        case Step::B1_Mul_Atk:
            if (mClick && m_hoverGrid == m_targetMove2 && !m_player->IsMoving()) { ProceduralAudio::GetInstance().PlayPowerSE(6); NextStep(); } break;
        case Step::B1_Mul_Apply:
            if (mClick && CheckButtonClick(600, 960, 220, 60)) { ProceduralAudio::GetInstance().PlayPowerSE(8); m_player->AddStocks(3); m_player->SetNumber(5); m_player->SetOp('\0'); NextStep(); }
            else if (mClick) ProceduralAudio::GetInstance().PlayErrorSE();
            break;
        case Step::B1_End: if (proceedInput && m_stepTimer > 30) m_currentStep = Step::Menu; break;

        case Step::B2_Welcome: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::B2_Div_Wait: if (!checkMoveFinish(m_targetMove1)) { if (mClick && m_hoverGrid == m_targetMove1 && !m_player->IsMoving()) moveAndChange(m_targetMove1, 6, '/'); } break;
        case Step::B2_Div_Atk:
            if (mClick && m_hoverGrid == m_targetMove2 && !m_player->IsMoving()) { ProceduralAudio::GetInstance().PlayPowerSE(6); NextStep(); } break;
        case Step::B2_Div_Apply:
            if (mClick && CheckButtonClick(600, 960, 220, 60)) { ProceduralAudio::GetInstance().PlayPowerSE(4); m_player->AddWarpNode({ 5, 7 }); m_player->SetNumber(3); m_player->SetOp('\0'); NextStep(); }
            else if (mClick) ProceduralAudio::GetInstance().PlayErrorSE();
            break;
        case Step::B2_Res: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::B2_End: if (proceedInput && m_stepTimer > 30) m_currentStep = Step::Menu; break;

        case Step::RuleN_Welcome: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::RuleN_Wait: if (!checkMoveFinish(m_targetMove1)) { if (mClick && m_hoverGrid == m_targetMove1 && !m_player->IsMoving()) moveAndChange(m_targetMove1, 6, '-'); } break;
        case Step::RuleN_Atk:
            if (mClick && m_hoverGrid == m_targetMove2 && !m_player->IsMoving()) { ProceduralAudio::GetInstance().PlayPowerSE(6); NextStep(); } break;
        case Step::RuleN_Apply:
            if (mClick && CheckButtonClick(850, 960, 220, 60)) { ProceduralAudio::GetInstance().PlayPowerSE(4); m_enemy->SetStocks(0); m_enemy->SetNumber(0); m_player->SetOp('\0'); NextStep(); }
            else if (mClick) ProceduralAudio::GetInstance().PlayErrorSE();
            break;
        case Step::RuleN_End: if (proceedInput && m_stepTimer > 30) m_currentStep = Step::Menu; break;

        case Step::RuleC_Welcome: if (proceedInput && m_stepTimer > 30) NextStep(); break;
        case Step::RuleC_Wait: if (!checkMoveFinish(m_targetMove1)) { if (mClick && m_hoverGrid == m_targetMove1 && !m_player->IsMoving()) moveAndChange(m_targetMove1, 3, '+'); } break;
        case Step::RuleC_Atk:
            if (mClick && m_hoverGrid == m_targetMove2 && !m_player->IsMoving()) { ProceduralAudio::GetInstance().PlayPowerSE(6); NextStep(); } break;
        case Step::RuleC_Apply:
            if (mClick && CheckButtonClick(600, 960, 220, 60)) {
                ProceduralAudio::GetInstance().PlayPowerSE(4);
                m_player->SetOp('\0');
                m_p1Score += 8;
                NextStep();
            }
            else if (mClick) ProceduralAudio::GetInstance().PlayErrorSE();
            break;
        case Step::RuleC_End: if (proceedInput && m_stepTimer > 30) m_currentStep = Step::Menu; break;
        }
    }

    void TutorialMaster::Draw() const {
        if (m_ui) m_ui->Draw(*this);
        if (m_currentStep == Step::Menu) { DrawMenu(); return; }

        unsigned int cMove = GetColor(0, 255, 255), cAtk = GetColor(255, 100, 100);

        if (m_currentStep == Step::Basic_Wait_Move1 || m_currentStep == Step::B1_Plus_Wait || m_currentStep == Step::B1_Minus_Wait ||
            m_currentStep == Step::B1_Mul_Wait || m_currentStep == Step::B2_Div_Wait || m_currentStep == Step::RuleN_Wait || m_currentStep == Step::RuleC_Wait) {
            DrawHighlightGrid(m_targetMove1, cMove);
        }
        else if (m_currentStep == Step::Basic_Wait_Move2) { DrawHighlightGrid(m_targetMove2, cMove); }
        else if (m_currentStep == Step::Basic_Wait_Move3) { DrawHighlightGrid(m_targetMove3, cMove); }
        else if (m_currentStep == Step::B1_Plus_Atk || m_currentStep == Step::B1_Minus_Atk || m_currentStep == Step::B1_Mul_Atk ||
            m_currentStep == Step::B2_Div_Atk || m_currentStep == Step::RuleN_Atk || m_currentStep == Step::RuleC_Atk) {
            DrawHighlightGrid(m_targetMove2, cAtk);
        }

        switch (m_currentStep) {
        case Step::Basic_Welcome: DrawMessageWindow("【 1. 基礎：パワーと移動の法則 】\n自分の「パワー（数字）」によって、移動できるマス数が変わるぞ。\n[ 1, 4, 7 ]は3マス、[ 2, 5, 8 ]は2マス、[ 3, 6, 9 ]は1マスだ。\n\n[次へ]"); break;
        case Step::Basic_Wait_Move1: DrawMessageWindow("君のパワーは現在「9」なので【1マス】しか移動できない。\n光っている隣のマスをクリックして移動してくれ！"); break;
        case Step::Basic_Res_Move1: DrawMessageWindow("パワーが 9 から【8】に減った！移動したマス数だけパワーを消費するのだ。\nパワーが 8 になったことで、次は【2マス】移動できるぞ！\n\n[次へ]"); break;
        case Step::Basic_Wait_Move2: DrawMessageWindow("今度は2マス先の光っているマスをクリックして移動しよう！"); break;
        case Step::Basic_Res_Move2: DrawMessageWindow("パワーが 8 から【6】に減ったね。\nここで特別にパワーを「7」に回復してあげよう！\nパワーが 7 なら、最大の【3マス移動】ができるぞ。\n\n[次へ]"); break;
        case Step::Basic_Wait_Move3: DrawMessageWindow("パワー7の機動力を活かして、3マス先の光っているマスへ移動だ！"); break;
        case Step::Basic_End: DrawMessageWindow("パワーが 7 から【4】に減った！\nこのように「先を読んでパワーをコントロールする」のがバトルの第一歩だ。\n\n[メニューへ戻る]"); break;

        case Step::B1_Welcome: DrawMessageWindow("【 2. 実践①：ノーマルバトルにおける演算子の真の力 】\n盤面の演算子を取ると、隣接する敵にバトルを仕掛けられる。\nまずは基本の「＋」から体験しよう。\n\n[次へ]"); break;
        case Step::B1_Plus_Wait: DrawMessageWindow("現在のパワーは5だから【2マス】移動できる。\n光っている [＋] をクリックして取得だ！\n(取得するとパワーは3になるぞ)"); break;
        case Step::B1_Plus_Atk: DrawMessageWindow("「＋」を取得してパワーが【3】になった。\n光っている敵(パワー4)をクリックしてバトル開始だ！"); break;
        case Step::B1_Plus_Apply: DrawMessageWindow("計算が発生したぞ！ 3 ＋ 4 ＝ 7 だ。\n今回は相手を攻撃したいので、右下の【相手】ボタンを押して反映しよう！"); DrawFakeCalcPanel(3, '+', 4, 7, false, false); break;
        case Step::B1_Plus_Res: DrawMessageWindow("見事！ (自分 3) ＋ (敵 4) ＝ 【 7 】\n相手のパワーを 7 に上書きしたぞ。\nノーマルバトルでは、1〜9の間なら単に数字が上書きされるだけだ。\n\n[次へ]"); break;

        case Step::B1_Minus_Setup: DrawMessageWindow("次は「－（引き算）」だ。\nどうすれば相手にダメージを与えられるか、実際に試してみよう！\n\n[次へ]"); break;
        case Step::B1_Minus_Wait: DrawMessageWindow("パワー3なので【1マス】移動できる。\n光っている [－] をクリックして取得だ！\n(取得するとパワーは2になるぞ)"); break;
        case Step::B1_Minus_Atk: DrawMessageWindow("光っている敵(パワー8)をクリックしてバトルだ！"); break;
        case Step::B1_Minus_Apply: DrawMessageWindow("計算が発生したぞ！ 2 － 8 ＝ -6 だ。\nこのダメージを相手に与えたいので、右下の【相手】ボタンを押そう！"); DrawFakeCalcPanel(2, '-', 8, -6, false, false); break;
        case Step::B1_Minus_Res: DrawMessageWindow("計算発生！ 2 － 8 ＝ 【 -6 】！\n結果が0以下になると【バッテリー消費（大ダメージ）】が発生する！\n敵を攻撃する時は、マイナスを狙ってバッテリーを破壊しろ！\n\n[次へ]"); break;

        case Step::B1_Mul_Setup: DrawMessageWindow("最後は「×（掛け算）」だ。\n計算結果が大きくなるとどうなるのか確認しよう！\n\n[次へ]"); break;
        case Step::B1_Mul_Wait: DrawMessageWindow("パワー7なので【3マス】移動できる。\n光っている [×] をクリックして取得だ！\n(取得するとパワーは4になるぞ)"); break;
        case Step::B1_Mul_Atk: DrawMessageWindow("光っている敵(パワー8)をクリックしてバトルだ！"); break;
        case Step::B1_Mul_Apply: DrawMessageWindow("4 × 8 ＝ 32 ！ 10を超えると【バッテリー大回復】になるぞ！\n敵を回復させないよう、必ず【自分】ボタンを押して自分を回復させよう！"); DrawFakeCalcPanel(4, '*', 8, 32, false, true); break;
        case Step::B1_End: DrawMessageWindow("自分に反映したことで、自分のバッテリーが一気に回復した！\n「×」は一撃必殺の大ダメージを生み出せる最強の演算子だ！\n状況に合わせて演算子を使い分けよう。\n\n[メニューへ戻る]"); break;

        case Step::B2_Welcome: DrawMessageWindow("【 3. 実践②：割り算（÷）とワープゲート 】\n「÷」は、ダメージを与えるのではなく\n【ワープゲートを設置する】という特殊な能力を持っている。\n\n[次へ]"); break;
        case Step::B2_Div_Wait: DrawMessageWindow("パワー8なので【2マス】移動できる。\n光っている [÷] をクリックして取得してくれ！\n(取得するとパワーは6になるぞ)"); break;
        case Step::B2_Div_Atk: DrawMessageWindow("「÷」を取得してパワーが【6】になった。\n敵(パワー2)をクリックして割り算を実行しよう！"); break;
        case Step::B2_Div_Apply: DrawMessageWindow("6 ÷ 2 ＝ 3！\n自分のワープを設置し、さらに割り切れた数値を得るため【自分】ボタンを押そう！\n※もし相手ボタンを押すと、相手のワープを設置してしまうぞ。"); DrawFakeCalcPanel(6, '/', 2, 3, true, true); break;
        case Step::B2_Res: DrawMessageWindow("自分に反映したことで、ワープが設置され、パワーが3になった！\nワープは移動の幅を劇的に広げる切り札だ。\n※割り切れない場合は、ワープが設置されるだけで数値の反映は起きない。\n\n[次へ]"); break;
        case Step::B2_End: DrawMessageWindow("今回は (X:6, Y:2) にワープが設置された。\n自分ボタンなら自分専用、相手ボタンなら相手専用のワープになるぞ！\n\n[メニューへ戻る]"); break;

            // ★論理修正を反映
        case Step::RuleN_Welcome: DrawMessageWindow("【 4. ルール編①：ノーマルバトルの勝利条件 】\nノーマルバトルは、相手にマイナスを叩き込んで\n【バッテリーを破壊し尽くす】のが目的だ。\n最後にトドメを刺す感覚を味わってみよう。\n\n[次へ]"); break;
        case Step::RuleN_Wait: DrawMessageWindow("敵の予備バッテリーはすでにゼロだ！\n目の前の「－」を取って（パワー6になる）、敵(パワー9)を攻撃しよう！\n[－]を取ってくれ！"); break;
        case Step::RuleN_Atk: DrawMessageWindow("光っている敵をクリックしてトドメだ！"); break;
        case Step::RuleN_Apply: DrawMessageWindow("6 － 9 ＝ -3！ この大ダメージを【相手】に叩き込もう！"); DrawFakeCalcPanel(6, '-', 9, -3, false, false); break;
        case Step::RuleN_End: DrawMessageWindow("6 － 9 ＝ 【 -3 】！ マイナスで大ダメージ！\n敵は耐えきれず、完全に破壊された！\nこれがノーマルバトルの基本戦術だ！\n\n[メニューへ戻る]"); break;

        case Step::RuleC_Welcome: DrawMessageWindow("【 5. ルール編②：カウントバトルの勝利条件 】\n「カウントバトル」は、相手を倒すのではなく\n【自分のスコアを目標値（今回は53）にピッタリ合わせる】ルールだ！\n\n[次へ]"); break;
        case Step::RuleC_Wait: DrawMessageWindow("現在の君のスコアは「45」。目標まであと「8」だ。\n[＋]を取って攻撃の準備をしよう！(パワーは3になる)"); break;
        case Step::RuleC_Atk: DrawMessageWindow("敵(パワー5)をクリックして攻撃だ！"); break;
        case Step::RuleC_Apply: DrawMessageWindow("3 ＋ 5 ＝ 8！ 目標スコアに近づけるため、\nこの数値を自分のスコアに加算したい！【自分】ボタンを押そう！"); DrawFakeCalcPanel(3, '+', 5, 8, false, true, true); break;
        case Step::RuleC_End: DrawMessageWindow("3 ＋ 5 ＝ 【 8 】！ 自分のスコアに 8 が加算され、\n見事【 53 】にピッタリ到達して勝利だ！\n※目標をオーバーすると跳ね返ってスコアが減るので注意しよう。\n\n[メニューへ戻る]"); break;
        }
    }

    void TutorialMaster::DrawMenu() const {
        int sw = 1920, sh = 1080, CX = sw / 2, CY = sh / 2;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210); DrawBox(0, 0, sw, sh, GetColor(5, 10, 25), TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        int fontTitle = GetCachedFont(60);
        int tw = GetDrawStringWidthToHandle("=== チュートリアル メニュー ===", 31, fontTitle);
        DrawStringToHandle(CX - tw / 2, CY - 300, "=== チュートリアル メニュー ===", GetColor(0, 200, 255), fontTitle);

        const char* items[] = {
            "【基礎】 パワーと移動の法則",
            "【実践】 演算子（＋,－,×）の真の力",
            "【実践】 割り算（÷）とワープゲート",
            "【ルール】 ノーマルバトルの勝利条件",
            "【ルール】 カウントバトルの勝利条件",
            "タイトルへ戻る"
        };
        int menuStartY = CY - 170, fontMenu = GetCachedFont(40);
        for (int i = 0; i < 6; ++i) {
            int iy = menuStartY + i * 85;
            unsigned int color = (m_menuCursor == i) ? GetColor(255, 180, 0) : GetColor(150, 150, 150);
            if (m_menuCursor == i) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100); DrawBox(CX - 400, iy - 10, CX + 400, iy + 50, color, TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                DrawStringToHandle(CX - 420, iy, ">>", color, fontMenu);
            }
            int textW = GetDrawStringWidthToHandle(items[i], (int)strlen(items[i]), fontMenu);
            DrawStringToHandle(CX - textW / 2, iy, items[i], color, fontMenu);
        }
    }

    void TutorialMaster::Release() { if (m_fontMsg != -1) { DeleteFontToHandle(m_fontMsg); m_fontMsg = -1; } }
    void TutorialMaster::NextStep() { m_currentStep = static_cast<Step>(static_cast<int>(m_currentStep) + 1); m_stepTimer = 0; }

    void TutorialMaster::DrawMessageWindow(const std::string& text) const {
        int sw = 1920, sh = 1080, winW = 1250, winH = 220;
        int winX = (sw - winW) / 2, winY = sh - winH - 50;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230); DrawBox(winX, winY, winX + winW, winY + winH, GetColor(10, 15, 30), TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(winX, winY, winX + winW, winY + winH, GetColor(0, 150, 255), FALSE);
        DrawBox(winX + 2, winY + 2, winX + winW - 2, winY + winH - 2, GetColor(0, 100, 200), FALSE);

        if (m_fontMsg != -1) DrawStringToHandle(winX + 40, winY + 30, text.c_str(), GetColor(255, 255, 255), m_fontMsg);
    }

    void TutorialMaster::DrawHighlightGrid(const IntVector2& gridPos, unsigned int color) const {
        Vector2 center = m_mapGrid.GetCellCenter(gridPos.x, gridPos.y);
        int alpha = (int)(150 + 105 * sin(GetNowCount() / 1000.0 * 3.14159 * 4.0));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(center.x - 40, center.y - 40, center.x + 40, center.y + 40, color, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
} // namespace App