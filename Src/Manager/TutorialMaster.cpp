#include "TutorialMaster.h"
#include <DxLib.h>
#include "../Input/InputManager.h"
#include "../Scene/SceneManager.h"
#include "../Manager/ProceduralAudio.h"

namespace App {

    TutorialMaster::TutorialMaster()
        : m_currentStep(Step::Msg_Welcome)
        , m_stepTimer(0)
        , m_hoverGrid(-1, -1)
        , m_mapGrid(80, Vector2(600, 120)) // BattleMasterと同じ設定
        , m_fontMsg(-1)
    {
    }

    TutorialMaster::~TutorialMaster() {
        Release();
    }

    void TutorialMaster::Init() {
        m_currentStep = Step::Msg_Welcome;
        m_stepTimer = 0;

        // 文字量が多いのでフォントサイズを24に設定して見やすくします
        m_fontMsg = CreateFontToHandle("BIZ UDゴシック", 24, 2, DX_FONTTYPE_ANTIALIASING);

        // ==========================================
        // チュートリアル専用の初期配置をセットアップ
        // ==========================================
        int p1X = 2, p1Y = 2; // 1P初期位置
        int p2X = 5, p2Y = 5; // 2P(敵)初期位置

        // 移動の目標マス
        m_targetMove1 = IntVector2(2, 4); // 上に2マス移動
        m_targetMove2 = IntVector2(4, 4); // 右に2マス移動（ここでアイテム取得とする）
        m_targetMove3 = IntVector2(4, 5); // 敵の隣に移動（隣接して攻撃）

        m_mapGrid.SetRuleModeAndStage(BattleRuleMode::CLASSIC, 0); // 仮でクラシックのステージ0

        // プレイヤーと敵を生成（残機は適当に3）
        // プレイヤーはパワー5（2マス移動）、敵はパワー3（1マス移動）
        m_player = std::make_unique<Player>(IntVector2(p1X, p1Y), m_mapGrid.GetCellCenter(p1X, p1Y), 5, 3, 3);
        m_enemy = std::make_unique<Enemy>(IntVector2(p2X, p2Y), m_mapGrid.GetCellCenter(p2X, p2Y), 3, 3, 3);

        m_player->SetOp('\0');
        m_enemy->SetOp('\0');

        m_ui = std::make_unique<BattleUI>();
        m_ui->Init();
        m_ui->AddLog(">>> チュートリアルを開始します");
    }

    void TutorialMaster::Update() {
        m_stepTimer++;

        // プレイヤー等のアニメーション更新
        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();

        auto& input = InputManager::GetInstance();
        m_hoverGrid = m_mapGrid.ScreenToGrid(input.GetMousePos());
        bool mClick = input.IsMouseLeftTrg();
        bool spaceTrg = input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN);
        bool proceedInput = (mClick || spaceTrg);

