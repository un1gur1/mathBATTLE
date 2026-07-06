#include "TutorialMaster.h"
#include <DxLib.h>
#include "../Input/InputManager.h"
#include "../Scene/SceneManager.h"
#include "ProceduralAudio.h"

namespace App {

    TutorialMaster::TutorialMaster()
        : m_currentStep(Step::Msg_Welcome)
        , m_stepTimer(0)
        , m_hoverGrid(-1, -1)
        , m_mapGrid(80, Vector2(600, 120))
        , m_fontMsg(-1)
    {
    }

    TutorialMaster::~TutorialMaster() {
        Release();
    }

    void TutorialMaster::Init() {
        m_currentStep = Step::Msg_Welcome;
        m_stepTimer = 0;

        m_fontMsg = CreateFontToHandle("BIZ UDゴシック", 24, 2, DX_FONTTYPE_ANTIALIASING);

        int p1X = 2, p1Y = 2; // 1P初期位置
        int p2X = 5, p2Y = 5; // 2P(敵)初期位置

        m_targetMove1 = IntVector2(2, 4); // 2マス移動
        m_targetMove2 = IntVector2(4, 4); // 2マス移動（＋を取得）
        m_targetMove3 = IntVector2(4, 5); // 1マス移動（敵に隣接）

        m_mapGrid.SetRuleModeAndStage(BattleRuleMode::CLASSIC, 0);

        m_player = std::make_unique<Player>(IntVector2(p1X, p1Y), m_mapGrid.GetCellCenter(p1X, p1Y), 7, 3, 3);
        m_enemy = std::make_unique<Enemy>(IntVector2(p2X, p2Y), m_mapGrid.GetCellCenter(p2X, p2Y), 3, 3, 3);

        m_player->SetOp('\0');
        m_enemy->SetOp('\0');

        m_ui = std::make_unique<BattleUI>();
        m_ui->Init();
        m_ui->AddLog(">>> チュートリアルを開始します");
    }

    void TutorialMaster::Update() {
        m_stepTimer++;

        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();

        auto& input = InputManager::GetInstance();
        m_hoverGrid = m_mapGrid.ScreenToGrid(input.GetMousePos());
        bool mClick = input.IsMouseLeftTrg();
        bool spaceTrg = input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN);
        bool proceedInput = (mClick || spaceTrg);

        switch (m_currentStep) {

        case Step::Msg_Welcome:
        case Step::Msg_WinLoseRule:
        case Step::Msg_MoveRule:
            if (proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                NextStep();
            }
            break;

        case Step::Wait_Move1:
            if (mClick && !m_player->IsMoving()) {
                if (m_hoverGrid == m_targetMove1) {
                    ProceduralAudio::GetInstance().PlayPowerSE(5);
                    m_ui->AddLog("【移動】 1P が移動しました！");

                    std::queue<Vector2> path;
                    path.push(m_mapGrid.GetCellCenter(m_targetMove1.x, m_targetMove1.y));
                    m_player->StartMove(m_targetMove1, path);

                    // ★ 7 から 2マス移動 ＝ 5
                    m_player->SetNumber(5);

                    NextStep();
                }
                else ProceduralAudio::GetInstance().PlayErrorSE();
            }
            break;

        case Step::Msg_NumberChanged:
        case Step::Msg_OpRule:
            if (!m_player->IsMoving() && proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                NextStep();
            }
            break;

        case Step::Wait_Move2:
            if (mClick && !m_player->IsMoving()) {
                if (m_hoverGrid == m_targetMove2) {
                    ProceduralAudio::GetInstance().PlayPowerSE(5);

                    m_player->SetOp('+');
                    m_ui->AddLog("【取得】 1P が [＋] を取得！");

                    std::queue<Vector2> path;
                    path.push(m_mapGrid.GetCellCenter(m_targetMove2.x, m_targetMove2.y));
                    m_player->StartMove(m_targetMove2, path);

                    // ★ 5 から 2マス移動 ＝ 3（修正完了！）
                    m_player->SetNumber(3);

                    NextStep();
                }
                else ProceduralAudio::GetInstance().PlayErrorSE();
            }
            break;

        case Step::Msg_BattleRule:
            if (!m_player->IsMoving() && proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                NextStep();
            }
            break;

        case Step::Wait_Move3:
            if (mClick && !m_player->IsMoving()) {
                if (m_hoverGrid == m_targetMove3) {
                    ProceduralAudio::GetInstance().PlayPowerSE(5);
                    m_ui->AddLog("【バトル発生！】");

                    std::queue<Vector2> path;
                    path.push(m_mapGrid.GetCellCenter(m_targetMove3.x, m_targetMove3.y));
                    m_player->StartMove(m_targetMove3, path);

                    // ★ 3 から 1マス移動 ＝ 2（バトル直前にパワーが確定）
                    m_player->SetNumber(2);
                    m_ui->AddLog("【計算】 1P の攻撃！ ( 2 + 3 = 5 )");

                    NextStep();
                }
                else ProceduralAudio::GetInstance().PlayErrorSE();
            }
            break;

        case Step::Wait_ApplyDamage:
            if (!m_player->IsMoving() && proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(4);
                // ダメージ確定（2 + 3 = 5）
                m_enemy->SetNumber(5);
                m_player->SetOp('\0');
                m_ui->AddLog("【反映】 2P のパワーを 5 に書き換えました");
                NextStep();
            }
            break;

        case Step::Msg_Finish:
            if (proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);
            }
            break;
        }
    }

    void TutorialMaster::Draw() const {
        if (m_ui) m_ui->Draw(*this);

        unsigned int colMove = GetColor(0, 255, 255);

        if (m_currentStep == Step::Wait_Move1)      DrawHighlightGrid(m_targetMove1, colMove);
        else if (m_currentStep == Step::Wait_Move2) DrawHighlightGrid(m_targetMove2, colMove);
        else if (m_currentStep == Step::Wait_Move3) DrawHighlightGrid(m_targetMove3, colMove);

        // ==========================================
        // メッセージウィンドウ（テキストを修正！）
        // ==========================================
        switch (m_currentStep) {
        case Step::Msg_Welcome:
            DrawMessageWindow("超計算マスBATTLEへようこそ！\nこのチュートリアルで、独自の戦闘ルールをマスターしよう。\n\n[次へ]");
            break;
        case Step::Msg_WinLoseRule:
            DrawMessageWindow("このゲームの目的は【相手のバッテリーを0にする】ことだ！\nバトルで大ダメージを与え、バッテリーを破壊し尽くせば勝利となるぞ。\n\n[次へ]");
            break;
        case Step::Msg_MoveRule:
            DrawMessageWindow("まずは基本の「移動」だ。\n現在のパワー（数字）によって、1ターンに移動できるマス数が変わる！\n・[ 1, 4, 7 ] は 【3マス移動】\n・[ 2, 5, 8 ] は 【2マス移動】\n・[ 3, 6, 9 ] は 【1マス移動】\n\n[次へ]");
            break;
        case Step::Wait_Move1:
            DrawMessageWindow("君のパワーは現在「7」だから【3マス】まで移動できるね。\n【青く光っているマス】（2マス先）をクリックして移動してみよう！");
            break;
        case Step::Msg_NumberChanged:
            DrawMessageWindow("よく見てくれ！2マス移動したことでパワーが【7】から【5】に減った！\n\nこのように【移動した分だけ自分の数字が変わる】のが最大の鍵だ。\n先の展開を読んで、欲しい数字になるように動く必要があるぞ。\n\n[次へ]");
            break;
        case Step::Msg_OpRule:
            DrawMessageWindow("次は「演算子」アイテムだ。\n盤面の演算子を取ると、それぞれ攻撃範囲が拡張されるぞ。\n\nだが注意しろ！【演算子を持ったまま次の移動をする】と、\n重負荷のペナルティとして、移動のたびにバッテリーが1つ減ってしまう！\n[次へ]");
            break;
        case Step::Wait_Move2:
            DrawMessageWindow("現在のパワーは「5」だから【2マス】まで移動できる。\nちょうど2マス先に演算子アイテムがあるぞ。\n【青く光っているマス】をクリックして取得だ！");
            break;
        case Step::Msg_BattleRule:
            DrawMessageWindow("見事！「＋」を取得し、さらにパワーが【3】に変化した。\n\n演算子を持った状態で相手と【隣り合う】とバトル発生だ！\n※本来はここで動くとバッテリーが減るが、今回は特別に免除しよう。\n\n[次へ]");
            break;
        case Step::Wait_Move3:
            DrawMessageWindow("君は今パワー「3」なので【1マス】しか移動できない。\nだが、敵はちょうど1マス隣（青く光るマス）にいるぞ！\n移動してバトルを仕掛けよう！");
            break;
        case Step::Wait_ApplyDamage:
            DrawMessageWindow("計算発生！ 移動したことで1Pのパワーは「2」になった。\n\n1P[2] ＋ 2P[3] ＝ 【5】 だ。\n今回はこの「5」を【相手のパワー】に押し付けてやろう！\n\n[スペースキー または クリックで相手に反映]");
            break;
        case Step::Msg_Finish:
            DrawMessageWindow("見事！相手のパワーを 5 に書き換えたね。\nこれで基本操作は完璧だ。\n\n移動で数字を調整し、計算を駆使して頭脳戦を制覇しよう！\n[クリック または SPACEキーでタイトルへ戻る]");
            break;
        }
    }

    void TutorialMaster::Release() {
        if (m_fontMsg != -1) {
            DeleteFontToHandle(m_fontMsg);
            m_fontMsg = -1;
        }
    }

    void TutorialMaster::NextStep() {
        int next = static_cast<int>(m_currentStep) + 1;
        m_currentStep = static_cast<Step>(next);
        m_stepTimer = 0;
    }

    void TutorialMaster::DrawMessageWindow(const std::string& text) const {
        int sw, sh;
        GetDrawScreenSize(&sw, &sh);

        int winW = 1000;
        int winH = 220;
        int winX = (sw - winW) / 2;
        int winY = sh - winH - 50;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
        DrawBox(winX, winY, winX + winW, winY + winH, GetColor(10, 15, 30), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(winX, winY, winX + winW, winY + winH, GetColor(0, 150, 255), FALSE);
        DrawBox(winX + 2, winY + 2, winX + winW - 2, winY + winH - 2, GetColor(0, 100, 200), FALSE);

        if (m_fontMsg != -1) {
            DrawStringToHandle(winX + 40, winY + 30, text.c_str(), GetColor(255, 255, 255), m_fontMsg);
        }
    }

    void TutorialMaster::DrawHighlightGrid(const IntVector2& gridPos, unsigned int color) const {
        Vector2 center = m_mapGrid.GetCellCenter(gridPos.x, gridPos.y);
        int halfSize = 40;

        double time = GetNowCount() / 1000.0;
        int alpha = (int)(150 + 105 * sin(time * 3.14159 * 4.0));

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(center.x - halfSize, center.y - halfSize, center.x + halfSize, center.y + halfSize, color, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

} // namespace App