        // ==========================================
        // ステートマシン：チュートリアルの進行制御
        // ==========================================
        switch (m_currentStep) {

        case Step::Msg_Welcome:
        case Step::Msg_MoveRule:
            if (proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                NextStep();
            }
            break;

            // 【実践1：普通の移動】
        case Step::Wait_Move1:
            if (mClick && !m_player->IsMoving()) {
                if (m_hoverGrid == m_targetMove1) {
                    ProceduralAudio::GetInstance().PlayPowerSE(5);
                    m_ui->AddLog("【移動】 1P が移動しました！");

                    std::queue<Vector2> path;
                    path.push(m_mapGrid.GetCellCenter(m_targetMove1.x, m_targetMove1.y));
                    m_player->StartMove(m_targetMove1, path);

                    NextStep();
                }
                else {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                }
            }
            break;

        case Step::Msg_OpRule:
            if (!m_player->IsMoving() && proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                NextStep();
            }
            break;

            // 【実践2：移動してアイテムを取る】
        case Step::Wait_Move2:
            if (mClick && !m_player->IsMoving()) {
                if (m_hoverGrid == m_targetMove2) {
                    ProceduralAudio::GetInstance().PlayPowerSE(5);
                    m_ui->AddLog("【移動】 1P が移動しました！");

                    // 移動と同時にアイテム取得の演出
                    m_player->SetOp('+');
                    m_ui->AddLog("【取得】 1P が [＋] を取得！");
                    ProceduralAudio::GetInstance().PlayPowerSE(5);

                    std::queue<Vector2> path;
                    path.push(m_mapGrid.GetCellCenter(m_targetMove2.x, m_targetMove2.y));
                    m_player->StartMove(m_targetMove2, path);

                    NextStep();
                }
                else {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                }
            }
            break;

        case Step::Msg_BattleRule:
            if (!m_player->IsMoving() && proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                NextStep();
            }
            break;

            // 【実践3：敵に隣接して攻撃する】
        case Step::Wait_Move3:
            if (mClick && !m_player->IsMoving()) {
                if (m_hoverGrid == m_targetMove3) {
                    ProceduralAudio::GetInstance().PlayPowerSE(5);

                    // バトル発生の演出
                    m_ui->AddLog("【バトル発生！】");
                    m_ui->AddLog("【計算】 1P の攻撃！ ( 5 + 3 = 8 )");
                    ProceduralAudio::GetInstance().PlayPowerSE(4); // 計算音

                    // 相手の数字を上書きし、演算子を消費する
                    m_enemy->SetNumber(8);
                    m_player->SetOp('\0');
                    m_ui->AddLog("【反映】 2P のパワーを 8 に書き換えました");

                    std::queue<Vector2> path;
                    path.push(m_mapGrid.GetCellCenter(m_targetMove3.x, m_targetMove3.y));
                    m_player->StartMove(m_targetMove3, path);

                    NextStep();
                }
                else {
                    ProceduralAudio::GetInstance().PlayErrorSE();
                }
            }
            break;

        case Step::Msg_DivideRule:
        case Step::Msg_Finish:
            if (!m_player->IsMoving() && proceedInput && m_stepTimer > 30) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                if (m_currentStep == Step::Msg_Finish) {
                    // タイトル画面へ戻る！
                    SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);
                }
                else {
                    NextStep();
                }
            }
            break;
        }
    }

    void TutorialMaster::Draw() const {
        // 1. 全ての基本描画は BattleUI に丸投げ！
        if (m_ui) m_ui->Draw(*this);

        // ==========================================
        // 2. チュートリアル用の特別な描画（一番手前に重ねる）
        // ==========================================

        // 入力待ちフェーズなら、クリックすべき場所をハイライトする
        if (m_currentStep == Step::Wait_Move1) {
            DrawHighlightGrid(m_targetMove1);
        }
        else if (m_currentStep == Step::Wait_Move2) {
            DrawHighlightGrid(m_targetMove2);
        }
        else if (m_currentStep == Step::Wait_Move3) {
            DrawHighlightGrid(m_targetMove3);
        }

        // メッセージウィンドウの描画
        switch (m_currentStep) {
        case Step::Msg_Welcome:
            DrawMessageWindow("超計算マスBATTLEへようこそ！\nこのチュートリアルで、独自の戦闘ルールをマスターしよう。\n\n[クリック または SPACEキーで次へ]");
            break;
        case Step::Msg_MoveRule:
            DrawMessageWindow("まずは基本の「移動」だ。\n現在のパワー（数字）によって、1ターンに移動できるマス数が変わるぞ！\n・[ 1, 4, 7 ] は 【3マス移動】\n・[ 2, 5, 8 ] は 【2マス移動】\n・[ 3, 6, 9 ] は 【1マス移動】\n\n[次へ]");
            break;
        case Step::Wait_Move1:
            DrawMessageWindow("君のパワーは現在「5」だから【2マス】移動できるね。\n【青く光っているマス】をクリックして移動してみよう！");
            break;
        case Step::Msg_OpRule:
            DrawMessageWindow("次は「演算子」アイテムだ。\n盤面の演算子を取ると、それぞれ移動範囲が拡張される！\n（＋：縦横、－：横のみ、×：斜め、÷：横と上下）\n\n※注意：取得したターンにバトルで使わないと、バッテリーが1つ減るペナルティがあるぞ！\n[次へ]");
            break;
        case Step::Wait_Move2:
            DrawMessageWindow("さあ、移動して演算子アイテムを取ってみよう。\n【青く光っているマス】をクリックして移動だ！");
            break;
        case Step::Msg_BattleRule:
            DrawMessageWindow("演算子を持った状態で相手と【隣り合う】とバトル発生だ！\n（重なった場合は何も起きないぞ）\n計算結果は画面下のパネルから【自分】か【相手】、好きな方に反映できる！\n\n[次へ]");
            break;
        case Step::Wait_Move3:
            DrawMessageWindow("君は今「＋」の演算子を持っている。\n敵の【隣のマス（青く光るマス）】に移動して、バトルを仕掛けよう！");
            break;
        case Step::Msg_DivideRule:
            DrawMessageWindow("見事！計算結果で相手のパワーを書き換えたね。\n最後に【÷ (割り算)】の特殊能力についてだ。\n\n割り算の結果は「分数」となり、その分子と分母の座標に【ワープ】を設置できる！\n設置後は、どこからでも移動コスト1でそのマスへ飛べるぞ。\n（※割り切れた場合は、そのままパワー数値を反映することも可能）\n\n[次へ]");
            break;
        case Step::Msg_Finish:
            DrawMessageWindow("ルール説明は以上だ！\n相手の手を読み、移動と計算を駆使して頭脳戦を制覇しよう！\n\n[クリック または SPACEキーでタイトルへ戻る]");
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
        int winH = 220; // テキストが多いので少し高さを広げています
        int winX = (sw - winW) / 2;
        int winY = sh - winH - 50;

        // 半透明の黒背景
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
        DrawBox(winX, winY, winX + winW, winY + winH, GetColor(10, 15, 30), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 枠線
        DrawBox(winX, winY, winX + winW, winY + winH, GetColor(0, 150, 255), FALSE);
        DrawBox(winX + 2, winY + 2, winX + winW - 2, winY + winH - 2, GetColor(0, 100, 200), FALSE);

        // テキストの描画
        if (m_fontMsg != -1) {
            DrawStringToHandle(winX + 40, winY + 30, text.c_str(), GetColor(255, 255, 255), m_fontMsg);
        }
    }

    void TutorialMaster::DrawHighlightGrid(const IntVector2& gridPos) const {
        Vector2 center = m_mapGrid.GetCellCenter(gridPos.x, gridPos.y);

        int halfSize = 40;

        // 点滅エフェクト
        double time = GetNowCount() / 1000.0;
        int alpha = (int)(150 + 105 * sin(time * 3.14159 * 4.0));

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(center.x - halfSize, center.y - halfSize,
            center.x + halfSize, center.y + halfSize,
            GetColor(0, 255, 255), TRUE); // シアン色でハイライト
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

} // namespace App